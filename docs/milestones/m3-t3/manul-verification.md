---
type: 人工核查入口
status: T2节点批准前置已解除，R1待人工复核与冻结交接
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-08
updated: 2026-10-09
tags:
  - area/milestones
---

# M3-T3 人工核查

这是本节点唯一、固定的人工核查入口。只维护**一张当期 checkbox 主表**，告诉用户推进至下一个节点前需要人工核验什么；随节点推进更新入口、版本、当期项目、结果和证据，不新建替代清单。要求来源：[T3 spec](../../spec/M3-T3-渲染器重构.md)；实施与自动验证见 [阶段报告](README.md)。

本项目的 Obsidian 库根为 `docs/`。R1 查验所需截图、报告和日志已复制到本 milestone 的 [R1 查验归档](evidence/r1/review-20261009-001/README.md)，本页使用库内相对链接；原始 `out/` 产物不移动。该归档是只读查验快照，不是可运行包或新验证成绩。JSON/日志若没有可用查看器，可阅读归档中的 Markdown 摘要，或执行附录 A 的查看命令。库外 `test/` 源码仍使用当前工程的本机外部地址，移动工程后需更新该地址。

## 当前节点与进入条件

2026-10-09 当前仍为 **R1 独立 D3D12 前置实验，目标是申请冻结 Renderer v1 并进入 R2**。E01-E03 已有本机 Debug/Release 自动运行及代理视觉复核记录，CPU 转接及声明消费者也已补证；不代表用户已人工通过本页，R1/v1 仍未冻结。

**T2 节点批准前置已解除，当前仍不能自动进入 R2**：用户已于 2026-10-09 明确批准 T2 正式通过，见 [正式节点批准](../m3-t2/README.md#t2-正式节点批准) 和 [批准身份](../m3-t2/evidence/t2-approval-20261009.json)；生产资产/采样、相机/网格版本转接、公开消费者和 target 图的 R1 冻结核对尚未关闭。代理应先交付这些报告及待审核 v1 设计；用户审阅结论和明确的接口/语义，不需要自行跑整套构建工具。MB01 方案2已确认 working set ≤ 704 MiB、private bytes ≤ 1408 MiB，FB01 方案2保持；首次可操作时间预算仍未确认。节点决定不批准 R1/v1 冻结或 R2 生产开发，也不把 T2 的 Win32 1175/Fix1 暂停和正式同源 Q06 未完成改判为已修复或实测通过，不再以这些遗留事项阻断 T2 节点。

当前 M3 功能/兼容只限定本机 RTX 5070 Ti，不要求其他 GPU 或第二台机器。下一玩法阶段前的中低端整机性能门槛仍独立保留，见 [项目范围](../../spec/project-scope.md)。生产 D3D12 及 Vulkan 入口尚未交付，不要求现在核查不存在的生产玩法。

| 入口 | 实际可用状态与用途 |
| --- | --- |
| [夹具说明](file:///F:/GameDevelop/OpenGLProject/Symocraft/test/experimental/d3d12-r1/README.md)、[便携包说明](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/README.txt) | 保留的独立 R1 Release 包，不是生产 Renderer 或完整世界包；本页建立时已核对说明及下方截图/报告文件存在 |
| [汇总与身份](evidence/r1-preparation-validation.json)、[实际运行结果](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json) | 已有独显运行记录，可阅读审核；汇总中的早期 T2 状态是历史快照，当前 T2 以 [阶段报告](../m3-t2/README.md) 为准 |
| [CPU/窗口冻结门槛](README.md#冻结门槛)、[CPU 契约清单](../../spec/M3-T1-T3-CPU契约清单.md) | 可先阅读已交付部分；完整冻结交接结论和最终 v1 审核材料仍待代理交付 |
| [本轮双配置与转接身份](evidence/r1-handoff-validation.json)、[转接报告](README.md#r1-转接准备与双配置复核) | Debug/Release GPU 各正常/超时 2/2，CPU 各 4/4；新证据是工作区实验，不是新的便携包，旧包不被覆盖 |

## 状态与维护规则

- `[ ]` 表示尚未人工核查通过；完整核查且达到预期才改为 `[x]`，同时记录日期、执行者、候选版本和证据。自动测试通过、代理检查或包交付不自动勾选。
- 未执行写明原因；失败记录步骤、实际结果及证据，保留原失败再记复测；不适用保持 `[ ]`，写明理由及用户确认。尚未交付不是不适用。
- 编号稳定、不复用。进入新节点时，把当前项目的编号、结论、版本及证据移入页末历史记录，再在同一主表换入下一节点的人工核验项目；历史表不用 checkbox，不形成第二张活动清单。
- 版本变化影响已核查行为时，保留原记录并重新待复核。自动硬边界、CPU 寿命、GPU fence/容量和数值采样由代理留证，用户审核报告；画面正常不能证明同步或回收正确。
- 如需动态复跑，先约定同机候选入口、操作与新证据目录，不覆盖已有结果；故障操作用交付的隔离副本/专用入口，不修改原资产。
- 勾选表示该人工项目成立，不自动批准冻结或进入下一节点。用户批准/退回的决定在页末单独记录。

### 本次核查身份

既有 R1 是 `d3d12-r1-release-001` / `desktop-smoke-001`、D3D12、RTX 5070 Ti、driver `32.0.16.1664`、LUID `00000000:000144d0`；这些只属于历史运行，不能代填新运行实际值。

| 字段 | 本次记录 |
| --- | --- |
| 日期、执行者、核查节点 | 未填写 |
| 候选包、提交/源码快照、构建配置 | 未填写；新候选交付后更新 |
| 可执行文件、操作入口与工作目录 | 未填写；现有 R1 入口见上表，生产入口待交付 |
| 请求/实际后端、实际 GPU 身份、驱动 | 未填写；须核对真实 RTX 5070 Ti，不从显示器连接或启动参数推定 |
| 截图/录像、日志、报告、原失败与复测记录 | 未填写 |

## 当期人工核验主表：R1 至 R2

本次所有用户格子为未执行，不继承代理已有结果。可先复核已交付证据；待报告项不要求用户自行补做工程验证。每个编号的查看命令及可选动态复跑入口见 [附录 A](#附录-a-人工核查命令)。

| 核查  | 稳定编号   | 时点           | 核查操作                                                       | 预期结果                                                                  | 结果/证据                                                                                                                                                                                                                                                   |
| --- | ------ | ------------ | ---------------------------------------------------------- | --------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [x] | T3-M01 | R1 候选复核      | 对照包说明、身份汇总和运行结果，核对包名/Release、实际后端、adapter、驱动及失败记录          | 实际设备为 RTX 5070 Ti、后端为 D3D12；没有 WARP、其他后端或降低能力基线冒充通过                   | 未执行；[身份汇总](evidence/r1-preparation-validation.json)、[实际结果](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json)                                                                                                |
| [x] | T3-M02 | R1-E01       | 打开两张纹理图，对照夹具说明检查可区分的纹理层、方向/UV、alpha 孔与 Linear/sRGB 差异      | 真实纹理可见，无倒置、错层或空白；颜色差异符合报告解释，不只是清屏/三角形                                 | 未执行；[Linear](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e01-linear.png)、[sRGB](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e01-srgb.png)                               |
| [x] | T3-M03 | R1-E02       | 对照更新及空网格图；审阅在途更新/删除、过期句柄和回收记录                              | 已接受更新反映到画面，空网格无旧内容；在途安全由 fence/诊断证据支持，不只看截图                           | 未执行；[更新](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e02-updated.png)、[空网格](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e02-empty.png)、[生命周期记录](README.md#保留的本机-rtx-记录) |
| [ ] | T3-M04 | R1-E03       | 审阅三次缩放、真实最小化至少 10 秒及恢复记录，检查恢复图；需要亲自看动态行为时再安排同机复跑           | 恢复后纹理/比例正确；零尺寸暂停无忙循环/死锁。显式零尺寸注入不冒称真实零像素传播，未重建 Window 对象               | 未执行；[恢复图](evidence/r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e03-restored.png)、[窗口记录](README.md#保留的本机-rtx-记录)                                                                                                          |
| [ ] | T3-M05 | R1 失败边界      | 审阅正常退出、合成超时与早期失败记录，确认故障边界说明可理解                             | 原始错误/有界等待有证据；正常与注入非零退出分开。合成超时不扩大为真实设备丢失/在途停滞安全证明；新 Debug 成绩与原失败记录分别保留 | 未执行；[验证与故障说明](README.md#本轮验证)、[新双配置身份](evidence/r1-handoff-validation.json)、[原汇总](evidence/r1-preparation-validation.json)                                                                                                                              |
| [ ] | T3-M06 | v1 冻结申请前     | 审阅代理整理的最终 Renderer v1 设计与生产转接结论：公开接口/所有权/失败语义、纹理采样与相机、网格版本 | 用户理解并认可明确的游戏侧契约；夹具坐标/采样不冒充生产语义，私有后端允许演进，不提前冻结完整通用 RHI                 | 待最终审核材料交付；[候选设计](../../spec/M3-T3-渲染器重构.md)、[CPU 契约](../../spec/M3-T1-T3-CPU契约清单.md)                                                                                                                                                                    |
| [ ] | T3-M07 | R1 至 R2 前置核对 | 审阅 T2 节点批准/交接与遗留风险、公开头消费者/target 图、CPU 与窗口冻结检查及剩余项         | T2 节点批准已明确记录，R1 自身门槛有报告，公共头无 SDK 传播；不需要用户重跑构建，也不凭实验通过直接进入 R2          | T2 于 2026-10-09 获用户正式批准，见 [正式批准与遗留事项](../m3-t2/README.md#t2-正式节点批准)；本项 R1 冻结核对仍待审核，[冻结门槛](README.md#冻结门槛)                                                                                                                                               |

## 后续节点如何更新本页

进入 R2/R3 后，主表换入已交付 D3D12 生产包的实际设备/后端身份、完整世界/纹理/相机、网格编辑与空网格、窗口恢复、隔离故障、诊断与发布结果审阅，以及适用 14 项玩法和至少 15 分钟连续操作。先提供实际启动/场景入口再要求核查，不用 R1 夹具替代生产接管。

进入 R4 后，主表换入 Vulkan 的独立实验与生产核查，再审核双现代后端整合；按后端单列版本、结果和证据，不继承 D3D12 成绩。OpenGL 只保留受影响回归与过渡对照，不把最终退役或永久三后端等价变成本次新门槛。

## 用户节点决定与历史记录

2026-10-09 用户决定：**T2 节点已正式通过，T3 的 T2 节点批准前置解除；未批准 R1/v1 冻结，未进入 R2**。以 [T2 权威记录](../m3-t2/README.md#t2-正式节点批准) 为准，不将其遗留风险改为已修复。本表建立时只建立入口；代理后来已补双配置 R1 与 CPU 转接证据，没有代填人工通过状态。既有 R1 准备授权不等于冻结或生产接管批准，本次 T2 批准也不代填本页用户格子。

当前尚无本页的人工完成记录。节点推进后在此保存已完成当期项目的稳定编号、节点、执行者/日期、版本、结论及证据；失败和退回记录保留，再追加复测及用户批准/退回决定，不覆盖旧结论。

## 附录 A 人工核查命令

以下命令供你在本机 **Windows PowerShell** 中执行，无需编译或安装开发 SDK。先执行一次公共准备，再按编号查看；查看命令不会启动 GPU 实验，也不会改原包或已有证据。只有 T3-M01 的可选复跑会新建目录并打开实验窗口。执行命令不自动勾选主表，仍需记录观察、日期、候选和结果。

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

复跑只在本机 RTX 5070 Ti 执行。窗口会自动绘制、更新、缩放、最小化至少 10 秒、恢复并退出；不要提前关闭或手动改变窗口。需要 Windows Graphics Tools 提供 Debug Layer，缺失就记录受阻，不关闭验证绕过。runner 每个子进程默认上限 60 秒；超限可能被结束，此时不能算正常退出。脚本总退出码应为 0，正常子进程为 0，合成超时子进程为 1。完成后重新执行本节查看命令，并按以下 M02–M05 查看 `$Evidence` 中的同一轮材料；复制来的 `desktop-smoke-001` 不算新成绩。此复跑仍是旧 Release 夹具，不是最新源码同源回归或生产接管。

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
