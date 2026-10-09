---
type: 类设计
status: 已验证
project: SymoCraft
module: benchmark
class_name: Benchmark::Process
inheritance: []
created: 2026-10-04
tags:
  - area/benchmark
---

# Benchmark Process 类设计

关联功能：[[Benchmark-执行与导出]]。功能验收集中在功能笔记，本篇仅维护所有权和不变量。

## 当前设计

职责：启动一个带独立 Job 的子进程、重定向输出、读取退出码、有限等待和终止。计划/摘要/重试由 `Run` 负责；Process 不知道场景和游戏内部结构。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `Handle` | `job_` | 空 → 自有 Job | 独占；KILL_ON_JOB_CLOSE，兜底终止未退出的自有子进程树 |
| `Handle` | `process_` | 空 → 子进程句柄 | 独占；查询、等待，销毁不代表游戏正常导出 |
| `DWORD` | `pid_` | 0 → 创建返回的 PID | 值；仅用于筛选 WM_CLOSE 目标窗口 |

辅助 `Handle` 独占可关闭的 Win32 HANDLE；忽略空和 INVALID_HANDLE_VALUE，析构不抛异常。局部日志、stdin、线程句柄也用它管理。

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| P1 | 成功构造后 `process_` 已加入 `job_`，随后才 ResumeThread | 构造返回后；构造中先 suspended |
| P2 | 子进程只继承显式指定的 stdin/stdout/stderr | CreateProcess 时；Job、进程句柄不被继承 |
| P3 | `pid_` 对应仍持有句柄的进程，Stop 只关闭该 PID 的窗口 | 构造后至析构；不按名称扫杀其他 SymoCraft |
| P4 | 退出码在 Running=false 后被当作最终码 | Run 的校验边界 |
| P5 | 任意异常路径都会释放自有句柄，仍存活的 Job 成员被收尾 | 构造失败、函数异常和正常析构 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `Process(exe,args,cwd,stdout,stderr)` | 宽字符路径，CRT 引号规则，CREATE_NO_WINDOW、挂起创建、Job 绑定、恢复 | 新日志文件；不经 cmd/PowerShell；P1/P2 |
| `Running()` | 非阻塞查询进程句柄 | WAIT_FAILED 抛异常；不把 STILL_ACTIVE 当真实已退出码 |
| `ExitCode()` | 查询系统退出码 | 调用者先等待完成，P4 |
| `Stop()` | 自有窗口 WM_CLOSE → 等 3 秒 → 必要时 TerminateJobObject → 最多再等 5 秒 | 返回是否强杀；已结束为空操作；P3/P5 |
| 析构（隐式，成员 RAII） | 释放进程句柄，再关闭 Job | 兜底保证不遗留子进程，不承诺强杀时有 CSV |

创建 Job 失败不启动程序。AssignProcessToJobObject 失败时终止仍挂起的子进程并抛错，绝不退回无约束执行。日志创建使用 CREATE_NEW，拒绝覆盖。允许嵌套 Job 的 Windows 环境正常运行；受限宿主不能绑定时显式失败。

Process 禁止拷贝和移动，单个 Run 工作线程拥有。Stop 由同一工作线程轮询 stop_token 后执行，GUI 不跨线程直接销毁句柄。先发关闭请求再等退出，是为了给游戏自己的导出/GL 清理机会；智能指针本身不能替代这个协议。

实现位置：`tools/benchmark/src/platform.h/.cpp`。验证：`benchmark.process`、`benchmark.executor` 的路径转义、瞬退、创建失败、取消、强杀；Debug/Release 通过。真实游戏最小化自行退出与终止码 4 见功能证据。

## 本次变更

无。已通过的进程管理约定合并到当前设计。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 需要友好的独立游戏取消 IPC | 增加版本化取消通道，仍保留 Job 最后兜底 |
| 运行其他类型的子程序 | 单独定义进程协议，不将 Process 变成任意 shell 执行器 |
