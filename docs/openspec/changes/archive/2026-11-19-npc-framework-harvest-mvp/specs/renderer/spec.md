---
tags:
  - area/spec
---

## ADDED Requirements

### Requirement: Static character meshes and shared instances

D3D12 与 Vulkan SHALL 消费同一中立静态角色网格及实例输入，按实体世界位置和整体朝向，以顶点色、硬边法线和简单光照显示完整模型。角色网格 SHALL 独立于基础方块网格。多个实例 SHALL 可共享同一模型，创建、移动或移除实例只改变对应实例的显示。模型和实例资源 SHALL 沿用现代渲染器的输入所有权、拒绝及在途寿命契约。

分件模型 SHALL 通过同一中立的部件网格与逐实例变换输入驱动，网格可共享，部件姿态按 NPC 独立。动作及演示重启不得使另一实例的姿态或仍在途的网格失效。

#### Scenario: NPC-MVP-A18 Same character input on both modern backends
- **WHEN** D3D12 和 Vulkan 分别显示同一场景中移动的人类 NPC 及 Idle 僵尸
- **THEN** 两后端各自能显示可区分的完整模型、正确位置和整体朝向；基础方块和选框仍可绘制

#### Scenario: NPC-MVP-A19 Shared model and instance removal
- **WHEN** 两个实例共享同一模型，一个实例移动或移除且先前绘制仍可能在途
- **THEN** 另一实例继续显示且保留原位姿，移除实例不再进入新帧，仍被使用的模型资源在完成前保持有效

### Requirement: State-driven rigid character motion

显示组合 SHALL 根据只读真实状态驱动简单摆腿、摆臂和采集动作。行走相位 SHALL 跟随实际水平位移，无实际位移时不继续原地走路；采集动作 SHALL 跟随当前动作的采集阶段及有效耗时，不以现实时间自行假装推进。暂停 SHALL 冻结姿态，继续时不补播暂停时间；退出相应动作、取消、失败或进入 Idle 后 SHALL 停止该动作并返回中立姿态。显示不推进模拟、不修改动作结果，且无需骨骼、蒙皮或动画库。

#### Scenario: NPC-MVP-A34 Walking follows actual movement
- **WHEN** 两名人类共享模型，其中一名实际移动，另一名 Idle 或受阻而无实际位移
- **THEN** 移动者摆腿摆臂，另一名不原地踏步；D3D12 与 Vulkan 各自消费一致的独立部件姿态，读取显示不改变物理或动作状态

#### Scenario: NPC-MVP-A35 Harvest pose and pause follow action state
- **WHEN** 人类进入真实采集阶段、手动暂停并继续，随后成功、失败或取消
- **THEN** 采集摆臂及进度基于有效动作时间，暂停时冻结且恢复不追赶现实时间；退出采集后停止摆臂，不把就绪表现成已结算成功
