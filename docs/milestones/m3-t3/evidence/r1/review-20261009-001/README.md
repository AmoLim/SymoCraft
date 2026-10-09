---
type: 证据归档
status: R1只读查验快照，不是新运行或冻结批准
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
tags:
  - area/milestones
---

# R1 人工查验归档

本目录在 2026-10-09 保存既有 R1 查验材料的原样副本，供 `docs/` Obsidian 库内阅读。运行事实仍属于原报告中的日期和候选，复制日期不是运行日期。本次没有执行新 GPU/CPU 验证，不批准 R1/v1 冻结或进入 R2。

原始材料保留在工程的 `out/m3-t3/`，没有移动或修改。按原目录层级摘录 114 个文件，共 783,092 bytes；[归档清单](archive-manifest.json) 记录每个源路径、归档路径、字节数和 SHA256，复制后均与原件一致。清单覆盖原样文件，不包含本说明页。缺失的早期失败终态不补造。

这里只保存 PNG、JSON、日志及包来源说明/身份，不含 exe、DLL、静态库、shader、runner 或构建缓存。`package/` 是包证据摘录，**不是完整可运行包**；不能从本目录启动、安装或运行便携验证。动态复跑仍用 [人工核查附录 A](../../../manul-verification.md#附录-a-人工核查命令) 中的原包隔离副本命令。

下方摘要方便在 Obsidian 阅读，原始 JSON/日志保留用于详细核对。点击图片可查看原图；如果系统没有 JSON/日志关联查看器，使用人工核查页的 PowerShell 查看命令。人工格子和批准决定仍只维护在 [唯一核查主表](../../../manul-verification.md)，不在这里新增第二张 checklist。

## T3-M01 候选与设备身份

保留包候选为 `d3d12-r1-release-001`，该轮结果为 `desktop-smoke-001`，配置 Release。

| 字段 | 原报告记录 |
| --- | --- |
| 后端 / 窗口 | D3D12 / Native SDL3，私有 HWND 桥接 |
| 实际设备 | NVIDIA GeForce RTX 5070 Ti，非软件设备 |
| LUID / 驱动 | `00000000:000144d0` / `32.0.16.1664` |
| 能力 | FL12_0 / SM6.0 |
| Debug Layer | 开启，错误 0 / 警告 0 |
| 正常运行 | `status=pass`，E01/E02/E03 均通过，正常 shutdown 与 cleanup 完成 |

来源：[包说明](package/d3d12-r1-release-001/README.txt)、[包清单](package/d3d12-r1-release-001/package-manifest.json)、[完整验证报告](package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json)、[正常运行报告](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/result.json)、[机器记录](package/d3d12-r1-release-001/evidence/desktop-smoke-001/machine.json)。旧说明中 Y9000P/T2 待批准文字保留历史原样，不重新变成当前门槛。

## T3-M02 纹理方向与颜色

检查两层纹理、方向/UV、alpha 孔及 Linear/sRGB 差异。以下是原 Release 夹具截图，不代表生产图集、采样器或游戏相机已验收。

### Linear

![Linear 纹理截图](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e01-linear.png)

### sRGB

![sRGB 纹理截图](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e01-srgb.png)

夹具中的预期灰度为 Linear 128 到 188、sRGB 128 到 128；详细输入、上传和采样约束见 [生命周期日志](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/lifecycle.log) 与 [阶段说明](../../../README.md#保留的本机-rtx-记录)。

## T3-M03 更新、空网格与回收

### 接受更新后的画面

![更新网格截图](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e02-updated.png)

### 空网格画面

![空网格截图](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e02-empty.png)

原报告记录更新时 submitted fence 3 / completed 2，删除时 submitted 5 / completed 4，之后收集 4 个退休项。结合 [生命周期日志](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/lifecycle.log) 的 `retire_enqueue`、逻辑失效/过期拒绝和后续 `retire_collect` 审阅；截图本身不能证明在途资源安全。

## T3-M04 缩放、最小化与恢复

![恢复后的纹理截图](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/e03-restored.png)

原运行记录三次实际像素尺寸 800x480、512x384、720x540；真实最小化至少 10 秒后恢复，暂停提交数为 0。显式零尺寸输入仍是标注的契约注入，不能写成 OS 实测零像素传播。细节见 [窗口与提交日志](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/lifecycle.log) 和 [运行报告](package/d3d12-r1-release-001/evidence/desktop-smoke-001/hardware/result.json)；需要亲自观察动态过程时另跑原包隔离副本。

## T3-M05 正常退出与故障边界

以下为 2026-10-08 已取得的双配置报告，不把上述旧 Release 包当作 Debug 包。

| 配置 | 正常用例 | 合成超时用例 | Debug Layer 错误 / 警告 | 原始报告 |
| --- | --- | --- | --- | --- |
| Debug | 通过，子进程 exit 0，`status=pass` | 预期失败验证通过，exit 1，`status=fail` | 两个用例均 0 / 0 | [Debug](evidence/r1prep-debug-001/verification.json) |
| Release | 通过，子进程 exit 0，`status=pass` | 预期失败验证通过，exit 1，`status=fail` | 两个用例均 0 / 0 | [Release](evidence/r1prep-release-001/verification.json) |

两配置 runner 各 2/2，无 runner 强制超时。注入超时只验证指定有限等待、原始错误和兜底清理路径，不代表真实设备丢失或在途 GPU 停滞已验证。旧包对应故障的 [result.json](package/d3d12-r1-release-001/evidence/desktop-smoke-001/injected-timeout/result.json)、[stderr](package/d3d12-r1-release-001/evidence/desktop-smoke-001/injected-timeout/stderr.log)、[lifecycle](package/d3d12-r1-release-001/evidence/desktop-smoke-001/injected-timeout/lifecycle.log) 同样保留。

原 `desktop-release-001` / `002` 在局部绘制后异常退出，缺正常终态，仍是失败；后续成功不覆盖它们。归档包含 [001 原验证](evidence/desktop-release-001/verification.json)、[002 原验证](evidence/desktop-release-002/verification.json) 及其当时实际留下的文件，没有补造缺失的 `hardware/result.json`。

## T3-M06 v1 设计与生产转接

本目录不把候选声明变成最终公开接口。请审阅 [T3 候选 spec](../../../../../spec/M3-T3-渲染器重构.md)、[CPU 契约清单](../../../../../spec/M3-T1-T3-CPU契约清单.md) 与 [当前转接报告](../../../README.md#r1-转接准备与双配置复核)。最终语义、公开/私有拆分、生产采样/颜色、相机及三角形同层规则仍按当前冻结核对处理，不因归档而通过。

## T3-M07 批准与冻结交接

T2 已于 2026-10-09 获用户节点批准，见 [权威批准记录](../../../../m3-t2/evidence/t2-approval-20261009.json)。本归档不将遗留风险改为已修复，也不批准 R1/v1 冻结或 R2。

独立 CPU 双配置各 4/4，包含原 70 项检查和新增 82 项生产转接检查；查看 [Debug 日志](log/r1/build/r1prep-cpu-debug-ctest-final-003.log)、[Release 日志](log/r1/build/r1prep-cpu-release-ctest-final-004.log) 与 [原转接身份](../../r1-handoff-validation.json)。该身份的源码/批准字段是原轮次快照，不能替代最终 T2 来源、变更影响和 R1 冻结交接结论。

## 后续归档规则

后续按节点新建独立目录，例如 `evidence/r2/review-日期-编号/`。同节点新候选或新运行也新建编号，不覆盖本次归档；只复制当前查验必需材料，保留源身份和校验值，运行产物仍留在 `out/`。归档不自动勾选人工通过，不修改原始报告里的绝对来源路径，也不冒充最新源码完整回归。
