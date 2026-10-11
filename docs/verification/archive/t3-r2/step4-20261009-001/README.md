---
type: 查验归档
status: R2顺序4候选查验快照，不是可运行包或R3批准
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
tags:
  - area/milestone
  - topic/rendering
---

# 顺序4查验归档

这里是Obsidian库内的只读查验副本，原始out和运行包未移动。raw共4430份；[复制核对](archive-check.json) 全部hash匹配，原件在归档时相同；[清单](validation-manifest.json) SHA256为 `fe63e4bef9407648b4ea52ede631549c83678cf6fb75fa69eaae508acf09a008`。不含exe/DLL/lib/PDB/obj，不能从此目录运行游戏。

实施/限制见 [顺序4报告](r2-step4-implementation.md)，人工主表与各编号命令仍唯一维护在 [manual-verification](milestones/m3-t3/manual-verification.md#附录-b-r2-生产候选命令)。本归档不另设checkbox；自动成绩不代填人工操作。

## 候选身份

| 候选 | manifest SHA256 | 用途 |
| --- | --- | --- |
| [Debug002](raw/out/m3-t3/package/d3d12-game-debug-002/package-manifest.json) | `b3db1103f9241f926a5dcf5f0df11eb0ec55315f4b302e5b0e03fb1c8e9ce8f8` | 完整生产过渡游戏，本机诊断，不可再分发 |
| [Release002](raw/out/m3-t3/package/d3d12-game-release-002/package-manifest.json) | `dbf317beb24ad0ece7d9535216bbf919999bb2501f479adf19a4c8a3464ff086` | 人工主入口，匹配app-local CRT，尚非新机器部署通过 |

实际包位于工程 `out/m3-t3/package/`，启动命令使用人工页中的绝对路径。两配置3282项生产来源逐项相同且归档时匹配当前输入；[汇总](raw/out/m3-t3/r2-step4-summary-001/delivery-summary.json) 固定actualexe/build/manifest/GPU身份与限制。旧001包保留，002另建并补齐notices；不要把001的运行材料当002的新成绩。

- [最终构建/完整CTest/打包](raw/out/m3-t3/r2-step4-final-delivery-002/result.json)：native Debug69/69启用项（1项原有禁用）、Release70/70，候选Python单测24/24。
- [最终CPU-only](raw/out/m3-t3/r2-step4-final-cpu-001/result.json)：Debug59/60、foundation.files Win32 1175失败，Release61/61。全量并非全部绿，不改Fix1/Q06状态。
- [Debug JUnit](raw/out/m3-t3/r2-step4-final-delivery-002/debug-ctest.xml)、[Release JUnit](raw/out/m3-t3/r2-step4-final-delivery-002/release-ctest.xml)、[CPU Debug失败](raw/out/m3-t3/r2-step4-final-cpu-001/debug-ctest.xml)。
- [补充关闭测试/工具配置收尾](raw/out/m3-t3/r2-step4-notests-matrix-001/closure.json)：沙箱C1902，提权被中断、未执行，不能记通过。

## 世界与画面

[Debug004完整结果](raw/out/m3-t3/r2-step4-candidate-debug-004/verification.json) 14/14、[Release002](raw/out/m3-t3/r2-step4-candidate-release-002/verification.json) 11/11检查通过；其中两配置各三组D3D12及实际普通400帧，Debug另三组GL。设备都是实际RTX5070Ti/LUID00000000:0001435b/driver32.0.16.1664；short workloads真实1920×1080/4x/Immediate/validation，完整441区块、seed424242。

| 画面 | Debug D3D12 | Release D3D12 | Debug GL对照 |
| --- | --- | --- | --- |
| static | [PNG](raw/out/m3-t3/r2-step4-candidate-debug-004/d3d12-static/final-frame.png) | [PNG](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-static/final-frame.png) | [PNG](raw/out/m3-t3/r2-step4-candidate-debug-004/opengl-static/final-frame.png) |
| walk | [PNG](raw/out/m3-t3/r2-step4-candidate-debug-004/d3d12-walk/final-frame.png) | [PNG](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-walk/final-frame.png) | [PNG](raw/out/m3-t3/r2-step4-candidate-debug-004/opengl-walk/final-frame.png) |
| edit | [PNG](raw/out/m3-t3/r2-step4-candidate-debug-004/d3d12-edit/final-frame.png) | [PNG](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-edit/final-frame.png) | [PNG](raw/out/m3-t3/r2-step4-candidate-debug-004/opengl-edit/final-frame.png) |

九图已逐张视觉复核，非空且无倒置，D/R三个native解码RGB相同；[像素观测](raw/out/m3-t3/r2-step4-summary-001/image-observations.json) 不作正式容差或性能通过。GL walk最终实际x与native不同，不能作为同位姿画面等价测试。现代透明discard不写深度已获用户批准，其余差异不能一概归为该修正。

### 报告入口

- Release static：[summary](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-static/summary.yaml)、[frames](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-static/frames.csv)、[status](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-static/status.yaml)。
- Release walk：[summary](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-walk/summary.yaml)、[frames](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-walk/frames.csv)。
- Release edit：[summary](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-edit/summary.yaml)、[frames](raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-edit/frames.csv)。
- GL walk：[frames](raw/out/m3-t3/r2-step4-candidate-debug-004/opengl-walk/frames.csv)，保留1个GPU缺失，不填0；旧GUI工具仍不消费现代中立CSV。
- 安装来源/运行依赖：[dependencies](raw/out/m3-t3/package/d3d12-game-release-002/provenance/dependencies.json)、[实际imports](raw/out/m3-t3/package/d3d12-game-release-002/provenance/imports-SymoCraft.exe.log)、[notices来源](raw/out/m3-t3/package/d3d12-game-release-002/licenses/SOURCES.txt)。

## 资源与窗口

首次58,818,904bytes/362calls；static/walk不重传cache几何，edit35次计划写入/105次网格更新/25,137,000bytes，最终摘要dc99578b13c5bcf2。计划编辑不是鼠标回放，普通互动不自动导出CSV或截图。

- 完整游戏窗口：[Debug003](raw/out/m3-t3/r2-step4-window-debug-003/verification.json)、[Release004](raw/out/m3-t3/r2-step4-window-release-004/verification.json)；各连续最小化12.508/12.067秒、恢复缩放、目标WM_CLOSE、exit0。暂停/恢复实际rendered_frames相同，恢复后继续增加；不是native fence/Present计数或恢复像素判定，Esc待人工。
- 资源/空网格/4x/数值/VSync/原生恢复：[Debug正常002](raw/out/m3-t3/r2-step4-native-probes-debug-002/normal/result.json)、[Release正常001](raw/out/m3-t3/r2-step4-native-probes-001/release-normal/result.json)。固定几何夹具非完整世界；无截图立即逻辑destroy未强制实际GPU重叠。
- shader构造失败后再成功构造与清理：[Debug缺shader](raw/out/m3-t3/r2-step4-native-probes-001/debug-missing-shader/result.json)、[Debug坏shader](raw/out/m3-t3/r2-step4-native-probes-001/debug-bad-shader/result.json)、[Release缺shader](raw/out/m3-t3/r2-step4-native-probes-001/release-missing-shader/result.json)、[Release坏shader](raw/out/m3-t3/r2-step4-native-probes-001/release-bad-shader/result.json)。

## 故障与限制

完整包故障只破坏隔离副本：缺DXIL exit2，坏DXIL/atlas/YAML exit3，非法renderer exit64，未交付Vulkan具名Unsupported exit3。查看同轮verification与stderr，检查原包hash未变，不能把它们当实际DeviceLost、提交失败、在途timeout或分配失败。

| 原失败 | 留存与复测 |
| --- | --- |
| 沙箱资源拒绝 | [candidate Debug002](raw/out/m3-t3/r2-step4-candidate-debug-002/verification.json)，在正常授权环境另轮复测；不改资产解析规避 |
| 验证器SystemRoot键大小写 | [Debug001](raw/out/m3-t3/r2-step4-candidate-debug-001/verification.json)，修正并新增单测 |
| 首次Ninja freshness误判 | [打包debug001](raw/out/m3-t3/r2-step4-package-debug-001/package.log)、[debug002](raw/out/m3-t3/r2-step4-package-debug-002/package.log)，白名单只容许glob/资产同步，真实重建仍拒绝 |
| 窗口提前恢复/隐藏启动焦点 | [Debug001](raw/out/m3-t3/r2-step4-window-debug-001/verification.json)、[Release001](raw/out/m3-t3/r2-step4-window-release-001/verification.json)、[Release002](raw/out/m3-t3/r2-step4-window-release-002/verification.json)，正常可见启动后复测通过 |
| native夹具首帧Occluded | [Debug正常001](raw/out/m3-t3/r2-step4-native-probes-001/debug-normal/result.json)，旧断言拒绝Skipped帧的真实GPU样本；隐藏控制台复跑002通过，生产renderer未改变 |
| 并发ZIP迁移测试假阴性 | [final-delivery001](raw/out/m3-t3/r2-step4-final-delivery-001/debug-ctest.xml)，Windows ZipInfo构造改写测试非法名称；并发修正夹具后002全量通过 |
| Win32 1175 | [最终CPU Debug](raw/out/m3-t3/r2-step4-final-cpu-001/debug-ctest.xml)及早期native原失败，仍未修复，不改Fix1/Q06结论 |

同主机裁剪开发变量不等于无SDK新机器；Debug不可再分发，Graphics Tools/driver仍由机器提供。现代独占/游戏全关闭、未执行的关闭测试/工具补充配置、旧GUI现代协议、正式参考/容差与性能预算、实际设备/提交/在途故障仍缺。14项人工玩法及15分钟未执行，T3-M08至M15均待核查；R3、默认切换及Vulkan未批准。
