---
type: 类设计
status: 已验证（T0范围）
project: Symocraft
module: assets
class_name: "Image / Assets 函数"
inheritance: []
created: 2026-10-05
tags:
  - area/architecture
---

# Assets-资源读取与解码-类设计

关联功能：[Assets-资源读取与解码](Assets-资源读取与解码.md)。不存在的管理类不为模板而增造；自由函数与数据结构按实际实现记录。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

职责：assets 保留以可执行文件为根的路径策略，并接管原来分散在 application / texture 内的 CPU 图片解码。文件字节与 原通道像素归调用方所有，GPU 创建与释放仍由 renderer 负责。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| int | Image::width / height | 0 | 解码成功后为正 |
| int | Image::channels | 0 | 成功后为 1 至 4，保留源格式 |
| vector<uint8_t> | Image::pixels | 空 | 拥有全部像素；复制会复制像素 |
| 私有临时缓冲 | DecodeImage 的 unique_ptr | 解码结果 | 函数离开时释放 stb 分配 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | 成功 Image 的 pixels.size()==width*height*channels | DecodeImage 返回时 |
| I2 | Resolve 返回路径在可执行文件 assets 目录以内 | 正常返回时 |
| I3 | 解码失败不返回半初始化图片 | 异常出口 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| ReadBytes(path) | 完整二进制读取 | 打开、长度、读失败以路径具名异常报告 |
| DecodeImage(path, flip_vertical) | 解码原通道像素 | 无需 OpenGL 上下文；不修改全局翻转开关 |
| Resolve(relative) | 安全定位资源 | 返回路径不保证文件存在，读取阶段再诊断 |

- Resolve 继续拒绝根路径、父目录越界、备用数据流及越界符号链接，不靠改变 CWD 修复资源问题。
- DecodeImage 保留源图片通道数（1 至 4），renderer 按原策略只接受 RGB / RGBA，行紧密排列；每次显式设置当前线程的翻转选项，避免调用间污染。
- 解码临时缓冲通过 RAII 释放；向 Image 的一次复制只发生于资源加载，不发生于逐帧渲染。
- 图片解码库与 Windows 路径实现均私有；模块不解释方块配置语义。

公开头：[include](../../../../game/modules/assets/include/symocraft/assets)；实现：[src](../../../../game/modules/assets/src)。本轮未对并发加载或重入作专项验证。

## 本次变更

本次将真实实现归入 assets，用公开契约替代旧聚合头依赖。成员、所有权与失效约束以上表为准；尚未完成验证的风险不以“拆库完成”代替。

### 验收案例

验收位置：[关联功能的验收案例](Assets-资源读取与解码.md#验收案例)。正式库图片/路径测试、独立消费者和真实损坏纹理退出检查已通过，覆盖本轮 I1-I3 正常与已列失败路径；不扩展为所有解码故障或内存不足场景已验证。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 新调用者需要改变生命周期 | 先修改契约与测试，再修改接口，不暴露存储布局解决临时需求 |

