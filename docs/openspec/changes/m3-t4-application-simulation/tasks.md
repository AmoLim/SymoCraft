---
tags:
  - area/spec
---

# T4 唯一活动计划

## 已交付范围

P01接口候选/消费者静态核对已交付，Q01-Q04仍未选择、人工意见待确认；不是接口设计冻结或整阶段入口。
2026-10-09 CPU局部授权包含旧基线/夹具及独立核心/单测，但仅基线交付：15物理+9玩法组，Debug/Release各重复三次；Debug启用58/58、world.allocations禁用1，Release59/59。关闭测试CPU生产构建通过、physics_system.cpp单编译、基线真实闭包无app/窗口/GPU；身份与原沙盒C1902失败均保留在[VERIFICATION](../../../verification/VERIFICATION.md#归档入口)。app参考未冻结，非完整可复现仓库/游戏包；没有新physics/会话、SDL/GPU/性能或D12结果，本轮不重跑这些历史测试。

## Implementation

以下继承原T4-S01-S06，不重新编号；均未整体完成。完整生产迁移须核对本轮独立授权，局部授权/P01/迁移/CLI完成均不扩大范围。A编号的完整行为由[五份delta](proposal.md#capabilities)维护，下表仅导航验收身份，不另开子批准台账。

- [ ] T4-S01 交接与正式化：按实施时实际T2/T3版本完成所需契约/消费者与H01-H04核对；确认Q01-Q04、整体审核目标并取得完整生产实施入口，保D01-D13及既有协议，批准只登记VERIFICATION。
- [ ] T4-S02 应用与平台生命周期（依赖S01对应入口）：实例Application、Runtime/Window明确owner与实际事实、D10输入交换/清理；补部分启动/重复关闭/回调寿命/真实输入与窗口回归，覆盖A01/A04-A06/A09/A20。
- [ ] T4-S03 模拟会话与镜头：独立预算/控制冷却、Pause/Resume/恢复、D09自有结果与设计注释、D11/D13double预检/Failed；移除复制相机实体，保中立输入/可选视图及编辑顺序，覆盖A07-A09/A13-A16/A24/A26/A27。
- [ ] T4-S04 物理方案2：仅独立核心/单测按已局部授权的实际范围推进，交付真实内部symocraft_physics、值/查询契约及独立消费者/负向边界/算法基线。生产适配切换、故障提交与撤旧全局预算/活动直连须依赖S01/S03对应独立授权和会话预算替换，不由局部核心授权推进全应用迁移；覆盖A01-A03/A10-A14。
- [ ] T4-S05 观测与场景分离：受控记录/Seal/独立导出结果与显式重试/Saved幂等，独立Scenario/app策略；保GPU关联/缺失原因/首错/旧文件协议，不恢复Fix1，覆盖A17-A21。
- [ ] T4-S06 组合与交付：实施方完成真实公开头/CPU/独立工具/关闭测试矩阵、D12每Advance/World提交对照、真实SDL/现代后端集成及必要成本/协议/部署自动验证；本机RTX5070Ti的D3D12/Vulkan分别进行四组人工短体验，每后端合计最多5分钟（含退出/重启），不另跑14项人工表。同步已落实架构，A01-A22/A24-A27适用项具实证并明示短体验未覆盖风险后申请用户T4整体验收，之后T5独立，不勾A23。

## 验证执行分工

2026-10-12用户确认本change采用自动验证加四组人工体验，替代原每后端14项逐项人工核查/至少15分钟连续操作；不是新的测试成绩、设计冻结或完整生产实施授权。四组操作和限时条件只由[application delta](specs/application-lifecycle/spec.md#requirement-target-completion-requires-applicable-evidence)维护；A编号继续追踪覆盖，不等于用户操作清单。

A01-A21/A24-A27由实施方按影响范围执行自动验证及必要源码核对，真实平台/后端组合与CPU/mock结果分开，交付一次必要结论和未覆盖项，不要求用户逐份审阅源码哈希、构建日志、CSV或旧CPU基线，不恢复退役M01-M06人工主表。A22人工只覆盖移动与碰撞、镜头与编辑、窗口恢复、退出与重启四组体验；每后端四组及重启合计不超过300秒，不按组/检查点/构建配置累加，不为限时未完成项自动延长或复跑。缺失/失败具名报告，修复后只提出受影响组的限时复查，额外人工范围另确认；自动化缺口不默认转为人工核查。

适用自动验证仍须实际成立，D12字段/阈值/全部序列、既有性能协议和关键节点批准不取消；不自动增加长时自动替代测试。短体验未证明原15分钟连续体验、持续资源增长、罕见时序或长期稳定性，风险随实际结果保存在verification.md；T3、M5及中低端下一玩法前门槛不受本次调整影响。

## 稳定验收映射

全部目标A项尚无新T4实施/整体验收结论；旧CPU基线只作前述依据，不勾以下项。已授权不等于已实现，原单测不替代新会话/核心验收。

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
| A16 镜头/编辑 | 单姿态权威、选择/库存/重叠保护/普通后提交/拒绝冷却：[simulation](specs/simulation/spec.md)及未改[现役编辑](../../specs/simulation/spec.md) |
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

证据须绑定源码/构建/包/配置/实际设备、输入时间顺序、错误/提交/关闭及原记录；性能沿[既有冻结入口](../../../verification/VERIFICATION.md#归档入口)或另行确认，不补造预算/轮数。[共享场景](../../../verification/scenarios.md)提供固定输入与玩法意图，不将其中14项人工表/15分钟路线重新作为T4人工门槛；本次四组/限时以application delta为准。CPU不替真实GPU，历史成绩不替候选，中低端下一玩法前门槛与T5/GL退役/服务拆分各自独立。
