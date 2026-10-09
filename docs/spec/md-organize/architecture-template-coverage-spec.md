---
type: 文档整理规格
status: 铺开中（行为表 28/43，非全量完成）
project: Symocraft
module: Documentation
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/spec
---

# Architecture 模板与函数行为覆盖 Spec

## 状态与范围

2026-10-06 根据聊天中的简单 spec 归档，当时仅授权保存。随后实施模板与四篇样板及系统 Mermaid 表达；用户再要求“继续进行spec的工作吧”，已授权铺开，不再等待样板实施确认。当前 43 篇适用设计/功能笔记中 28 篇具备两类行为表，15 篇尚待整理，不能标成全量完成。前序实施段落保留当时快照，当前进度以文末铺开记录为准。

归属依据：[mdspec](mdspec.md)。前序工作：[数据与系统笔记迁移 Spec](data-system-migration-spec.md)。审核入口：[模块架构索引](../../project/architecture/Overview.md)、[对象笔记覆盖清单](../../project/architecture/对象笔记覆盖清单.md)。

目标：让每个已确定主体的公开接口与内部函数都能逐项审核，清楚区分职责、正常行为、前提、失败后状态与依据。前序术语统一解决了名称表达，但未解决公开/内部函数混写、多个函数压缩在一段文字中等结构问题。

范围为现有 `docs/Obsidian-功能笔记模板/` 及 `docs/project/architecture/`；只修改真正受影响的说明、导航和登记。技术实验、问题修复模板不承担 architecture 接口覆盖，本轮不改。不修改引擎实现，不引入产品机制，不改写历史验收结论。

## 模板清单

模板源使用实际已有目录 `docs/Obsidian-功能笔记模板/templates/`，不另建 `docs/template/` 或 `docs/模板/`。

| 模板 | 调整 | 对应当前 architecture |
| --- | --- | --- |
| [09-namespace API设计](../../Obsidian-功能笔记模板/templates/09-namespace API设计.md) | 新增：公开入口、内部函数、namespace 状态及初始化/清理顺序 | Renderer、ChunkManager、Application，以及 Assets 等函数集合 |
| [10-构建与依赖契约](../../Obsidian-功能笔记模板/templates/10-构建与依赖契约.md) | 新增：CMake 入口、内部检查函数、target 可见性及生成产物 | 构建契约设计 |
| `03/04-类设计` | 补齐：公开接口与私有函数分别记录，消除重复接口表 | GpuTimer、Window、Registry、Camera、Batch 等 |
| `07-数据与存储设计` | 补齐：数据访问接口、内部存储操作及失效规则 | Scene、组件存储、Image、Block 等 |
| `08-系统与数据流设计` | 补齐：系统入口、内部算法函数及执行顺序 | Simulation、Physics 等 |
| `01/02-功能` | 补齐：公开入口与内部处理入口的索引表，链接权威行为表 | 模块功能、跨对象流程与验收 |

不为 facade、PImpl 或 singleton 另建模板；按真实对象或 API 形态使用上述模板。数据/系统模板继续采用游戏引擎语境的 Data-Oriented Design（DOD），不要求纯函数或不可变数据。

## 统一行为表

所有用于 architecture 的模板必须分别保留以下两个章节，不能用一个混合表代替。

### 公开接口预期行为

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 待填 | 模块消费者或类调用方 | 明确写出返回什么、修改什么 | 生命周期、线程、输入范围 | 异常、错误值、部分完成及能否再次使用 | 定义链接、已有不变量编号 |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 待填 | 谁调用、在哪个阶段调用 | 明确读写状态、关键分支及返回结果 | 内部状态要求、禁止操作 | 是否上抛、保留哪些状态、谁清理 | 定义链接、已有不变量编号 |

### 填写要求

- 遵循 [mdspec：文稿风格](mdspec.md#文稿风格)：默认精简、精准，保留逐函数契约；用户显式指出阅读不明确的位置后再定点扩充。
- 每个已定义的函数都有明确审核落点；行为不同的 overload 分开，不用“其他辅助函数”笼统归纳。调用其他主体的函数时链接其权威笔记，不重复抄写。
- 引用、指针、view 和 handle 的返回行为必须说明所有权、有效期及失效条件。
- 区分实现保证、调用者前提、目标行为和已知缺口。当前设计只写现有实现；目标行为放本次变更，不能把期望写成已有保证。
- 源码核对不等于运行测试通过。未核对或未验证的部分明确标注，不编造失败恢复、线程安全或性能收益。
- 没有公开或私有函数时，表内写“不适用及原因”，不虚构接口。纯数据类型另说明构造、复制、移动和字段访问语义。
- 功能笔记保留两张入口索引表，详细契约链接到唯一权威笔记，不重复维护完整函数契约。
- 沿用常见英语技术术语，保留中文说明；普通生命周期、初始化和清理说明不强制翻译。

## 流程表达

2026-10-06 用户追加要求：系统设计模板与样例的调用顺序、提交及其他明显流程优先用 Mermaid，再用文字补充必要说明。函数行为表、不变量与验收记录仍保留，不被图替代。

- 用 flowchart 表达实际顺序、条件分支、循环或提交边界；节点标明 owner/入口，边写明必要判断，图的适用路径单独说明。
- 系统内部处理与跨阶段调用按需要分图，不把同一流程重复写成图、长段文字和伪代码；数值、前提、失效与失败后状态用文字补充。
- 不画出未实现的调度、异步队列、屏障或回滚；虚线含义显式说明，正常返回不等于成功确认。
- 当前设计的图只记录现有实现，候选流程放本次变更。教学示例说明表达更新日期，保留原摘录与运行验收范围。

## 分类边界

namespace 没有 C++ 的 public/private：按模块公开头、内部共享头、translation-unit 内部 helper 区分，并在表格中标明实际调用范围。类的 public 成员也不自动等于模块公开 API；protected 函数注明可见性，纳入内部契约覆盖。

每篇笔记确定一个主模板：描述调用边界用 namespace API，描述对象用类设计，描述布局用数据设计，描述算法与阶段顺序用系统设计，描述构建关系用构建契约。其余内容通过链接关联，避免两套权威正文。

namespace API 模板记录实际状态负责人、借用关系与启动/使用/清理顺序；不据此认定实现了 Facade pattern、实例类或 PImpl。构建模板记录 configure/build 等实际阶段，不虚构运行时构建管理器。

## 实施顺序

1. 取得实施确认后，重新读取最新模板、笔记和源码，保留用户已有修改。
2. 新增两个模板，补齐现有 architecture 模板与选择说明，不改变无关模板。
3. 先用 Renderer、GpuTimer、Simulation、构建契约各整理一篇样板，分别覆盖 namespace API、类、系统与构建契约。
4. 确认样板清晰后再铺开；纯数据主体用不适用说明演示无函数情形，不能为满足表格虚构设计。
5. 核对函数审核落点、权威正文关系、导航和标签，记录静态检查结果与未验证项。

## 保护与验收

- [x] 两个新模板已建立，既有 architecture 模板均具备公开接口与私有函数两个独立章节。
- [ ] 四篇样板能逐项找到函数、看懂正常及失败行为、定位源码；其余笔记在确认后按相同规则整理。
- [ ] 每个函数有明确审核落点，无函数主体有不适用原因，功能笔记未复制第二套权威契约。
- [ ] 文件名与旧链接默认保留；tags、不变量编号、原验收状态及证据关系保持原义。需要调整分类属性时按真实主体核对，不扩大旧验证范围。
- [ ] 正式笔记唯一 `area/architecture`，spec 唯一 `area/spec`；模板源不预填板块标签，示例仍归 `area/templates`，不新增每类标签或图谱颜色。
- [ ] YAML、Markdown/双链/锚点与源码导航静态检查通过；没有把文档整理标成引擎或 Obsidian UI 验收通过。
- [x] 未修改引擎代码、原始证据或历史验收结论；本次工作与未来产品设计明确分开。

用户已授权继续，不再因样板确认阻塞；以上未勾选项仍包含全量覆盖验收，不能因已整理部分静态检查通过就全部勾选。归档文件存在不代表全量整理完成。

## 2026-10-06 模板与四篇样板实施

新增 09/10 两份模板，补齐 01/02/03/04/07/08 六份模板。八份 architecture 模板均有两个独立行为表；03 的重复接口表已删除，05/06 实验与修复模板不变。选择说明、类设计说明与样板索引同步，旧 examples 保留教学快照，不复制第二套生产正文。

| 样板 | 公开表记录 | 私有表记录 | 当前审核重点 |
| --- | --- | --- | --- |
| [Renderer](../../project/architecture/renderer/Renderer%20namespace%20API.md#公开接口预期行为) | 15 个公开入口 | 1 个 Debug callback | namespace API 实际形态、初始化与重载部分失败、GPU 与借用边界 |
| [GpuTimer](../../project/architecture/renderer/GpuTimer%20Class.md#公开接口预期行为) | 8 个可调用接口 + 2 个删除的特殊成员 | 明确不适用：无私有函数 | 非完整 state machine、槽回收、零绘制段与 Poll 部分完成 |
| [Simulation](../../project/architecture/simulation/Simulation-系统设计.md#公开接口预期行为) | 18 行，含 2 行链接 FixedStepBudget 独立契约 | 7 个 Physics static helper | 公开 PlayerMath 不误标私有、内部碰撞行为、跨调用预算负责人 |
| [构建契约](../../project/architecture/build/构建契约设计.md#公开接口预期行为) | 4 个项目集成入口 | 4 个检查器 helper，注明测试授权 | CMake 实际作用域、configure/generate/build、无失败事务回滚 |

Renderer 的 type 按真实主体改为 namespace API设计，构建契约改为构建设计；其余属性、板块、文件名和旧标题锚点保留。私有表覆盖内部函数，不把私有成员或其他对象的方法虚构为本主体函数。表格按当前实现填写，缺口不当作新增保证。

本轮静态检查通过：八份模板的 YAML、无预填板块、三段结构与两类行为表；四篇样板的六列表格、公开/内部函数审核落点、源码/文档链接和锚点；旧标题、原链接、非类型属性、不变量定义行与验收行保持。检查器辅助发现，数量不是 AST 完整语义或运行证明。

标签清单仅追加两个 reusable-template 例外，不改变纳管笔记数量、历史 staticVerification、图谱设置或颜色。当前工作区快照对照中，game/cmake/test、05/06 模板、旧示例及证据未变；检测到 Obsidian workspace、用户开发笔记和候选架构稿同期变化，未写入或回退这些内容，不宣称它们字节不变。

未运行引擎构建、玩法、性能、故障注入或 Obsidian UI 测试。审计脚本/快照/报告在忽略目录 `out/docs-audit/template-coverage-audit.cjs`、`template-coverage-before.json`、`template-coverage-validation.json`。下一步先由用户审核四篇样板，再整理其余 40 篇 architecture 笔记中的适用内容；索引/覆盖清单不强行套成函数设计笔记。

### 系统流程 Mermaid 表达追加

用户随后要求修改系统模板与样例。08 模板以两张 Mermaid 图分别表达内部处理与跨阶段调用/提交；Simulation 教学示例和实际样板以两张图分别表达普通玩法主调用链与 UpdateInteraction 的同步编辑分支。核对 app 实际顺序：逐事件输入、每帧 Transform/Character、每帧一次预算消费及 0..8 子步、重生后相机同步、可选编辑与选择框、网格重建及同步复制、渲染与呈现。图未将 void 世界接口正常返回标成编辑成功。

写作说明同步改为明显流程优先 Mermaid、文字补充必要边界。原函数行为表、元数据、约束与验收行保留；未开始其余 architecture 的全量整理，未新增引擎测试或 Obsidian UI 验收结论。本轮 Mermaid 语法与文档静态结果单独保存在忽略目录 `out/docs-audit/mermaid-flow-validation.json`，不改写前序样板校验快照。

6 幅 Mermaid 图经 Mermaid 解析器语法校验通过，范围文档的原表格行、元数据、旧标题和引用保持。用户已改名的 Renderer/GpuTimer 及其 Obsidian 最短路径 Markdown 链接保留，短链接目标在 vault 内唯一可解析；不将这种校验表述成任意 Markdown 浏览器都支持的相对路径。未在实际 Obsidian UI 验证渲染；检测到 workspace 设置同期变化，未写入或回退。校验依赖只安装在忽略的审计目录，不改项目依赖。

## 2026-10-06 行为表铺开记录

用户授权继续后，重新读取笔记及对应源码，新增/整理 24 篇适用笔记；加上前序四篇，累计 28/43。architecture 当前 46 篇，另外三篇为 Overview、对象笔记覆盖清单、App 休眠源码清单，不强套函数模板。前序“四篇之后其余 40 篇”包含这三个导航/清单，适用数量应为当时 37 篇；本次新建两个 API 权威主体后总适用数量为 43。

| 模块 | 本次处理 | 当前行为表状态 |
| --- | --- | --- |
| renderer | Batch、Shader、ShaderProgram、Texture、TextureArray 逐项契约；功能入口索引与调用图 | 8/8；原 Renderer/GpuTimer 名称保留用户修改 |
| assets | 原摘要按实际主体改 type 为 namespace API设计；7 个公开函数、2 个 TU helper；Image N/A/值语义，功能索引 | 3/3；旧文件名、旧标题锚点与 I1-I3 归属保留 |
| simulation | Camera 14 个显式方法、FixedStepBudget 两项、RigidBody::zero_forces、纯数据 N/A、功能索引 | 5/5；系统样板不复制为第二套正文 |
| scene | 纯数据两表及 span 别名/构造复制移动说明；功能借用/复制流程图 | 2/2；不虚构 Scene manager |
| platform | Window 22 项显式成员 + 3 项输入接口；Impl、两 helper、四 callback；功能索引 | 3/3，含新增 [Platform API](../../project/architecture/platform/Platform-namespace-API.md) |
| telemetry | Session 11 项（两个 Add 分开）、Output/Optional；数据 N/A 与功能索引 | 4/4，含新增 [Telemetry API](../../project/architecture/telemetry/Telemetry-namespace-API.md) |
| foundation | Value 24 项显式接口，构造与索引 overload 分开；as visit 内部分支 | 1/3；诊断/旧分配器两篇未完成 |
| build | 功能入口链接原构建权威表，configure 分支用 Mermaid | 2/2；不新增运行验证 |
| world / ecs / app | 未补齐本轮行为表 | 0/5、0/6、0/2；原对象覆盖不等于函数逐项覆盖 |

两个新 API 笔记避免把 SampleProcessMemory/GraphicsBridge 混成 Window 方法，或把 Statistics/YAML adapter 混成 Session/Frame 方法。内部共享头的 struct public、公开头自由函数、TU helper 与 callback 均明确区分；不因为只有供内部调用就误标 C++ private。模板不增加类别，复用已有 09。

资源绘制、编译/链接、atlas 切片、路径/解码、输入发布、采样导出/文件发布等明显流程优先 Mermaid。函数表维护正常与失败状态，图不替代错误保证。明确保留 Shader/ShaderProgram 无 GPU 释放析构、Texture factory 返回新值、ReadBytes 不自动 Resolve、Poll 延迟输入异常不是恢复协议、CSV/summary 不属全套原子事务等现状。

### 剩余实施队列

以下 15 篇继续按现有授权整理，无需再次确认样板；不得仅插 N/A 行来宣称完成。

| 顺序 | 模块与剩余文件 | 核对重点 |
| --- | --- | --- |
| 1 | foundation：Foundation-基础契约-类设计.md、Foundation-基础契约.md | AmoBase allocator/logger 全部入口、DebugMemoryAllocation 运算符及 TU helper、Publish；保持旧分配器缺口 |
| 2 | world：World-类设计.md、Chunk-类设计.md、Block-数据设计.md、Generator-类设计.md、World-功能.md | namespace API、配置与格式、编辑反馈、生成/网格流程、私有 helpers，区分当前实现与 T1 目标 |
| 3 | ecs：ECS-存储边界-类设计.md、ComponentContainer-类设计.md、Iterator-类设计.md、RegistryViewer-类设计.md、ECS-组件存储-数据设计.md、ECS-存储边界.md | 活动接口与保留序列化分别记录，overload、引用失效、版本/删除/OOM 缺口；不能补造 T4 保证 |
| 4 | app：Application-类设计.md、App-运行编排-功能.md | namespace 启动/运行/退出、CLI/main/helper 审核落点，普通/采样/暂停/失败路径与生命周期 |

### 静态检查与保护

本次使用新快照与独立报告，不重写旧样板/迁移验收。检查器区分已整理行为表与待实施名单，`fullScopeComplete=false`，不是全量合格声明。静态结构检查不提供 AST 完整性证明，也不替代人工核对函数行为。

已整理 28 篇与八份模板的六列表格检查、46 篇 architecture 元数据/唯一板块、Markdown 链接和锚点、范围内 Mermaid 解析及标签登记核对完成。保留已有笔记非 type 属性、旧标题锚点、不变量定义行及原验收行；Assets 仅按真实主体修正 type，旧文件名与兼容锚点保留。详细数量与发现以 `out/docs-audit/full-coverage-validation.json` 为准；审计脚本与快照亦在同一忽略目录。

标签清单同步用户 Renderer/GpuTimer 重命名并新增两项 architecture：纳管 105、默认视图 95、architecture 46；历史 staticVerification、architectureMigration 与图谱/UI 结论不重写。范围内 Obsidian 最短路径链接按 vault 内唯一目标检查，不宣称所有 Markdown 浏览器可移植。未改 `.obsidian` 配置、源代码、05/06 模板、教学快照、原始证据与引擎历史验收；workspace、用户开发笔记与 M3-T1 草稿同期变化保留，不归因于本次编辑。

本次未运行构建、游戏、Benchmark、故障注入或实际 Obsidian UI。剩余 15 篇与全量验收仍未完成，spec 状态保持铺开中。
