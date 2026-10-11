---
type: 消费者核对
status: P01静态核对完成，动态验证未执行，T3交接仍待核对
project: Symocraft
module: M3-T4-App-Platform-Simulation
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
---

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# T4 现有消费者核对

核对日期：2026-10-09。范围为活动 `game/`、根级 `test/`、`tools/benchmark/` 与相关构建入口，排除 inactive legacy 源码作为生产消费者；实验源码另列保留范围。以下是工作区静态源码核对，不是编译/链接或运行结果。关键文件身份见 [来源清单](consumer-source-manifest.json)，T3 仍在修改的工作区不能当成冻结交付。

## 消费者清单

| 编号/消费者 | 当前实际使用 | T4接口迁移要求 | 当前判定 |
| --- | --- | --- | --- |
| C01 main | [main.cpp](../../../game/app/src/main.cpp)：解析/预检，持有可选Session，调用namespace Init/Run/Free，catch RenderError/std/未知异常，Export(bool) | 私有Application owner；保留CLI 64/资产2/运行与关闭3/正常无效4；Q03关闭事实后Seal，再Export回执，不把Error日志当传播 | 调用已定位，迁移未实施 |
| C02 普通帧循环 | [application.cpp](../../../game/app/src/application.cpp)：Window Input借用，MapInput、逐指针输入、角色/Physics、SyncCamera、DoRayCast后TryEdit | 交换自有InputFrame → app策略 → const意图/span → Advance自有结果 → 普通编辑提交；两个冷却/视角/零步跳跃保留；预算迁到会话 | D09–D13可对接，Q01/Q02/Q04待选 |
| C03 场景与CPU工具入口 | 同文件PrepareWorld/PrintWorldSummary及walk/edit分支；场景现在随performance指针启用 | CPU世界摘要不进入Runtime；场景和观测配置独立，walk意图、edit模拟前提交、原始elapsed/seed/摘要保持 | 无窗口推进尚非现有Session消费者，不冒称新入口已有 |
| C04 生命周期/玩家装配 | 当前相机先创建Transform实体，CreatePlayer从相机读取初始pose；Run才装配World；Free清静态owner | 去掉相机实体前先选Q01；World/Registry先于会话且覆盖寿命，失败丢弃专属装配范围，不承诺共享Registry事务；不让新构造依赖窗口比例 | 发现实际装配耦合，不仅是改函数名 |
| C05 模拟内部消费者 | [player.cpp](../../../game/modules/simulation/src/player.cpp)、[character_system.cpp](../../../game/modules/simulation/src/character_system.cpp)、[playercontroller.cpp](../../../game/modules/simulation/src/playercontroller.cpp)、[physics_system.cpp](../../../game/modules/simulation/src/physics_system.cpp) | Controller/镜头/冷却实例化，simulation适配调用physics小型值；RayCastStatic从World解释碰撞移到适配，错误不能当空气；消除全局预算 | 核心仍混合ECS/查询/积分，未有独立target或新错误保证 |
| C06 T3现代相机消费者 | [render_conversion.cpp](../../../game/app/src/render_conversion.cpp)：Camera getters→CameraParameters；[GameRenderer](../../../game/app/src/game_renderer.cpp)：现代FrameInput、resize/呈现/截图 | 输入改为T4自有视图值；Renderer v1不变，仍FOV度→弧度、RH、near0.1/far2000；新镜头不要求aspect或组件地址 | 已看到生产适配源码，未由此推定R2验收 |
| C07 旧GL对照消费者 | 同GameRenderer旧分支由Camera提供view/projection；[legacy_renderer.h](../../../game/modules/renderer/legacy/include/symocraft/renderer/legacy_renderer.h)真正接受Scene CameraView矩阵值 | app用同一中立pose/lens与实际像素派生RH_NO矩阵；无需保留相机ECS实体，保留GL受影响回归而非新增永久后端矩阵 | 可保持Renderer旧签名，不能直接删除旧消费路径 |
| C08 平台及原生桥 | [window.h](../../../game/modules/platform/include/symocraft/platform/window.h)、[input_state.h](../../../game/modules/platform/src/input_state.h)，T3桥继续借用Window | 原型保留SymoCraft::Window及桥友元；width/height读取改事实值；benchmark_目前在reducer过滤键/指针和保留Escape，迁到app必须先按Q02补足事实 | “删benchmark_”不足以保持D10 |
| C09 观测生产消费者 | main/Application/GameRenderer直接写metadata/startup/completed；PollGpu分别SetGpu(row)和SetGpuSample(frame_id) | 全部写入改Recording受控入口；最终缺失GPU事实、关闭结果在Seal前收齐；保留T3现代追加列、统计口径与旧GL关联 | 当前没有Seal，Export会Invalidate并判有效性，与D08目标有实际差距 |
| C10 现有模拟/转接测试 | [simulation_contract_tests](../../../test/unit/simulation/simulation_contract_tests.cpp)、[player_math_tests](../../../test/unit/simulation/player_math_tests.cpp)、[render_conversion_tests](../../../test/unit/app/render_conversion_tests.cpp) | 留旧行为基线，再用真实会话/物理/新转接补测试；旧math Consume非法时间返回0不等于新Session应接受非法时间；去掉Camera实体后重建夹具而非删断言 | 覆盖输入/数学/交互/相机转接，不是现成A24/A25 |
| C11 平台/观测/清理测试 | [key_snapshot_tests](../../../test/unit/platform/key_snapshot_tests.cpp)、[performance_tests](../../../test/unit/telemetry/performance_tests.cpp)、[renderer_performance_tests](../../../test/unit/telemetry/renderer_performance_tests.cpp)、[cleanup_sequence_tests](../../../test/unit/app/cleanup_sequence_tests.cpp) | Q02补事实/交接寿命；Q03保留继续清理/首因；观测测试在Seal前写completed等，成功后另建新Session；失败重试用同一个Sealed记录 | 原测试只定位现有行为，本轮未运行 |
| C12 现代运行与报告消费者 | [d3d12_backend_smoke](../../../test/integration/renderer/d3d12_backend_smoke.cpp)、[game_report_probe](../../../test/integration/renderer/game_report_probe.cpp) | Window新owner/实际尺寸和T3Shutdown顺序需重编译/运行；报告probe继续接受同源记录，CPU检查不能替代真实SDL/GPU | 依赖T3实际候选交接 |
| C13 头/构建消费者 | [PublicHeaderTests](../../../cmake/PublicHeaderTests.cmake)、[ModuleBoundaries](../../../cmake/ModuleBoundaries.cmake)、[VerifyBuildMatrix](../../../test/integration/VerifyBuildMatrix.cmake)、[app CMake](../../../game/app/CMakeLists.txt) | 当前生成头检查只枚举九模块，新增内部physics需要独立include/消费者检查；simulation/physics CPU闭包不引入app_render_data/platform/renderer；共享CMake由实施方集中合入 | 源码依赖图核对，不是本轮构建证明 |
| C14 外部benchmark | [runner.cpp](../../../tools/benchmark/src/runner.cpp)：进程外读summary/status/CSV，校验版本2与GL身份 | 不链接游戏或直接调用Session；D08退出/finished/完整文件语义保持。现代报告无gl_renderer字段，传入现有ValidateCapture会不兼容，不能伪造GL字段绕过 | 工具默认旧GL路径与现代消费者交接分开，见H03 |

## 历史实验与搜索边界

`test/experimental/platform-sdl3`、`sdl3`、`platform-handoff`、`input-comparison` 和 `d3d12-r1` 仍有 Window/输入/Camera 转接的源码消费者；`glfw-baseline` 为历史基线，不改写成T4接口。实施前检查哪些仍被构建或主动维护：活动验证程序需明确适配或退出理由，历史声明/证据不机械改写、不升级为生产接口。没有将实验helpers列为新公开头。

消费者定位结合入口名与include/target；本轮未执行自动编译穷举，不能声称所有别名、隐藏模板实例化或全部构建选项均已证明兼容。正式切换需公开头独立编译、实际消费者链接、禁止依赖负向检查和真实平台/后端验证。

## 关键交接差异

| 现状 | 接口落地要求 |
| --- | --- |
| InputState::CaptureKeys将down与pressed合成一个keys数组；benchmark_参与过滤及跨失焦Escape锁存 | D05迁策略需要可区分的原始事实或等价完整事件，不改变D10清理边界。Q02选择数据表达，不重选app负责策略 |
| PlayerIntent本身还有汇总mouse/scroll，ApplyInput可再次应用它们 | 新意图不带汇总指针，app只传有序中立事件；保留原逐事件裁剪，不能双消费 |
| Camera ctor创建实体，CreatePlayer读取该实体，SyncCamera遍历后复制姿态 | Q01先解决装配入口；视图改自有pose/lens派生，CPU无需相机实体，不借旧循环中的组件引用跨帧 |
| Registry::GetComponent有日志断言；AddComponent直接解引用返回指针；内部Add分配失败只日志/返回 | D13不能靠catch解决exit或空解引用，创建/存储必需路径需最小安全适配/修复与故障证据；本轮不执行T5全面审计 |
| Export(bool)目前改变无效原因并返回valid_run | 改为先汇入最终事实Seal，Export返回保存回执；成功导出不代表有效，失效不等于保存失败 |

普通暂停、Benchmark allow-unfocused/Escape及最小化按D10；当前`--frames`普通自动化路径的焦点豁免不在本轮擅自取消或推广为后台玩家输入。T3最小适配不自动完成T4的全局状态/场景/输入政策重构。

## 交接依赖

H01–H04 是实施前外部核对项，不是要用户重新选择已冻结T3方案，也不因本轮建立文档而标为完成。

| 编号 | 需接收/复核的事实 | 当前边界 |
| --- | --- | --- |
| H01 T3相机/平台消费者 | 实际接管版本的GameRenderer、ToRendererCamera、桥及尺寸读取清单；确认T4用中立值替换旧Camera不改v1 | 活动源码已进入现代路径，R2报告仍保留顺序3/4未交付状态；以真实交接版本为准 |
| H02 T3关闭与GPU样本 | 正常/故障Shutdown的终结责任、不能证明安全回收时的处置、最后已就绪GPU样本交接、截图排除身份 | T4只聚合事实，不新增等待、不把缺失记0、不吞T3首因或覆盖其安全边界 |
| H03 报告和工具协议 | 旧GL与现代metadata/追加列/单位、summary/status终态、实际使用的benchmark/report验证器 | 原runner仍校验GL身份；现代接入需由对应实施方确认消费者适配范围，不修改旧冻结协议或谎填GL字段 |
| H04 同源身份与修改归属 | T3交付来源、T4接收版本，app/telemetry/公共构建文件的合入时点 | 本轮只读生产源码；T3修改所涉43份文件后复核来源清单和受影响行，不在一个采样轮次中混不同实现 |

## 验证状态

源码静态定位和接口差异核对已完成；未编译候选声明，未修改或运行测试，也未进行SDL/GPU/性能采样。对应A01–A27仍未验收，A23继续是D02后续项。接口审阅、Q01–Q04选择、H01–H04交接、整体计划批准和源码实施授权分别记录，不能用本页“已核对”替代其中任一项。
