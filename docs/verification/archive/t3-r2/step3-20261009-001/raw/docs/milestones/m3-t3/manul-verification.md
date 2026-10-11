---
type: 人工核查入口
status: R1人工审阅已通过并冻结，R2已批准进入，生产候选待交付
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-08
updated: 2026-10-09
tags:
  - area/milestones
---

# M3-T3 人工核查

这是本节点唯一、固定的人工核查入口。只维护**一张当期 checkbox 主表**，告诉用户推进至下一个节点前需要人工核验什么；随节点推进更新入口、版本、当期项目、结果和证据，不新建替代清单。要求来源：[T3 spec](spec/M3/M3-T3-渲染器重构.md)；实施与自动验证见 [阶段报告](README.md)。

本项目的 Obsidian 库根为 `docs/`。R1 查验所需截图、报告和日志已复制到本 milestone 的 [R1 查验归档](evidence/r1/review-20261009-001/README.md)，本页使用库内相对链接；原始 `out/` 产物不移动。该归档是只读查验快照，不是可运行包或新验证成绩。JSON/日志若没有可用查看器，可阅读归档中的 Markdown 摘要，或执行附录 A 的查看命令。库外 `test/` 源码仍使用当前工程的本机外部地址，移动工程后需更新该地址。

## 当前节点与进入条件

2026-10-09 用户已完成 R1 审阅、勾选 T3-M01–T3-M07，并明确决定：“我已经审查了R1了，可以冻结R1，批准进入R2”。**R1 与 Renderer v1 公开契约/target 设计基线已冻结，当前进入 R2 生产实施。** 见 [冻结记录](evidence/r1/freeze-20261009-001/README.md) 与 [R2 入口](r2-entry.md)。本页保存人工审阅结论，不据批准虚构新的动态运行或生产验收。

**T2 与 R1 节点批准均已明确记录**：T2 见 [正式节点批准](../m3-t2/README.md#t2-正式节点批准) 和 [批准身份](../m3-t2/evidence/t2-approval-20261009.json)，R1 见本页历史记录及 [冻结身份](evidence/r1/freeze-20261009-001/freeze-manifest.json)。MB01 方案2已确认 working set ≤ 704 MiB、private bytes ≤ 1408 MiB，FB01 方案2保持；首次可操作时间预算仍未确认。T2 的 Win32 1175/Fix1 暂停和正式同源 Q06 未完成不改判为已修复或实测通过。R1 认可设计与已交付报告，不将生产资产/采样/相机 GPU 画面、真实生产 target 或完整玩法标为通过；这些按 R2/R3 实施与验收。

当前 M3 功能/兼容只限定本机 RTX 5070 Ti，不要求其他 GPU 或第二台机器。下一玩法阶段前的中低端整机性能门槛仍独立保留，见 [项目范围](project-scope.md)。顺序3已有真实 D3D12 完整游戏的开发短测，但可交付生产候选尚未准备，Vulkan尚未实施；不要求现在核查不存在的运行包。

| 入口 | 实际可用状态与用途 |
| --- | --- |
| [夹具说明](file:///F:/GameDevelop/OpenGLProject/Symocraft/test/experimental/d3d12-r1/README.md)、[便携包说明](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/README.txt) | 保留的独立 R1 Release 包，不是生产 Renderer 或完整世界包；本页建立时已核对说明及下方截图/报告文件存在 |
| [汇总与身份](evidence/r1-preparation-validation.json)、[实际运行结果](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json) | 已有独显运行记录，可阅读审核；汇总中的早期 T2 状态是历史快照，当前 T2 以 [阶段报告](../m3-t2/README.md) 为准 |
| [CPU/窗口冻结门槛](README.md#冻结门槛)、[CPU 契约清单](spec/M3/M3-T1-T3-CPU契约清单.md)、[冻结基线](evidence/r1/freeze-20261009-001/README.md) | 用户已审阅批准 R1，冻结记录固定批准前来源；真实生产实现和依赖图在 R2 落实，不将实验消费者冒充已实现门面 |
| [本轮双配置与转接身份](evidence/r1-handoff-validation.json)、[转接报告](README.md#r1-转接准备与双配置复核) | Debug/Release GPU 各正常/超时 2/2，CPU 各 4/4；新证据是工作区实验，不是新的便携包，旧包不被覆盖 |

## 状态与维护规则

- `[ ]` 表示尚未人工核查通过；完整核查且达到预期才改为 `[x]`，同时记录日期、执行者、候选版本和证据。自动测试通过、代理检查或包交付不自动勾选。
- 未执行写明原因；失败记录步骤、实际结果及证据，保留原失败再记复测；不适用保持 `[ ]`，写明理由及用户确认。尚未交付不是不适用。
- 编号稳定、不复用。进入新节点时，把当前项目的编号、结论、版本及证据移入页末历史记录，再在同一主表换入下一节点的人工核验项目；历史表不用 checkbox，不形成第二张活动清单。
- 版本变化影响已核查行为时，保留原记录并重新待复核。自动硬边界、CPU 寿命、GPU fence/容量和数值采样由代理留证，用户审核报告；画面正常不能证明同步或回收正确。
- 如需动态复跑，先约定同机候选入口、操作与新证据目录，不覆盖已有结果；故障操作用交付的隔离副本/专用入口，不修改原资产。
- 勾选表示该人工项目成立，不自动批准冻结或进入下一节点。用户批准/退回的决定在页末单独记录。

### 已完成 R1 审阅身份

既有 R1 是 `d3d12-r1-release-001` / `desktop-smoke-001`、D3D12、RTX 5070 Ti、driver `32.0.16.1664`、LUID `00000000:000144d0`；这些只属于历史运行，不能代填新运行实际值。

| 字段 | 本次记录 |
| --- | --- |
| 日期、执行者、核查节点 | 2026-10-09；用户；R1 报告/设计审阅及明确批准，不声明新动态运行 |
| 候选包、提交/源码快照、构建配置 | 既有 Release 包及 R1 Debug/Release 实验材料；批准前 12 份来源/SHA 固定于 [冻结身份](evidence/r1/freeze-20261009-001/freeze-manifest.json) |
| 可执行文件、操作入口与工作目录 | 既有 R1 入口见上表与历史附录；本次未声明重新执行，R2 生产入口待交付 |
| 请求/实际后端、实际 GPU 身份、驱动 | 审阅既有 D3D12 / RTX 5070 Ti / driver 32.0.16.1664 / LUID 00000000:000144d0 的报告；不代填为新运行身份 |
| 截图/录像、日志、报告、原失败与复测记录 | [R1 查验归档](evidence/r1/review-20261009-001/README.md)；早期失败和复测原样保留 |

## 当期人工核验主表：R2 至 R3

R1 七项已移入下方无 checkbox 的历史表，编号不复用。**R2 生产候选尚未交付，当前没有可执行的人工核验行。** 代理先落实 [R2 实施交接](r2-entry.md)，提供真实包/场景/身份与查验材料，再在此唯一主表新增 T3-M08 起的适用项目及逐项命令；不要求用户核查不存在的入口，也不把 R1 夹具作为生产包。当前不自动生成或勾选下一组项目。

顺序1的 CPU API/PImpl 核心及构建验证已交付，见 [实施报告](r2-step1-implementation.md) 与其库内查验归档；它没有原生后端、GPU 运行或生产玩法包，不据自动通过增加人工勾选，也不改变 R1 七项历史结论。

顺序2的真实 D3D12 后端及 RTX 有限验证另见 [顺序2报告](r2-step2-implementation.md)、[查验归档](evidence/r2/step2-20261009-001/README.md)。用户已明确批准现代透明 discard 不写深度，报告保留与旧 GL 的差异；没有代填人工通过。完整玩法/现代包、实际 DeviceLost/在途 timeout 仍待交付，Win32 1175 仍复现，不据此增加 T3-M08 起项目或申请 R3。

顺序3统一调用与完整世界短测见 [实施报告](r2-step3-implementation.md)、[查验归档](evidence/r2/step3-20261009-001/README.md)。Debug/Release的D3D12及Release的旧GL三组开发运行均通过；这不是人工玩法、生产运行包或R3批准。顺序4交付真实包后再添加稳定编号及每项实际命令，当前不新增checkbox或代填人工结论。

## 后续节点如何更新本页

进入 R2/R3 后，主表换入已交付 D3D12 生产包的实际设备/后端身份、完整世界/纹理/相机、网格编辑与空网格、窗口恢复、隔离故障、诊断与发布结果审阅，以及适用 14 项玩法和至少 15 分钟连续操作。先提供实际启动/场景入口再要求核查，不用 R1 夹具替代生产接管。

进入 R4 后，主表换入 Vulkan 的独立实验与生产核查，再审核双现代后端整合；按后端单列版本、结果和证据，不继承 D3D12 成绩。OpenGL 只保留受影响回归与过渡对照，不把最终退役或永久三后端等价变成本次新门槛。

## 用户节点决定与历史记录

2026-10-09 早先决定：T2 正式通过、T3 的 T2 节点批准前置解除，当时尚无 R1 冻结批准；见 [T2 权威记录](../m3-t2/README.md#t2-正式节点批准)。本表建立时与代理补证时均未代填人工通过，旧材料保留原状态。

2026-10-09 后续决定：用户已勾选 T3-M01–T3-M07，并明确表示“我已经审查了R1了，可以冻结R1，批准进入R2”。因此记录 **R1 人工审阅通过、R1/v1 冻结、R2 已获准进入**。批准前已勾选原表见 [原字节快照](evidence/r1/freeze-20261009-001/sources/docs/milestones/m3-t3/manul-verification.md)，新权威记录见 [冻结身份](evidence/r1/freeze-20261009-001/freeze-manifest.json)。不推断用户重新执行了附录命令，不据此次决定更改旧运行成绩或未完成的生产验收。

### R1 已完成项目

以下七项结论均为 **用户审阅通过，2026-10-09，来源本聊天明确批准及用户勾选**；版本为上方 R1 审阅身份，不是新运行。完整原操作/预期保存在原表快照，历史表不形成第二张活动清单。

| 稳定编号 | R1 核查内容 | 结论 | 证据 |
| --- | --- | --- | --- |
| T3-M01 | 候选、Release 与实际设备/后端身份 | 用户审阅通过 | [身份汇总](evidence/r1-preparation-validation.json)、[实际结果](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json) |
| T3-M02 | 纹理层、UV/方向、alpha 与颜色差异 | 用户审阅通过；生产采样不因此通过 | [纹理查验](evidence/r1/review-20261009-001/README.md#t3-m02-纹理方向与颜色) |
| T3-M03 | 更新/空网格、在途资源与回收记录 | 用户审阅通过 | [资源查验](evidence/r1/review-20261009-001/README.md#t3-m03-更新空网格与回收) |
| T3-M04 | 缩放、真实最小化与恢复 | 用户审阅通过；显式零尺寸仍区分真实传播 | [窗口查验](evidence/r1/review-20261009-001/README.md#t3-m04-缩放最小化与恢复) |
| T3-M05 | 原失败/复测、有界等待与合成超时边界 | 用户审阅通过；非真实设备丢失证明 | [故障说明](README.md#本轮验证)、[双配置身份](evidence/r1-handoff-validation.json) |
| T3-M06 | Renderer v1、CPU/生产转接设计与所有权/失败语义 | 用户审阅通过并批准冻结设计基线 | [冻结范围](evidence/r1/freeze-20261009-001/README.md#冻结范围)、[CPU 契约](spec/M3/M3-T1-T3-CPU契约清单.md) |
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
