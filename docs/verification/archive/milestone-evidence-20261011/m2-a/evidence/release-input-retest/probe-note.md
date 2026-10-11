# 本轮探针失败说明

这是依据当次工具输出补记的说明，不是自动生成的通过记录。

本轮已安装 Release 游戏输出了 `loop finished` 和 `shutdown complete; exit_code=0`，并保留了画面、材料切换日志及位置变化。随后 PowerShell 探针出现：

```text
verify-runtime.ps1: You cannot call a method on a null-valued expression.
```

原因是空 `stderr.log` 经 `Get-Content -Raw` 读取后，在本次环境中得到空值，后续调用 `Contains` 失败。探针未生成 `result.json`，因此本轮不能写成完整的自动探针通过。

修复为使用 `System.IO.File.ReadAllText` 读取 stdout/stderr。修复后的独立复验保留在相邻的 `debug-final-120` 和 `release-final-exit` 目录；没有补造或覆盖本轮 JSON。后者在用户停止电脑自动操作后仅等待进程结束，不作为自动 Esc 操作用例的完整通过证据。
