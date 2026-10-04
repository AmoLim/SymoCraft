---
type: 实验
status: 已验证
project: SymoCraft
module: benchmark
created: 2026-10-04
---

# Benchmark 桌面交付验证

## 要回答的问题

- 朋友侧能否不用 PowerShell 完成启动、失焦采样、结果检查与导出？
- 关联功能：[[Benchmark-执行与导出]]、[[Benchmark-文件协议]]、[[Benchmark-App类]]、[[Performance-失焦采样]]、[[CMake-模块边界]]。
- 设计选择：保留独立 C++ 执行器，以进程和文件协议连接游戏；旧脚本只用于开发与历史回归，不作为交付入口。

## 假设与判据

- 假设：原生执行器不借助 shell 或脚本，即可完成采样流程，并准确区分失焦、最小化和取消。
- 支持判据：真实游戏三场景失焦快速检查有效；GUI 导出成功；最小化返回无效；隔离构建和错误路径测试通过；发行包不包含脚本。
- 否定判据：需要更改执行策略、执行器依赖引擎对象、缺失数据仍报告有效，或取消影响其他进程。
- 证据不足边界：单台 NVIDIA 开发机短测不能证明 AMD/Intel 驱动、干净机依赖、长时间稳定性或正式性能达标。

## 最小实验

- 实机：Ryzen 7 9700X、RTX 5070 Ti、64 GB；Windows build 26200；实际 GL 4.6.0 NVIDIA 616.64。
- 构建：MSVC 19.38.33145 x64、CMake 4.3.1、Release。另执行 Debug 回归。
- 固定条件：seed 424242、441 区块、1920×1080、VSync 关闭、4x MSAA；静态/移动/编辑各预热 10 秒并采样 10 秒，一轮，无异常帧剔除。
- 本次变量：原生执行流程、allow-unfocused、中文和空格便携路径、最小化与轮次间取消。
- GUI 短测手填电源/显卡模式/频率声明保留“未记录”，不把此前正式基线的声明回填成此次采集事实。电源 GUID、AC 状态等自动字段保留在会话中。
- 观测：原生测试、游戏 summary/CSV、进程退出码、GUI 截图、包文件清单和 SHA256。只在 `docs` 保留小型证据；完整 CSV、程序和 ZIP 在 `out`。

```text
构建并跑自动测试 → 检查三种隔离构建
安装到中文空格目录 → GUI 快速检查 → 导出 ZIP
GUI 轮次间取消与继续 → 独立游戏最小化探针
小幅界面/生命周期修订 → 最终双配置回归
安装、打包、解压资源预检 → 固化最终包身份
```

## 实验步骤

1. 分别构建 Debug/Release，运行现有 17 项与新增 3 项测试。新增项覆盖协议、进程参数/取消和执行器端到端夹具，不以夹具替代真实驱动。
2. 使用 `tools/benchmark/tests/verify-boundaries.cmake` 实际构建根工程仅游戏、根工程仅执行器、执行器独立入口；检查 Ninja 依赖图。
3. 从 `out/benchmark-native/便携 测试包` 启动 GUI，选择快速检查和允许失焦，保存至 `out/benchmark-native/界面 短测`。保持游戏未获焦点，运行三种场景并点击导出。
4. 再开一组快速检查，在轮次之间取消，再通过 GUI 重试剩余轮次。独立启动 allow-unfocused 游戏探针并最小化，核对退出与摘要。
5. 完成最后小幅修订后重新跑 Debug/Release 全部测试，生成最终 Release 包，解压到新目录，执行游戏 `--check-assets`。此次未再次打开最终版本 GUI 或执行 GPU 短测。

## 结果记录

| 条件 / 输入 | 预期 | 实际观察 | 日志 / 数据 / 证据 |
| --- | --- | --- | --- |
| 最终 Debug / Release | 全部测试通过 | 各 20/20 | [Debug](evidence/ctest-debug.log)、[Release](evidence/ctest-release.log) |
| 三种独立构建 | 无反向模块依赖 | 3/3 构建与依赖检查通过 | [构建边界](evidence/build-boundaries.log) |
| GUI 三场景失焦短测 | 完成并记录实际失焦 | 3/3 有效，所有正式样本均未获焦点 | [会话](evidence/desktop-quick/session.yaml) |
| GUI 导出 | 不调用 PowerShell，ZIP 不含程序 | 导出成功，显示完整结果路径 | [截图](evidence/gui-quick-export.png)；原始 ZIP 在 out |
| GUI 轮次间取消后继续 | 保留已完成轮次，标记重试 | 最终完成，retried=true，continuous_full_protocol=false | [会话](evidence/gui-cancel-resume-session.yaml) |
| 真实游戏最小化 | 无效并停止 | 退出码 4；framebuffer-changed-or-minimized、duration-not-completed | [摘要](evidence/minimized/summary.yaml)、[日志](evidence/minimized/stdout.log) |
| 原生异常路径夹具 | 不把异常当成功 | 瞬退、缺文件、旧协议、错误统计、时长不足、超时/取消均覆盖 | 两份 CTest 日志中的 benchmark 测试 |
| Intel / AMD 夹具 | 不凭品牌拒绝，可缺少 NVX 显存 | 通过；仅为合成字符串/文件协议验证 | benchmark.executor，非真实驱动实测 |
| 最终便携包 | 含 CRT/资源，不含脚本或开发产物 | 20 个文件；无 ps1/bat/cmd；解压资源预检通过 | [最终身份](evidence/build-identity.json)、[PE 依赖](evidence/pe-dependencies.log) |

短测性能仅用于确认指标确实产生，不用于显卡排名、性能达标或替代 36 分钟基线：

| 场景 | 正式样本数 | P95 帧时 ms | 吞吐 FPS | 编辑 / 重建区块 |
| --- | --- | --- | --- | --- |
| static | 1689 | 6.7010 | 168.8722 | 0 / 0 |
| walk | 1683 | 6.7708 | 168.3082 | 0 / 0 |
| edit | 1684 | 6.8091 | 168.4612 | 40 / 120 |

对应 `evidence/desktop-quick` 各场景摘要和标准流日志均保留；完整逐帧数据保留在 `out/benchmark-native/界面 短测/run-20261004-053307-156Z`。GPU 查询有尚未返回的尾帧，按缺失计数保留，不填零或伪造。

### 二进制身份

本次基于提交 `17e23e0bc4e51921c9ad8b53c04f809375cdb509` 的未提交工作树，不能仅用该提交标识新实现。

| 阶段 | 游戏 SHA256 | 执行器 SHA256 |
| --- | --- | --- |
| 实机 GUI/GPU 短测 | `30e69da18ec62731b8081a5aea4c2b3ffaf0a955168c9bffb9d0ed0783ce73eb` | `35b725902996c95b6263148951dcbc684c08ba7ed1956e0f50b1d6689955b4d4` |
| 最终包 | `3717AF7168452CD741F294BBCF9C24C2259F660A3B12E257782A7DFA14C4E744` | `2E2210D1A938790A3251228F4D5DCAE02CE786EE865F4C254B86BB2F17225709` |

两阶段之间调整了 GUI 旧结果清空、GPU 长文本提示、标签背景、WM_NCDESTROY 解绑及异常退出收尾；禁止 Session 拷贝并把 GPU 温度元数据改成厂商中性措辞。最终构建与自动回归已通过，但不将旧哈希的 GUI/GPU 证据冒充最终哈希实测。历史摘要中旧的 nvidia-smi 措辞原样保留，并非原生工具实际调用了它。

最终包：`out/packages/SymoCraft-Benchmark-windows-x64.zip`，1,164,769 字节，SHA256 `419FFC4B5D6EF5AC629041E15E6B5EB7A208C857753ECA26CAB4581A20AC5B09`。安装目录为 `out/install/benchmark-native`。

113 项项目源码/构建输入的完整哈希清单位于 `out/benchmark-native/source-input-sha256.json`，清单哈希为 `CD133BBAB56555F240105464F6416FF7CD5205420DE61C2F05E43AFD29CE081F`。范围不含 vendor 源码，资源以最终安装文件哈希另记；这不是整个工作树的完整快照。

### 失败与复测

- 初次测试夹具缺少 iostream 头文件，补齐后编译通过；不影响交付入口。
- 两项资源重定位探针在受限执行环境中失败，随后在获准的正常 Windows 执行环境中复测通过；未将环境限制作为代码修复处理。
- 自定义构建目录曾把 CTest 日志写到预设目录，见 [[CMake-自定义构建目录日志修复]]。最终证据已由修复后的正确目录重新采集。

## 结论

- 判定：支持在本机采用独立原生执行器，朋友侧不需要 PowerShell 或调整执行策略。ZIP 压缩直接启动系统 tar，不经 shell；缺少 tar 时保留导出文件夹并报错，不安装替代工具。
- 不能推出：最终包全部 GUI 路径已重测、36 分钟稳定性/正式性能达标、Intel/AMD 真驱动兼容、Y9000P 或无开发环境电脑可用。真实轮次内取消只由夹具覆盖，本次 GUI 操作是轮次间取消。
- 不宣布 M2 或跨机器验收通过；旧 v1 正式基线不与此次 v2 短测合并。
- 待验收：最终包快速检查、原生版九轮正式基线、Y9000P、至少 AMD/Intel 实机、干净 Windows 机器、高 DPI/低分辨率布局。
- 当前包是工程测试候选版；第三方许可缺件仍见仓库及包内 inventory，不能宣称已完成公开发行许可审查。

## 复现与回写

- 朋友入口：完整解压后双击 `SymoCraftBenchmark.exe`，先快速检查，再正式基线，详见 [[使用与交付]]。勿最小化游戏；切换窗口允许但后台负载仍影响结果。
- 开发入口：CLion 的 Debug/Release CMake 预设；CTest 用明确的构建目录；CPack 使用 Release 的 `CPackConfig.cmake` 生成 ZIP。开发侧保留的 `.ps1` 不进入安装包。
- 本文依照 `05-技术实验` 模板记录有限范围实验，类与功能设计分别回写对应笔记。
- [x] 已记录环境、步骤、结果和两个版本身份。
- [x] 已回写执行器功能的验收表。
- [x] 已标明证据范围与尚未执行项；“已验证”不是全硬件交付认证。
