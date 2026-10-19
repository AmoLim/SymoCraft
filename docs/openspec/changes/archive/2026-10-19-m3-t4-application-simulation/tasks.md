---
tags:
  - area/spec
---

# T4 已完成任务（归档）

## 已交付范围

P01接口候选/消费者静态核对已交付；Q01方案2、Q02当前方案1、Q03方案2、Q04方案3均已确认，Q02向3演进及公开Inspect仅未来方向，见[decisions](decisions.md#未决接口)。不是接口设计冻结或整阶段入口。
2026-10-09 CPU局部授权包含旧基线/夹具及独立核心/单测，当时仅基线交付：15物理+9玩法组，Debug/Release各重复三次；Debug启用58/58、world.allocations禁用1，Release59/59。关闭测试CPU生产构建通过、physics_system.cpp单编译、基线真实闭包无app/窗口/GPU；身份与原沙盒C1902失败均保留在[VERIFICATION](../../../../verification/VERIFICATION.md#归档入口)。app参考未冻结，非完整可复现仓库/游戏包。

2026-10-13本聊天文档已同步至refactor-application-simulation worktree，三个同推理强度子agent继续S01静态交接与S04局部实现：新增真实独立核心、公开消费者、合成地形单测和嵌套owner边界；实际检查/交接缺口见[verification](verification.md#2026-10-13-局部核心与交接核对)。没有Session/生产World适配切换、SDL/GPU/性能或D12结果，不整项勾选S01/S04。旧15组物理基线仅作本轮定向重测，不将历史58/59全套结果冒称本轮成绩。

同日用户收回子agent开启授权；后续由主agent独自推进，未经重新明确授权不启动子agent。本轮仅浓缩测试组织及重复脚手架，覆盖和局部实施范围不变，结果仍维护在同一verification。

本聊天后续docs更新及时同步至原工作区 `F:/GameDevelop/OpenGLProject/Symocraft/docs` 供用户审核；同步前核对并保留原目录独立修改，冲突不直接覆盖。只同步本任务相关可维护文档，不含个人/冻结材料，也不代表代码合并、OpenSpec规格sync或Git推送授权。

2026-10-15用户先授权独自推进最小ECS修补，并在公共分配器确认点后进一步明确授权限定修补Allocate/ReAlloc及故障回归，完成初始化、注册转交和扩容失败清理，不扩大T5。该最小前提已实施并完成CPU回归；通用Remove、存储/对齐/并行和完整内存审计仍独立。实际范围/结果见[verification](verification.md#2026-10-15-最小-ecs-前提修补)，不是完整Session/应用迁移入口，S01仍不整项勾选。

## Implementation

2026-10-15已取得[完整生产实施入口](../../../../verification/VERIFICATION.md#关键节点)，覆盖既定S01-S06实施与适用验证；Q/D组合一致性由实施方核对，不新增选型或逐份人工审核关卡。仍须报告新设计冲突/范围扩大，既有人工短体验/整体验收及前序真实候选验证不由实施授权代替。

以下继承S01-S06不重新编号；S02-S05生产实现与定向自动回归已完成，2026-10-17按上游已批准604c2b58整合并核对H01-H04后S01交接收口，2026-10-19完成新工具链正式性能及用户人工反馈后S06技术交付收口，任务6/6。随后用户明确批准T4整体完成并保留风险，节点依据见[完成批准](verification.md#2026-10-19-t4整体完成批准)，不由勾选推定；自动失败及覆盖边界仍见[收口对账](verification.md#2026-10-19-s06收口与t4整体验收申请)。A编号完整行为由[五份delta](proposal.md#capabilities)维护，下表仅导航，不另开子批准台账。

- [x] T4-S01 交接与正式化：按实施时实际T2/T3版本完成所需契约/消费者与H01-H04核对；落实已选Q01方案2的注册/装配安全前提、Q02当前方案1的事实/意图边界、Q03方案2的报告/异常汇合及Q04方案3的窄私有采集授权，整体审核目标并取得完整生产实施入口，保D01-D13及既有协议，批准只登记VERIFICATION。
- [x] T4-S02 应用与平台生命周期（依赖S01对应入口）：实例Application、Runtime/Window明确owner与实际事实、D10输入交换/清理；补部分启动/重复关闭/回调寿命/真实输入与窗口回归，覆盖A01/A04-A06/A09/A20。真实SDL的输入为受控队列事件，非人工键盘体验。
- [x] T4-S03 模拟会话与镜头：独立预算/控制冷却、Pause/Resume/恢复、D09自有结果与设计注释、D11/D13double预检/Failed；移除活动复制相机实体，保中立输入/可选视图及编辑顺序，覆盖A07-A09/A13-A16/A24/A26/A27。旧Camera低层兼容API非活动权威，未擅自全删。
- [x] T4-S04 物理方案2：真实内部symocraft_physics、值/查询契约及独立消费者/负向边界/算法基线；完整授权后接通正式World/ECS逐体候选提交、Session预算，撤旧全局预算/活动碰撞算法，覆盖A01-A03/A10-A14。正式第十模块/A23仍不在本轮。
- [x] T4-S05 观测与场景分离：受控记录/Seal/独立导出结果与显式重试/Saved幂等，独立Scenario/app策略；保GPU关联/缺失原因/首错/旧文件协议，不恢复Fix1，覆盖A17-A21。
- [x] T4-S06 组合与交付：实施方完成真实公开头/CPU/独立工具/关闭测试矩阵、D12每Advance/World提交对照、真实SDL/现代后端集成及必要成本/协议/部署自动验证；本机RTX5070Ti的D3D12/Vulkan分别进行四组人工短体验，每后端合计最多5分钟（含退出/重启），不另跑14项人工表。同步已落实架构，A01-A22/A24-A27适用项具实证并明示短体验未覆盖风险后申请用户T4整体验收，之后T5独立，不勾A23。

2026-10-17至18独自续行S06：已补全单/双现代及GL、CPU、根/独立工具和关闭测试范围的构建，双配置各8项真实增量及有限原生故障组合；发布1175与导出失败保留，不恢复Fix1或刷重试通过。正式性能仅提出27轮30+90秒/预计65-80分钟方案，未启动；窗口与新体验包准备结果随执行更新[同一验证记录](verification.md#2026-10-17-s06组合验证续行)。本段是进度导航，不替代人工短体验、成本结论或整体验收，S06仍不勾选。

2026-10-18用户确认S06正式切换Visual Studio 2026配套MSVC并授权继续实施，顺序为：核实固定工具链身份→必要构建入口适配→全新目录构建及受影响自动回归→新包/协议核对→单独确认后正式性能对照→申请整体验收。切换机制及其他依赖保持边界见[design](design.md#工具链交付基线)，新产物依据沿用A01/A03/A21等原身份；不重开S01-S05、修改D/Q或提前勾S06。旧候选人工/自动结论保留原义，具体新增执行矩阵先确认，正式27轮采样未授权启动；Fix1继续暂停，升级不是1175修复。

本轮随后获用户确认的六目录范围已构建/检查，构建入口默认选择已切到MSVC14.51.36231/原SDK；游戏D/R、CPU Release及有限帧/安装检查成立，CPU Debug保留1175、独立工具保留两项Publish YAML失败，见[工具链验证](verification.md#2026-10-18-正式编译器切换)。切换产物已形成但回归非全绿，不据此勾S06或启动正式采样；两项工具发布失败尚未完成因果诊断，旧人工结果不转给新包。

2026-10-18后续按用户限定授权完成独立工具发布兼容性修补：新回归修补前具名错误5，修补后Debug/Release独立工具各一次9/9，关闭测试构建成功；真实阻塞/首次竞争及注入错误的边界见[限定修补验证](verification.md#2026-10-18-工具发布兼容性限定修补)。原两项失败的历史持有者仍未知，保留原始失败；未修改foundation或恢复Fix1，不把工具回归通过当成1175闭环，正式性能及S06整体验收仍未完成。

## 验证执行分工

2026-10-19最新反馈：用户明确通过新编译器候选的人工验收并要求S06汇总/整体审阅；本次按所给D3D12/Vulkan普通入口四组短体验范围登记，绑定同一Release候选，不补造用时/逐项checkbox。S06交付对账完成；当前提请用户整体批准，不把该人工反馈扩为T4已批准或1175已修复，下列“未关闭/未人工/未采样”仍为历史时点。

2026-10-19正式性能检查完成：原批GL walk-3失焦停止后，按用户两次明确要求分别在002/003重采并通过，再依明确授权完成剩余21轮。主配对固定原五轮+最新003+续行21轮，所选27/27有效，两现代各9/9组同时满足原帧时间/内存预算，原CSV独立复算一致；不择优或覆盖原失败。身份/原始数据来源、跨批间隔及限制见[同一验证记录](verification.md#2026-10-19-t4新工具链正式前台采样)。此前“未授权启动/未续跑/进行中”保留历史时点；正式性能完成不等于新编译器人工覆盖或T4整体验收，Fix1/T5不扩大，S06仍不勾选。

2026-10-18用户汇总反馈本次人工短体验无异常，记录与候选范围见[人工反馈](verification.md#2026-10-18-人工短体验反馈)；不补填逐项checkbox或据此关闭S06。剩余正式性能对照与发布失败风险处置尚未完成，Fix1仍暂停。

2026-10-12用户确认本change采用自动验证加四组人工体验，替代原每后端14项逐项人工核查/至少15分钟连续操作；不是新的测试成绩、设计冻结或完整生产实施授权。四组操作和限时条件只由[application delta](specs/application-lifecycle/spec.md#requirement-target-completion-requires-applicable-evidence)维护；A编号继续追踪覆盖，不等于用户操作清单。

A01-A21/A24-A27由实施方按影响范围执行自动验证及必要源码核对，真实平台/后端组合与CPU/mock结果分开，交付一次必要结论和未覆盖项，不要求用户逐份审阅源码哈希、构建日志、CSV或旧CPU基线，不恢复退役M01-M06人工主表。A22人工只覆盖移动与碰撞、镜头与编辑、窗口恢复、退出与重启四组体验；每后端四组及重启合计不超过300秒，不按组/检查点/构建配置累加，不为限时未完成项自动延长或复跑。缺失/失败具名报告，修复后只提出受影响组的限时复查，额外人工范围另确认；自动化缺口不默认转为人工核查。

适用自动验证仍须实际成立，D12字段/阈值/全部序列、既有性能协议和关键节点批准不取消；不自动增加长时自动替代测试。短体验未证明原15分钟连续体验、持续资源增长、罕见时序或长期稳定性，风险随实际结果保存在verification.md；T3、M5及中低端下一玩法前门槛不受本次调整影响。

## 稳定验收映射

适用A01-A22/A24-A27的交付依据已在[收口对账](verification.md#a项交付对账)汇总，人工与T4整体均已由用户明确通过；既有发布失败、跨工具链未重跑范围及长期/跨机限制不改判通过，A23不适用不勾。下表继续仅作稳定身份导航，不增设逐项人工批准表或把旧CPU基线当新会话验收。

| M3-T4身份 | 检查目标与权威delta |
| --- | --- |
| A01 模块/公开头 | app私有、无SDK/兄弟私有头及target环：[physics](specs/physics-core/spec.md)、[application](specs/application-lifecycle/spec.md) |
| A02 独立物理 | Debug/Release真核心消费者与禁止依赖/include负向：[physics](specs/physics-core/spec.md) |
| A03 CPU/工具/关闭测试 | 真实生产单owner、无fixture/重复cpp：[physics](specs/physics-core/spec.md)、[application](specs/application-lifecycle/spec.md) |
| A04 部分装配失败 | 各装配阶段/专属范围/首错，mock与真实分开：[application](specs/application-lifecycle/spec.md)、[platform](specs/platform/spec.md) |
| A05 重复关闭 | 幂等事实/关闭顺序/回调/独立进程增长：[application](specs/application-lifecycle/spec.md) |
| A06 输入交接 | 自有交换/pending/消费完成复用/ResetDestroy/Wait/失败：[platform](specs/platform/spec.md) |
| A07 双CPU会话 | 不共享预算/选材/冷却，不推多窗口或共享Registry：[simulation](specs/simulation/spec.md) |
| A08 零/多/长帧 | 两层时间、旧float余数、有效零步跳跃、规则一次/无缓冲：[simulation](specs/simulation/spec.md) |
| A09 暂停恢复 | 清理范围/首次成功dt0/held/旧输入/同轮焦点/真实像素：[simulation](specs/simulation/spec.md)、[application](specs/application-lifecycle/spec.md) |
| A10 积分/sensor | 旧顺序/传固定步/sensor清接地：[physics](specs/physics-core/spec.md) |
| A11 接触夹具 | 地/墙/角/低顶与旧算法局限，不称CCD：[physics](specs/physics-core/spec.md) |
| A12 支撑/坐标 | 当前World、域外/负坐标/边界、无网格前置：[physics](specs/physics-core/spec.md) |
| A13 参数/查询 | double/指针预检/修正、形状/绑定/坏定义/射线边界：[simulation](specs/simulation/spec.md)、[physics](specs/physics-core/spec.md) |
| A14 写入/故障 | 单实体未提交/此前提交/Failed、结果构建/旧ECS非日志传播：[simulation](specs/simulation/spec.md) |
| A15 调试/出生点 | sensor/重力/恢复速度接地跳跃预算无全局残余：[simulation](specs/simulation/spec.md) |
| A16 镜头/编辑 | 单姿态权威、选择/库存/重叠保护/普通后提交/拒绝冷却：[simulation](specs/simulation/spec.md)及未改[现役编辑](../../../specs/simulation/spec.md) |
| A17 场景/观测 | Session存在不控制路线，static/walk/edit顺序：[telemetry](specs/telemetry-lifecycle/spec.md) |
| A18 GPU/封闭 | ID/缺失/迟到拒绝/Seal同事实幂等冲突拒绝/原帧顺序：[telemetry](specs/telemetry-lifecycle/spec.md) |
| A19 关闭/保存 | 所有最终事实再Seal、失败留部分/显式重试/Saved无重写/终态/退出0/4/3：[telemetry](specs/telemetry-lifecycle/spec.md) |
| A20 模式范围 | app隔离/局部Escape跨失焦/不后台键/最小化/无新增档位：[platform](specs/platform/spec.md)、[telemetry](specs/telemetry-lifecycle/spec.md) |
| A21 文件/CWD | 旧新协议/元数据/seed/统计、无关/空格/中文安装运行、不覆盖冻结：[application](specs/application-lifecycle/spec.md) |
| A22 真现代组合 | 本机RTX5070Ti两个现代后端自动集成依据加四组人工体验，每后端合计最多5分钟；adapter/driver/版本真实、不fallback、保未覆盖风险：[application](specs/application-lifecycle/spec.md) |
| A23 后续正式physics | 当前T4不适用不勾；T5后/NPC前独立授权搬所有者、保行为/头/target再验：[physics](specs/physics-core/spec.md) |
| A24 真无窗口 | 同一生产target/必要资产、不Runtime/Window/Renderer/图形相机：[simulation](specs/simulation/spec.md) |
| A25 逐次对照 | 每Advance/World提交、离散精确/固定浮点阈值、首分歧和全部序列：[simulation](specs/simulation/spec.md) |
| A26 寿命/策略 | 旧结果自有/视图可选重复无副作用/非法比例只拒视图/只读采集：[simulation](specs/simulation/spec.md) |
| A27 D09渐进记录 | 生产结果处理由/所有权/触发注释、落实后实际架构，不提前方案2：[simulation](specs/simulation/spec.md)、[decisions](decisions.md#d09-演进触发) |

证据须绑定源码/构建/包/配置/实际设备、输入时间顺序、错误/提交/关闭及原记录；性能沿[既有冻结入口](../../../../verification/VERIFICATION.md#归档入口)或另行确认，不补造预算/轮数。[共享场景](../../../../verification/scenarios.md)提供固定输入与玩法意图，不将其中14项人工表/15分钟路线重新作为T4人工门槛；本次四组/限时以application delta为准。CPU不替真实GPU，历史成绩不替候选，中低端下一玩法前门槛与T5/GL退役/服务拆分各自独立。
