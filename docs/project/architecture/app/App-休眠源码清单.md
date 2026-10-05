---
type: 迁移清单
status: 已记录
project: Symocraft
module: app
created: 2026-10-05
tags:
  - area/architecture
---

# App 休眠源码清单

本清单为 T0 处置证据，不将旧源码当作当前支持模块。文件保持移动前字节完全不变，移动时逐个校验 SHA-256；扩展名增加 `.disabled`，不被 CMake 编译、安装，也不向模块开放 include。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。源码唯一归属与负向边界检查已通过；保留休眠源码并不表示其算法或并发正确性通过验收。

## 处置依据

| 类别 | 旧状态 | 本次处置与原因 |
| --- | --- | --- |
| input | `Input::ProcessKeyEvent/EndFrame`、`KeyHandler::Update` 没有接入现行 Application 循环，旧 handler 反向访问 Application | 保留原文供 T4 审计；正在使用的 key_snapshot 已迁入 platform，实际输入为新的 Window 快照 |
| event | EventListener 的 Init/Update/QueueMainEvent 没有现行运行入口；未完成的序列化/播放分支不属于当前玩法 | 保留但不编译，不把其不完整回放能力描述为已支持 |
| thread_pool | Application 中全局池访问原本是注释；没有创建线程池或派发任务的生产调用 | 不引入通用线程池模块，不启用旧并发实现 |
| aggregate | core.h 混入图形/YAML/内存/业务公共依赖；utils.h 的数值辅助由 world 私有实现承接，DebugStats 无现行调用 | 新模块均显式包含所属头；旧聚合头仅存档，禁止重新 include |

调用证据为迁移前 Application 实际代码，以及迁移后生产树中无这些入口的搜索；仅同一休眠源码内部互相调用不构成可达游戏入口。原来被列入构建不等于运行时使用。没有借此宣称旧线程池/事件代码正确，未来启用必须重新设计依赖和测试。

## 路径与字节校验

| 原路径 | 保存路径 | 移动前后相同的 SHA-256 |
| --- | --- | --- |
| `include/core.h` | `game/app/legacy/aggregate/core.h.disabled` | `370845d3b4459d786666ddbea5256ce8d8417eee5ef7e2fdcf8c4b6a84486c0a` |
| `include/core/utils.h` | `game/app/legacy/aggregate/utils.h.disabled` | `e8f512e032f067d81e66fc552becc84dbe2d3ca1f4c9cfe8f4cf34988188573a` |
| `src/event/event.cpp` | `game/app/legacy/event/event.cpp.disabled` | `10050f7160cc2bec5933746b8ba3ae5758b9a1ebe39d7ce20662ace920273c57` |
| `include/event/event.h` | `game/app/legacy/event/event.h.disabled` | `2f17242a2f83a1cb7972e898b2df75bd593836c5e1c5b3d4748a9cb70f4d0250` |
| `src/input/input.cpp` | `game/app/legacy/input/input.cpp.disabled` | `83088f63301590bc8b416a8c64fbfbeac5e96ea375c3f89f57a890eb082d5df9` |
| `include/input/input.h` | `game/app/legacy/input/input.h.disabled` | `69e1326e40c6853933ad15ba53e616827d80ec5bf0eb92b0e688f617da1671cd` |
| `src/input/key_bindings.cpp` | `game/app/legacy/input/key_bindings.cpp.disabled` | `d1cc6ebd9c0fbe6013e1b31a77dd2fbb227f0ac37ec5d6d0d657345cff85db87` |
| `include/input/key_bindings.h` | `game/app/legacy/input/key_bindings.h.disabled` | `2ad22f5497fe9c4d8929963cb87c0efb3a6dc0e820726d1d7b35225235c4cbcc` |
| `src/input/key_handler.cpp` | `game/app/legacy/input/key_handler.cpp.disabled` | `dbab8a0d5136eaf4215d107b8f7f4416f7a1e4624ad640ca59ce61d18ec5c2c9` |
| `include/input/key_handler.h` | `game/app/legacy/input/key_handler.h.disabled` | `f3f6c0933c3096f3db46b7fc8ba4c68ead90d4939f9a66ff8794949ee305ca6b` |
| `src/core/global_thread_pool.cpp` | `game/app/legacy/thread_pool/global_thread_pool.cpp.disabled` | `8ea637a0e0ec076020db25e30f28782fa77a87af455ec6e5b9041e10eb91544a` |
| `include/core/global_thread_pool.h` | `game/app/legacy/thread_pool/global_thread_pool.h.disabled` | `1b1f335f342b031aa69f97521b65371c08d0f302d880235a711a098fb45952dc` |
| `include/core/pool.hpp` | `game/app/legacy/thread_pool/pool.hpp.disabled` | `c60c11d1e1eb67279fd55136a847838773222faa504990145a69953e58d0df39` |

## 后续约束

- S0 清单应覆盖上述旧路径；此处保存的哈希可用于对照，阶段总报告负责记录对照结论。
- 不给生产 target 增加 legacy 目录；边界检查不把 disabled 文本视作编译源码，但必须拒绝其进入 target 的源文件清单。
- T4 可在明确审计和用户授权后处置休眠源码；本轮不批量删除。
