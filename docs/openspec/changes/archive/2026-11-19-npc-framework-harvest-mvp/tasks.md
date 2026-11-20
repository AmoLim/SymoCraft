---
tags:
  - area/spec
---

# NPC 框架与耗时采集 MVP 任务

## Implementation

按 [已确认选型](decisions.md)和 [设计](design.md)推进。任务编号标识工作与证据，节点仍为 N1、N2、N3。实施须有独立授权；开工时确认代码根及适用验证。每项只有实现与约定检查均有依据才勾选，验证结果集中记入本 change 的 `verification.md`。

2026-11-16 扩展动作表现与可观察演示。原已勾选项保持此前静态候选的完成范围，不包含新增要求；新增 C2、N1.5、N3.3–N3.5、V3 的实施及验证状态以下方清单和 verification.md 为准，V2 仍须用户整体验收。不得以旧验证替代新增功能验证。

正式 physics 模块迁移是 [T4 D02/A23 独立节点](../../../specs/physics-core/spec.md#scenario-m3-t4-a23-later-formal-ownership-move)。交接须确认正式 owner、现有公开接口、单一实现和迁移回归已验收。该前置卡住 N1.3 的真实刚体接入与后续生产联调；交接契约、World 规则、树替身和角色静态显示可在各自获授权后先行。

### 共同交接

- [x] **C1 约定三节点交接。** 按[交接表](design.md#n1n2n3-交接)落实动作请求、标识与终态、一次性帧编辑批次、显示值和故障交接的最小字段、所有权与消费时点；核对公开接口和 CPU-only 构建的依赖方向。N1 持有会话与应用接线，N2、N3 通过值契约独立验证。验收关联 NPC-MVP-A15、A20。
- [x] **C2 扩展动作表现与反馈交接。** 依赖 C1。提供带本轮/实体/动作身份的只读实际位移、动作阶段及有效耗时、提交终态与资源值，核对省略/重复读取不改变模拟、就绪不冒充成功，以及 CPU-only/公开消费者边界。验收关联 [NPC-MVP-A15](specs/simulation/spec.md#requirement-headless-npc-advance-and-action-handoff)、A20、A34–A38。

### N1 实体、World 与采集动作

- [x] **N1.1 显式装配和移除。** 依赖 C1。实现人类和僵尸的 typed ECS 装配、活实体检查、重复顺序键拒绝和活动动作清理。验证失败装配、组件与动作隔离、实体移除后旧标识及批次请求失效。验收关联 [NPC-MVP-A01](specs/npc-entities/spec.md#scenario-npc-mvp-a01-creation-and-removal)、A25。
- [x] **N1.2 World 采集规则与目标标识。** 依赖 C1。落实可选采集规则和 Q01-A 的稀疏单格版本，在同一个受控编辑边界提交方块与版本。验证旧配置、参数拒绝、同种方块原位替换、同区块其他格编辑、无变化编辑和表项准备或版本耗尽失败。验收关联 [NPC-MVP-A13、A14、A31](specs/world/spec.md#requirement-harvest-properties-in-checked-block-definitions)、A27。
- [x] **N1.3 真实地形移动。** 依赖 N1.1 与正式 physics 独立节点验收。按显式刚体步进顺序接入会话共用预算，完成工作点选择、合法到达、地形受阻退出及角色可重叠行为。用真实生产核心和 World 核对位移、停止与僵尸 Idle。验收关联 [NPC-MVP-A02、A03](specs/npc-entities/spec.md#requirement-terrain-only-movement-to-a-work-position)、A15。
- [x] **N1.4 采集推进与串行提交。** 依赖 N1.1、N1.2；提交可先用受控合法工作初态验证，完整交接依赖 N1.3。实现 Q02-A 时间接线、暂停与取消、S1 排序、F1 准备和终态发布，以及批次拒绝与 Failed 清理。用真实 World 验证零步、长帧、恢复首帧、重复交付、反转装配顺序、竞争、World 拒绝与故障注入。验收关联 [NPC-MVP-A04–A07、A26–A30](specs/npc-entities/spec.md#requirement-timed-harvest-action)、[A15、A24、A25](specs/simulation/spec.md#requirement-ordered-harvest-commit-after-player-edits)。
- [x] **N1.5 演示场景与控制。** 依赖 C2、N1.4、N2.2。增加默认单人主演示、双人争抢对照和侧边 Idle 僵尸，保留三 NPC 正确性夹具。实现 Ready/Playing/Paused/Completed、开始、暂停/继续、从初态重新演示及切换；核对焦点与手动暂停叠加、恢复首帧、旧输入、轮次身份、批次拒绝、装配失败及旧 World/会话借用清理。重启不重置渲染完成序列，不增加历史回放或拆放干预。验收关联 [NPC-MVP-A36–A38](specs/application-lifecycle/spec.md#requirement-observable-harvest-demo-controls)、A21、A23–A25。

### N2 行为树与策划工作流

- [x] **N2.1 运行时依赖与离线制作链。** 依赖 C1。核验并固定 BehaviorTree.CPP 的源码、版本、许可与必要配置，验证 MSVC/DBG01 和 CPU-only。实现启动校验、节点描述导出、受限表达式及每 NPC 的独立树和黑板，用动作替身验证端口、生命周期和状态。交付可由 Groot2 FREE 离线编辑的人类与僵尸树。验收关联 [NPC-MVP-A08–A11](specs/npc-ai/spec.md#requirement-offline-authoring-and-startup-validation)、A20。
- [x] **N2.2 绑定正式动作与诊断。** 依赖 N2.1、N1.4。绑定移动、采集、终态与取消，验证人类成功或失败后保持 Idle，僵尸保持独立 Idle。交付状态变化日志和只读快照，区分真实动作结果与替身结果。验收关联 [NPC-MVP-A10–A12](specs/npc-ai/spec.md#requirement-isolated-behavior-and-controlled-action-binding)、A21。

### N3 角色资产与动作表现

- [x] **N3.1 Blender 资产与 CPU 导入。** 依赖 C1。核验并固定 cgltf 来源及许可，交付人类和僵尸的可编辑源与实际导出 GLB。实现资产路径、静态几何转换和错误报告，用正式 CPU 导入验证文件、属性、坐标、缓冲寿命与坏资产。验收关联 [NPC-MVP-A16、A17](specs/character-assets/spec.md#requirement-validated-static-character-asset-import)、A20。
- [x] **N3.2 双后端角色实例。** 依赖 C1、N3.1。用相同中立输入在 D3D12/Vulkan 显示可区分的角色，验证位姿更新、模型共享、单实例移除、在途寿命及关闭。先用约定显示值验证，真实实体接线由 N1 在联合验证完成。验收关联 [NPC-MVP-A18、A19](specs/renderer/spec.md#requirement-static-character-meshes-and-shared-instances)、A20。
- [x] **N3.3 分件资产与 CPU 导入。** 依赖 N3.1。更新 Blender 源和实际导出 GLB，保留躯干、头、左右臂/腿和明确肩髋轴；扩展中立自有部件/变换数据，验证必需部件、重复/循环/非法变换、输入上限、释放原缓冲及整体静态输入兼容。验收关联 [NPC-MVP-A32、A33](specs/character-assets/spec.md#scenario-npc-mvp-a32-owned-multipart-character-data)、A16、A17、A20。
- [x] **N3.4 程序动作与共享部件绘制。** 依赖 C2、N3.2、N3.3。以逐部件变换驱动随实际位移的摆腿摆臂及随有效耗时的采集动作，验证受阻不踏步、暂停冻结、终态回中立、两 NPC 姿态隔离及动作切换清理。在 D3D12/Vulkan 核对共享网格、反复重播/移除和在途寿命，不接蒙皮或动画库。验收关联 [NPC-MVP-A34、A35](specs/renderer/spec.md#requirement-state-driven-rigid-character-motion)、A18、A19。
- [x] **N3.5 演示控制入口与真实反馈。** 依赖 N1.5、N3.4。补充必要的最小文字/进度/控制显示路径及资源部署，在两个现代后端提供可发现的操作和身份、阶段、目标、进度、资源及结果提示。核对 Ready 与暂停仍可观察、成功仅来自提交、败方不获资源、未知事实和重播后旧提示清理，不扩为通用调试器或预选 UI 库。验收关联 [NPC-MVP-A36–A38](specs/application-lifecycle/spec.md#requirement-observable-harvest-demo-controls)、A20。

### 联合验证与交付

- [x] **V1 正式候选的 B 联合验证。** 依赖 N1.3、N1.4、N2.2、N3.2。N1 完成 Application 接线，用同一生产 CPU 路径核对两名人类独立采集及同目标争抢、玩家与 NPC 同帧竞争、僵尸 Idle 与资源隔离；核对真实实体显示和故障退出。分别执行适用构建、CPU-only、关闭测试、公开消费者、部署、双现代后端及保留 GL 回归。保留玩家冷却及 `static/walk/edit` 的初态和编辑时点。验收关联 [NPC-MVP-A21、A23](specs/application-lifecycle/spec.md#requirement-npc-mvp-assembly-and-bounded-evidence)及受影响的 A01–A31。
- [x] **V3 动作演示扩展联合验证。** 依赖 C2、N1.5、N3.3–N3.5。用正式生产路径验证分件输入、动作/结果交接、完整单人采集、争抢反馈、暂停/恢复及反复重播/切换；覆盖焦点、旧身份/批次、装配失败、资源及在途寿命。执行适用的 Debug/Release、CPU-only、关闭测试、公开消费者、安装资源、双现代后端和受影响 GL/玩家/性能场景回归。新结果集中记入既有 verification.md，保留旧候选范围和失败事实。验收关联 NPC-MVP-A32–A38 及受影响 A01–A31。
- [x] **V2 短体验与整体验收。** 依赖 V1、V3。按 [NPC-MVP-A22](specs/application-lifecycle/spec.md#scenario-npc-mvp-a22-bounded-modern-backend-check)在本机 D3D12、Vulkan 各体验一次，单后端至多 3 分钟，总计至多 6 分钟，观察外观、摆腿摆臂、采集进度/结果、开始/暂停/从头重新演示、场景切换及退出。全部子场景与重启共用上述时限；分别记录真实 CPU、替身和 GPU 证据及失败或未执行项，提交用户整体验收。
