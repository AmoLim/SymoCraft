---
tags:
  - area/spec
---

# Profiling Module Boundaries Delta

## MODIFIED Requirements

### Requirement: Production ownership and narrow public headers

生产源码 SHALL 归属 `game/modules/` 的 foundation、scene、assets、world、ecs、simulation、platform、renderer、telemetry、窄 profiling 编译适配或 `game/app/`；旧 `tools/benchmark/` 产品退役，不再是活动构建/安装入口。每个启用的生产实现只能有一个真实编译 owner，测试 SHALL 链接正式生产库，不重编译生产实现，也不成为生产依赖。公开头 SHALL 从所属窄 include 根消费，不暴露私有存储、解析器、SDL、图形 SDK 或 Tracy 类型。

#### Scenario: M3-T0-A01 A02 A04 A09 ownership and consumers
- **WHEN** 配置启用某生产实现，或独立消费者只包含该模块一个公开头
- **THEN** 实现具有唯一生产 owner，消费者不借全仓库、vendor、聚合头或其他模块 src 补依赖；生产不编译或包含 test/support

#### Scenario: Retired source is not an active fallback
- **WHEN** 历史 .disabled 源、旧 GLFW、已退役 benchmark 实现或未授权私有路径被加入活动生产 target
- **THEN** 边界检查 SHALL 具名拒绝，不以文件仍在仓库或冻结包中作为兼容授权

### Requirement: Acyclic authorized dependencies

模块及 app SHALL 遵守[允许依赖方向](../../../../../architecture/engineering.md#生产所有权与依赖)，生产 target 图 SHALL 无环，模块不反向依赖 app，已退役独立 benchmark 不得成为活动生产依赖或反向复用游戏实现，不再要求保留该产品。PUBLIC SHALL 只表达真实公开头依赖；PRIVATE/最终链接的 `LINK_ONLY` 不得被当作公开头、SDK 或编译选项传播。renderer 的窄私有 backend contract 与 platform 三桥属于明确授权例外，不是开放其他模块私有目录。

#### Scenario: M3-T0-A03 actual boundary violations

- **WHEN** 实际 source/link/include 属性或显式 include 引入跨 owner、重复生产实现、测试反向依赖或依赖环
- **THEN** 配置期检查 SHALL 拒绝并标识违规对象，而不是只根据目录名推定隔离

#### Scenario: Current renderer interfaces are explicit exceptions

- **WHEN** 创建 renderer CPU API、CPU STATIC 门面、窄私有 backend contract 或原生后端
- **THEN** 其依赖 SHALL 按[当前分层](../../../../../architecture/engineering.md#renderer-的工程分层)隔离；只有审核过的消费者可取得私有契约，旧 GL 不混入现代 CPU 门面

### Requirement: Product and configuration isolation

CPU-only SHALL 不发现、配置或链接 SDL、GLAD、platform、原生图形后端或 Tracy client，局部关闭 profiling 与 GPU 注释；它仍可包含 renderer CPU 门面及 app CPU 转接，不沿用 T0 时“没有 renderer target”的过时描述。独立 benchmark/runner-only/benchmark-only 产品及安装入口 SHALL 退役，旧开启该产品的配置请求明确拒绝、给出迁移提示，不静默成功构建空工具。`BUILD_TESTING=OFF` SHALL 不注册测试及 fixture、不安装其产物。模式选项首次默认值与缓存独立状态仍须明确，不破坏游戏、CPU 模块及已交付后端的现有隔离。

#### Scenario: M3-T0-A05 CPU-only build graph
- **WHEN** `SYMOCRAFT_CPU_ONLY=ON`
- **THEN** 当前配置局部强制 GAME/原生后端/Tracy/注释 OFF、CPU_MODULES ON，不发现 profiler 或图形依赖；CPU 图形隔离不被解释成全部 renderer 或必要源资产消失，旧 BENCHMARK 缓存不能复活工具

#### Scenario: M3-T0-A07 A08 separate products
- **WHEN** 关闭测试、安装普通游戏，或显式请求旧独立 benchmark 配置
- **THEN** 游戏不依赖测试源码/fixture且不安装旧工具/Viewer/RenderDoc；旧产品请求明确拒绝，其历史包/配置/结果保持点时身份而非现役构建保证

### Requirement: Narrow typed dependency and test access

ECS 的通用 typed 门面 SHALL 允许模板实现所需的受控 EnTT 头依赖，但消费者只需链接正式 ECS target、包含项目公开头，不得自行拼接全 vendor/src 路径或使用原生 Registry/view/ID 逃生口。EnTT SHALL 固定本地官方版本/配置，相关翻译单元及测试一致；CPU-only 不增加图形 SDK 或不相关依赖，已退役独立 benchmark 不恢复。必要极端状态测试 SHALL 经模块私有窄适配，只向指定根测试授权，返回自有事实；不公开 NativeRegistry、任意存储修改器或生产依赖 test/support，不使用改变生产布局/配置的测试专用 ECS 副本。

#### Scenario: M3-T5-A21 public consumers and private adapter
- **WHEN** 编译多翻译单元公开消费者、关闭测试、CPU-only，或尝试越界包含私有测试适配
- **THEN** 生产与测试链接同一正式实现，配置/类型身份一致，公开头不泄露图形 SDK/Tracy；只有指定测试取得窄头，非法私有访问被拒绝，关闭测试无测试源码/产物依赖；历史独立工具验证不再要求新候选构建已退役产品

## ADDED Requirements

### Requirement: Optional fixed profiler dependency closure

Tracy SHALL 由显式项目构建开关选择，普通 Debug/Release 默认 OFF，固定本地官方来源与匹配 Viewer，禁止配置时在线下载/跟随 master。关闭时 SHALL 无 vendor 必需输入、Tracy 编译/链接/线程或连接；开启但依赖缺失 SHALL 配置期具名失败。每个最终程序 SHALL 只有一个静态 client owner，编译定义/ABI 一致，第三方头与链接仅经窄 profiling 适配进入已授权 app/world/simulation/renderer 的私有埋点消费者，不沿 foundation、ECS 公开头或独立 physics 传递。适配 SHALL 不反向依赖业务模块、不承载业务状态或第二套 Session/导出框架。

#### Scenario: DB-A01 off on and missing dependencies
- **WHEN** vendor 不存在但 profiler OFF，或 profiler ON 缺源码，或多个已授权生产 target 被同一程序消费
- **THEN** 前者可离线配置/构建且无 Tracy 符号依赖，中者明确配置失败，后者仅有一个 client 定义且不扩大公开 include/compile 传播；CPU-only 与公开消费者仍满足原隔离

### Requirement: Consistent diagnostic configuration and runtime isolation

开启路径 SHALL 仅接受本机连接、按需采集，不启系统追踪、自动调用栈采样、全局分配器/异常挂钩或等待 Viewer 才退出。profiling 配置 SHALL 保留 Release 优化及匹配符号；Debug 全闭包保持 DBG01 `_ITERATOR_DEBUG_LEVEL=0`、原 Debug CRT/调试及检查，不混用旧缓存/对象。工具许可与来源 SHALL 纳入对应构建的部署核对，不强制安装 Viewer/RenderDoc。

#### Scenario: DB-A02 consistent diagnostic builds
- **WHEN** 构建 profiling preset、普通 Debug 或无测试安装产物
- **THEN** 优化/符号及 DBG01/CRT 全闭包身份可核对，工具开关不暗改后端/validation/MSAA，关闭测试无 fixture，普通产品不需要外部工具或新增进程权限运行

### Requirement: Preserve non-performance document adaptation

现有 Data::Value 的 YAML 编解码/文件加载 SHALL 脱离轻量 telemetry，由 foundation 所属独立适配 target 唯一编译并私有消费 yaml-cpp；基础 foundation 不反向链接该适配，独立 physics 不因此增加 YAML/Tracy 依赖。公开接口 SHALL 保持中立数据与路径，不泄露 YAML 节点或新增通用服务。world-summary、文档往返及仍有效诊断消费者 SHALL 原义保留，不把一切 YAML 输出都当旧性能协议删除。

#### Scenario: DB-A03 retained document consumers
- **WHEN** 旧性能链退役后执行 world-summary、foundation 文档往返或 GPU 故障诊断，或检查独立 physics 最终依赖
- **THEN** 原编码/加载及错误含义保持，消费者使用正式唯一适配实现，不借旧 Performance::Session；physics 与基础 foundation 的允许依赖闭包不增长，不保留 telemetry 反向充当序列化 owner
