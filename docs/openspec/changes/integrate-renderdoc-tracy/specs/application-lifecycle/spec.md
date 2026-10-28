---
tags:
  - area/spec
---

# Application Lifecycle Delta

## MODIFIED Requirements

### Requirement: Private single run assembly and bounded shutdown

game/app 私有 Application SHALL 独占本次配置、Runtime/Window、World/Registry/SimulationSession、Renderer/Bindings、必要调试快照及可选 Scenario，不拥有旧性能记录/导出 Session；活动 namespace 静态玩法状态不恢复。一实例 SHALL 一次 Run、主线程串行非重入、不复制移动；成功装配才可 Run，部分失败保留可关闭 owner 及首诊断。Shutdown SHALL 尝试全部适用清理、保持 Renderer→Window→Runtime 依赖、幂等交接同一关闭事实且不无限重试；析构 noexcept 有界兜底、不导出数据或等待 Viewer。World/Registry SHALL 覆盖会话寿命，Window 覆盖 Renderer，Runtime 覆盖 Window。

#### Scenario: M3-T4-A04 session factory assembly
- **WHEN** app 或 CPU 调用方按 Q01 方案2拥有专属 World/Registry 并注册所需组件后装配会话
- **THEN** SimulationSession 工厂 SHALL 在该存储及中立初态中创建玩家，完整装配成功才发布会话；失败由 caller 销毁专属装配范围，不承诺共享 Registry 事务，会话析构不删除借用存储或玩家实体

#### Scenario: M3-T4-A04 staged assembly failure
- **WHEN** CPU 预检、Runtime、Window、Renderer、World/Registry 或会话装配失败
- **THEN** 不发布半初始化运行，回收已取得资源；专属 Registry 装配范围丢弃，不宣称任意共享 Registry 精确回滚/重试，mock 与真实结果分开

#### Scenario: M3-T4-A05 repeat shutdown and secondary errors
- **WHEN** 显式关闭有次要错误或重复 Shutdown/独立进程重新启动
- **THEN** 首原因保持、次要事实分别记录，关闭不重试取得资源或回调失效 Window，重复独立进程启动无未解释资源增长；未知/未取得不写成安全回收，后端终止保留不是正常清理通过

#### Scenario: M3-T4-A05 A19 owned shutdown error report
- **WHEN** 按 Q03 方案2执行显式 Shutdown 或重复交接其结果
- **THEN** SHALL 先尝试全部适用清理并保存事实，成功返回自有 ShutdownReport，失败再抛自有完整报告的 app 私有 ShutdownError；重复返回/抛出同事实不重新清理，调用方提取报告并与成功路径汇合退出结果，不再 Seal/Export，不覆盖原运行首因，OOM 仅承诺有限诊断

### Requirement: Local frame policy and commit order

Application SHALL 依次处理平台事实→关闭/暂停/输入来源→普通意图或 Scenario 模拟前编辑→每次 Advance 角色一次/物理零到八步→恢复/射线结果→普通 World 校验编辑→World 网格/绑定→Render/Present→中立观测。窗口回调 SHALL 不推进会话/改 ECS/提交 GPU；关闭始终优先不继续新模拟/渲染。普通暂停 SHALL 丢旧玩法记录并 Reset 平台，映射会话 Pause/Resume，恢复用当前 held、无旧短按/指针回放及首运动保护；同轮失焦再获焦也清一次，零实际像素不绘制，有限帧失焦诊断例外保留。场景策略 SHALL 按 platform 契约独立，不沿旧性能 Session 或工具连接分支；不因撤掉采集器把场景 VSync-off 请求改回普通 VSync-on。

#### Scenario: M3-T4-A09 A16 A17 ordinary and scenario composition
- **WHEN** 普通编辑、static/walk/edit 场景、暂停/恢复或窗口关闭发生
- **THEN** 普通编辑在模拟后、场景编辑在模拟前且保原动作/游标/预条件；普通可读调试值、场景可无记录，真实像素与 renderer 比例一致；关闭/Wait/同轮失焦不会误推进或重放


### Requirement: Target completion requires applicable evidence

T4 SHALL 保持 M3-T4-A01-A27 的稳定身份：A01-A22及A24-A27适用项须真实依据与用户整体验收；A23按D02后续不适用，不勾通过。纯CPU/静态/局部候选不替代真实SDL、现代后端、部署、性能及用户批准。缺环境/未执行/失败 SHALL 明示，不因迁移或planningComplete通过。

按2026-10-12用户确认，T4 SHALL 采用自动验证加四组人工体验，替代本change原每后端14项逐项人工核查及至少15分钟连续操作要求。14项共享玩法意图 SHALL 按适用契约由自动测试、真实集成及下述人工体验共同覆盖，不要求用户逐项复跑或审阅工程日志。此次只调整T4验证方式与人工时长，不改变游戏行为、D01-D13、D12对照字段/阈值、T3验收条件、性能协议或M5长时要求，不构成实施入口或阶段通过。

本 requirement 的 T4 阶段验收 SHALL 保留历史候选含义，不重开已归档节点。旧工具、封闭/导出和文件协议不再是本轮新候选交付条件；装配/清理、输入、CPU 对照及部署的有效行为仍须按受影响范围验证，不继承旧成绩。

#### Scenario: M3-T4 automated verification responsibilities
- **WHEN** 实施及交付T4适用A01-A21/A24-A27验证
- **THEN** 实施方 SHALL 执行真实生产库的契约/故障测试、公开消费者/负向边界及适用构建、平台/后端集成、文件协议与部署检查，完成D12每Advance/World提交对照；A27注释及架构由实施方源码核对。源码/包/设备身份、退出状态、诊断与结果由实施方核对并在同一verification.md汇总，不要求用户逐份审阅哈希/日志或另设子步骤批准；mock、合成事件、CPU与真实SDL/GPU依据分别标明，自动化缺口不得改成默认人工清单或标通过

#### Scenario: M3-T4-A01 A03 A21 build and deployment
- **WHEN** 验证当前公开头/target、CPU-only/关闭测试与安装包，或核对历史 T4 工具/文件协议材料
- **THEN** app 私有且无反向/SDK 泄漏或生产重复编译，必要数据及资源在无关/空格/中文 CWD 成立，seed/摘要/workload 身份不漂移；当前不要求构建旧工具或提供旧性能文件协议。原协议/CPU-GPU-present 口径及其结果保持历史身份，不覆盖冻结记录或用当前安装运行推定旧采样通过

#### Scenario: M3-T4-A01 A03 A21 compiler baseline transition
- **WHEN** T4正式交付从Visual Studio 2022配套MSVC切换至Visual Studio 2026配套MSVC
- **THEN** SHALL 固定实际完整工具链版本并使用全新构建目录，为新产物重新建立适用构建、CPU对照、消费者边界、真实后端及部署依据；保Windows x64/C++20/CMake-Ninja和既定D12阈值/故障语义，旧测试与人工反馈保持原候选身份，不自动继承为新产物通过。SDK/DXC/CRT等实际身份及失败/未执行明确记录，不附带升级其他依赖；具体新增执行范围先确认，不自动追加完整人工体验或正式性能采样，不将暂未复现1175解释为Fix1根因修复

#### Scenario: M3-T4-A22 modern local gameplay
- **WHEN** 申请当前T4整体验收
- **THEN** D3D12/Vulkan SHALL 各自用本机RTX5070Ti同一真实调用方完成下述四组短体验，每后端人工操作与观察合计不超过300秒；四组及退出/重启共用该上限，不按组、检查点或Debug/Release重复追加。实施方留实际adapter/driver/源码包/配置身份及自动结果，用户只反馈四组体验结论与异常；缺能力具名受阻、不fallback，GL仅受影响回归，不待其他GPU或称Y9000P/M4已通过

| 人工体验组 | 合并操作与必要观察 |
| --- | --- |
| 移动与碰撞 | 出生/落地、移动/松键、跳跃，代表性墙角或低顶接触及调试模式恢复；无明显穿地、失控或恢复异常，完整数值/边界由CPU与集成验证覆盖 |
| 镜头与编辑 | 转向/俯仰/滚轮、选框/选材、拆放及代表性区块交界；目标与画面反馈一致，无错块或持久裂缝，库存/冷却/编辑拒绝等完整规则由自动验证覆盖 |
| 窗口恢复 | 静止及按住移动键切出切回、最小化约10秒后恢复、尺寸变化；无旧输入重放、持续失控、比例错误或不可恢复黑屏，精确dt/事件次序由自动验证覆盖 |
| 退出与重启 | Esc和关闭按钮退出并重启确认画面/操作恢复；用户不逐读日志，退出分类、关闭顺序/幂等/资源事实由实施方自动检查 |

#### Scenario: M3-T4-A22 bounded short check and uncovered risks
- **WHEN** 短体验失败、达到300秒仍未完成某组，或用短体验提出T4结论
- **THEN** SHALL 记录失败/未执行及原因，不为填满清单自动延长或反复重跑；修复后只对受影响组提出限时复查，额外人工核验须另获确认。短体验不证明原15分钟连续体验、持续资源增长、罕见时序故障、长期稳定性或正式性能预算，未覆盖风险在verification.md保留；不自动追加长采样、长时自动替代测试或修改历史成绩，适用自动验证与用户整体验收仍须成立

## ADDED Requirements

### Requirement: Standalone bounded profiling scenarios

游戏 SHALL 提供 `--profile-scenario static|walk|edit`，不要求 `--output` 或安装外部工具；`--scenario-seconds` SHALL 仅用于该模式、接受整数 1..1200、默认 60 秒，以场景开始后的单调实际时间控制正常结束，不分预热/采样。默认采用 seed 424242、既有 regression v1/workload v2、1920×1080、4x MSAA、VSync-off 请求，显式支持的配置覆盖须如实核验。场景 SHALL 与资源预检/world-summary/有限帧诊断及冲突 scene/checkpoint/test-edits 组合明确互斥，不能静默忽略参数；普通入口语义保持。

场景初态 SHALL 明确映射为 walk 使用 spawn、static/edit 使用 four-chunk，位置与 yaw/pitch 保持现有 TestScene::Checkpoint 定义，初始 FOV 为 45 度；不统一沿用默认 spawn，不以首次物理步后的结果代替初态核对。

#### Scenario: DA-A01 direct scenario and duration
- **WHEN** 直接启动任一场景而未给输出目录/未启动工具，随后达到设定时限
- **THEN** 完整工作负载照常运行、由显式 Shutdown 正常结束；edit 沿真实时间每 0.25 秒执行既有正向/反向恢复循环，不减少世界/画质或额外推进模拟来补足工具事件

#### Scenario: DA-A02 invalid and retired options
- **WHEN** 给出未知场景、非整数/越界时限、无场景的时限、冲突模式或旧 benchmark/输出/预热采样/焦点准备参数
- **THEN** 在初始化窗口前具名拒绝、给出新入口提示并保持解析错误 64，不生成旧文件或静默兼容；旧 CLI 文本/输出协议不再是现役支持承诺

#### Scenario: DA-A04 preserved initial checkpoints
- **WHEN** 分别解析 static/walk/edit 并完成真实生产会话装配、尚未执行首次 Advance，且工具或完整性能记录均不存在
- **THEN** walk 的 checkpoint 为 spawn、static/edit 为 four-chunk，实际初始 position/yaw/pitch 与对应现有定义一致、镜头 FOV 为 45 度；解析和装配两层分别核对，不只验证 seed/场景名或通过改视点/路线掩盖负载变化

### Requirement: Explicit run result without performance export

app SHALL 区分正常结束/用户取消、参数错误、资产预检失败与运行/关闭错误，保留既有 0/64/2/3 分类；场景超时正常完成与 Esc/关闭用户取消 SHALL 有可辨诊断，取消不宣称工作负载已完整结束。场景失去有效尺寸/最小化 SHALL 明确停止为运行失败并有界清理，不挂起等待或生成“有效成绩”。不再使用旧观测无效退出 4，不以任何 summary/status 文件决定成功。

#### Scenario: DA-A03 cancellation and cleanup failure
- **WHEN** 场景收到 Esc/关闭、失效窗口或关闭过程中出现次要错误
- **THEN** 正常取消无后续模拟/提交且不输出完成成绩，窗口失效失败为 3；任一实际运行/关闭失败优先于正常/取消结果并保持首诊断，不靠导出成功覆盖清理错误
