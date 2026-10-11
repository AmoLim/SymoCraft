---
type: 实验
status: 已验证
project: SymoCraft
module: benchmark
created: 2026-10-04
tags:
  - area/benchmark
---

# Benchmark 桌面交付验证

**早期原生执行器交付实验：本机 GUI 三场景失焦短测、导出、轮次间取消后继续、最小化失败路径和隔离构建均有证据。** 最终包又经自动回归与资源预检，但没有重跑最终哈希的 GUI/GPU 短测。本报告不是 36 分钟正式基线；后续修复版实验见 [基准冻结入口](Benchmark-基准冻结-20261005.md)。

## 条件与判据

Ryzen 7 9700X / RTX 5070 Ti / 64 GB；Windows build 26200、OpenGL 4.6.0 NVIDIA 616.64；MSVC 19.38.33145 x64、CMake 4.3.1，Release 实机与 Debug 回归。

seed 424242、441 区块、1920×1080、4x MSAA、VSync 关闭；static / walk / edit 各预热 10 秒、采样 10 秒、一轮。变量为原生入口、allow-unfocused、中文/空格路径、最小化和轮次间取消。电源/显卡模式/频率手填声明未记录，不挪用其他实验条件。

判据：朋友侧无需 PowerShell 或更改执行策略，缺失/无效数据不得报成功，取消不得影响其他进程，执行器不能反向依赖游戏对象。流程为双配置测试、三种隔离构建、GUI 短测/导出/取消与继续、最小化探针，随后小幅修订并重新回归、打包及预检。

## 结果与证据

| 检查 | 结果及边界 | 证据 |
| --- | --- | --- |
| 最终 Debug / Release | 各 20/20 | [Debug](../evidence/ctest-debug.log)、[Release](../evidence/ctest-release.log) |
| 根工程仅游戏 / 仅执行器 / 工具独立入口 | 3/3 构建及依赖检查通过 | [构建边界](../evidence/build-boundaries.log) |
| GUI 三场景失焦短测 | 3/3 有效，正式样本全部失焦 | [会话](../evidence/desktop-quick/session.yaml) |
| GUI 原生导出 | 成功，ZIP 不含程序，不经 PowerShell | [截图](../evidence/gui-quick-export.png) |
| GUI 轮次间取消后继续 | 完成；retried=true、continuous_full_protocol=false | [会话](../evidence/gui-cancel-resume-session.yaml) |
| 真实游戏最小化 | exit 4；framebuffer-changed-or-minimized、duration-not-completed | [摘要](../evidence/minimized/summary.yaml)、[日志](../evidence/minimized/stdout.log) |
| 异常路径夹具 | 瞬退、缺文件、旧协议、统计错误、时长不足、超时/取消通过 | CTest 中的 benchmark 测试 |
| Intel / AMD 夹具 | 不凭品牌拒绝，允许缺少 NVX；不是实机驱动验证 | benchmark.executor |
| 最终便携包 | 20 文件，含 CRT/资源，无 ps1/bat/cmd；解压资源预检通过 | [身份](../evidence/build-identity.json)、[PE 依赖](../evidence/pe-dependencies.log) |

| 短测场景 | 正式样本数 | P95 ms | 吞吐 FPS | 编辑 / 重建 |
| --- | ---: | ---: | ---: | --- |
| static | 1689 | 6.7010 | 168.8722 | 0 / 0 |
| walk | 1683 | 6.7708 | 168.3082 | 0 / 0 |
| edit | 1684 | 6.8091 | 168.4612 | 40 / 120 |

短测数字仅确认指标产生，不用于排名、性能达标或正式基线。GPU 尾帧未返回按缺失保留。完整数据在 `out/benchmark-native/界面 短测/run-20261004-053307-156Z`，docs 保留摘要和标准流。

## 版本边界

基于提交 `17e23e0bc4e51921c9ad8b53c04f809375cdb509` 的未提交工作树，不能只用该提交标识新实现。

| 阶段 | 游戏 SHA256 | 执行器 SHA256 |
| --- | --- | --- |
| GUI/GPU 短测 | `30e69da18ec62731b8081a5aea4c2b3ffaf0a955168c9bffb9d0ed0783ce73eb` | `35b725902996c95b6263148951dcbc684c08ba7ed1956e0f50b1d6689955b4d4` |
| 最终包 | `3717AF7168452CD741F294BBCF9C24C2259F660A3B12E257782A7DFA14C4E744` | `2E2210D1A938790A3251228F4D5DCAE02CE786EE865F4C254B86BB2F17225709` |

两阶段间修订 GUI 旧结果清空、长文本提示、背景、WM_NCDESTROY 解绑和异常收尾，禁止 Session 拷贝，调整 GPU 温度元数据措辞。旧哈希的真实运行不能冒充最终包实测；历史摘要的 nvidia-smi 措辞原样保留，不代表原生工具调用过它。

- 最终包 `out/packages/SymoCraft-Benchmark-windows-x64.zip`：1,164,769 字节，SHA256 `419FFC4B5D6EF5AC629041E15E6B5EB7A208C857753ECA26CAB4581A20AC5B09`；安装目录 `out/install/benchmark-native`。
- 113 项源码/构建输入清单 `out/benchmark-native/source-input-sha256.json`，SHA256 `CD133BBAB56555F240105464F6416FF7CD5205420DE61C2F05E43AFD29CE081F`；不含 vendor，不是完整工作树快照，资源另记。

## 失败与结论

夹具最初缺 iostream，补齐后通过；两个资源重定位探针受执行环境限制失败，获准在正常 Windows 环境复测通过，没有伪装成代码修复。自定义目录 CTest 日志偏移已按 [[CMake-自定义构建目录日志修复]] 修复并重新采证。

- [x] 本机原生交付流程在列明范围内成立，已记录版本/证据并回写功能验收。ZIP 使用系统 tar、不经 shell；缺 tar 时保留导出目录报错，不安装替代工具。
- 本阶段未验证最终包完整 GUI 路径、轮次内真实取消（仅夹具覆盖）、36 分钟正式基线、Y9000P、AMD/Intel 真驱动、干净 Windows、高 DPI/低分辨率；这些是当时的范围，不用后续修复版成绩追认旧包。
- 不宣布 M2、跨机器、长期稳定性或公开发行许可审查通过；旧 v1 正式基线不与本次 v2 短测混合。
- 操作入口见 [使用与交付](../使用与交付.md)；设计与案例维护在 [[Benchmark-执行与导出]]、[[Benchmark-文件协议]]，本篇只维护本次实验事实。
