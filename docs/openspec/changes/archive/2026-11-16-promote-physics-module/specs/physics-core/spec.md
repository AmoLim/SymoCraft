---
tags:
  - area/spec
---

## MODIFIED Requirements

### Requirement: Independent neutral physics core

symocraft_physics MUST 由 `game/modules/physics` 唯一拥有生产实现、公开头及 STATIC target 定义，以 `symocraft/physics` 公开路径及 `SymoCraft::Physics` 命名空间仅依赖标准库/foundation 数学；不得直接/传递依赖 ECS/World/scene/platform/renderer/assets/telemetry/app/图形 SDK、profiling/Tracy 或 document_io/YAML，不取得 simulation include/src 根。核心 SHALL 不遍历实体、不编辑世界、不累计帧预算，不建立第二运动权威或两套算法。simulation SHALL 继续负责固定步预算、ECS 有序遍历、World 只读查询适配和单体求解成功后的逐实体提交，保持既有借用寿命、自有结果及失败部分提交语义。正式迁移 MUST 保持 target 名、公开 include、命名空间、类型/签名/默认值及物理行为，旧路径不得保留活动生产 owner、实现副本或兼容转发层。

D02 的正式模块搬迁 SHALL 在 T5 审计后、NPC 复用前作为独立授权/验收节点完成；本节点承接 M3-T4-A23，不改变 T4 当时的完成门槛或归档中 A23 后续不适用的历史结论。

#### Scenario: M3-T4-A01 A02 A03 independent consumer
- **WHEN** Debug/Release 独立核心消费者、CPU-only/现役独立消费者/关闭测试或负向 include/link 检查执行
- **THEN** 实际核心可无 World/Registry/窗口运行，越界依赖被拒绝，测试链接唯一生产实现而非复制 cpp；关闭测试不引入 fixture，旧工具的历史验证身份保留而不恢复已退役 benchmark 产品

#### Scenario: M3-T4-A23 later formal ownership move
- **WHEN** 在 T5 后、NPC 复用前验证正式 physics 迁移独立节点
- **THEN** 唯一源码及 target 定义 owner 为 `game/modules/physics`，namespace/include/target/接口及行为保持；核对迁移前后核心源码与公开头字节，使用同工具链及等价配置/输入分别运行既有核心、玩家与会话回归，并通过 Debug/Release、CPU-only、关闭测试、公开头、独立消费者、负向边界与生产游戏最终链接验证
- **AND** 方案 A SHALL 保持 simulation 的固定步、碰撞、射线、失败部分提交及玩家编辑时点场景和 D12 阈值；同构建两会话配对不单独作为迁移前后等价证明。失败/禁用/未执行及 GPU 运行未覆盖分别报告，用户独立验收后才解除 NPC 真实刚体复用前置；不把本节点结果回写为 T4 当时 A23 已通过
