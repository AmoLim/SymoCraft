---
type: 数据设计
status: 草稿
project: Symocraft
module: assets
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# Image CPU 像素数据

## 当前设计

### 工作负载与布局

资源加载时 DecodeImage 返回紧密行排列原通道字节，renderer 读取并上传；vector<uint8_t> 连续自有像素，不留 stb 缓冲或 GPU ID。无需图形上下文；不进入逐帧解码。像素规模依图片，访问/复制瓶颈未测量，不宣称已优化。

### 字段与有效期

width/height/channels 默认 0，pixels 默认空。成功解码尺寸正、channels 1..4；顺序/翻转由 DecodeImage(path,flip_vertical=false) 决定。Image 可按值 deep copy；标准移动后的源 vector 与尺寸不构成重新解码成功保证。pixels 的指针/引用会在扩容、赋值或销毁失效。

### 不变量

原 Assets I1：DecodeImage 成功返回 pixels.size()==width*height*channels。
原 Assets I3：解码失败抛出，不返回半初始化图片。
这两条限定函数出口，**Image 字段公开，调用者可构造不一致值**，不是所有任意 Image 的强制检查。路径安全 I2 留 [Assets 汇总](Assets-资源读取与解码-类设计.md#不变量)。

### 创建、消费与失败

DecodeImage 使用私有 stb 的局部 unique_ptr 清理解码内存，检查失败并复制到拥有的 pixels；显式当前线程翻转选项避免全局污染。文件/长度/解码/分配错误以异常报告，不生成 GPU。renderer 按原策略仅接受 RGB/RGBA，不因为 assets 能解码 1/2 通道就宣称 GPU 加载支持。

无存档版本、异步任务、共享缓存、并发/重入专项保证。所有字段单位为像素/通道/字节，无行 padding。
源码：[image.h](../../../../game/modules/assets/include/symocraft/assets/image.h)、[image.cpp](../../../../game/modules/assets/src/image.cpp)。
原 [功能验收](Assets-资源读取与解码.md#验收案例)、[T0 报告](../../../milestones/m3-t0/README.md)保持范围；2026-10-06 仅源码核对，未重新运行图片/损坏纹理/OOM 验收。见 [覆盖清单](../对象笔记覆盖清单.md)。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用：Image 无显式成员函数 | assets / renderer | 聚合默认数值 0、pixels 空；复制深拷贝 vector，移动按标准库语义 | 字段公开，可构造不一致值；pixels 地址在扩容/赋值/销毁失效 | vector 分配可抛异常；无字段校验器 | [image.h](../../../../game/modules/assets/include/symocraft/assets/image.h) |
| `ReadBytes` / `DecodeImage`（外部创建入口索引） | 模块公开函数，不是 Image 方法 | [Assets 权威行为表](Assets-资源读取与解码-类设计.md#公开接口预期行为) | 返回拥有的数据，不借用 stb 缓冲 | 创建失败与清理详见权威表 | I1/I3 |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 不适用 | Image | 无 private/helper 函数 | 解码属于 Assets，不是本值类型算法 | 标准 vector 管理字节生命周期 | image.h |

## 本次变更

无。

## 后续考虑

新的通道转换/共享加载另定消费格式、寿命和代表负载。

