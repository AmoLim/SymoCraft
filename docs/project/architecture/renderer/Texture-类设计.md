---
type: 类设计
status: 草稿
project: Symocraft
module: renderer
class_name: "::Texture"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# Texture 纹理所有权

## 当前设计

### 成员与所有权

拥有 m_texture_Id（初始 0）；m_filepath 为 string；m_width/m_height/m_channel_amount、m_texture_format/m_internal_format 初始 0。CPU 解码图片仅创建阶段临时存在，GPU ID 属 Texture。

### 接口与生命周期

CreateRegularTexture(path,pixelated) 是实例方法但**返回新的 Texture**，不是修改 this。DecodeImage 翻转像素，要求正尺寸、不超过 GL 上限、仅 RGB/RGBA；创建二维纹理/存储，保存 unpack alignment、设置 1、上传后恢复，按 pixelated 选 nearest/linear。错误抛异常，由局部 Texture 析构释放已获取 ID；并不证明所有 GL 状态异常均事务恢复。

Destroy noexcept 删除非零 ID 并置 0，**不清路径/尺寸/格式**；析构调用 Destroy。禁止复制；显式 noexcept 移动转移 ID/数值并清零源数值，字符串为标准移动后状态。移动赋值先释放目标，处理自赋值。当前上下文必须在释放时有效，只主线程。

### 公开接口预期行为

内部类 public；定义见 [texture.h](../../../../game/modules/renderer/src/texture.h)、[texture.cpp](../../../../game/modules/renderer/src/texture.cpp)。

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `Texture()` | renderer 内部 | 空路径、ID/数值为 0 | 无 GPU 创建 | 默认构造语义 | texture.h |
| `~Texture()` | owner | Destroy；非 virtual | context 有效，不多态 delete | 不提供 context 失效恢复 | I1/I5 |
| `Texture(const Texture&) = delete` | 禁止 | 不复制 ID | 编译期 | 编译不通过 | texture.h |
| `operator=(const Texture&) = delete` | 禁止 | 不复制赋值 | 编译期 | 编译不通过 | texture.h |
| `Texture(Texture&&) noexcept` | 值返回 / owner | 经移动赋值取得资源；源 ID/数值为 0 | 有效源对象，路径处于合法 moved-from 状态 | noexcept；不复制 GPU | texture.cpp |
| `operator=(Texture&&) noexcept` | Renderer 资源替换 | 非自赋值先 Destroy 目标，再转移路径/ID/数值，返回 *this | 有效 context；自赋值不变 | 不提供 context 失效恢复 | texture.cpp / I5 |
| `CreateRegularTexture(string_view, bool)` | renderer / TextureArray | 返回新 2D 纹理，不改 this；解码翻转，配置过滤、上传并恢复 alignment | RGB/RGBA、正尺寸且不超 GPU 上限 | 解码/校验/GL 检查异常上抛；局部析构释放新 ID，无全局事务保证 | texture.cpp |
| `Destroy() noexcept` | owner / 析构 | 删除非零 ID 并清零，其他属性保留 | context 有效，可重复 | 不检查所有 GL 错误 | texture.cpp / I5 |
| `CubeMap::CreateCubeMap(string_view)` | 保留声明，无活动调用 | 尚无实现，不是可用加载 API | 不应调用并宣称已支持 | 调用将缺定义；未运行验证 | texture.h |

### 私有函数预期行为

类无 private 函数；此表覆盖与 TextureArray 共享的 TU helper。

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| `CheckTextureError(const std::string&)` | 2D 分配/上传、数组分配/创建 | 读取 glGetError，非 NO_ERROR 拼接操作名抛 runtime_error | context 有效；可能读取先前 GL 错误 | 上抛，由局部 Texture 析构释放 ID；不恢复全部 GL 状态 | texture.cpp |

### CubeMap

CubeMap : Texture 及 CreateCubeMap 声明保留于 texture.h；未发现定义/活动调用，不算已支持的立方体纹理加载。不是此次新增独立可用对象。

### 继承与审核风险

Texture 析构**非 virtual**；[TextureArray](TextureArray-类设计.md)按具体值销毁，不允许据继承宣称可安全通过 Texture* delete 派生对象。公开 ID/属性依赖内部调用约定。没有并发、多设备或重入保证。

依据：[头](../../../../game/modules/renderer/src/texture.h)、[实现](../../../../game/modules/renderer/src/texture.cpp)、[Image](../assets/Image-数据设计.md)、[Renderer I1/I5](Renderer%20namespace%20API.md#不变量)、[历史验收](Renderer-显式场景输入-功能.md#验收案例)、[覆盖清单](../对象笔记覆盖清单.md)。2026-10-06 仅源码核对，未重新运行损坏图片、移动/驱动失败测试。

## 本次变更

无。

## 后续考虑

CubeMap 需独立实现与验收授权；本次仅明确保留声明边界。

