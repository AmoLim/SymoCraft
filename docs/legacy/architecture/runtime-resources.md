---
tags:
  - area/legacy
  - topic/resources
  - topic/rendering
---

# 运行期图形资源与失败处理

> 历史架构参考：M2-A。文中的“当前”指该阶段，不代表最新实现；归档不表示相关机制全部废弃。源码链接固定到迁移前 `cadc349` 快照，保留旧路径定位；该快照不等同于本篇原阶段的精确版本，历史结论仍以原报告为准。参见 [历史架构索引](README.md)；后续设计约定见 [项目范围](../../spec/project-scope.md)，不据此推定重构已经完成。

## 范围

本文说明 M2-A 对窗口、OpenGL 上下文、shader、纹理及选择框的修改。它是代码行为与接口约定，不是运行测试通过记录。构建、真实 GPU 启动、故障注入和玩法结果由 M2 集成报告记录；不能从“加入清理代码”推断已经证明没有泄漏。

资源定位沿用 M1 的 [Assets 模块](asset-paths.md)，不改变工作目录。`--check-assets` 只核对部署所需文件存在，不代替 shader 编译、YAML 解析、图像解码或 GPU 分配验证。

## 创建顺序与失败边界

```text
Application
  -> Window::Init                    GLFW 库初始化
  -> Window::Create                  窗口、当前 GL 上下文、GLAD
     -> 输出实际 GL vendor / renderer / version
  -> Renderer::Init                  shader、Batch、方块配置
  -> Application::Run
     -> TextureArray::CreateAtlasSlice
     -> 主循环
```

[Window::Init/Create](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/core/window.cpp) 失败时抛出带操作说明的异常，不返回一个可供主循环继续使用的无效窗口。`Create` 用临时 `unique_ptr` 持有 Window；创建 GLFW 窗口之后任一步骤失败，会销毁该窗口并删除临时对象。它不调用 `glfwTerminate`，因为 GLFW 库的生命周期由 Application 的统一清理负责。

GLAD 只在窗口上下文建立后加载一次。Renderer 要求已经存在当前上下文和可用函数入口，不重复加载 GLAD。项目仍要求 OpenGL 4.6，没有在本阶段添加较低版本的兼容渲染路径。

## 释放顺序与所有权

```text
Run 返回或异常展开
  -> 局部 TextureArray 析构，释放纹理
Application 统一清理，仍保留当前 GL 上下文
  -> ChunkManager::FreeAllChunks：清空区块 CPU 数据
  -> Renderer::Free：Batch、两个 shader program
  -> 其他 CPU 资源
  -> Window::Destroy
  -> Window::Free：glfwTerminate
```

这个顺序的重要条件是：**GL 对象的删除必须发生在所属上下文仍有效时**。C++ 析构本身不能延长 OpenGL 上下文的寿命。不要把拥有纹理的对象改成静态对象，或让它活到 Window 销毁之后。

| 类型 | 所有权规则 | 释放方式 |
| --- | --- | --- |
| Window | Create 成功后交给 Application；Create 失败自行回收临时窗口 | `Destroy` 幂等，清零 `window_ptr`；`Free` 只在 GLFW 已初始化时终止库 |
| Shader | 不可复制；句柄初值为 0；编译过程失败会释放已创建句柄 | 显式 `Destroy` 幂等 |
| ShaderProgram | 不可复制；局部编译/链接失败回收两个临时 shader 和临时 program | Renderer 在上下文有效时显式 `Destroy`；不会在静态析构阶段隐式调用 GL |
| Texture / TextureArray | 不可复制、可移动；移动转移句柄并将源句柄清零 | 析构自动调用幂等 `Destroy`，支持异常展开 |

[Texture](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/include/renderer/texture.h) 的路径字段现在拥有 `std::string`，不再保留外部 `string_view`。工厂入口仍接收 `string_view`，但立即复制路径。现有 `texture_array = texture_array.CreateAtlasSlice(path, true)` 可以继续使用，右侧临时对象通过移动交出唯一的 GL 句柄。

ShaderProgram 的 uniform 位置仍有缓存。删除 program 时清空缓存，后续使用按需重新查询，避免 OpenGL 重用数值句柄后沿用旧位置。全局 program 保留显式释放方式，是为了由 Application 控制上下文与资源的先后关系，不是把退出清理交给不可控的全局析构顺序。

## 内容与 GPU 错误

- [Shader::Compile](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/renderer/shader.cpp)：文件无法打开、空文件、读取失败、未知 shader 类型或 GL 编译失败均抛异常。错误包含资源路径；编译失败附驱动日志，日志长度为 0 也不越界访问缓冲区。
- [ShaderProgram::CompileAndLink](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/renderer/shader_program.cpp)：任一 shader 失败会释放另一已创建 shader；链接失败回收临时 program 并附链接日志。成功时才替换当前 program。
- [Texture 工厂](https://github.com/AmoLim/SymoCraft/blob/cadc349aa752e252454a481c3199281c112998c9/src/renderer/texture.cpp)：首先检查解码结果，再读取通道和尺寸。当前接收 RGB/RGBA 图像，使用对应的 8 位标准化纹理格式，并检查 GPU 尺寸和纹理数组层数上限。
- 图集尺寸必须是 64 的整数倍，层数必须有效；不能把尺寸错误静默截断成不完整图集。纹理上传前按字节对齐设置 `GL_UNPACK_ALIGNMENT`，随后恢复原值。当前只分配与过滤配置实际使用的一级纹理，不再对一级存储生成无用 mipmap。
- CPU 解码像素由带 `stbi_image_free` 删除器的 `unique_ptr` 管理。GL 分配或上传错误抛出时，局部纹理的析构负责回收已有句柄。
- Renderer 在 `LoadBlocks` 异常外补充“方块配置加载失败”语境；具体 YAML 结构、方块编号及纹理索引有效性仍属于方块配置模块的职责，不能用解析成功替代语义校验。

文件存在检查与内容检查发生在不同层次。检查后文件仍可能变化，因此正常加载路径也必须报告失败；不应把 preflight 当作对后续读取永不失败的保证。

## 窗口大小与选择框

Window 使用 GLFW **framebuffer size** 回调保存实际绘制像素宽高并更新 viewport，而不是把逻辑窗口大小直接当作像素大小。初次创建也读取 framebuffer 尺寸。最小化时宽高可为 0，投影宽高比计算使用至少 1 的分母和分子避免除零；恢复后的实际尺寸由回调更新。

选择框使用 `LineVertex3D` 的浮点坐标和真实成员偏移。对命中的方块先取整数格原点，再围绕该方块中心轻微放大边框，以减少与方块面深度重合。它不再依赖未正确初始化的缩放矩阵，也不会按世界原点缩放导致远处选择框偏移。

## 诊断与验证边界

Debug 请求 GLFW debug context，并在可用时启用同步 OpenGL debug callback。非通知级消息写入 stderr，包含类型、严重程度、编号和驱动文本；回调不抛 C++ 异常穿过 C API。释放 Renderer 时注销回调。

建议集成验证：

1. 正常启动，保存实际 GL 设备日志；检查退出前释放顺序。
2. 在隔离发布副本破坏一个 shader、图片或 YAML，确认清楚报错、非零退出且不进入主循环；保留原始资源。
3. 调整窗口大小、最小化并恢复，核对 viewport、宽高比和可操作性。
4. 在正负坐标、近远位置观察选择框，确认与命中方块一致。
5. 重复退出和重新启动，结合 GL 诊断与内存检查；一次正常退出不能证明长期无泄漏。

本阶段没有完整迁移旧 shader/图像/YAML 加载器的窄字符串路径接口。Assets 的 Win32 宽字符定位成功，不等于所有加载器都支持任意 Unicode 安装目录；这一限制须独立验证和处理。并发上传、上下文共享和后台资源线程不在本次修改范围内。
