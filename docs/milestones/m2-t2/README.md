---
tags:
  - area/milestones
  - topic/world
---

# M2-T2 可复现世界与测试场景

日期：2026-10-03。对应 [项目范围 M2-T2](../../spec/project-scope.md)。本项实现与列明的自动/有限帧验证完成，交由用户验收；**M2 整体、完整人工玩法、性能基线和 Y9000P 验收仍未通过**。

## 交付内容

- 显式 `--seed`，默认普通游戏仍随机开局；日志保存可重建的实际 seed、所有派生噪声 seed 和配置。
- 生成版本 1：区块坐标有序遍历，植被随机序列按 seed 和区块坐标派生；所有地形先完成，再固定顺序写植被。
- 方块四字段的坐标有序摘要，真实 exe 的 `--world-summary` 无窗口模式，3 项新增 CTest。
- 测试场景版本 1：7 个姿态检查点、固定移动路线、16 步边界编辑计划，覆盖正负交界、四区块交点、墙角、低顶和单块落地。
- [生成模块技术说明](../../legacy/architecture/world-generation.md) 与 [场景操作及边界说明](../../project/benchmark/testing/reproducible-scenes.md)。没有接入存档、输入回放、后台线程或流式区块。

## 验证结果

| 验证 | 结果 | 证据 |
| --- | --- | --- |
| Debug 当前全套 CTest | 13/13，退出码 0 | [最终构建/测试日志](evidence/build/m2-t2-debug-verified-unrestricted.log)、[逐项输出](evidence/build/debug-LastTest.log) |
| Release 当前全套 CTest | 13/13，退出码 0 | [最终构建/测试日志](evidence/build/m2-t2-release-verified-unrestricted.log)、[逐项输出](evidence/build/release-LastTest.log) |
| 重复生成与清理重建 | 同配置完整方块快照和摘要相同；反向创建区块不改变结果；不同 seed 改变结果 | `world.generation`，同时验证植被确实存在、关闭植被、场景/编辑重建 |
| 跨进程完整世界 | Debug、Release 各两个独立进程，全 441 区块报告逐字一致 | [Debug first](evidence/headless/debug/first.yaml)、[second](evidence/headless/debug/second.yaml)；[Release first](evidence/headless/release/first.yaml)、[second](evidence/headless/release/second.yaml) |
| 随机 seed 重建 | 默认 headless 开局记录 seed `1077478039`，显式重建相同配置、摘要和出生点；两份报告只有 seed_source 不同 | [随机报告](evidence/headless/random-world.yaml)、[显式重建](evidence/headless/replayed-world.yaml) |
| Debug 真实渲染 | 四区块交点、预应用编辑、120/120 帧、退出码 0、无超时、GL 诊断 0 | [结果](evidence/runtime-debug/result.json)、[stdout](evidence/runtime-debug/stdout.log)、[stderr](evidence/runtime-debug/stderr.log) |
| Release 安装版真实渲染 | 同场景 120/120 帧、退出码 0、无超时、GL 诊断 0；从独立证据目录启动 | [结果](evidence/runtime-release/result.json)、[stdout](evidence/runtime-release/stdout.log)、[stderr](evidence/runtime-release/stderr.log) |

实际设备为 `NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2`，GL `4.6.0 NVIDIA 616.64`。真实运行经授权在沙箱外执行；没有注入键鼠或进行新的人工视觉验收，两个进程均自动结束。有限帧运行只证明这条生成、网格、真实驱动绘制、清理路径走通，不能证明画面细节、碰撞手感和交互正确。

最终 CTest 也在授权的沙箱外执行。此前沙箱内两个既有资源测试的重定位子进程返回 1；同一测试实现外部复测通过，按执行环境限制保留记录，不通过删测或放松断言掩盖。CTest 与 preset 联用时，本机 `LastTest.log` 写到 `out/build/windows-*/Testing/Temporary`，其 Command 字段已核对指向实际 `out/m2-t2/*` 测试程序；报告中的副本来自这些文件。

## 固定输入结果

配置：`seed=424242`，生成版本 1，半径 10，植被开启，`regression` 版本 1，`four-chunk`，应用 16 步编辑。

| 状态 | 方块摘要 |
| --- | --- |
| 地形和植被 | `fnv1a64:cd80ebb0446c15c6` |
| 测试场景覆盖后 | `fnv1a64:bddd435ea737fa62` |
| 编辑序列执行后 | `fnv1a64:45c511cf1e148da2` |

本次两个配置的无窗口与真实渲染日志均得到上述值。它们不是跨平台金标；今后修改生成版本或配置时应更新基准并保留旧证据，不默默沿用。

## 构建与候选版本

独立目录使用本机 MSVC 19.38.33145 x64、CMake 4.3.1、Ninja 1.13.2，未改动 CLion 设置。没有新增第三方依赖，vendor/lib 未修改。构建来自基础提交 `17e23e0bc4e51921c9ad8b53c04f809375cdb509` 加本次未提交源代码，不能只用该提交号标识新 exe。

[构建身份清单](evidence/build-identity.json) 记录 98 个项目输入文件的 SHA256、两个 exe 及安装资源身份。哈希是身份核对，不是源码归档；用户已有文档改动保留，没有生成 Git 提交。

```powershell
./scripts/build.ps1 -Configuration Debug -Action Test -CLionPath 'E:/Applications/JetBrains/CLion' -BuildDirectory out/m2-t2/debug
./scripts/build.ps1 -Configuration Release -Action Test -CLionPath 'E:/Applications/JetBrains/CLion' -BuildDirectory out/m2-t2/release
& 'E:/Applications/JetBrains/CLion/bin/cmake/win/x64/bin/cmake.exe' --install out/m2-t2/release --prefix out/install/m2-t2

./out/install/m2-t2/SymoCraft.exe --scene regression --checkpoint four-chunk
./out/install/m2-t2/SymoCraft.exe --world-summary --seed 424242 --scene regression --checkpoint four-chunk --test-edits
```

可运行候选版：`out/install/m2-t2/SymoCraft.exe`，保持旁边的 `assets` 完整。Release exe SHA256：

```text
1737F033362BA974D674301D84E13F5A9DA7B8F21B11DAD5B581DC751DB3FCDB
```

这是本机开发候选安装目录，不是已通过无开发环境部署验证的最终发行包；没有替换 M2-A 的历史包身份，也没有宣称运行库部署问题已解决。

## 失败与修复记录

| 过程 | 处理与复测 |
| --- | --- |
| [首次 Debug 构建](evidence/build/m2-t2-debug-build-test.log) 暴露 FastNoiseLite 旧接口不支持 const 采样 | 在生成适配器标注噪声成员的 legacy API 约束，不修改 vendor、不声称线程安全 |
| [第二次 Debug 构建](evidence/build/m2-t2-debug-retest.log) 暴露头文件 `Remap` 多重定义 | 给已有头文件函数加 `inline`，新生成编译单元与区块同时使用时可正确链接 |
| [沙箱内最终复测](evidence/build/m2-t2-debug-verified.log) 11/13，两项重定位资源测试失败 | 外部环境 Debug/Release 各 13/13；失败证据保留 |

另保留 [Debug 中间成功日志](evidence/build/m2-t2-debug-final.log)、[Release 首轮完整构建](evidence/build/m2-t2-release-build-test.log)、[安装日志](evidence/build/m2-t2-install.log)。最终列明结果包含检查点视角微调及测试警告修正，不用中间版本替代。旧内存模块的遮蔽/未使用参数、`localtime` 等编译警告未在本项扩大范围处理。

## 尚未验证与后续

1. 固定路线是位置/朝向约定，不是自动输入回放；`--test-edits` 是启动阶段有序世界写入，不经过射线、防抖或逐帧编辑重建。真实玩法仍按 M2-T4 检查。
2. 夹具初始 AABB 不嵌入实心块已有测试，实际墙角、低顶及单方块落地尚未逐项人工验收；没有新增这些动作的画面或录像证据。
3. 现有 `loading_ms` 含新增摘要扫描，不含整个初始化或首帧 GPU 呈现。没有采集正式 CPU/GPU、上传、内存、P95/P99 基线，不能据此宣称性能提高或达到 60 FPS。
4. 笔记本、双机各 15 分钟连续游玩、完整资源显示及故障回归仍需对最终候选版本执行；本项没有重复 M2-T1 的全部资源损坏探针。
5. 同 seed 仅保证所述环境内初始方块确定性，不保证浮点跨平台、真实玩家轨迹或透明物体绘制次序一致。

用户确认本项后，下一项为 M2-T3：基于这些冻结输入补齐性能观测和双机基线，不提前开始 M3 大规模架构迁移。
·
