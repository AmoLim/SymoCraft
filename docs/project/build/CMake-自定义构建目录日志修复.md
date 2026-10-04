---
type: 修复
status: 已验证
project: SymoCraft
module: build
created: 2026-10-04
---

# CMake 自定义构建目录日志修复

## 问题描述

- 预期：开发构建助手指定自定义 BuildDirectory 后，CTest 执行位置和 Testing 日志都属于该目录。
- 实际：测试命令指向 benchmark-debug / benchmark-release，但日志更新在预设的 windows-debug / windows-release，容易把旧日志当成本轮证据。
- 范围：`scripts/build.ps1` 的 Test / Install 路径；该助手不交付朋友，原生执行器不调用它。
- 环境：Windows x64、CMake/CTest 4.3.1、MSVC 19.38；关联 [[CMake-模块边界]]、[[Benchmark-桌面交付验证]]。

## 最小复现

- 触发条件：BuildDirectory 与 CMake preset 的 binaryDir 不同，CTest 同时接收 `--preset` 与 `--test-dir`。

1. 用 windows-debug 预设配置到 `out/build/benchmark-debug`。
2. 旧助手把预设和自定义目录同时传给 CTest。
3. 检查两个构建目录的 `Testing/Temporary/LastTest.log` 更新时间及 Command 字段。

观察事实：预设目录中出现本轮时间的日志，而日志中的可执行文件路径属于自定义目录。测试通过本身不能证明证据文件保存在预期位置。

## 定位记录

| 假设 | 检查方法 | 观察证据 | 结论 |
| --- | --- | --- | --- |
| 测试执行了旧目录程序 | 检查 LastTest.log 的 Command | 指向 benchmark-debug / benchmark-release | 否定，不是旧二进制执行 |
| CTest 预设目录与 test-dir 同时影响状态 | 对照助手参数与日志路径，取消混用后复测 | 明确目录后日志归位 | 在本版本确认；不外推所有 CTest 版本的内部机制 |

已确认原因：助手混合了预设目录与覆盖目录两个入口，没有维持“自定义构建的执行和证据位置一致”的约定。

## 修复设计

- 显式 BuildDirectory：只传 `--test-dir PATH -C CONFIG --output-on-failure --no-tests=error`。
- 未显式指定：保留原先 `--preset PRESET` 路径。
- 不修改测试内容、测试目标、用户已有输出或既有默认构建目录。

```text
显式自定义目录 → CTest 明确目录与配置，不传 preset
默认目录       → CTest 使用已有 preset
```

## 验证与回归

| 案例 | 输入 / 步骤 | 预期 | 修复前结果 | 修复后结果 / 证据 |
| --- | --- | --- | --- | --- |
| 自定义 Debug | benchmark-debug，Action Test | 20 项测试与日志均在该构建树 | 日志在预设目录 | 20/20；[Debug 日志](../benchmark/evidence/ctest-debug.log) |
| 自定义 Release | benchmark-release，Action Test | 同上 | 同类目录偏移 | 20/20；[Release 日志](../benchmark/evidence/ctest-release.log) |
| 默认 preset | 不传 BuildDirectory | 保持既有入口 | 原分支存在 | 分支未改；本次未额外执行该分支，不标新实测通过 |

## 实现记录

- 修改：`scripts/build.ps1` 的 Test / Install 条件块，按是否显式自定义目录分支。
- 最终日志已从正确构建目录复制到模块 evidence；不依赖旧目录时间戳推定结果。
- 相关提交：本次工作树尚未提交。
- 限制：只在当前 CTest 4.3.1 验证；没有因此要求朋友安装或运行 PowerShell。

## 完成检查

- [x] 原问题已观察，执行位置与证据位置分别核对。
- [x] 修复依据有证据，没有把日志路径问题写成测试失败。
- [x] 自定义 Debug/Release 均验证；默认路径的未重跑限制已说明。
- [x] 构建边界和交付验证笔记已关联本修复。
- [x] 已更新 status。
