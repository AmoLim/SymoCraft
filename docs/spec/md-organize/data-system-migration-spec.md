---
type: 文档迁移规格
status: 已整理（UI待验收）
project: Symocraft
module: Documentation
created: 2026-10-05
updated: 2026-10-06
tags:
  - area/spec
---

# 数据与系统笔记迁移 Spec

## 状态与交付边界

提出日期：2026-10-05，当时仅授权 spec 和两份示例。2026-10-06 用户确认“可以开始根据 spec 进行迁移与覆盖”，现已进入正式实施；以下旧校验记录是对应时点的历史快照，不代表当前仍未执行。授权仅限文档、导航和清单，不修改引擎代码，不改变 M3-T0 至 T4 工程验收。

2026-10-06 追加工作规格：对各生产模块已确定对象逐个整理，补齐 GpuTimer 等审核入口。当时只更新 spec，随后按用户确认实施。范围从首批 DOD 适配扩展到下文对象覆盖，原“暂不拆”不再是省略对象理由，不扩大工程重设计范围。

归属规范：[mdspec](mdspec.md)。写法依据：[模板使用说明](../../Obsidian-功能笔记模板/00-使用说明.md)、[07 数据与存储设计](../../Obsidian-功能笔记模板/templates/07-数据与存储设计.md)、[08 系统与数据流设计](../../Obsidian-功能笔记模板/templates/08-系统与数据流设计.md)。当前实现入口：[模块架构索引](../../project/architecture/Overview.md)。

[面向对象与数据导向架构初稿](../project-architecture/面向对象与数据导向架构设计初稿.md)只作为候选设计背景；其中 Application 实例、VoxelWorld、SimulationSession、网格版本或异步机制不得被迁入当前实现章节。

## 目标与非目标

- 按真实设计主体选择功能、类、数据、系统模板，消除为了类模板而将值类型、视图、自由函数和处理管线统称为“类设计”的表达。
- 每个模块的已确定对象均须可定位、可审核：独立对象有自己的笔记，关联值与私有辅助类型有明确覆盖位置，不能只在模块标题、class_name 或成员表中出现一个名字。
- 保留字段语义、所有权、失效条件、流程顺序、失败边界、风险、原验收记录与证据；精简重复表达，而不是删掉尚未解决的问题。
- 保持 Obsidian 唯一板块归属和真实链接；模板类型不成为新的板块、主题标签或虚构的图谱枢纽。
- 不为填表建立 System 基类、管理器、任务调度器、SoA、archetype、命令缓冲、世界快照或新存档格式。
- 不将文档迁移解释为 ECS 正确性升级、并行能力、性能提升、多实例支持或后续阶段完成。

## DOD 语境与填写要求

本 spec 中的数据导向设计明确指游戏引擎语境的 **Data-Oriented Design（DOD）**；用户此前的 DOP 也按这个语义理解，不混用以通用不可变数据为核心的另一套 Data-Oriented Programming 方法论。DOD 不等同于 Unity DOTS，也不要求 ECS、SoA、并行任务、纯函数或不可变组件。对象所有权、RAII 和受控原地更新均可保留。

迁移后的数据笔记必须把实际工作负载与访问模式对应到布局选择；系统笔记必须说明真实处理单位、查询/遍历路径与复制、打包、重排或原地写回。已有纯值契约仍可使用数据模板，但不能因重分类就称为经过 DOD 优化的实现。

规模、访问分布、目标平台、阶段预算与瓶颈没有记录时写“未知 / 未测量 / 无指定要求”，不要编造。若日后提出布局或执行路径优化，另按实验模板记录行为等价、代表性负载和基线/候选测量；本次仅补充文档表达，不新增性能优化任务。

## 模板选择与拆分规则

| 记录主体 | 主模板 | 保留内容 |
| --- | --- | --- |
| 可观察行为与跨主体协作 | 功能 | 范围、流程摘要与唯一验收记录 |
| 真实对象的状态、资源和生命周期 | 类设计 | 成员、持有与销毁、不变量、接口 |
| 值类型、组件、视图、记录集合与存储 | 数据设计 | 工作负载、访问模式与布局对应，语义、身份、有效期和结构变化契约 |
| 查询、批处理、更新阶段与数据变换 | 系统设计 | 读写集、处理单位与数据移动、规则、频率、依赖、提交与正确性 |

同一主体原地维护；只有存在不同维护责任和独立契约时拆分。不按模块机械生成四种笔记。涉及拥有像素、元数据树或采样数组的真实对象，也不因为“成员是数据”就取消其类设计。

## 首批文件对应关系

以下目标相对 `docs/project/architecture/`。原路径以历史文字保留，活链接指向迁移后正文；2026-10-06 已按本表实施。

| 当前笔记 | 计划目标 | 操作与责任分配 |
| --- | --- | --- |
| 原 `scene/Scene-共享场景数据-类设计.md` | [Scene 数据](../../project/architecture/scene/Scene-共享场景数据-数据设计.md) | 改名为数据设计；保留布局、CameraView 副本、MeshView 借用及公开头边界 |
| 原 `simulation/Simulation-类设计.md` | [Simulation 数据](../../project/architecture/simulation/Simulation-数据设计.md) | 改名为数据设计；维护输入/交互值、组件、所有者、有效期及状态负责人 |
| 同上，结合 [Simulation 功能](../../project/architecture/simulation/Simulation-功能.md) 和实际调用方 | `simulation/Simulation-系统设计.md` | 抽出系统入口、查询读写集、处理顺序、每帧/固定步区别、交互提交、暂停与重置；不重复字段表 |
| [ECS 类设计](../../project/architecture/ecs/ECS-存储边界-类设计.md) | 原路径保留 | 聚焦 Registry 独占 Storage、接口转发、不可复制、Free/析构及生命周期；不把 Registry 变成纯数据记录 |
| 同上，结合 ECS 实现与 [存储边界功能](../../project/architecture/ecs/ECS-存储边界.md) | `ecs/ECS-组件存储-数据设计.md` | 抽出实体表、类型注册、组件池、查询、结构变化、引用失效和 T4 未解决风险 |

Scene 和 Simulation 原文件只在正式迁移时移除，同期修正可维护文档中的入链；不长期保留双份正文或默认建立跳转页。ECS 原路径不移动。新系统/数据笔记通过链接连接功能与实际所有者。

### 不变量归属映射

编号是原契约身份，不按新模板的 D/S 提示强制重编号。跨笔记引用必须带所属笔记链接和编号。

| 来源 | 计划唯一维护位置 |
| --- | --- |
| Scene I1 / I2 / I3 | Scene 数据设计；分别保留尺寸、借用有效期和公开头边界 |
| Simulation S1 / S5 | Simulation 数据设计；系统笔记引用值所有权和组件引用有效期 |
| Simulation S2 / S3 | Simulation 系统设计；保留选材/视角与射线返回请求 |
| Simulation S4 | FixedStepBudget 独立类笔记维护原预算边界；Simulation 系统设计引用该编号并说明实际调用频率，不重复维护 |
| ECS I1 / I4 | 原 ECS 类设计；保留唯一所有者和清理边界 |
| ECS I2 / I3 | ECS 数据设计；原类设计与 Simulation 数据设计引用，不再次维护完整条件 |

迁移不把“调用者前提”改成“实现已检查的保证”。若拆分后的条件确需细化，沿用原编号定位旧条件，新条件使用未占用编号，并记录关系；不改动已归档证据中的旧编号。

## 其余现有笔记的处理决定

| 笔记 / 区域 | 本轮计划 |
| --- | --- |
| [Camera](../../project/architecture/simulation/Camera-类设计.md)、[Window](../../project/architecture/platform/Window-类设计.md) | 复用已有独立类笔记，补齐逐对象审核字段与关联辅助类型覆盖位置 |
| [World](../../project/architecture/world/World-类设计.md)、[Generator](../../project/architecture/world/Generator-类设计.md) | World 汇总保留世界存储摘要，Chunk/Block 分别建对象笔记，Generator 复用已有笔记；生成与网格系统专项仍不自动扩写 |
| [Renderer](../../project/architecture/renderer/Renderer%20namespace%20API.md)、[Telemetry](../../project/architecture/telemetry/Telemetry-采样与导出-类设计.md) | 模块汇总降为协作者摘要与导航，GpuTimer/Batch/Shader/ShaderProgram/Texture/TextureArray、Session 分别整理独立笔记 |
| [Assets](../../project/architecture/assets/Assets-资源读取与解码-类设计.md)、[Foundation](../../project/architecture/foundation/Foundation-基础契约-类设计.md) | Image 数据契约和 Value 对象契约分别具备独立审核入口；函数不虚构成类 |
| app、构建契约及其余功能文档 | 如实记录当前 namespace 与值类型；同步对象覆盖导航，不虚构 Application 或构建管理类 |
| `project/benchmark/` | 本次不拆现有类/功能笔记；其协议与实验不因新模板重分类 |
| `legacy/`、`milestones/`、`evidence/` | 不重写历史设计与结论；可维护历史文档仅修正必要导航，证据文件不可改写或移动 |

2026-10-06 对象覆盖追加规格以本表和下文为准。其余未列的管线专项、Benchmark 重分类和全仓库迁移仍不在范围内；不能用对象笔记整理授权引擎实现改动。

## 追加工作：逐对象审核覆盖

### “已确定”与覆盖粒度

- 已实现：有真实定义、所在生产 target 与实际用途；区分活动使用、仅编译保留、未启用及休眠状态。头文件前向声明、测试替身、外部库类型不计作本项目新增对象。
- 已确定采用但未实现：必须引用明确的批准依据；笔记保持草稿，当前设计写“暂无”，批准方案写本次变更。不能把架构初稿里的候选类全部当成已经批准或实现。
- 有独立资源、生命周期、状态不变量或审核责任的对象，每个对象一篇独立笔记。class/struct 语法不决定模板：Shader/ShaderProgram 虽为 struct，仍按资源对象写类设计；Block/Image 等值契约可用数据设计。
- 私有对象也需要覆盖，不能因不在 public include 就漏掉。嵌套数据记录、哈希器、局部 RAII 辅助等可放所属对象的明确小节，不为它们机械建立大量文件；清单必须给出类型名、理由和可跳转的位置。
- 独立笔记只能维护一个主对象，相关成员类型可随笔记解释；不能再用 `Renderer / Batch / Shader / ...` 的 class_name 代替逐对象覆盖。模块摘要仅保留职责、协作和链接，不复制对象全文。

### 初始对象与目标笔记清单

本表是 2026-10-06 只读初筛，不是完成清单。目标文件均相对 `docs/project/architecture/`；执行前必须再次核对生产构建与实际调用链，补齐遗漏，记录未覆盖项而非声称正则扫描已经穷尽全部类型。

| 模块 | 当前已识别主体与源码依据 | 计划覆盖位置 |
| --- | --- | --- |
| foundation | [Data::Value](../../../game/modules/foundation/include/symocraft/foundation/document.h)；现有 AmoBase 是命名空间，不是类 | `foundation/Value-类设计.md`；诊断/旧分配器函数和 DebugMemoryAllocation 记录在原基础契约笔记的明确小节 |
| scene | BlockVertex3D、LineVertex3D、CameraView、MeshView | 复用首批 `scene/Scene-共享场景数据-数据设计.md`，每个数据类型有明确小节；不制造场景管理类 |
| assets | [Image](../../../game/modules/assets/include/symocraft/assets/image.h) 及资源读取函数 | `assets/Image-数据设计.md`；读取/解码行为留功能笔记，链接 Image 的成功/失败数据契约 |
| world | [Chunk](../../../game/modules/world/src/chunk.h)、[Block](../../../game/modules/world/include/symocraft/world/block.h)、[Generation::Generator](../../../game/modules/world/include/symocraft/world/generation.h) | `world/Chunk-类设计.md`、`world/Block-数据设计.md`；复用 `world/Generator-类设计.md`。BlockFormat、Settings/Timings、NoiseState、HashFunction 与测试场景值各注明所属小节 |
| ecs | [Registry、Iterator、RegistryViewer](../../../game/modules/ecs/include/symocraft/ecs/registry.h)、[Internal::ComponentContainer](../../../game/modules/ecs/src/component_container.h) | 原 `ecs/ECS-存储边界-类设计.md` 聚焦 Registry；新增 `ecs/Iterator-类设计.md`、`ecs/RegistryViewer-类设计.md`、`ecs/ComponentContainer-类设计.md`，连接首批存储数据笔记。Storage/SparseSetPool 与保留的 RawMemory/SizedMemory 明确覆盖位置和未启用边界 |
| simulation | Camera、[PlayerMath::FixedStepBudget](../../../game/modules/simulation/include/symocraft/simulation/player_math.h)，组件/输入/交互记录和系统函数 | 复用 `simulation/Camera-类设计.md`，新增 `simulation/FixedStepBudget-类设计.md`；各组件和值类型由首批 Simulation 数据笔记分节覆盖，系统笔记维护处理顺序与引用 |
| platform | [Window](../../../game/modules/platform/include/symocraft/platform/window.h)、[GraphicsBridge](../../../game/modules/platform/src/graphics_bridge/graphics_bridge.h)，输入与内存采样值 | 复用 `platform/Window-类设计.md`，明确 Window::Impl 小节；GraphicsBridge 作为无实例的受限接口在平台功能笔记分节；输入/快照/ProcessMemory 各有覆盖小节 |
| renderer | [GpuTimer](../../../game/modules/renderer/include/symocraft/renderer/gpu_timer.h)、[Batch<T>](../../../game/modules/renderer/src/batch.hpp)、[Shader](../../../game/modules/renderer/src/shader.h)、[ShaderProgram](../../../game/modules/renderer/src/shader_program.h)、[Texture / TextureArray](../../../game/modules/renderer/src/texture.h) | `renderer/GpuTimer-类设计.md`、`renderer/Batch-类设计.md`、`renderer/Shader-类设计.md`、`renderer/ShaderProgram-类设计.md`、`renderer/Texture-类设计.md`、`renderer/TextureArray-类设计.md`；命名遵循实际符号 GpuTimer，不另建 GPUTimer 同义笔记 |
| telemetry | [Performance::Session、SessionConfig、Frame、Memory](../../../game/modules/telemetry/include/symocraft/telemetry/performance.h) | `telemetry/Session-类设计.md`；SessionConfig/Frame/Memory 及 YAML 适配行为在关联数据/功能笔记的明确小节维护，保留与 benchmark 协议的链接 |
| app / startup | 当前 [Application](../../../game/app/src/application.h) 是命名空间；[StartupOptions](../../../game/app/src/startup_options.h)、PreparedWorld 是值结构 | 现有 app 运行编排与 Application 笔记必须注明真实形态并分节覆盖启动/准备值；不虚构已实现 Application 实例。批准未来实例设计时另标候选状态 |
| 构建 / 测试边界 | CMake target、测试入口与 include 白名单，不是引擎对象类 | 原构建笔记保留，不为“每模块一类”新增管理类。测试 fixture 与工具进程对象不纳入生产对象数 |

Renderer 的 Result/Slot、VertexAttribute、DrawArraysIndirectCommand、ShaderVariable/HashShaderVar、RenderStats、DeviceInfo/DeviceMemory 也须逐项给出所属笔记小节，不作为独立资源对象重复建类笔记。CubeMap 当前在 texture.h 中有声明，本次未发现实现或活动调用；列为“声明保留，实施时复核”，不能遗漏，也不能写成已支持的立方体纹理功能。

独立 benchmark 工具保持 `project/benchmark/` 既有职责，不将其 Process/App 等笔记搬入游戏架构制造第二个权威正文；如全局清单提及它们，只链接原覆盖位置并注明工具边界。

### GpuTimer 的强制审核内容

当前 [Renderer 汇总](../../project/architecture/renderer/Renderer%20namespace%20API.md) 仅列出部分槽位成员、约束和简化接口，不是完全没有提及 GpuTimer；缺口是没有独立、完整的对象审核入口。计划从该汇总抽出 GpuTimer 的真实契约，相关 I4 以旧编号和来源记录迁移关系，不因拆笔记删除或重编号其历史依据。

独立笔记至少覆盖以下实际项目，不允许仅用“异步 GPU 计时”一句话代替：

1. 构造/析构、不可复制与实际移动能力；app 中谁持有对象，Renderer/Batch 如何临时借用，为什么必须早于当前图形上下文销毁。
2. `slots_`、`current_`、`supported_`、`active_draw_`；64 个槽、每槽 2 个 query；Slot 的 frame/count/pending 与 Result 的单位、所有权。
3. 全部真实接口：`Supported`、`BeginFrame`、`BeginDraw`、`EndDraw`、`EndFrame`、`Poll`；调用顺序、有效前提、返回值和失败后状态。不得用并不存在的统一 Begin/End 接口替代。
4. 槽位与绘制段的状态变化：开始帧、开始/结束绘制段、标记 pending、检查可用、读取/求和/回收；明确哪些约束由实现检查，哪些只是调用者前提。
5. 不支持查询、池满丢测量、结果未就绪、超过每帧绘制段容量、零绘制段帧和异常/析构时活动查询的实际行为；没有验证的路径明确标注，不能把当前缺少的顺序保护写成已有状态机保证。
6. `Result.frame` 与 app/Session 样本索引的关联、纳秒转毫秒、缺失值处理与退出时结果收集范围；计时范围按 Batch 实际 draw 调用解释，不泛称整帧 GPU 耗时。
7. 公开/私有边界、主上下文线程约束、与 Batch、Renderer、app、telemetry 的关联；性能代价和未测范围。
8. 依据链接：[GpuTimer 实现](../../../game/modules/renderer/src/gpu_timer.cpp)、[Batch 调用](../../../game/modules/renderer/src/batch.hpp)、[app 调用](../../../game/app/src/application.cpp)、[现有单测](../../../test/unit/renderer/gpu_timer_tests.cpp)和原验证记录。源码有测试不等于本轮重新运行通过。

### 全局覆盖清单与审核入口

正式整理新增 [对象笔记覆盖清单](../../project/architecture/对象笔记覆盖清单.md)，现已创建。每个主体一行，包含模块、完整符号、源码定义与构建归属、活动/保留/批准候选状态及依据、主笔记与小节、独立/合并理由、审核缺口和验证依据。

独立对象链接独立笔记；合并的值/辅助类型必须链接专用小节或明确锚点，不能只指向十几个对象混杂的模块总页。全局清单与各模块入口交叉链接，Overview 的模块表区分功能/系统流程、独立对象、数据契约。用户应能从模块入口直接找到 GpuTimer、Batch、Session 等对象，而不必搜索 class_name 或从源码猜测覆盖位置。

规范化符号与文件名只保留一套：源码 GpuTimer 对应 GpuTimer 笔记；必要的旧称在正文解释，不用重复文档解决检索。正式笔记继续使用唯一 `area/architecture`，清单也归同板块；不为每个类新增标签、颜色组或虚构类型。

### 对象笔记的最小审核契约

每篇至少提供真实职责与非职责、成员与所有权、不变量及成立边界、接口前提/输出/失败状态、构造与释放顺序、拷贝/移动、线程/回调/重入、关键状态变化、与调用方/协作者关系、实现和验证依据、已知风险。复杂资源对象用完整类模板，小对象可用轻量模板，纯值与存储使用 DOD 数据模板，不按文件数量虚构设计层次。

整理是归纳已确定设计，不是借笔记补造缺失保证。声明与实现不一致、生命周期依赖、未测试失败路径、内部全局缓存、继承/销毁风险或仅保留的 API 均应写成审核问题；不擅自修复代码，也不把“已整理”标成“已验证”。

## 内容与元数据迁移规则

1. 实施前重读最新工作区与源码，生成原路径、目标路径、入链、约束编号和证据链接清单；不覆盖用户在本 spec 之后的修改。
2. 已实现约定归入当前设计。上一轮已完成变更只保留必要的理由、版本和证据；本次变更只保留仍未落地的候选，已合并则写“无”。未承诺能力留后续考虑。
3. 原验收表保留在功能笔记的唯一记录处；设计笔记只链接案例及相关约束。文档静态检查不替换旧游戏验收、不补勾未运行的案例。
4. 改为数据/系统设计时更新 `type` 并移除不再适用的 `class_name`、`inheritance`；保留原 `project`、`module`、`created`、已有兼容标签和原验证范围。拆出的新笔记使用实际创建日期，注明来源及其证据，不冒充独立验收通过。
5. 正式实现笔记仅用 `area/architecture`；spec 用 `area/spec`；示例用 `area/templates`，并保留 `示例` 标识。不增加第二个 `area/*` 或 DOTS 类型 tag；模板源文件仍不预填板块。
6. 逐一重算移动后的源码、报告、图片和同模块链接；检查 Markdown、Obsidian 双链、显式锚点与正文路径引用。更新模块索引、功能关联、总入口、结构参考、spec 和标签清单中真正受影响的项，不做无关措辞重写。
7. 查询、结构变化、数值与时间语义按真实实现填写。尤其保留有序指针事件逐项裁剪、Character 每帧更新、Physics 每帧消费一次预算后执行子步、交互冷却每帧递减和同帧世界提交。
8. 存储布局、线程安全、分配失败、ID 代数、删除压缩等未知或未验证事项明确列出；没有证据就不把期望补成当前保证。发现原文与源码不一致时记录依据和影响，不能默默改写历史验收结论。

## 代表性示例交付

示例保存到实际已有目录 `docs/Obsidian-功能笔记模板/examples/`，不新建第二套 `docs/模板/` 目录，不修改 Obsidian 模板配置。

| 示例 | 来源与演示重点 |
| --- | --- |
| [Symocraft-Scene-数据设计示例](../../Obsidian-功能笔记模板/examples/Symocraft-Scene-数据设计示例.md) | 从 Scene 笔记及公开头抽出顶点、相机值与网格视图；演示无需虚构类的值契约、尺寸与借用失效，以及“不适用”项 |
| [Symocraft-Simulation-系统设计示例](../../Obsidian-功能笔记模板/examples/Symocraft-Simulation-系统设计示例.md) | 从 Simulation 笔记和 app 实际调用链抽出普通玩法更新；演示读写集、逐事件/每帧/固定步、同帧编辑和串行限制 |

两份示例采用对应新模板的三段结构，元数据分别为 `type: 数据设计` / `系统设计`、`status: 草稿`、`project: Symocraft`、实际 module、`tags: [示例, area/templates]`。正文标明摘录日期、来源、核对与未测范围；不是新的生产设计权威副本。

本次变更写“无”，表示示例没有提议产品改动，不是假定迁移已经完成。验收链接原功能笔记与 T0 报告，不复制 `[x]` 或杜撰新实验。示例内部编号沿用来源以方便核对，但仅供展示，不承担产品约束维护。

示例是教学快照：当生产笔记迁移时只修正来源链接；仅在专门更新示例时重新核对摘录事实并注明日期。不能通过复制示例把 `area/templates` 或未核对的状态带入生产文档。

## 后续实施顺序

1. 用户确认正式迁移后，重新盘点并保存迁移前的属性、契约、验收和证据快照。
2. 先迁移 Scene 数据笔记，核对旧 I1-I3 和所有入链，再迁移 Simulation 数据/系统拆分。
3. 抽出 ECS 存储数据笔记，保留 Registry 类笔记，并落实 I1-I4 唯一维护位置。
4. 先补 Renderer 的 GpuTimer、Batch 等独立资源对象笔记，再按覆盖表整理其他模块的已确定对象；复用已有独立笔记，不保留两套权威正文。FixedStepBudget 独立笔记落实 S4 的唯一维护位置。
5. 建立全局对象覆盖清单，同步模块入口、互链、标签清单和模板选择规则；旧汇总保留必要协作摘要和迁移编号出处，不改候选架构设计的批准/实现状态。
6. 执行下表静态检查，记录实际通过项与未完成项；再单独核对 Obsidian 中目标、标签和双链的呈现。

## 验收标准

### 本轮 Spec 与示例

- [x] spec 明确目标、非目标、首批映射、保留/暂缓项和后续实施授权边界。
- [x] 两个示例已保存，主体、字段、流程与抽取来源一致，没有候选能力混入。
- [x] 新文件 YAML 可解析、归属唯一、无错误类属性和未替换模板占位符；已纳入标签清单。
- [x] 本轮新增导航目标和锚点可解析；未写入旧模板、原示例、生产笔记、代码、证据或 Obsidian 设置。用户同期修改单独记录，不覆盖或回退。

### 正式迁移阶段

- [x] 首批目标文件和唯一正文维护关系符合文件表；不存在遗留旧正文或错位入链。
- [x] 原字段含义、I/S 编号、所有权、借用/失效、时间顺序、失败后状态与未解决风险均可追溯。
- [x] 原验证范围、游戏验收状态、证据路径与文件内容不变；新增摘录不冒充新测试。
- [x] YAML、唯一板块、Markdown/双链/锚点及源码链接检查通过；标签清单与实际路径一致。
- [ ] 在 Obsidian 检查正式笔记属于 architecture，示例属于 templates，spec 属于 spec；未作 UI 验证则明确保持待验收。

### 逐对象覆盖阶段

- [x] 逐模块复核源码定义、生产 target 与使用状态；每个已确定对象均有一行覆盖记录，没有用前向声明、测试替身或候选初稿冒充已实现类。
- [x] 独立对象均有单对象笔记；辅助/数据类型均有明确小节和合并理由。GpuTimer、Batch、Shader、ShaderProgram、Texture、TextureArray、Session 等不能仅靠模块汇总算覆盖完成。
- [x] GpuTimer 独立笔记完整覆盖上述八项，与实际 API、成员、调用链和测试入口一致；未测或缺少保证的项明确可审核。
- [x] 从 Overview、模块入口和覆盖清单均能找到对象笔记；所有链接/锚点唯一可解析，不存在 GPUTimer/GpuTimer 两套同义正文。
- [x] 原 I/S 编号及验收引用可追溯，拆分后的契约有唯一维护位置；功能验收和性能实验不重复、不新增未经运行的通过状态。
- [x] 生产笔记拥有唯一 `area/architecture`，清单登记与实际文件一致；未改程序、构建、证据与无关 Obsidian 设置。

## 本轮校验记录

- 2026-10-05 已交付本文和两份示例；在 mdspec 与示例维护说明中增加入口，标签清单仅追加三篇的归属记录与相应计数，没有执行正式笔记迁移。
- 三篇新文档的 YAML、类型、草稿状态、唯一板块、无类属性、无模板占位符检查通过；两个示例均有当前设计/本次变更/后续考虑，未复制已通过的验收勾选。
- 新文档的 48 个仓库内链接和 3 个锚点检查通过，模板维护说明相关导航复查通过。未重新宣称整个 docs 的历史链接均有效；旧 D3D12Lab 的两条外部本机来源链接继续保留。
- 迁移前后聚合 SHA256 对照：`docs/project/`、`legacy/`、`milestones/`、8 个模板源文件、`game/`、`cmake/`、`test/` 及原两个 D3D12Lab 示例均未改变。首批计划目标文件尚未生成。
- 校验期间 `.obsidian/` 与已有候选架构 spec 检测到同期变化，本轮没有写入这两处，保留用户改动；不把它们记录为字节不变或将其恢复到旧快照。
- 标签清单的 82 篇/默认视图 73 篇仅是原 79 条清单加本轮三篇后的登记计数，不是全 vault 重新盘点或 UI 实测；其他同期新增、移动笔记不在本轮批量重分类范围。此前 staticVerification 与 M3-T0 快照记录不改写。
- 未运行游戏构建、模拟、玩法或性能测试，未在 Obsidian 做 UI 验收；正式迁移验收项全部保持未完成。静态检查脚本位于 `out/docs-audit/validate-migration-spec.cjs`，不属于产品实现。

### DOD 语义校准

2026-10-05 用户明确本项目的 DOP 实际指游戏引擎语境的 DOD。两个模板原本面向引擎存储与处理管线，但偏重正确性契约；本次明确术语，并把工作负载、字段访问与布局对应、真实批处理和数据移动提升为核心填写内容。使用说明、本文及两个示例同步更新，不要求不可变数据或纯函数，不生成新的优化任务。

模板静态校验、示例/本 spec 的 48 个仓库内链接和 3 个锚点复查通过；规模、平台或瓶颈未测项仍如实保留。生产笔记、程序源码、证据和 Obsidian 设置未写入，正式迁移仍未执行。已有候选架构 spec 的同期变化保留，未由本次模板校准覆盖。

### 2026-10-06 对象审核覆盖追加

用户要求各模块已确定类均能逐个审核，明确提出 Renderer 中 GpuTimer 缺少独立笔记。本文追加初始对象/目标清单、私有与辅助类型覆盖规则、GpuTimer 必填审核项、全局覆盖索引及待执行验收，并相应更新原暂缓安排与 S4 归属计划。本轮只读核对相关定义和调用，不创建目标笔记、不迁移生产正文、不执行引擎构建或运行测试；完成的是工作 spec 更新，不是对象整理验收。

本轮仅修改本文及 mdspec 的对应规格入口。静态校验通过：本文与两份既有示例的 71 个仓库内链接、3 个锚点可解析，YAML 与登记属性仍一致；差异空白检查通过。此前的 48 链接记录保留为当时快照，不改写成全量 docs 或运行验收结果。

### 2026-10-06 正式迁移与覆盖实施

用户随后明确授权实施。Scene 与 Simulation 原类笔记直接改名为数据笔记，旧文件移除、可维护入链同步；新增 Simulation 系统、ECS 存储数据和 15 个单主体笔记。原 Camera/Window/Generator/Registry 复用补充；模块汇总仅保留协作/数据摘要与真实门面形态，完成变更归入当前设计，本次变更写“无”。

[对象笔记覆盖清单](../../project/architecture/对象笔记覆盖清单.md)登记 76 个主体（含数据、私有辅助、门面与保留声明，不是 76 个独立类）；[模块入口](../../project/architecture/Overview.md)直接链接各资源对象。architecture 笔记由 26 篇变为 44 篇。新建/拆出的笔记均为草稿，说明仅源码核对；旧已验证状态仍限定 T0。

新增笔记明确记录 Shader/ShaderProgram 无资源释放析构、Texture 非虚析构、TextureArray 实际 64×64 tile/GPU atlas 复制、GpuTimer 缺少完整顺序保护及零绘制段语义、Registry 版本/Iterator 截断/组件删除与分配失败风险；不以原注释或期望补造当前保证，不改引擎实现。

模板示例只修正生产来源链接，教学快照的摘录日期与草稿身份不变。候选架构稿没有改为已批准/实现，benchmark 协议/实验、legacy/阶段历史、模板源和证据不迁移重写。

静态核验通过：50 篇范围文档（44 篇 architecture + 两份 spec/两个示例/两个总入口）的 YAML、唯一板块、698 个仓库内链接和 142 个锚点目标可解析；范围内无双链实例，检查器支持双链解析但不宣称已实测 UI 双链呈现。76 行覆盖记录均有真实目标；扫描发现的 68 处 class/struct 定义（包括保留辅助/声明形态）与人工核对范围无遗漏；不是 AST 完整语义或运行证明。

迁移前快照与收尾对照：740 个受保护文件字节未变（game/cmake/test、旧模板源、legacy/阶段证据与 Obsidian 设置）；原架构功能验收表勾选行逐字一致。标签清单仅替换 architecture 项，现登记 102 篇/默认视图预期 92 篇，其中 architecture 44；保留原 staticVerification 和 M3-T0 快照，不代表全 vault 重分类或 UI 实测。

静态结果：[标签清单](tag-inventory.json)；本机可复查脚本/快照/报告位于 `out/docs-audit/migrate-architecture.cjs`、`validate-architecture-migration.cjs`、`architecture-before.json`、`architecture-validation.json`（忽略的审计输出，不属产品）。Obsidian UI 未执行，正式迁移的 UI 验收继续未勾选；游戏构建/运行/性能与故障注入均未运行。本节不改变旧校验时点或旧游戏验收勾选。
