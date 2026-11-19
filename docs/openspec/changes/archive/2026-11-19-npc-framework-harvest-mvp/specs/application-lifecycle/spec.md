---
tags:
  - area/spec
---

## ADDED Requirements

### Requirement: NPC MVP assembly and bounded evidence

应用 SHALL 装配明确配置的 NPC MVP 场景，默认验证规模为两名人类与一名 Idle 僵尸，支持独立目标及同目标争抢两种验证。普通玩家的即时编辑及现有独立 `static/walk/edit` 场景继续按既有契约运行。NPC 场景的启动、运行及关闭 SHALL 沿用现行装配和清理规则。MVP 验证 SHALL 分别记录正式 CPU/World/物理、测试替身和真实 GPU 的结果；人工在本机 D3D12、Vulkan 各体验一次，每后端操作及观察合计最多 3 分钟，两后端合计最多 6 分钟。各后端的子场景、配置切换及重启共用该上限。

#### Scenario: NPC-MVP-A21 Configured production scene
- **WHEN** 分别运行独立目标与争抢配置，或运行现有的独立性能场景
- **THEN** NPC 场景可核对各自进度、World 移除、计数隔离和僵尸 Idle，既有性能场景继续使用原初态、动作和编辑时点

#### Scenario: NPC-MVP-A22 Bounded modern backend check
- **WHEN** 分别在真实 D3D12 与 Vulkan 后端观察角色外观、移动、采集反馈及退出
- **THEN** 按实际后端分别记录结果、失败或未执行项，达到时限即停止；短体验与正式 CPU 规则证据分开记录

### Requirement: Observable harvest demo controls

应用 SHALL 提供便于观察的采集演示：默认由一名人类展示行走、到达、耗时采集和产出全过程，可切换两名人类争抢同一资源的对照场景，Idle 僵尸放在不遮挡人类、目标及反馈的侧边。原两人独立目标和同目标争抢的三 NPC 正确性验证 SHALL 保留。演示使用真实生产移动和采集路径，完成或失败后仍进入 Idle，不自动搜索或重试。

演示 SHALL 提供可发现的开始、暂停/继续、从头重新演示和场景切换入口。初始 Ready 状态 SHALL 展示场景并等待开始；手动暂停 SHALL 停止树更新、模拟及采集提交，保留进度与姿态，在窗口可绘制时继续展示画面和控制。恢复 SHALL 不补算暂停时间，不回放旧输入；重新获焦不得解除仍有效的手动暂停。关闭、失焦、最小化及不可绘制窗口继续服从现行平台规则。

#### Scenario: NPC-MVP-A36 Start pause and resume an observable demo
- **WHEN** 默认单人演示在 Ready 等待，用户开始后于移动或采集中暂停，再经历失焦/获焦并选择继续
- **THEN** Ready 不推进，开始后显示真实动作；暂停期间位置、有效耗时、姿态和资源保持不变且可绘制时仍显示控制，获焦不自行继续，继续时不补算暂停时间或回放旧输入

### Requirement: Demo restart and scene isolation

从头重新演示和切换场景 SHALL 从新一轮初态开始，恢复资源方块及 NPC 的动作、树和资源计数，清除旧姿态和反馈。旧实体身份、动作及未提交批次 SHALL 不进入新一轮；渲染帧及在途资源继续满足现有生命周期，不因演示重置而提前释放。装配失败 SHALL 报告错误并停止演示。此控制不提供任意时刻回退、历史录制/恢复或历史续算，不改变普通玩法和既有性能场景。

#### Scenario: NPC-MVP-A37 Restart or switch without stale state
- **WHEN** 在进行中、暂停或完成后从头重新演示，或反复切换单人与争抢场景，旧帧仍可能在途
- **THEN** 每次回到新一轮 Ready，资源目标和 NPC 状态恢复初态，旧动作/批次不能影响新轮，旧进度和结果消失，其他共享网格及在途资源保持有效；装配失败则明确停止

### Requirement: Truthful harvest demo feedback

演示 SHALL 在画面内显示各人类身份、当前阶段、目标、有效采集进度、资源数量及成功/失败原因。显示 SHALL 使用真实只读交接：就绪不等于成功，只有 World 改变且产出入账完成后才提示成功，争抢败方不得显示获产出；未知事实保持未知。反馈读取不消费或重复结算结果，重播及切换后不残留旧轮提示。该显示限演示控制和采集反馈，不扩为通用调试面板。

#### Scenario: NPC-MVP-A38 Visible completion and contest feedback
- **WHEN** 分别播放单人完整采集和双人争抢场景，并重复读取或绘制已完成结果
- **THEN** 清楚看到行走、采集动作及真实进度，僵尸不遮挡；成功移除者只获一次真实产出，争抢败方显示目标失效且未获产出，终态和资源不因重复显示改变，完成后保持 Idle 等待操作

## MODIFIED Requirements

### Requirement: Local frame policy and commit order

Application SHALL 依次处理平台事实→关闭/暂停/输入来源→普通意图或 Scenario 模拟前编辑→每次 Advance 角色一次/物理零到八步→恢复/射线结果→普通玩家 World 校验编辑→本帧已就绪 NPC 采集提交→World 网格/绑定→Render/Present→中立观测。窗口回调 SHALL 不推进会话/改 ECS/提交 GPU；关闭始终优先不继续新模拟/渲染。普通暂停 SHALL 丢旧玩法记录并 Reset 平台，映射会话 Pause/Resume，恢复用当前 held、无旧短按/指针回放及首运动保护；同轮失焦再获焦也清一次，零实际像素不绘制，有限帧失焦诊断例外保留。场景策略 SHALL 按 platform 契约独立，不沿旧性能 Session 或工具连接分支；不因撤掉采集器把场景 VSync-off 请求改回普通 VSync-on。

Application SHALL 在普通玩家编辑取得结果后使用 [simulation 的 NPC 提交规则](../simulation/spec.md#requirement-ordered-harvest-commit-after-player-edits)。若 NPC 提交阶段报告不可恢复故障，Application SHALL 停止后续推进和绘制，以既有运行错误路径交接首诊断及执行有界清理。

演示的 Ready、手动 Paused 和 Completed 状态 SHALL 按上述演示控制契约抑制新模拟，同时允许可绘制窗口更新控制画面；这是演示专属策略，不替代普通失焦暂停或独立 `static/walk/edit` 策略。从头重新演示及场景替换 SHALL 在串行帧安全边界完成，不越过正在执行的 World 提交。

#### Scenario: M3-T4-A09 A16 A17 ordinary and scenario composition
- **WHEN** 普通编辑、static/walk/edit 场景、暂停/恢复或窗口关闭发生
- **THEN** 普通编辑在模拟后、场景编辑在模拟前且保原动作/游标/预条件；普通可读调试值、场景可无记录，真实像素与 renderer 比例一致；关闭/Wait/同轮失焦不会误推进或重放

#### Scenario: NPC-MVP-A23 Player and NPC attempt to remove the same block
- **WHEN** 玩家对资源方块发出有效即时移除请求，且同一帧有 NPC 对该方块的采集动作就绪
- **THEN** 玩家按原交互规则和冷却先完成移除；NPC 提交时报告原目标失效，进入 Idle，资源计数不变；后续网格更新消费真实 World 编辑结果
