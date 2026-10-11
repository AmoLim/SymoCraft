---
type: 人工核查入口
status: R1已冻结，R2顺序4候选已交付，T3-M08至M15待人工核查
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-08
updated: 2026-10-09
tags:
  - area/milestones
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# M3-T3 人工核查

本页保留 2026-10-09 的人工反馈、候选入口与逐项命令，原勾选不改写。2026-10-10 起，后续核查要求和唯一活动计划在 [T3 tasks](../../openspec/changes/m3-t3-renderer/tasks.md)，节点批准只在 [VERIFICATION](../../verification/VERIFICATION.md#关键节点)；本页不再维护平行当期主表。下文“当前”及旧入口政策只具记录时点含义，历史要求已迁入新规格，不是新增批准。

本项目的 Obsidian 库根为 `docs/`。R1 查验所需截图、报告和日志已复制到本 milestone 的 [R1 查验归档](../../verification/archive/t3-r1/review-20261009-001/README.md)，本页使用库内相对链接；原始 `out/` 产物不移动。该归档是只读查验快照，不是可运行包或新验证成绩。JSON/日志若没有可用查看器，可阅读归档中的 Markdown 摘要，或执行附录 A 的查看命令。库外 `test/` 源码仍使用当前工程的本机外部地址，移动工程后需更新该地址。

## 当前节点与进入条件

2026-10-09 用户已完成 R1 审阅、勾选 T3-M01–T3-M07，并明确决定：“我已经审查了R1了，可以冻结R1，批准进入R2”。**R1 与 Renderer v1 公开契约/target 设计基线已冻结，当前进入 R2 生产实施。** 见 [冻结记录](../../verification/archive/t3-r1/freeze-20261009-001/README.md) 与 [R2 入口](r2-entry.md)。本页保存人工审阅结论，不据批准虚构新的动态运行或生产验收。

**T2 与 R1 节点批准均已明确记录**：T2 见 [正式节点批准](../m3-t2/README.md#t2-正式节点批准) 和 [批准身份](../../verification/archive/t2-approval-20261009/approval.json)，R1 见本页历史记录及 [冻结身份](../../verification/archive/t3-r1/freeze-20261009-001/freeze-manifest.json)。MB01 方案2已确认 working set ≤ 704 MiB、private bytes ≤ 1408 MiB，FB01 方案2保持；首次可操作时间预算仍未确认。T2 的 Win32 1175/Fix1 暂停和正式同源 Q06 未完成不改判为已修复或实测通过。R1 认可设计与已交付报告，不将生产资产/采样/相机 GPU 画面、真实生产 target 或完整玩法标为通过；这些按 R2/R3 实施与验收。

当前 M3 功能/兼容只限定本机 RTX 5070 Ti，不要求其他 GPU 或第二台机器。下一玩法阶段前的中低端整机性能门槛仍独立保留，见 [项目范围](../../scope/project-scope.md)。顺序4现已交付真实完整游戏候选 `d3d12-game-debug-002` 与 `d3d12-game-release-002`，见 [实施报告](r2-step4-implementation.md) 和 [查验归档](../../verification/archive/t3-r2/step4-20261009-001/README.md)。这不是 R3 批准，Vulkan仍未实施。

| 入口 | 实际可用状态与用途 |
| --- | --- |
| [夹具说明](file:///F:/GameDevelop/OpenGLProject/Symocraft/test/experimental/d3d12-r1/README.md)、[便携包说明](../../verification/archive/t3-r1/review-20261009-001/package/d3d12-r1-release-001/README.txt) | 保留的独立 R1 Release 包，不是生产 Renderer 或完整世界包；本页建立时已核对说明及下方截图/报告文件存在 |
| [汇总与身份](../../verification/archive/t3-r1/preparation-validation.json)、[实际运行结果](../../verification/archive/t3-r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json) | 已有独显运行记录，可阅读审核；汇总中的早期 T2 状态是历史快照，当前 T2 以 [阶段报告](../m3-t2/README.md) 为准 |
| [CPU/窗口冻结门槛](README.md#冻结门槛)、[CPU 契约清单](../../openspec/changes/m3-t3-renderer/design.md#architecture)、[冻结基线](../../verification/archive/t3-r1/freeze-20261009-001/README.md) | 用户已审阅批准 R1，冻结记录固定批准前来源；真实生产实现和依赖图在 R2 落实，不将实验消费者冒充已实现门面 |
| [本轮双配置与转接身份](../../verification/archive/t3-r1/handoff-validation.json)、[转接报告](README.md#r1-转接准备与双配置复核) | Debug/Release GPU 各正常/超时 2/2，CPU 各 4/4；新证据是工作区实验，不是新的便携包，旧包不被覆盖 |

## 状态与维护规则

- `[ ]` 表示尚未人工核查通过；完整核查且达到预期才改为 `[x]`，同时记录日期、执行者、候选版本和证据。自动测试通过、代理检查或包交付不自动勾选。
- 未执行写明原因；失败记录步骤、实际结果及证据，保留原失败再记复测；不适用保持 `[ ]`，写明理由及用户确认。尚未交付不是不适用。
- 编号稳定、不复用。进入新节点时，把当前项目的编号、结论、版本及证据移入页末历史记录，再在同一主表换入下一节点的人工核验项目；历史表不用 checkbox，不形成第二张活动清单。
- 版本变化影响已核查行为时，保留原记录并重新待复核。自动硬边界、CPU 寿命、GPU fence/容量和数值采样由代理留证，用户审核报告；画面正常不能证明同步或回收正确。
- 如需动态复跑，先约定同机候选入口、操作与新证据目录，不覆盖已有结果；故障操作用交付的隔离副本/专用入口，不修改原资产。
- 勾选表示该人工项目成立，不自动批准冻结或进入下一节点。用户批准/退回的决定在页末单独记录。

### 已完成 R1 审阅身份

既有 R1 是 `d3d12-r1-release-001` / `desktop-smoke-001`、D3D12、RTX 5070 Ti、driver `32.0.16.1664`、LUID `00000000:000144d0`；这些只属于历史运行，不能代填新运行实际值。

| 字段                   | 本次记录                                                                                                                |
| -------------------- | ------------------------------------------------------------------------------------------------------------------- |
| 日期、执行者、核查节点          | 2026-10-09；用户；R1 报告/设计审阅及明确批准，不声明新动态运行                                                                              |
| 候选包、提交/源码快照、构建配置     | 既有 Release 包及 R1 Debug/Release 实验材料；批准前 12 份来源/SHA 固定于 [冻结身份](../../verification/archive/t3-r1/freeze-20261009-001/freeze-manifest.json) |
| 可执行文件、操作入口与工作目录      | 既有 R1 入口见上表与历史附录；本次未声明重新执行，R2 生产入口待交付                                                                               |
| 请求/实际后端、实际 GPU 身份、驱动 | 审阅既有 D3D12 / RTX 5070 Ti / driver 32.0.16.1664 / LUID 00000000:000144d0 的报告；不代填为新运行身份                               |
| 截图/录像、日志、报告、原失败与复测记录 | [R1 查验归档](../../verification/archive/t3-r1/review-20261009-001/README.md)；早期失败和复测原样保留                                                    |

## 当期人工核验主表：R2 至 R3

R1 七项已移入历史表，编号不复用。下列入口均对应实际交付的 `002` 生产候选，不用 R1 夹具代替；自动验证和代理审阅不自动勾选。每项的查看/启动命令见 [附录 B](#附录-b-r2-生产候选命令)。查验原件按节点复制至库内 evidence；可运行包仍在工程 `out/m3-t3/package/`。

| 状态 | 稳定编号 | 核验内容与通过要求 | 实际入口 / 证据 | 人工记录 |
| --- | --- | --- | --- | --- |
| [ ] | T3-M08 | 核对 D/R 候选、源码/构建/包 SHA 与实际 D3D12、RTX 5070 Ti、LUID、driver、4x、present、validation 身份；Debug不可对外再分发 | [来源与包](../../verification/archive/t3-r2/step4-20261009-001/README.md#候选身份)；[命令](#t3-m08-来源与设备身份) | 待核查；记录日期、执行者、版本 |
| [ ] | T3-M09 | 固定441区块世界、真实图集/UV/层号、相机/深度/剔除/选框画面正确；现代透明像素不写深度是已批准差异，不能把所有GL差异一概归于它 | [画面](../../verification/archive/t3-r2/step4-20261009-001/README.md#世界与画面)；[命令](#t3-m09-世界材质与相机) | 待核查；记录截图/操作 |
| [ ] | T3-M10 | 审阅静止/移动0几何重传、编辑只更新发布网格，正负坐标/四块交点无错块；结合CPU/原生夹具审核空网格及输入/句柄/回收寿命，区别完整游戏和夹具范围 | [资源范围](../../verification/archive/t3-r2/step4-20261009-001/README.md#资源与窗口)；[命令](#t3-m10-编辑缓存与资源) | 待核查；不能仅凭画面证明fence安全 |
| [ ] | T3-M11 | 实际缩放、连续最小化至少10秒、恢复画面/比例/输入，Esc及关闭按钮正常退出，再启动正常；自动WM_CLOSE不替代实际Esc | [窗口记录](../../verification/archive/t3-r2/step4-20261009-001/README.md#资源与窗口)；[命令](#t3-m11-窗口与正常退出) | 待核查；记录两种退出与重启 |
| [ ] | T3-M12 | 审核隔离缺/坏DXIL、坏PNG/YAML、无效或未交付后端的原错、分类、有限退出和清理；不可修改原包，不能当作实际DeviceLost/提交失败/在途timeout | [故障及原失败](../../verification/archive/t3-r2/step4-20261009-001/README.md#故障与限制)；[命令](#t3-m12-隔离故障) | 待核查；本行通过不关闭A14 |
| [ ] | T3-M13 | 审阅非空PNG方向/内容、GPU来源frame ID/缺失原因、中立CSV/YAML、旧GL对照；核对无关CWD、带空格目录、DXIL/assets/CRT/notices齐全及部署限制 | [发布与协议](r2-step4-implementation.md#发布与协议)；[命令](#t3-m13-报告与发布目录) | 待核查；不宣称无SDK新机器实测 |
| [ ] | T3-M14 | 审核公开头/CPU-only/target及增量边界和D/R全量报告；区分当前过渡包与未完成的现代独占/全关闭、工具/关闭测试补充配置 | [构建与边界](r2-step4-implementation.md#构建与边界)；[命令](#t3-m14-构建与边界) | 待核查；未测组合不改为通过 |
| [ ] | T3-M15 | 在本候选逐项完成适用14项玩法及至少15分钟连续实际操作；记录实际后端/GPU、活动时段、问题、正常退出，不继承M2/T2旧成绩 | [玩法清单](../../testing/gameplay-smoke.md)；[场景](../../verification/archive/project-history-20261011/benchmark/testing/reproducible-scenes.md)；[命令](#t3-m15-玩法与十五分钟) | 待执行；逐项记录未执行/不适用及用户确认 |

**独立技术缺口仍未关闭**：实际 DeviceLost、提交失败与真正在途 timeout/分配失败的动态证据，现代独占及游戏全后端关闭组合，旧GUI基准工具的现代协议适配，正式画面参考/容差与同条件性能预算均不能由这八个人工勾选替代。Debug Layer不可用应报受阻，不关闭验证绕过。核查中出现的问题退回修复并保留原失败；完成这些人工项也不自动批准R3或切默认。

本轮候选主入口是 Release `002`；Debug `002` 用于本机诊断。短测为1920×1080/4x/Immediate，普通玩法窗口为4x/Fifo，必须按具体运行记录实际尺寸/模式，不把请求参数代填实际值。每条人工记录同时写候选版本、日期、执行者、实际身份、操作与证据路径；普通交互没有benchmark Session，不自动导出逐帧CSV或截图。

## 后续节点如何更新本页

进入 R2/R3 后，主表换入已交付 D3D12 生产包的实际设备/后端身份、完整世界/纹理/相机、网格编辑与空网格、窗口恢复、隔离故障、诊断与发布结果审阅，以及适用 14 项玩法和至少 15 分钟连续操作。先提供实际启动/场景入口再要求核查，不用 R1 夹具替代生产接管。

进入 R4 后，主表换入 Vulkan 的独立实验与生产核查，再审核双现代后端整合；按后端单列版本、结果和证据，不继承 D3D12 成绩。OpenGL 只保留受影响回归与过渡对照，不把最终退役或永久三后端等价变成本次新门槛。

## 用户节点决定与历史记录

2026-10-09 早先决定：T2 正式通过、T3 的 T2 节点批准前置解除，当时尚无 R1 冻结批准；见 [T2 权威记录](../m3-t2/README.md#t2-正式节点批准)。本表建立时与代理补证时均未代填人工通过，旧材料保留原状态。

2026-10-09 后续决定：用户已勾选 T3-M01–T3-M07，并明确表示“我已经审查了R1了，可以冻结R1，批准进入R2”。因此记录 **R1 人工审阅通过、R1/v1 冻结、R2 已获准进入**。批准前已勾选原表见 [原字节快照](../../verification/archive/t3-r1/freeze-20261009-001/sources/docs/milestones/m3-t3/manul-verification.md)，新权威记录见 [冻结身份](../../verification/archive/t3-r1/freeze-20261009-001/freeze-manifest.json)。不推断用户重新执行了附录命令，不据此次决定更改旧运行成绩或未完成的生产验收。

### R1 已完成项目

以下七项结论均为 **用户审阅通过，2026-10-09，来源本聊天明确批准及用户勾选**；版本为上方 R1 审阅身份，不是新运行。完整原操作/预期保存在原表快照，历史表不形成第二张活动清单。

| 稳定编号 | R1 核查内容 | 结论 | 证据 |
| --- | --- | --- | --- |
| T3-M01 | 候选、Release 与实际设备/后端身份 | 用户审阅通过 | [身份汇总](../../verification/archive/t3-r1/preparation-validation.json)、[实际结果](../../verification/archive/t3-r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json) |
| T3-M02 | 纹理层、UV/方向、alpha 与颜色差异 | 用户审阅通过；生产采样不因此通过 | [纹理查验](../../verification/archive/t3-r1/review-20261009-001/README.md#t3-m02-纹理方向与颜色) |
| T3-M03 | 更新/空网格、在途资源与回收记录 | 用户审阅通过 | [资源查验](../../verification/archive/t3-r1/review-20261009-001/README.md#t3-m03-更新空网格与回收) |
| T3-M04 | 缩放、真实最小化与恢复 | 用户审阅通过；显式零尺寸仍区分真实传播 | [窗口查验](../../verification/archive/t3-r1/review-20261009-001/README.md#t3-m04-缩放最小化与恢复) |
| T3-M05 | 原失败/复测、有界等待与合成超时边界 | 用户审阅通过；非真实设备丢失证明 | [故障说明](README.md#本轮验证)、[双配置身份](../../verification/archive/t3-r1/handoff-validation.json) |
| T3-M06 | Renderer v1、CPU/生产转接设计与所有权/失败语义 | 用户审阅通过并批准冻结设计基线 | [冻结范围](../../verification/archive/t3-r1/freeze-20261009-001/README.md#冻结范围)、[CPU 契约](../../openspec/changes/m3-t3-renderer/design.md#architecture) |
| T3-M07 | T2 交接/批准、R1 实验/公开消费者与 target 设计、剩余风险 | 用户审阅通过并批准进入 R2；生产 target 实测仍待实施 | [冻结门槛](README.md#冻结门槛)、[T2 正式批准](../m3-t2/README.md#t2-正式节点批准) |

## 附录 A 人工核查命令

以下保留的是 **R1 历史核查命令**，不是 R2 生产玩法入口；本次用户批准不要求复跑。供本机 Windows PowerShell 查看，无需编译或安装开发 SDK。先执行一次公共准备，再按编号查看；查看命令不会启动 GPU 实验，也不会改原包或已有证据。只有 T3-M01 的可选复跑会新建目录并打开实验窗口；复跑应另留新身份与结果，不改已冻结的历史结论。

### 公共准备

```powershell
$ErrorActionPreference = 'Stop'
$Repo = 'F:\GameDevelop\OpenGLProject\Symocraft'
$Milestone = Join-Path $Repo 'docs\milestones\m3-t3'
$ReviewRoot = Join-Path $Milestone 'evidence\r1\review-20261009-001'
$Package = Join-Path $Repo 'out\m3-t3\package\d3d12-r1-release-001'
$Evidence = Join-Path $ReviewRoot 'package\d3d12-r1-release-001\evidence\desktop-smoke-001'
$PSDefaultParameterValues['Get-Content:Encoding'] = 'UTF8'
$PSDefaultParameterValues['Select-String:Encoding'] = 'UTF8'
```

默认查看的是 R1 归档内的 Release 包身份和 `desktop-smoke-001` 历史结果；日期、GPU、驱动不能填成你本次新运行的事实。`$Package` 仍指向原始完整可运行包，只供可选复跑复制；归档中没有 exe、DLL、shader 或 runner，不能直接运行或打包验证。需要重新观察动态窗口时执行下面的复跑段，之后 `$Evidence` 会指向本次新结果。旧包说明/JSON 中的 Y9000P 和 T2 待批准文字是历史快照；当前仅验本机 RTX 5070 Ti，T2 节点批准以 2026-10-09 的权威记录为准。

### T3-M01 候选与设备身份

只审核现有报告时执行：

```powershell
$ReviewPackage = Split-Path -Parent (Split-Path -Parent $Evidence)
Get-Content -LiteralPath (Join-Path $ReviewPackage 'package-manifest.json')
$Verification = Get-Content -Raw -LiteralPath (Join-Path $Evidence 'verification.json') | ConvertFrom-Json
$Verification | Select-Object passed, executable, executable_sha256, portable_package
$Verification.production_imports | Select-Object configuration
$Verification.cases | Select-Object case, passed, expected_exit_code, exit_code, timed_out, failure
Get-Content -LiteralPath (Join-Path $Evidence 'hardware\result.json')
```

核对 `configuration=Release`、`backend=D3D12`、实际 `adapter=NVIDIA GeForce RTX 5070 Ti`、LUID/driver、`software_adapter=false`、FL12_0/SM6.0、Debug Layer 开启及错误/警告为 0。设备库存不是实际选择证明；新复跑也要人工核对实际 adapter，不能只看脚本总结果。

**可选：在隔离副本动态复跑一次正常流程和合成超时。** 本段自动复制完整旧包到新目录，不改冻结包。便携 runner 只允许证据位于它自身包的 `evidence` 内，所以不能把原包 runner 的输出直接指向另一个包外目录。

```powershell
$RunName = 'manual-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 8)
$RunRoot = Join-Path $Repo ('out\m3-t3\manual\' + $RunName)
if (Test-Path -LiteralPath $RunRoot) { throw '目录已存在，请重新执行生成新名称。' }
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$RunPackage = Join-Path $RunRoot 'package'
Copy-Item -LiteralPath $Package -Destination $RunPackage -Recurse
$Evidence = Join-Path $RunPackage 'evidence\local-rtx-manual'
& powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File (Join-Path $RunPackage 'verify-probes.ps1') `
  -Executable (Join-Path $RunPackage 'SymoCraftD3D12R1Probe.exe') `
  -OutputDirectory $Evidence `
  -PackageManifest (Join-Path $RunPackage 'package-manifest.json') `
  -RunTimeoutProbe
$RunExitCode = $LASTEXITCODE
Write-Host "验证脚本退出码：$RunExitCode；本次证据：$Evidence"
if ($RunExitCode -ne 0) { throw '验证未通过，请保留本次目录并查看报告，不勾选通过。' }
```

复跑只在本机 RTX 5070 Ti 执行。窗口会自动绘制、更新、缩放、最小化至少 10 秒、恢复并退出；不要提前关闭或手动改变窗口。需要 Windows Graphics Tools 提供 Debug Layer，缺失就记录受阻，不关闭验证绕过。runner 每个子进程默认上限 60 秒；超限可能被结束，此时不能算正常退出。脚本总退出码应为 0，正常子进程为 0，合成超时子进程为 1。完成后重新执行本节查看命令，并用以下 M02–M03 的查看命令及上方历史表的核查要求审阅 `$Evidence` 中同一轮材料；历史表链接仍指向旧归档，不算本次新证据，复制来的 `desktop-smoke-001` 也不算新成绩。此复跑仍是旧 Release 夹具，不是最新源码同源回归或生产接管。

### T3-M02 纹理方向与颜色

```powershell
Invoke-Item -LiteralPath (Join-Path $Evidence 'hardware\e01-linear.png')
Invoke-Item -LiteralPath (Join-Path $Evidence 'hardware\e01-srgb.png')
Select-String -LiteralPath (Join-Path $Evidence 'hardware\lifecycle.log') `
  -Pattern 'fixture_clip_mapping|texture_snapshot|e01|color|pixel'
```

在系统图片查看器中比较两张图：纹理层可区分、方向和 alpha 孔正确。报告中的 Linear 灰度 128 到 188、sRGB 灰度 128 到 128 是夹具的预期，不代表生产图集已完成相同采样/颜色核对。

### T3-M03 更新、空网格与回收

```powershell
Invoke-Item -LiteralPath (Join-Path $Evidence 'hardware\e02-updated.png')
Invoke-Item -LiteralPath (Join-Path $Evidence 'hardware\e02-empty.png')
Select-String -LiteralPath (Join-Path $Evidence 'hardware\lifecycle.log') `
  -Pattern 'mesh_snapshot|mesh_update|mesh_destroy|mesh_handle_rejected|retire_|gate_|frame_submit'
Get-Content -LiteralPath (Join-Path $Evidence 'hardware\result.json')
```

更新图应反映接受后的内容，空网格图不应残留旧画面。结合 `submitted_fence > completed_fence` 时的更新/删除、逻辑失效和后续 `retire_collect` 记录审阅，不用截图代替在途安全证据。

## 附录 B R2 生产候选命令

以下是本机Windows/D3D12入口，文件查看与自动化默认Python 3.10+、标准库。运行游戏本身不要求Python或开发SDK，但本轮验证要求本机Graphics Tools提供Debug Layer。命令均为完整本机路径，可从任意CWD启动；查看已有证据不会开窗。可选自动复跑必须使用新输出目录，目录存在就换新编号，不覆盖旧轮；GPU运行不要并发，也不要手动干扰自动窗口测试。R1附录A保留为历史命令，不把其PS脚本改名当作新交付。

### T3-M08 来源与设备身份

```text
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/package/d3d12-game-release-002/package-manifest.json"
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-summary-001/delivery-summary.json"
```

核对归档候选SHA与当前包完全一致，D/R的3282项生产输入相同；实际设备字段来自运行，不是库存。需要同时审阅Debug时，将首条中的 `release` 换为 `debug`；不能据Debug包的CRT存在获得再分发资格。

### T3-M09 世界、材质与相机

```text
python -X utf8 -c "import subprocess; subprocess.run(['F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/package/d3d12-game-release-002/SymoCraft.exe','--renderer','d3d12','--msaa','4','--validation','1','--seed','424242','--scene','regression','--checkpoint','four-chunk'],cwd='C:/Windows/System32',check=True,creationflags=subprocess.CREATE_NO_WINDOW)"
```

这是实际交互游戏，按Esc或关闭窗口结束；没有frame limit或自动编辑。核对四块交点、面方向/UV、选择框、转向/FOV/深度，并在 [归档画面](../../verification/archive/t3-r2/step4-20261009-001/README.md#世界与画面) 对照自动截图。分别重新启动，把最后的 `four-chunk` 换成 `positive-x`、`negative-x`、`wall-corner`、`low-ceiling`、`single-block` 或 `spawn`，即可进入实际固定检查点；没有未实现的游戏内传送命令。

### T3-M10 编辑、缓存与资源

```text
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-candidate-release-002/verification.json"
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-native-probes-debug-002/normal/result.json"
```

查看static/walk资源bytes/calls为0、edit35次计划/105次网格更新、各帧实际重建关联及原生空网格/输入寿命记录。原生立即销毁探针未强制实际GPU重叠，不能冒称已逼出在途回收；真实gate依据仍区分R1实验。人工编辑用M09命令在正/负坐标和四块交点逐次Q/E选材、左右键破坏/放置，间隔至少0.25秒；记录实际裂缝/错块情况。普通玩法没有逐帧CSV，自动 `edit` 是世界写入计划，不是鼠标回放。完整游戏没有“一键清空区块”入口，不虚构命令。

### T3-M11 窗口与正常退出

```text
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-window-release-004/verification.json"
python "F:/GameDevelop/OpenGLProject/Symocraft/test/integration/renderer/verify-d3d12-window.py" --package-directory "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/package/d3d12-game-release-002" --output-directory "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/manual/window-release-9001" --resize
```

第二条是可选复跑，约半分钟，只操作自己启动的PID/HWND，连续最小化12秒、恢复、缩放后发送WM_CLOSE并要求正常exit0；它不发全局键盘输入，也不证明Esc。人工用M09命令再次启动，实际调整窗口大小、最小化至少10秒、恢复检查画面比例/操作，先Esc退出；再启动一次用关闭按钮退出，再启动核查。记录每次实际身份与结果，不能用被超时强制结束的进程算正常退出。

### T3-M12 隔离故障

```text
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-candidate-release-002/verification.json"
python "F:/GameDevelop/OpenGLProject/Symocraft/test/integration/renderer/verify-d3d12-candidate.py" --package-directory "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/package/d3d12-game-release-002" --report-probe "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/r2-step4-matrix-004/native-release/test/integration/renderer_game_report_probe.exe" --output-directory "F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/manual/candidate-release-9001"
```

第二条可选复跑会验证完整包清单、复制带空格隔离包、执行普通400帧/三组短测及资源故障；约一分钟，每个子进程上限180秒。它只破坏复制品，不修改原包。预期缺DXIL exit2，坏DXIL/PNG/YAML exit3，无效backend exit64，未实施Vulkan exit3；必须同时核对原始stderr、分类/原生码/资源名/清理标记和原包未变。实际DeviceLost/提交失败/在途timeout与分配失败尚无已交付动态入口，不提供虚构命令，不手动TDR或卡死GPU替代。

### T3-M13 报告与发布目录

```text
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/package/d3d12-game-release-002/provenance/dependencies.json"
python -X utf8 -c "from pathlib import Path; print(Path('F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-candidate-release-002/d3d12-static/summary.yaml').read_text(encoding='utf-8'))"
```

实际PNG/CSV/YAML和GL对照链接见归档；同一host裁剪PATH不等于无SDK新机器实测。旧GUI基准工具尚不识别现代中立CSV，不能通过它读D3D12结果；已交付Python候选验证器+真实C++报告探针负责校验。源码/provenance中的绝对路径是来源，不是运行时资产依赖。

### T3-M14 构建与边界

```text
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-final-delivery-002/result.json"
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-final-cpu-001/result.json"
python -X utf8 -m json.tool "F:/GameDevelop/OpenGLProject/Symocraft/docs/milestones/m3-t3/evidence/r2/step4-20261009-001/raw/out/m3-t3/r2-step4-notests-matrix-001/closure.json"
```

原始JUnit/边界结果与 [构建说明](r2-step4-implementation.md#构建与边界) 一起审核；最后一条明确补充关闭测试/工具构建被沙箱阻断，提权复跑未执行，不将其算通过。此项只审阅已交付结果，不让用户自行编译来补代理缺口；现代独占/全关闭组合仍待工程实现和验证。

### T3-M15 玩法与十五分钟

先按M09命令选择检查点逐项完成 [14项玩法](../../testing/gameplay-smoke.md)，再做至少15分钟连续实际操作。需覆盖出生/着陆、WASD/斜向/Shift、Space跳跃与空中、墙角/低顶/单块、鼠标/FOV/缩放、射线/选框、Q/E八种材料/左右键/按住或同时按、正负及四块交点、边界高度、Caps/Ctrl、失焦/长帧/最小化、Esc/关闭/重启和已交付隔离故障。旧M2已通过记录不移作本候选成绩。

下面命令将普通交互stdout/stderr和进程耗时写入新目录，游戏正常退出后返回。目录若存在须换编号；耗时含加载/暂停，不是活跃操作或内存峰值证明，`manual_result` 留待人工填写，不自动勾选。

```text
python -X utf8 -c "from pathlib import Path; import subprocess,time,json; d=Path('F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/manual/gameplay-release-9001'); d.mkdir(parents=True); t=time.monotonic(); o=(d/'game.stdout.log').open('wb'); e=(d/'game.stderr.log').open('wb'); p=subprocess.run(['F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/package/d3d12-game-release-002/SymoCraft.exe','--renderer','d3d12','--msaa','4','--validation','1','--seed','424242','--scene','regression','--checkpoint','spawn'],cwd='C:/Windows/System32',stdout=o,stderr=e,creationflags=subprocess.CREATE_NO_WINDOW); o.close(); e.close(); (d/'session.json').write_text(json.dumps(dict(process_elapsed_seconds=time.monotonic()-t,exit_code=p.returncode,manual_result='not_recorded'),indent=2),encoding='utf-8'); print(d,p.returncode)"
```

在主表记录实际活动开始/结束、暂停时段、14项逐项结果与问题、实际adapter/driver、截图/录像及正常退出。没有完成的适用项保持待执行；不适用须写原因和用户确认。八行核查完成后仍需独立节点决定及技术缺口收尾，当前没有R3或默认切换批准。
