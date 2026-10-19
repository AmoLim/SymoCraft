---
tags:
  - area/spec
---

# T4 验证记录

旧CPU夹具采用真实生产simulation/World/ECS，24组案例串行运行，每案显式Reset旧共享固定步预算，无窗口/renderer。物理15组与玩法9组在Debug/Release各重复三次；旧原地改写、共享预算限制保留，不证明D04/D11/D13、双会话或D12新旧装配对照。历史工具为MSVC14.38.33145.0/SDK10.0.22621.0，原构建目录out/m3-t4/cpu-b02-debug及cpu-b02-release；完整旧方法和命令由全局退役材料恢复导航读取，本轮未重测。

本次只整理既有验证入口，未构建、执行 CPU/GPU 测试或运行游戏，也未扩大 T4 实施授权。关键节点批准仅见 [全局索引](../../../verification/VERIFICATION.md#关键节点)。

2026-10-09 CPU 局部授权内仅旧行为基线/夹具已交付：15 物理和 9 玩法组，Debug/Release 各重复三次；Debug 启用 58/58、world.allocations 禁用 1，Release 59/59。关闭测试 CPU 生产构建、physics_system.cpp 单编译及真实闭包检查有既有依据，原沙盒 C1902 失败仍保留。结果来源见 [原任务范围](tasks.md#已交付范围)、[T4 验证记录](verification.md)和 [旧冻结包](../../../verification/archive/milestone-evidence-20261011.7z)，不复制或重新验证旧包内材料。
P01 接口候选和消费者静态核对已交付，Q01-Q04 仍未选择、人工意见待确认；app 参考未冻结，不是完整可复现仓库/游戏包。没有新 physics/SimulationSession、双会话、SDL/GPU、性能或 D12 结果，不能由基线成绩推定目标验收通过。

后续结果统一更新本页，默认无附件，无特别要求不附原始数据；已确认的逐次对照、完整序列或性能留存要求按 [治理规则](../../../DOCUMENTATION_MAP.md#每个-change-一份验证记录) 执行，不补造预算、轮数或人工通过。旧冻结材料不动。
