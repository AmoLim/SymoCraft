---
tags:
  - area/spec
---

# T4 Simulation Delta

未实现的目标；未改动的[现役编辑契约](../../../../specs/simulation/spec.md)及下列MODIFIED中全部继续有效场景保留。选择见 [decisions](../../decisions.md)，机制见 [design](../../design.md)，A编号均限定 M3-T4。

## MODIFIED Requirements

### Requirement: Explicit simulation storage and timing

SimulationSession SHALL 同步借用稳定、专属的 World/Registry，调用方保证其寿命覆盖会话；不自行查找 Application/窗口。每会话 SHALL 独立拥有唯一 120 Hz、最多 8 步预算及控制状态，不共享静态余数；核心单步不消费第二预算。Advance SHALL 接受显式 double 秒，先验证 finite/非负，再在 double 中裁剪至 0.1 秒，最后转 float 为角色规则/冷却及预算的共同输入。预算 SHALL 保留原最多 8/120 秒累计、余数/取整容差；规则时间不缩为 8/120。调用方 MUST 串行、非重入推进；不承诺整次推进回滚。

#### Scenario: Existing zero and long frame budget
- **WHEN** Advance 收到零时间、正长帧或不足一固定步的合法时间
- **THEN** 零时间不强行补步或清余数，仍执行一次角色规则/输入/交互及有效跳跃；正长帧规则最多 0.1 秒、物理最多 8 步，短时间余数留到后续，预算不是每实体各消费一次

#### Scenario: Existing shared timing reset
- **WHEN** 加载后的会话开始、显式暂停/恢复或安全出生点恢复需要清预算
- **THEN** 只重置本会话余数，另一 World/Registry 会话不受影响；不得保留第二条活动全局预算路径

#### Scenario: M3-T4-A07 A08 independent sessions and time
- **WHEN** 两份 World/Registry 会话交错推进相同或不同显式时间，或基准场景使用实际经过时间
- **THEN** 预算/材料/冷却独立，角色每 Advance 一次、核心每子步一次，返回有效规则 float 时间；场景实际时间不改成裁剪时间，不承诺同 Registry 多会话或多窗口

### Requirement: Existing character and pointer rules

角色规则 SHALL 与物理预算分开，每Advance先处理有序指针/意图、更新朝向与角色规则，再执行零到多物理步。指针 SHALL 逐事件修改yaw/pitch/FOV，保持pitch [-89,89]、原灵敏度和镜头限制，不汇总后裁剪。有效跳跃 SHALL 在角色阶段立即改向上速度、清接地/待跳并设跳跃状态，即使零物理步；位置仅由后续正常步推进。玩家组件 SHALL 是位姿唯一权威，镜头按值管理，不保留只复制玩家姿态的相机实体。

#### Scenario: Accepted jump with zero physical steps
- **WHEN** 重力角色已接地、非sensor且接受有效跳跃，本次预算零步
- **THEN** 跳跃状态/速度立即生效且请求消费，不延迟或补步，不新增落地前预输入/离地宽限

#### Scenario: Ordered pointer clipping and input priority
- **WHEN** 正反指针事件依次输入或前后/左右同时按下
- **THEN** 指针按次序裁剪，保持前优先后、右优先左，不重复应用事件和汇总增量

### Requirement: Existing voxel movement and query semantics

每物理步 SHALL 保持位置→加速度→重力→逐轴限速→碰撞/接地、sensor跳碰撞/清接地、y降序x/z升序扫描、严格重叠/既有轴平局及速度/加速度清理/接地容差，不宣称CCD。World适配 SHALL 将OutsideWorld/高度越域为非碰撞，未定义方块具名报错而非空气；运动按D04值计算成功后才提交该实体，失败范围按下述Rejection requirement。

射线 SHALL 返回中立自有RayHit并保持单位格DDA等边界同时跨越、法线/最大距离/无命中语义；非法数值/查询参数用中立具名错误拒绝，不再混为无命中。查询无draw参数或绘制副作用。

#### Scenario: Sensor and current support
- **WHEN** sensor运动或当前支撑被移除后执行真实物理步
- **THEN** sensor不碰撞且清接地，非sensor按当前World查支撑，不依赖GPU网格

#### Scenario: Ray boundaries and query failure
- **WHEN** 射线从整数边界负向出发、同时跨多个轴、无命中或读取未定义方块
- **THEN** 保持格/法线/距离及正常无命中值；查询/参数错误具名传播，不伪造命中或把坏定义吞为空气

## ADDED Requirements

### Requirement: Owned compute result and optional view

Advance SHALL 按 D09 返回最小自有编辑请求、选择、必要姿态/镜头及推进统计，不包含组件地址、World/Registry 指针、GPU句柄或完整存储副本；下一次调用不改旧结果。意图 const 引用与有序中立指针事件 const span SHALL 只同步借用，不保存地址、不直接消费平台 InputFrame。会话 SHALL 不需要 Runtime/Window/Renderer、画面比例或 GPU 样本；图形视图按需从姿态/镜头值派生，不推进规则/预算或建立第二可写相机实体。

#### Scenario: M3-T4-A16 A24 A26 compute and view separation
- **WHEN** 纯 CPU 调用方推进真实生产会话，或省略/重复视图派生与呈现、提供非法比例
- **THEN** 必要计算照常成立，旧结果保持；非法比例只拒绝视图、不改 CPU 状态；普通编辑仍由调用方在 Advance 后经 World 校验提交

#### Scenario: M3-T4-A27 value return evolution
- **WHEN** 实施 Advance 结果构建/交接
- **THEN** 简洁代码注释记录自有值选择、调用方所有权与 D09 方案2评估触发条件，落实后更新当前架构；不提前采用输出缓冲或承诺零分配

### Requirement: Explicit pause and recovery

会话 SHALL 只接受显式 Pause/Resume，不解释窗口状态。普通暂停 SHALL 清移动意图、待跳、水平速度和预算，不额外重置材料、镜头、冷却或全部运动状态。首次成功恢复 Advance SHALL 有效 dt=0，不追赶暂停时间；Paused 必须 Resume 后推进。安全出生点恢复 SHALL 保留原掉出世界后的速度/接地/跳跃/预算复位规则。

#### Scenario: M3-T4-A09 A15 pause and safe spawn
- **WHEN** app/CPU 显式暂停恢复，或玩家低于既有恢复边界
- **THEN** 仅清约定状态，普通零时间调用不被误当暂停；恢复不重放旧短按/指针，当前 held 按规则生效，安全出生点不留下全局余数

### Requirement: Rejection and partial advance failure

Advance MUST 在改角色/镜头/冷却/预算前完成 double 时间及全部中立指针 finite/安全表示参数预检；非法参数或调用状态 SHALL 以中立具名 Rejected 类别拒绝且相关状态不变，不进入 Failed。实际推进后的查询/存储/数值或结果构建故障 SHALL 先停止后续提交并置 Failed，再报告中立具名 AdvanceFailed 的操作/阶段及已知提交范围。每实体小型值计算成功且校验后才提交；失败实体不提交，前序实体/子步和本次已执行角色规则不回滚。错误不携带可误提交的成功编辑载荷，不依赖 renderer 错误或全工程错误框架。

#### Scenario: M3-T4-A13 preflight and explicit repair
- **WHEN** 时间为负/NaN/Inf、指针非有限/不可安全表示，随后调用方显式修正再调用
- **THEN** 第一次拒绝前后状态/预算一致，修正可重新调用；不自动回放平台已消费事件，不把相反方向同时按下改为非法

#### Scenario: M3-T4-A14 partial commit and Failed
- **WHEN** 某实体/子步查询、存储、数值或结果构建失败
- **THEN** 失败实体不提交，前序提交及规则事实保留并可定位；Failed 拒绝继续 Advance、不自动恢复，不冒称整帧事务；诊断按可用资源保留，未知/分配故障也不得继续

#### Scenario: M3-T4-A13 A14 legacy ECS and query errors
- **WHEN** 必需实体/组件缺失、旧 ECS 分配/空值路径阻断，或查询无法解释方块
- **THEN** 先检查已知前提，最小修补阻断路径或具名报告；Logger/Error/Assert/exit 不被 catch 宣称成安全传播，查询错误不当 false/空气；T5完整审计仍独立

### Requirement: Same production CPU comparison

D12 对照 MUST 调用同一生产 simulation/physics/World/ECS，以相同源码/构建、玩法资产/seed/配置、初态、意图/有序事件/double时间/编辑顺序，在每 Advance 和每 World 提交后分别比较。实体按逻辑角色对应，不比较原 EntityId/WorldId、地址、GPU/网格版本或耗时；采集只读且不推进预算，不暴露整个 Registry。离散值 MUST 精确，所有有效浮点 MUST 有限、逐分量固定绝对误差：位置/速度/加速度/射线距离 <=1e-4，yaw/pitch/FOV <=1e-4 度（不额外取模），冷却 <=1e-6秒，余数 <=1e-9秒；有效规则 float 时间精确一致。不得加相对/累计/坐标放宽或以阈值覆盖离散分歧。

#### Scenario: M3-T4-A24 A25 normal sequences
- **WHEN** 比较零/多步/长帧、跳跃、墙/地/角/低顶、支撑移除、有序裁剪、冷却/编辑拒绝、暂停恢复及双会话
- **THEN** 接地/跳跃/待跳/sensor/重力/选材/步数、命中存在性/格/法线、编辑存在性/操作/坐标/ID/次序、World状态/拒绝原因/相关方块精确；有效浮点满足固定阈值，无命中/无请求不比较无语义字段

#### Scenario: M3-T4-A25 A26 failure and lifetime sequences
- **WHEN** 比较预检拒绝/显式修正、部分推进失败、可选/重复/非法视图或持有旧结果
- **THEN** 拒绝状态不变、故障提交阶段/Failed语义一致，视图/采集不影响计算及旧结果；留首个分歧的版本/序列位置/字段/左右值/阈值或错误阶段，不要求异常文字相同，不宣称跨平台/不同dt确定性
