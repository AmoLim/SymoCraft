---
tags:
  - area/spec
---

# T5 验证

归档状态：2026-10-25 用户明确选择先同步主规格再归档，T5 的 ecs 与 module-boundaries 增量已同步，本 change 已移入当日归档；详见[同步与归档记录](#2026-10-25-主规格同步与归档)。不表示 F 生产代码或 Git 已同步。

最新状态：[2026-10-25 T5 与 M3 已获整体批准](#2026-10-25-t5与m3整体完成批准)，保留已列风险；其他机器 benchmark 改在现代性能分析工具接入后执行。以下按日期保留当时结果与待审状态，不将历史失败/未执行改判。

## 2026-10-18 S01.2 与 S01.3 独立前置实验

本轮聊天授权仅覆盖依赖身份核对及隔离 CPU 关键适配实验，不授权 T4 生产代码/构建修改、T5 生产替换、依赖/设计冻结、退役或 Git 提交。实验根为 `E:/Codex/Worktrees/refactor-ecs-memory/Symocraft`，基于 `feature-refactoring` 的 `11b4d6d9`；该工作树没有带入 F 目录未提交的 T5 规划，因此本页仍在 F 的唯一活动 change 中维护，不另建规划副本。未合入独立 T4 候选。

结论：**S01.2 身份与条件核对完成，S01.3 仅完成下述独立部分，关键适配门槛未通过。** 两项阻塞为当前 MSVC 19.38/C++20 的编译失败，以及补充 MSVC 19.44 普通 Debug 下真实分配失败触发 terminate。S01.1、S01.3、S01.4 和生产任务均不据此勾选。

### 依赖身份与接入条件

从[官方仓库](https://github.com/skypjack/entt)按[发布 tag v4.0.0](https://github.com/skypjack/entt/releases/tag/v4.0.0)下载至实验根的 `out/experiments/entt-v4.0.0`；没有改上游源码，核对后上游工作树干净。tag/commit 对应核对不是签名认证，也不是正式冻结。

| 身份 | 实际核对值 |
| --- | --- |
| 完整 commit | `85c6bba014049b5de8fad49d25424df2f1f6a8c1` |
| Git tree | `dff6c1dcda0dfc39f45fb773fe20ca5cf954c17b` |
| `git archive --format=tar HEAD` | 6727680 字节，SHA256 `8ccea2c4b00ae689e5501e5c1ab3362c09715eb8aa21c4108af8cf73d3c2e820`；本地生成的 Git 源码归档摘要，不冒称 GitHub 下载包摘要 |
| 本地 `src` 110 个受跟踪文件 | 相对路径排序后，将 `[路径,文件字节SHA256]` 列表以紧凑 ASCII JSON 编码再 SHA256：`3b46fe2fbf333900641117e2ce24d7bfecd833083c4a4be20f4f244d2573d32e`；包含本地检出换行形式 |
| 原始 LICENSE blob | SHA256 `73547f6370ee16ea560665329bb62d4e2d8250f41eaa204b47a6c694ca3286ea` |

[上游 CMake](https://github.com/skypjack/entt/blob/85c6bba014049b5de8fad49d25424df2f1f6a8c1/CMakeLists.txt)声明最低 CMake 3.28、header-only `EnTT::EnTT` 与 `cxx_std_20`。项目当前最低 CMake 3.22；实验自行建立窄 INTERFACE target，只传播固定 `src` 头目录、C++20 和 `ENTT_NO_ETO`，不执行上游顶层配置，不提高工程门槛。实际使用 CMake 4.3.5、Ninja、Windows SDK 10.0.22621.0；未用 CMake 3.22 二进制实测。

实验两 TU 共用同一 target/config：默认 32 位 entity/type key、默认 4096/1024 sparse/packed page、正常异常和标准分配；只额外定义 `ENTT_NO_ETO`，未定义 `ENTT_NO_EXCEPTION`、`ENTT_STANDARD_CPP`、`ENTT_USE_ATOMIC`、`ENTT_NO_MIXIN` 或自定义 STL。没有 EnTT ext 配置覆盖、AmoMemory、图形/SDL 链接，也没有定义 `_ITERATOR_DEBUG_LEVEL` 绕开普通 Debug。未来生产接入仍须统一所有消费者的配置；上游 MSVC detect_mismatch 没有覆盖 ETO，不能只依赖它证明全部宏一致。

[LICENSE](https://github.com/skypjack/entt/blob/85c6bba014049b5de8fad49d25424df2f1f6a8c1/LICENSE)为 MIT，版权为 Michele Caini、2017-2026。后续正式分发须保留完整版权与许可文本；现有 `game/app/CMakeLists.txt` 按 `licenses/` 安装第三方通知，未来接入时须补 EnTT 的原始 LICENSE（例如 `EnTT-LICENSE.txt`）及来源记录。本轮保留下载源码中的原文件，未改游戏安装规则，未生成/验证含 EnTT 的正式安装包。

### 实际配置与机制结果

当前 F 游戏缓存指向 MSVC 14.38.33130。最终实验用显式开发环境分别验证如下，不以目录名或请求的工具集代替实际编译器身份：

| 配置 | 实际结果 |
| --- | --- |
| MSVC 19.38.33145、C++20、Debug | 编译失败。`hashed_string.hpp:64/67`、`mixin.hpp:111`、`view.hpp:68/267` 等依赖类型语法报 C2061/C4346；未进入运行实验。移除实验显式 `/permissive-` 后仍失败。没有修改上游或放宽解析规则 |
| MSVC 19.44.35222、C++20、Debug | 机制程序构建并运行；`/MDd /Od /RTC1`，实际 `_ITERATOR_DEBUG_LEVEL=2`。这是已安装工具集的补充诊断，不是生产升级或 19.38 的通过依据 |
| MSVC 19.44.35222、C++20、Release | 相同源码/EnTT 配置构建并运行；`/MD /O2 /DNDEBUG`，实际 `_ITERATOR_DEBUG_LEVEL=0`，不是特殊 Debug ABI |

下表的运行结果均限定于 MSVC 19.44 两配置，不是新项目 Registry 的契约验收：

| 核对机制 | 结果与边界 |
| --- | --- |
| 空组件真实对象与 RAII | `is_empty` 的有构造/析构计数类型实际创建对象并返回引用/指针；非尾删除、Clear、复用及析构后构造/析构计数平衡。编译断言确认空类型 packed page 非零，未走无对象 tag 存储 |
| 对齐与资源移动 | `alignas(128)`、`unique_ptr` 资源组件地址满足对齐；非尾删除后另一实体值保持，资源/live 计数归零。没有跨结构操作继续解引用旧借用 |
| const | 可写 Registry 的 `get<T>` 为 `T&`，`get<const T>` 为 `const T&`，const Registry 的 get/try_get 只读；const 类型身份归一。两配置编译负例均因 const 写入报 C3892，不以其他编译错误充当负例通过 |
| 跨 TU 与可控碰撞 | 另一 TU 创建 Shared 后本 TU 正常读取，同一类型的 hash/inline token 一致。人为令不同类型 hash 相同，native `type_info::operator==` 确实判相等，但独立 inline token 地址不同且跨 TU 一致；没有访问错误类型池或依靠断言。仅证明单可执行文件内该检查候选可行，未冻结项目机制，不涵盖 DLL/跨构建稳定性 |
| 末代销毁 | 默认 traits 的 entity mask=`0xFFFFF`、version mask=`0xFFF`，保留版本 4095；使用合法 create hint 准备版本 4094。实验预检可在移除前拒绝并保留值；随后特意绕过预检执行原生 destroy，实际回收到版本 0 且相同槽复用。因此项目必须自行拒绝末代，不能依赖 native 防回绕；不是生产末代入口已实现 |
| destroy 回收与原生 Clear | 对已建池/非抛析构资源组件，普通 destroy 和整 registry clear 的实际 new/aligned-new 计数均为 0；对象已释放、实体失效。原生 clear 保留空池，重复 clear 和重新挂载成功。源码 `registry.hpp:504` 先逐池 remove 再 entities.erase，`clear:922` 先清池再移除活实体；只证明此有界负载与这些类型，不推广为任意用户析构/监听器无分配 |

### 真实失败与冲突

隔离可执行程序替换实际全局 new/new[]、aligned new 及配套 delete；各故障在独立子进程运行。先测该操作分配序列，再注入选定序号；故障触达一次后关闭注入，再做断言/诊断和重试。没有全量分配序号扫描、旧 allocator probe 或模拟返回失败。

| 操作与零基分配序号 | Debug / Release 实测 |
| --- | --- |
| Registry 构造 #0 | Debug 的 16 字节分配失败进入 terminate；Release 的 64 字节分配失败可捕获，重新构造/创建成功 |
| 首次 Resource 池创建 #0 / #1 | Debug #0（248 字节）可恢复；#1（16 字节）进入 terminate。Release #0（200 字节）及 #1（64 字节）可恢复；可恢复项均未发布该池，原 Shared 值保留且重试成功 |
| entity packed 增长 #0 | 两配置捕获 bad_alloc，活实体计数/既有值保留，重新 create 成功 |
| 对齐组件页 #4 / 组件自有资源 #5 | 两配置分别触达 131072 字节、alignment=128 的页分配和 4 字节资源分配；失败未发布组件，旧值保留、重试及析构计数正常 |
| Shared packed 增长 #0 / 数据页 #1 | 两配置捕获真实 bad_alloc，目标组件未发布，旧值保留并可重试 |
| 组件构造主动抛错 | 两配置目标组件未发布，旧值保留，后续正常构造及清理成功；不宣称回滚构造输入/外部副作用 |

Debug 两个终止子进程均已记录真实注入命中；安装 terminate handler 仅将终止事实转成退出码 86，**不是捕获或恢复成功**。源码核对表明 MSVC 14.44 的 `vector:670-675` 在 noexcept 构造中调用 `_Alloc_proxy`，`xmemory:1219-1223` 实际申请 `_Container_proxy`；EnTT 默认 STL vector 路径使用它。这与运行观察一致，但本轮未用调试器逐帧回溯所有代理路径。Release 的成功不覆盖普通 Debug，也没有关闭 iterator debug 伪造通过。

### 复现与剩余门槛

实验源码留在实验根 `test/experiments/entt-preflight/`，未加入根 CMake/CTest 或游戏 target；构建及故障观察留在 `out/experiments/entt-14.44-Debug`、`entt-14.44-Release`，失败构建留在 `entt-14.38-Debug`。早期 Visual Studio generator 两个目录实际都用了 19.38，不作为 19.44 证据；最终结果以上述显式 Ninja 开发环境为准。未复制日志/原始数据到本 change，也没有新增 evidence 附件。

在实验根用可用 Python 3.10+ 执行：

```text
python test/experiments/entt-preflight/run.py --identity-only
python test/experiments/entt-preflight/run.py --toolset 14.38 --configuration Debug --build-only
python test/experiments/entt-preflight/run.py --toolset 14.44 --configuration Debug --faults --negative
python test/experiments/entt-preflight/run.py --toolset 14.44 --configuration Release --faults --negative
```

本机实际 Python 为 Codex runtime 的 `C:/Users/ldh20/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`；PATH 的 `python` 与 `py -3` 不可用。运行脚本复用项目 `scripts/build.py` 的 MSVC 环境初始化，路径显式指向本机 VS2022/CLion，不改全局环境。脚本对预期失败/终止的观察完成返回 0 只表示复现实验完成，不表示候选适配通过。

尚未执行项目 owner/incarnation 耗尽、Clear 身份先准备/注册名称保留、查询保护/有序准备失败、项目 Failed 状态、完整注册/对象契约、T4 消费者或 GPU/安装组合回归；未实施 S05.1 盘点或任何删除。S01.3 不勾选。进入 S01.4 前须先确认编译兼容与普通 Debug noexcept 分配冲突的处置方向；本轮没有升级/更换候选、关闭 Debug 检查、改上游或削弱目标失败保证。新 Registry、T5 及 M3 均未据此通过。

## 2026-10-18 Debug 冲突定位

用户随后授权先明确 Debug 冲突处置方向。本次继续 S01.3 的隔离机制诊断，不修改 T4/游戏构建、EnTT 或 MSVC STL，不选择新失败契约，也不升级生产工具链。新增独立 `stl_noexcept_diagnostic` target，仅编译 `test/experiments/entt-preflight/stl_noexcept.cpp`，不包含/链接 EnTT，不定义 `ENTT_NO_ETO` 或覆盖 `_ITERATOR_DEBUG_LEVEL`。源码 SHA256 为 `f33ec7f00c6b658d6a482bb62348c13c5b4aa3eadd79682a5c08f12ee95dfd9f`。

每配置分别运行默认构造、显式 allocator 构造、移动构造、数据 reserve 四种操作的正常分配计数及下一次真实 new 失败子进程；已复核的每配置八个观察如下。诊断使用与前述实验相同的 CMake/Ninja/SDK 和显式 MSVC 开发环境；仅目标不同，不受 19.38 无法解析 EnTT 的问题影响。

| 实际配置 | vector 默认/allocator/移动构造 | reserve 数据增长 |
| --- | --- | --- |
| MSVC 19.38.33145、普通 Debug、iterator level 2 | 三个操作均声明 noexcept，各发生一次 16 字节 new；注入命中后均以活动 bad_alloc 进入 terminate，子进程退出 86，外层 catch 未恢复 |
| MSVC 19.44.35222、普通 Debug、iterator level 2 | 同上，三条路径均复现；不是单纯换到 19.44 即消失的问题 |
| MSVC 19.44.35222、普通 Release、iterator level 0 | 三个操作均未申请内存，故障未命中，属于不发生该代理分配，**不计为失败恢复通过** |
| 上述三个配置的数据增长 | 每次均发生一次 412 字节 new；故障命中一次、外层捕获 bad_alloc、原三元素值保留，关闭故障后 reserve 重试成功 |

两份 Debug 中的 `is_nothrow_move_constructible_v<std::vector<int>>` 均为 true，但移动构造仍会申请代理并在其失败时终止。因此静态不抛移动约束不能证明普通 Debug 下任意真实 OOM 可恢复；影响不只限 Registry 初始化，也包括含标准容器组件的潜在移动路径。未执行所有标准容器/组件类型的穷举或真实物理内存耗尽。

机制依据为[微软 STL vector 源码](https://github.com/microsoft/STL/blob/vs-2022-17.14/stl/inc/vector)：noexcept 构造/移动中调用 `_Alloc_proxy`；实际安装的 14.44 `vector:670-675/759-765`、`xmemory:1219-1223` 与该机制一致。[微软配置说明](https://learn.microsoft.com/en-us/cpp/standard-library/iterator-debug-level?view=msvc-170)确认普通 Debug 默认 level 2。运行证据是标准容器最小复现和真实注入，不冒称已取得原 EnTT 失败现场的逐帧调试器回溯。

由此排除“只在 Registry 外层加 catch 就可恢复”的办法；把代理预热到初始化阶段也不能保证初始化本身或以后新对象/移动时不会再分配。`Registry::Failed` 只适用于控制流实际返回项目层的错误，不能在 terminate 后保存摘要、正常析构或继续使用。单换 EnTT 版本也没有证据能避开同一 MSVC 标准容器行为；本轮未更换候选、修改分配器或引入应急内存。

构建/观察留在实验根 `out/experiments/stl-14.38-Debug`、`stl-14.44-Debug`、`stl-14.44-Release`，复现为原运行脚本添加 `--stl-diagnostic`，例如：

```text
python test/experiments/entt-preflight/run.py --toolset 14.38 --configuration Debug --stl-diagnostic
python test/experiments/entt-preflight/run.py --toolset 14.44 --configuration Debug --stl-diagnostic
python test/experiments/entt-preflight/run.py --toolset 14.44 --configuration Release --stl-diagnostic
```

实际观察已区分捕获恢复、noexcept 终止与未发生分配，未将终止/未触发当绿灯；没有新增 evidence 附件。当前明确需要用户决定是否保留普通 Debug 检查并接受该已知平台不可恢复边界，或改选其他构建/存储路线；聊天中的推荐不记为已选。本轮不改 A18/A23、CT01 或生产配置，S01.3/S01.4 保持未完成，19.38 编译兼容问题仍独立开放。

## 2026-10-18 VS2026 配对实验

用户安装工具集后授权先配对验证 EnTT 编译。本轮仍属于 T5 S01.3 的隔离候选核对，不构成生产工具链升级或依赖冻结。安装器现报告 VS2026 `18.11.0 / 18.11.12306.390` 安装完整且可启动；显式使用 `E:/Applications/Microsoft VS/2026`、工具集目录 `14.51.36231`，CMake 实际识别编译器 **MSVC 19.51.36260.0**，标准容器运行报告 `_MSC_FULL_VER=195136260`。Windows SDK 仍为 `10.0.22621.0`，CMake 4.3.5/Ninja、x64、C++20 不变。

实验前再次核对固定 EnTT commit、Git 源码归档与本地 src 摘要，均与本页首节一致，结束时上游工作树仍干净。沿用相同 C++ 实验源码、INTERFACE target 与 `ENTT_NO_ETO`，不改 EnTT/STL、根构建或游戏源码；仅给实验运行脚本增加 `--visual-studio` 参数并打印/检查实际工具集来源。Debug/Release 在各自新目录配置，不复用旧编译器的构建缓存。

| 项目 | 普通 Debug | 普通 Release |
| --- | --- | --- |
| 配置及编译链接 | 成功；`/MDd /Od /RTC1`，iterator level 2 | 成功；`/MD /O2 /DNDEBUG`，iterator level 0 |
| 空对象、资源/对齐、const、跨 TU/碰撞、末代、原生 Clear、构造异常 | 既有独立 CPU 用例通过 | 相同用例通过 |
| const 写入编译负例 | 在 `const_negative.cpp:3` 按预期拒绝，C3490 | 相同拒绝，C3490 |
| 八个代表性实际分配失败子进程 | 六个可捕获并按既有断言核对状态/重试；Registry 构造 #0 与首次池创建 #1 仍因 16 字节代理分配失败而 terminate，退出 86，**不算恢复通过** | 八个均实际命中、捕获并完成相应状态/重试检查；不推广为全进程 OOM 保证 |
| 不含 EnTT 的标准容器对照 | 默认/allocator/移动构造三条代理 OOM 均以活动 bad_alloc 终止；reserve 数据分配失败可捕获且旧值保留、重试成功 | 三条构造/移动无代理分配，未触发故障，不算恢复通过；reserve 的真实数据分配失败可恢复 |

首次运行主程序已编译链接成功，但旧驱动只识别 C3892，因新编译器改报 C3490 而误报负例失败；现仅对指定负例第 3 行接受这两个 const 诊断编号，并已重跑两配置验证。没有把任意编译错误当作负例通过。两配置均有预期的 Resource 对齐填充警告 C4324，不是编译失败。

产物仍在实验根的 `out/experiments/entt-14.51.36231-Debug`、`entt-14.51.36231-Release`；对应主程序 SHA256 分别为 `f770a2dfeda01068328092df794a93dbe965f8597ec54866fde4c83553a74112`、`5c62e62f3f3d1414ed6c6822ae3eefabac2cc362926a692f4605ed166eddc618`。标准容器对照位于同层 `stl-14.51.36231-Debug/Release` 各自目录；不复制原始输出到 change。

在实验根用上述可用 Python 执行下列两条即复现 EnTT 配对；将末尾 `--negative --faults` 改为 `--stl-diagnostic` 可复现标准容器配对：

```text
python test/experiments/entt-preflight/run.py --visual-studio "E:/Applications/Microsoft VS/2026" --toolset 14.51.36231 --configuration Debug --negative --faults
python test/experiments/entt-preflight/run.py --visual-studio "E:/Applications/Microsoft VS/2026" --toolset 14.51.36231 --configuration Release --negative --faults
```

结论：**VS2026 的该 MSVC 工具集能正常编译固定 EnTT v4.0.0 候选，且无需提高 C++ 标准或改上游；但仍未消除普通 Debug noexcept 代理分配失败冲突。** 本轮确认了可编译的 19.51 候选，不把它自动设为生产基线，不覆盖 19.38 的历史失败，也不表示比已验证的 19.44 带来 OOM 恢复改善。完整 S01.3/S01.4 仍未通过；未修改目标失败保证或替用户选择 Debug 限制，整体任务仍为 1/26。

## 2026-10-18 Debug OOM 候选配置

用户随后批准 T5 工作范围直接使用上述工具集，并要求优先处理已复现的 Debug 代理 OOM。本轮暂停其他适配补充，仅在独立实验配置中验证构建级处置；不把工具集授权推定为接受降低检查能力或生产配置迁移。

新增默认 OFF 的 `PREFLIGHT_OOM_DEBUG_PROFILE`：同一实验 ABI target 将 `_ITERATOR_DEBUG_LEVEL=0` 传播给 EnTT 候选、两 TU 消费者、const 负例及独立 STL 对照，保留 MSVC 19.51.36260、C++20、`ENTT_NO_ETO`、`/MDd /Zi /Od /RTC1`。实际编译命令确认两个 TU 均使用 level 0，运行脚本还检查每个子进程的实际 level。普通 Debug 和候选分别使用原目录及 `-oom-profile` 新目录，不混用对象文件，也没有修改任何 C++ 用例、上游或生产构建。

| 核对项 | 普通 Debug 对照 | 候选 Debug 配置 |
| --- | --- | --- |
| iterator level | 2，原两个 EnTT 代理 OOM 终止仍复现 | 0，不是原普通 Debug 已修复 |
| Registry 构造/首次资源池的分配 | 构造 14 次、首次池 8 次，含 16 字节代理 | 构造 3 次、首次池 2 次；相应代理不再存在 |
| 八个代表性真实分配失败用例 | 六个恢复、两个 terminate | 八个均命中实际 new/aligned-new、捕获 bad_alloc 并完成相应状态/重试检查，无 terminate |
| vector 默认/allocator/移动构造对照 | 前节已确认代理分配会终止 | 三条路径均为零分配、零注入命中；证明消除了该分配，不冒称捕获代理 OOM 成功 |
| vector reserve 数据增长 | 可捕获恢复 | 实际 412 字节失败命中，原值保留并可重试 |
| 对象/const/对齐/跨 TU/末代/Clear | 正常路径通过 | 同一正常路径及 C3490 const 负例通过 |

序号相同不代表分配对象相同：候选的构造 #0 已是 64 字节 bucket 分配，不是普通 Debug 的 16 字节代理。结果支持“去掉问题代理路径后，剩余代表性失败可恢复”，不支持“原代理异常可被 catch”或“全进程 OOM 已获保证”。候选主程序 SHA256 为 `302fb653bcbf152834b227aa1212f089c3d2e579d58f8941b3d1b86068c98eb8`，产物位于实验根 `out/experiments/entt-14.51.36231-Debug-oom-profile`；STL 对照位于 `stl-14.51.36231-Debug-oom-profile`。复现是在前节 Debug 命令中追加 `--oom-debug-profile`；普通 Debug 不加该开关，仍保持 level 2。原始观察不进入 change 附件。

**处置边界：候选配置已验证可消除本次代理 OOM 终止路径，但代价是真正关闭 STL checked iterators/iterator debugging，不是保留原检查的修补。** [微软配置说明](https://learn.microsoft.com/en-us/cpp/standard-library/iterator-debug-level?view=msvc-170)明确 level 0 的检查损失；本机 `xmemory:1446-1451/1520-1530` 也显示非零级别仍使用代理，不能仅把 level 2 改为 1 就假定不分配。若正式采用，相关库/消费者须统一配置并重新构建，[微软 LNK2038 说明](https://learn.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-error-lnk2038?view=msvc-170)指出不同 level 可改变 STL 对象表示，不能只改单个测试或 ECS TU。

本轮没有把该专用配置提升为已批准生产基线，没有改变 A18/A23/CT01 或将普通 Debug 恢复判通过。正式采用这条统一 Debug 配置方向仍待用户确认，S01.3/S01.4 保持未完成；其他 S01.3 补充按最新要求暂未继续。

## 2026-10-18 DBG01 配置批准登记

上述候选实验后，用户明确批准采用该统一 Debug 配置方向并要求具名记录。批准来源、具体配置、接受的检查损失、统一 ABI 重建要求和授权边界唯一记录于 [M3-T5-DBG01](decisions.md#m3-t5-dbg01-统一-debug-abi-配置)；前节“待用户确认”保留为当时状态，由本次批准更新，不改写历史实验结果。

本轮仅登记该配置选择，未修改生产构建、重新执行 CPU 实验或扩大失败恢复保证；既有实验结论与限制不变。S01.3/S01.4 仍未完成，整体保持 1/26；不据此新增全局阶段批准或阶段验收通过记录。

## 2026-10-18 S01.3 独立 CPU 适配补充

用户授权继续 S01.3。本轮使用 DBG01 已批准的 Debug 配置及普通 Release 对照，沿用前述 MSVC 19.51.36260.0、SDK 10.0.22621.0、C++20 和固定 EnTT 候选。结束时重新核对 commit、归档/本地 src 摘要与许可证摘要，均不变，上游工作树干净。实验仅新增 `test/experiments/entt-preflight/mechanisms.cpp` 并接入同一实验 target/驱动；三个 TU 的实际 Debug 编译命令均为 `_ITERATOR_DEBUG_LEVEL=0`、`ENTT_NO_ETO`、`/MDd /Zi /Od /RTC1`，没有修改生产代码、根构建或上游。

以下是**隔离机制原型**的实际结果，不是生产 Registry 实现或完整 A01-A26 验收。原型的 uint64 串行批次源、准备对象、注册元数据和查询 guard 只服务于前置核对，最终布局、公开签名及机制采纳仍归 S01.4。

| 机制 | 两配置实际结论 |
| --- | --- |
| owner/incarnation 与 Clear | 相同原生实体值在两个 owner 下仍区分；Clear 后刻意复用完全相同的 native 值，旧项目 ID 仍拒绝。成功/重复/空 Clear 保留注册与首次自有名，资源正常终结 |
| 身份准备及耗尽 | 新批次先准备再原生清理；真实 8 字节准备分配失败保留旧 ID、组件值及注册，关闭故障后可重试。失败消耗的串号不回退；独立合法末值夹具签发 UINT64_MAX 后持续拒绝，不回绕；析构不申请身份或分配 |
| 注册与类型碰撞 | 纯注册读取不创建池；原名称销毁/改写后首次副本保持，重复注册仍拒绝空/null 名称。两个碰撞类型分别先注册时，后者均在调用 native assure 前拒绝；旧组件保留，跨 owner 注册顺序不耦合 |
| 末代及销毁准备 | 实际 traits 的 null 索引/保留版本核对；4093 销毁后回收为 4094，随后销毁在移除前拒绝并保留值。非尾及索引 9001 的资源组件销毁在下一次真实分配已设故障的条件下零分配，资源计数正确 |
| 创建容量候选 | 源码 `storage.hpp:965-979` 的原生新索引耗尽检查依赖断言，不能直接充当项目异常。隔离候选用一个单调 fresh-index 高水位配合 native hint/free list，不另建槽映射；合法边界夹具直接准备索引 1048574，下一 fresh 创建在原生调用前具名拒绝，既有值保留且回收槽仍可创建。真实创建分配失败不发布实体或提交高水位，原 hint 可重试；没有自然创建百万实体，也未声称原生断言边界可恢复 |
| 查询及无变化结构入口 | 必需/排除集合及无必需组件时的空实体均正确。内层 Stop/异常只释放内层 guard；重复注册、已有 GetOrAdd、缺失 Remove、创建/销毁/新增/Clear 均先被查询上下文拒绝，原生结构触碰计数不变；visitor 抛错不回滚既有值修改 |
| 有序准备 | keySelector 在 guard 内，唯一键按键序遍历；平局、键生成/比较异常及真实暂存 OOM 均发生于零业务 visitor 阶段。外层查询捕获内层有序暂存 OOM 后仍受保护，全部退出后可重试 |
| Failed 分支 | 仅合法注入原型的一致性错误分支，不破坏内存。Failed 优先于查询上下文/非法参数；普通读取、注册、有效性与 Clear 拒绝，最小自有摘要无分配/不读存储，未知实体事实不补零，RAII 仍释放资源；不冒称复现了实际存储损坏 |

真实故障覆盖为每配置 **17 个子进程**：原有 8 个 native 路径全部复跑；新增 owner 准备 #0、Clear 准备 #0、有序暂存 #0、嵌套有序暂存 #0、受控 fresh 创建 #0，以及注册准备 #0-#3 共 9 个。注册四点依次实测为名称副本 96 字节、元数据容量 144 字节、native 池对象 200 字节、池索引容器 96 字节。每个点均确认实际 new/aligned-new 注入命中、捕获 bad_alloc，并核对对应未发布/旧值及关闭故障后的重试，无 terminate。零分配清理只记“不发生分配”，不充当 OOM 恢复；这也不是全分配序号扫描或全进程 OOM 保证。

既有空对象、资源/128 对齐、const、跨 TU 类型身份、native Clear、构造抛错与末代对照同时复跑通过，const 负例两配置仍在指定第 3 行报 C3490。Release 中故意必抛回调的实例化产生 C4702 不可达代码警告，既有 Resource 仍有 C4324 对齐填充警告；未忽略编译错误或将负例诊断推广为任意报错通过。本轮未重跑 level 2 的新原型，原普通 Debug 的历史终止仍有效，不混作 DBG01 成绩。

最终 `mechanisms.cpp` SHA256 为 `b691837e9682910a8090d89040375f05a88052d4b42734e0b3481eef06198c7a`；Debug/Release 主程序分别为 `9c8766f3c038837d2eab13a3c236954ffc076dcc835d6026997f54f6b6cd8637`、`3c3d05474132f626b74bbe2736bf06a67d2d155c50212aa042b89bbcae71f008`。产物及驱动观察仍留各自 `out/experiments/entt-14.51.36231-Debug-oom-profile`、`entt-14.51.36231-Release`，不新增验证附件。实验根复现命令为：

```text
python test/experiments/entt-preflight/run.py --visual-studio "E:/Applications/Microsoft VS/2026" --toolset 14.51.36231 --configuration Debug --oom-debug-profile --mechanisms --negative --faults
python test/experiments/entt-preflight/run.py --visual-studio "E:/Applications/Microsoft VS/2026" --toolset 14.51.36231 --configuration Release --mechanisms --negative --faults
```

**S01.3 的有界关键适配核对完成，整体进度更新为 2/26；S01.4 仍待精确版本/配置、候选机制及公开重载冻结与生产替换入口确认。** 本结论仅关闭独立机制实验任务，不宣布生产 Registry 已通过目标契约。生产状态机/完整注册与模板负例、固定种子模型、调用方和 GPU/安装组合仍属后续任务，未执行、未推定通过；未合并其他 worktree、删除残留或进行 Git/sync/archive 操作。

## 2026-10-18 S01.4 冻结准备审阅

用户授权进行冻结准备，并明确确认“同意，仅更新待审稿”。本轮只更新 proposal 的前置/依赖状态和 [design 待审候选](design.md#s014-待审冻结候选)，本页记录审阅结论；没有新增设计冻结、生产入口或阶段批准。已批准 DBG01 保持原义，tasks 仍为 2/26，S01.4 未勾选。

已独立预读现役六份 specs 全文/scenarios、scope/治理/config、当前 proposal instructions 及 T5 全部规划，并核对相关 T4/T3 的 specs/design/decisions。候选是既有 ecs/module-boundaries 增量的机制/重载细化，没有修改目标或现役规格，不形成单独读规格报告。重新运行依赖身份核对，commit/三类源码与许可摘要均与 S01.2 相同、EnTT clone 干净；本轮没有重跑 CPU/编译/GPU 实验。

静态审阅识别两类不能直接照搬的细节：`SerialSource::take` 的 8 字节堆准备只是实验故障路径，生产候选推荐 uint64 按值准备，旧 OOM 成绩不能推广成该布局已验证；`CheckedCreator` 与 Probe 是分开的原型，不能宣称已经覆盖完整生产组合。固定 EnTT `registry.hpp:921-930` 的 clear 调用实体 erase，`sparse_set.hpp:232-236` 的 swap_only 保留 packed 槽并调整活跃边界，因此候选 Clear 必须保留 fresh 高水位，而不是将其简单置 0。相关“Clear→用尽回收槽→再新建”、失败/边界整合及项目重载实例化均列为准确待核对项，本轮未执行。

接入审阅另确认当前 `symocraft_header_view` 排除 `/src/`，ECS 外部 target 白名单为空，安装通知中尚无 EnTT：待审设计要求窄专用头 target、定向白名单和原始许可部署，而非改通用过滤或开放 vendor 根。上游 config 的 detect_mismatch 未覆盖 ETO，仍须项目统一配置/校验。以上是源码审阅，不等于 CMake/真实消费者/安装验证通过。

当前 T3 已批准及同步 F 的事实按上游明确记录修正早期现状文字，不回写历史验证；T4 实际交付身份、最终工具链/ABI 组合及 T5 生产范围仍由 S01.1 确认。未改其他 change、生产/实验源码、根构建或全局批准索引；未合并、删除、提交、sync/archive。后续先补待审稿列明的定向核对，再分别提请正式冻结和实施入口确认，不以草案、OpenSpec validate 或已有原型成绩代批。

本轮 OpenSpec strict validate 与受影响已跟踪文档的 diff --check 通过。只读文档审计检查 62 篇/940 条本地链接，仍报告其他位置的 17 项链接/历史材料问题，未报告本次 T5 文档链接错误；全库审计未通过，不改无关文档或补造缺失冻结材料来消除错误。

## 2026-10-19 按值批次与创建高水位组合核对

用户授权定向核对前节待审稿的第一项缺口，并明确暂不启动游戏实例。本轮只在实验根新增 `test/experiments/entt-preflight/identity_highwater.cpp`，接入已有独立 target 与 `--identity-highwater` 驱动；不连接游戏 target、不初始化 SDL/GPU、不运行游戏或性能采样，不修改生产代码、根构建、EnTT/STL。正式规划及本页仍维护于 F 的唯一 OpenSpec 根，没有复制另一套规划到实验 worktree。

依赖身份重新核对与 S01.2 一致，实验结束 EnTT clone 干净。仍使用 MSVC 19.51.36260.0（目录 14.51.36231）、SDK 10.0.22621.0、x64/C++20、`ENTT_NO_ETO`；四个 TU 的实际 Debug 命令均为已批准 DBG01 的 `_ITERATOR_DEBUG_LEVEL=0 /MDd /Zi /Od /RTC1`，Release 为 `/MD /O2 /DNDEBUG`、运行 level 0。新实验用独立 `identity-highwater-*` 构建目录，未覆盖前节机制产物；未重跑或改判普通 Debug level 2 的历史终止。

原型将串行 uint64 批次按值保存在 owner 中，先取批次、后构造 native Registry；Clear 先准备下一批次，原生清理成功后才提交，fresh 高水位不回退。共享批次源仅由测试宇宙串行拥有，边界夹具在新建宇宙起点设置末值，不重置或复用已签发身份；这不是正式全局身份源或生产 Registry 实现。

每配置 11 个独立子进程均完成相应断言：4 个正常/边界场景、1 个故意错误的反例对照、1 个受控失败分支和 5 个真实分配失败点，共 22 个子进程。反例和受控分支通过是指捕获了预期行为，不是正常机制或实际损坏恢复通过。

| 组合场景 | Debug / Release 实测与边界 |
| --- | --- |
| Clear → 用尽回收槽 → 新建 | 创建索引 0/1/2，Clear 后 batch 更新、旧 ID 失效，实体槽数和高水位仍为 3。先用尽三个回收槽，随后新建准确取得索引 3，高水位变 4；普通销毁/回收、多轮 Clear 及空 Clear 后仍可继续增长至索引 4。注册名称/池对象保持，资源计数平衡，另一 owner 的 ID 不受影响 |
| 不发生分配 | 按值准备加该负载原生 Clear，以及预留槽回收，均在下一次实际 new 已设故障的情况下实测分配数 0、命中数 0；属于没有分配，不计 OOM 捕获恢复。未制造原堆准备方案的 8 字节假故障 |
| 身份源耗尽 | 独立夹具从 UINT64_MAX-1 开始，成功 Clear 签发最大值；之后两次 Clear 和新 owner 构造均在进入原生清理/构造分配前拒绝，原 ID/值/注册和高水位保留。已有 owner 仍可创建无需新批次的实体，析构零分配，不签新批次 |
| 索引边界及末代 Clear | 两个近边界真实槽用尽后，fresh 高水位为 1048575，下一 fresh 创建在 native 调用前拒绝；Clear 保留该高水位，两个保留槽仍可回收，之后再次拒绝 fresh。此为跳过未用前缀的有界夹具，不是百万活实体测试。版本 4094 的普通销毁先拒绝，随后 native Clear/回收确实变为版本 0，但新 batch 保证旧项目 ID 不复活，下一 fresh 索引仍为 1 |
| 错误清零的负对照 | 故意将 Clear 后的高水位改为 0，用尽回收槽后请求 hint 0；EnTT 因槽 0 在用而回退，实际创建索引 3。原型检测返回值不符、拒绝发布项目 ID 并进入 Failed；原生已经变化，不能当作无变化拒绝或简单回滚高水位继续使用。这直接支持“保留 native 槽时不能清零高水位”，不是 EnTT 不遵守 hint 契约 |
| 受控清理提交失败 | 在实际 native Clear 完成后、项目 batch 提交前故意抛受控异常，验证进入 Failed、保留自有旧 batch/高水位摘要、不重新使用已消耗批次，普通操作拒绝，最小摘要零分配且不读 native 存储，资源正常释放。此为合法分支注入，**不是真实 native Clear OOM 或任意损坏恢复** |

实际全局 new 失败每配置覆盖 5 点，均命中一次、捕获原始 bad_alloc、退出 0，无 terminate：native 构造 #0；首次 fresh 创建 #0；Clear 并用尽回收/packed 容量后的 fresh 增长 #0；跨 entity sparse page 的指针表增长 #0 及页分配 #1。构造失败消耗但不复用已签批次，重新构造取得下一批次；其余点失败不发布新实体、不提交高水位、不更换 batch，已有 ID/组件值/注册保持，关闭故障后以完全相同的未提交 hint 重试并挂载组件成功。页边界为 4095→4096，独立稀疏夹具未遍历该前缀；不是全分配序号扫描或全进程 OOM 保证。

`identity_highwater.cpp` SHA256 为 `2a343aba4cc467a6457c577dd7611e3d250b1df520f517d955c48cb78a9c10e8`，本轮驱动 SHA256 为 `1b048502c664073186266a49c0c41450d47c581b40c9f3da2d8e48e2b44a0292`；Debug/Release 可执行文件分别为 `600583cb74e0d0cd1a02203071941fd37761facdd92f2f8ac1f3cf675eda24a7`、`1c20f64d1b6601b83885dcbe6ef2781f838cbf755911bd9f2938555a6e5944c9`。产物及驱动观察只留实验根的 `out/experiments/identity-highwater-14.51.36231-Debug-oom-profile`、`identity-highwater-14.51.36231-Release`，不新增验证附件。沿用旧 TU 的 C4324/C4702 警告，本轮新 TU 无编译诊断。复现：

```text
python test/experiments/entt-preflight/run.py --identity-only
python test/experiments/entt-preflight/run.py --visual-studio "E:/Applications/Microsoft VS/2026" --toolset 14.51.36231 --configuration Debug --oom-debug-profile --identity-highwater --faults
python test/experiments/entt-preflight/run.py --visual-studio "E:/Applications/Microsoft VS/2026" --toolset 14.51.36231 --configuration Release --identity-highwater --faults
```

结论：待审稿第一项组合缺口已取得上述有界实测依据，支持按值批次配合保留 fresh 高水位的候选；不改写前节当时“尚未运行”的记录，也不据此宣布完整新 Registry 契约已通过。公开重载声明/实例化、S01.1 实际输入与生产范围、正式冻结及实施入口仍待各自核对/确认；S01.4 不勾选，整体仍为 2/26。本轮没有启动游戏实例或执行合并、删除、提交、sync/archive。

本轮 OpenSpec strict validate 通过，apply 状态复核仍为 ready、2/26，S01.4 未完成。T5 已跟踪文档及新增验证记录/实验源码的空白检查无错误诊断；Git 仅提示将来写入时的 LF/CRLF 转换，未执行全库换行规范化。实验 worktree 仍仅有未跟踪的独立实验，无已跟踪生产差异；本轮未重跑全库文档审计或原有全部 CPU/编译负例矩阵，不以本组合实验代替它们。

## 2026-10-19 公开重载候选定向核对

用户授权核对 const、名称参数和查询协议。本轮仅在既有实验根增加 `api_surface.hpp`、两个正例 TU、一个按宏隔离的编译负例 TU 和标准库 Python 驱动 `run_api.py`；独立 CMake 开关 `PREFLIGHT_API_SURFACE` 默认 OFF，各 target 不参加默认构建。未修改生产 Registry/根构建、现役规格或待审设计，未接入游戏、SDL、GPU、正式消费者或安装目标，也未启动游戏实例或性能采样。

沿用固定 EnTT commit `85c6bba014049b5de8fad49d25424df2f1f6a8c1`，两轮驱动均先核对身份，归档/src/许可证摘要与 S01.2 一致，结束时上游 clone 干净。MSVC 19.51.36260.0（目录 14.51.36231）、SDK 10.0.22621.0、x64/C++20、`ENTT_NO_ETO` 不变；实际正例两 TU 和负例命令均核对，Debug 使用 DBG01 的 level 0 与 `/MDd /Zi /Od /RTC1`，Release 为 `/MD /O2 /DNDEBUG`、运行 level 0。未重跑普通 Debug level 2，也没有改判其已知代理 OOM 终止。

隔离原型使用真实 native 存储执行 typed 转发，模板函数体实际实例化，不仅检查声明或 requires 表达式；查询刻意按实体存储扫描，检验协议而非冻结生产查询算法。私有夹具只提供本轮所需实体和自有观测，原型 ID 未整合前节批次/高水位机制，不构成第二个正式 Registry 或生产门面。

| 核对面 | Debug / Release 最终结果 |
| --- | --- |
| const 与结构入口 | Get/TryGet 的可写/只读 owner × T/const T 共 8 项准确返回类型断言通过，实际读取同一对象、池数不变；第二 TU 的重复注册与读取/值修改成功。具参 Emplace 支持不可默认构造类型，GetOrAdd 正常默认构造/保留已有值，Remove 返回 bool。非法 ID 与正常缺失不混同；未注册状态读取不造池 |
| 名称重载 | 字面量、const char*、string_view、临时 std::string 均可调用，无歧义；首次名称有自有副本，后续合法重复不覆盖。nullptr、null 指针变量、空字符串/空 view 在初次或重复注册时具名拒绝，不发布新注册；自有计数确认 null 拒绝未先求长度。查询中拒绝先于名称读取；Failed 与查询/非法名称同时存在时仍先拒绝 Failed。有序 key 准备阶段也受到查询保护。调用前由调用方构造 view 的参数求值不属于入口可保护范围 |
| 查询重载与协议 | Each/EachOrdered 的可写/const、有/无 Exclude 八类重载均实例化并执行。按声明顺序传 ID 和组件引用，混合 const、反向组件顺序、排除不增加参数、move-only visitor 均成立；零必需路径含无组件实体，零必需有序路径也执行。void/Continue/Stop 协议与 Completed/Stopped 返回正确，空结果 Completed，最后一实体或唯一匹配明确 Stop 仍 Stopped；有序按只读键排序。visitor 原异常传播、退出后 guard 恢复且可再注册 |

每配置有 3 个正常运行子进程（const、names、queries），全部退出 0；另有 **36 个独立对象编译负例**均因预定原因失败，两配置合计 6 个正常运行子进程及 72 个预期编译拒绝。负例涵盖 Get/TryGet 写 const、注册/结构/Has/注册状态的非法 const 或引用、非法读取/查询修饰、底层类型重复、Exclude 修饰/重复及必需排除重叠、遗漏 ID/参数乱序、普通及有序查询的 const 写权限、可写 keySelector、bool/int visitor、零必需返回协议、非默认 GetOrAdd、不抛移动约束和错误构造参数。

驱动对约束拒绝要求 `api_surface.hpp` 实际 C2338 及对应精确断言标识，对直接 const 写入要求负例指定源码行的 C3490/C3892；不接受任意编译错误、链接错误或仅声明存在作为通过。首轮正例发现夹具错误调用 iterable 的 size，已改为迭代计数；中间冗余返回产生的 C4702 已清理。最终同一源码两配置完整复跑，正例编译链接无诊断；初次失败未计作编译负例成功。此轮没有进行分配失败注入，也不产生新的 OOM 恢复结论。

最终 `api_surface.hpp` / `api_surface.cpp` SHA256 分别为 `90a32cab061b8ab3b0fc553b2c40637e6a11a9710646614f9e77a5baddd6d53a` / `bf78ca799dc855c8c4ef2f68c54c05bee0842334e4a51fd727c38297346e038b`；负例源码 / 驱动分别为 `f9dd9126af935bdc4e2e93f3034565bb9cd600fb50945f890de2a54a05e77dfd` / `8f532ad7610d618e15f78e58a489c7ff791ca0d5aff4f35f672ac8c1a9764eec`。Debug/Release 可执行文件分别为 `4a6adc032ee981fe99f468f5ae0a1f23227f132d53bdafc25e4317a8bf46cd23` / `bf10694873875aa8ef533c8e0e70001e344b051103ef447c8a91379abd930291`。产物及驱动观察只留实验根 `out/experiments/api-surface-14.51.36231-Debug-oom-profile`、`api-surface-14.51.36231-Release`，未覆盖其他实验目录或新增验证附件。用此前记录的可用 Python 在实验根复现：

```text
python test/experiments/entt-preflight/run_api.py --configuration Debug --oom-debug-profile
python test/experiments/entt-preflight/run_api.py --configuration Release
```

结论：待审稿所列 const/名称/查询公开重载的定向缺口已有上述有界可编译、可拒绝和运行依据，未发现需要削弱目标契约或修改 EnTT 的冲突；不等于全部公开表面、完整新 Registry 或生产消费者契约已验收。特别是正式 EntityId/状态摘要、生产类型碰撞/失败事务、生产查询算法与真实消费者/依赖隔离仍需正式实现后的相应验证。本轮不登记版本/设计冻结或生产入口，不勾选 S01.4，整体保持 2/26；S01.1 的实际交付输入、代码根和生产范围仍须另行确认。未执行合并、删除、提交、sync/archive。

本轮 OpenSpec strict validate 通过，已跟踪 T5 文档 diff --check 无错误；新增验证段及本轮六个实验文件的尾随空白检查无匹配。实验 worktree 仍无已跟踪生产差异，其他并发修改保留；未重跑全库文档审计、身份/高水位实验、旧全部机制矩阵或生产集成，不继承其历史结果为本轮成绩。

## 2026-10-19 T4主工作区状态同步

用户要求核对主工作区 T4 状态、更新 T5 相关文件并同步到主工作区，随后明确确认 proposal/design/tasks/本页四份文件的更新范围。本轮直接修改 `F:/GameDevelop/OpenGLProject/Symocraft/docs/openspec/changes/m3-t5-ecs-memory/` 唯一规划正文，不建立 E 规划镜像，不复制旧实验树覆盖 F，不修改 T4 归档或全局批准记录。

已核对[T4 完成批准](../2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)、[scope](../../../../scope/project-scope.md#m3-t4-完成应用平台与模拟模块接口)、[工程状态](../../../../architecture/engineering.md)、当前 AGENTS/config 及现役规格：T4 于 2026-10-19 获本机 RTX 5070 Ti 范围整体验收，五份增量已同步为现役契约，change 已归档；后续独立明确授权又将生产源码和必要构建/测试配套带入 F 的 feature-refactoring。T3 同样已批准归档，D3D12 默认、Vulkan 显式可选、GL 保留。归档或旧验证中“F 仅 T3/仅审核 docs”的文字是当时事实，本轮不改写它们，也不再将它们当当前未交付状态。

只读源码确认 F 已有 `SimulationSession::Create/Advance/Pause/Resume`、`physics_adapter.cpp` 的按实体取小值/求解成功后提交，以及 simulation/physics 内真实 STATIC `symocraft_physics`；app 的真实 owner 为 `symocraft_application`，旧 runtime 名仅为 alias。F ECS 仍是 T4 限定修补后的旧 typed/byte 池，不是新 EnTT Registry。原 `E:/Codex/Worktrees/refactor-ecs-memory/Symocraft` 实验树没有 `game/modules/simulation/src/session.cpp`，其原实验身份不变，不能直接充当最新 T4 生产输入；本轮没有将其源码同步到 F 或反向合入 T4。

主工作区 CLI 基线已是 VS2026、MSVC tools `14.51.36231` / cl `19.51.36260.0`、SDK `10.0.22621.0`，与 T5 已验证工具集一致；这不证明 T5 DBG01 的 level 0 已在生产库/消费者全链路统一或完成重建。S01.1 仍须确认 T5 实际采用源码/构建身份、统一 ABI 和代码根/生产范围，S01.4 仍待正式冻结/实施入口确认，任务保持 2/26。本次同步只纠正输入状态，不新增行为/能力、重选决策或授权生产替换。

T4 的 CPU Debug foundation.files/1175、历史导出/缺码失败与未覆盖风险仍按归档保留，Fix1 继续暂停；上游整体批准不等于全部测试全绿，也不将旧成绩转成 T5 替换后的成绩。本轮仅静态/文档核对，未构建、测试 CPU/GPU、启动游戏、采样或追加人工验收，未提交、合并、删除、规格 sync/archive。

主工作区 T5 strict validate 通过，apply 复核仍为 ready、2/26，S01.1/S01.4 未勾选；本轮四份文档的尾随空白检查无匹配、已跟踪文档 diff --check 无错误，仅 Git 换行提示。只读文档审计覆盖 49 篇/705 条本地链接，T5 文件未报告链接或锚点错误；全库仍有其他位置的 17 项缺失材料/锚点问题，未扩修或宣称全库通过。

## 2026-10-20 上游输入同步与正式入口

用户本轮明确要求将已验收的上游代码同步至本 worktree、开始 T5 正式工作，并在获知冻结范围后明确回复“确认冻结，按此进入正式替换”。设计冻结和正式入口分别登记于[关键节点](../../../../verification/VERIFICATION.md#关键节点)；代码根为 `E:/Codex/Worktrees/refactor-ecs-memory/Symocraft`，规划正文继续在 F 的唯一 change 维护，不修改 actionContext 伪造授权。删除、Git 提交/推送/合并、规格 sync/archive、额外采样/人工及阶段验收不由本次授权。

同步前 F 已跟踪源码干净，HEAD 为 `6fea31e4631e267f132aa9b1608dc6538b562e13`；E 已跟踪源码干净、仅有既有独立实验，HEAD 为 `11b4d6d9a2d8f4d925bf6e2d977fcb96e048a350`。上游差异含个人 Obsidian/生成技能去跟踪，因此未整树 checkout 或移动 HEAD/index。只同步上游跟踪的 assets/cmake/game/lib/scripts/test/tools/vendor 和七个必要根文件，不动 docs、个人设置、技能、旧 out/缓存和冻结材料；无代码删除项，无未跟踪目标冲突。

4362 项选定输入同步后逐字节匹配 F，实际复制 354 项（含检出换行字节差异，不冒称全部为语义改动）；原 15 个实验文件哈希不变，源工作区未改，HEAD/index 保持。输入清单留 E 的 `out/t5-input-20261020-001/source-input.json`，相对路径排序后对紧凑 ASCII JSON 的 path/sha256/bytes 列表取 SHA256 为 `9ae823989c922ed446145b0fc9bef0b19badf6c3ddf5949e1f9b99f135517828`；不新增验证附件或将其当 Git 提交。此后 E 的生产差异须区分已同步上游与真正 T5 改动。

同步输入的未替换基线使用全新 `out/t5b1-Debug` / `t5b1-Release`，实际 MSVC 19.51.36260.0、tools 14.51.36231、SDK 10.0.22621.0、x64/C++20，CPU-only。两配置六个实际测试 target 构建成功，定向 CTest 各 8/8：ECS 普通/跟踪、真实旧 allocator 故障普通/跟踪、simulation contracts、physics core/独立消费者、Session（含现有 D12）。这是旧 ECS 上游输入基线，不是新 EnTT 契约或完整 CPU/GPU 矩阵；没有执行 foundation.files 或把已知1175改判。实际生产 registry/session/physics 编译命令单独记录，输入 Debug 仍 `/MDd /Zi /Od /RTC1` 且未显式设 level 0，后续 DBG01 必须全链路新目录重建，不能复用这两个基线缓存。

S01.1 的采用输入、代码根/范围、工具链与 ABI 处置已明确；S01.4 的固定候选/配置、身份/高水位与接口方向在既有两轮定向核对后获明确冻结。两项据实完成，任务更新为 4/26，S02 起的实际实现/验证仍独立。新 Registry、T5/M3 均未验收；未启动游戏或重复 T4 性能采样。

## 2026-10-20 固定依赖与统一 ABI 接入

本轮开始 E 代码根内的正式生产改动，F 仍只维护规划与验证正文。固定本地 `vendor/entt/src` 的 110 个文件逐字节匹配前述官方 commit，源码清单摘要仍为 `3b46fe2fbf333900641117e2ce24d7bfecd833083c4a4be20f4f244d2573d32e`；LICENSE 使用 Git 原始 blob，SHA256 为 `73547f6370ee16ea560665329bb62d4e2d8250f41eaa204b47a6c694ca3286ea`。没有复制上游构建、测试或嵌套仓库，没有修改 EnTT/STL；既有实验 clone 保持干净。

收尾为 EnTT 目录增加窄 `-text` 属性，避免后续 Git 换行转换破坏原始源码/许可身份；check-attr 对 LICENSE/version.h 确认 text 为 unset。其余已存在字节保留规则不变，没有规范化其他文件。

新建窄 `symocraft_entt` INTERFACE target，仅提供固定 src include、C++20 和统一配置；真实 `symocraft_ecs` PUBLIC 消费它。项目 detail 头在实际 registry.cpp 中实例化版本、实体 traits、页大小、32 位身份与空组件真实对象断言，拒绝不支持的覆盖配置，没有 NativeRegistry 公开逃生口。边界检查只为真实 ECS owner 到该 target 放行公开传播，非 ECS 的直接原生头引用仍拒绝。许可证安装规则及来源通知已添加，但本轮没有执行游戏安装，不能宣称部署验证完成。

DBG01 通过根构建及独立 benchmark 入口共用的 CMake ABI 配置作用于 Debug C++ 依赖/消费者，保留 `/MDd /Zi /Od /RTC1`。最终 CPU Debug 生成的 137 条 C++ 编译命令、独立工具 Debug 的 38 条均核对显式 `_ITERATOR_DEBUG_LEVEL=0`；这是生成命令覆盖数，不冒称全部 target 已构建。工具编译命令不含 EnTT 路径，独立工具未因 ABI 统一而获得 ECS 依赖。Release 不强制 Debug 宏，实际运行 level 0。仍接受 DBG01 的迭代器检查损失，不将其称为普通 Debug level 2 OOM 修复或全进程失败恢复保证。

全部使用新的构建目录，MSVC/SDK 与同步输入基线一致：

| 最终验证目录（E 的 out 下） | 实测范围与结果 |
| --- | --- |
| `t5a1-cpu-Debug-003` | 六个输入基线 target 加实际 ECS 链接的配置测试构建；定向 CTest 9/9，配置编译拒绝 5/5 |
| `t5a1-cpu-Release-003` | 同一源码与相同定向范围；CTest 9/9，配置编译拒绝 5/5 |
| `t5a1-tool-Debug-002` | 独立工具构建及其测试 9/9，统一 Debug ABI，无 EnTT 依赖 |
| `t5a1-tool-Release-002` | 独立工具构建及其测试 9/9，无 EnTT 依赖 |

两配置配置测试直接运行均输出 `EnTT=4.0.0 MSVC=195136260 iterator=0 empty-page=1024`。每配置五个负例分别为缺失 ETO、显式 iterator level 2、错误 sparse page、原子配置及 64 位 id_type；驱动要求相应项目诊断及 C1189/C2338，不接受任意编译失败。最终另跑 source/negative boundaries 为 2/2，负例脚本原有 100 例加本轮 6 例共 106 例（13 接受、93 拒绝），包含 ECS 原生 include/精确公开传播正例与 simulation/app/world/其他 owner 的拒绝。测试进程未初始化游戏、SDL/GPU 或采样。工具构建保留既有 publication_tests.cpp:116 的 C4834 警告，没有忽略编译错误。

中间失败与产物均保留：首个 `t5a1-cpu-Debug` 配置被原有“禁止 PUBLIC 第三方传播”规则拒绝，后改为精确 target/link 例外，未扩大通用白名单。`t5a1-cpu-Debug-002` 曾完成 9/9 与五个负例；`t5a1-cpu-Release-002` 构建及 9/9 完成，但错误 iterator 负例先被 STL 自身拒绝，缺少要求的项目原因，故该轮负例不记通过。随后将显式非零 ABI 覆盖的项目拒绝移到 STL include 前，并以最终两份 CPU `-003` 从新目录完整复跑成功。工具不消费该 ECS 头，其 `-002` 结果对应的源闭包未变。未覆写旧缓存/结果，也未将失败配置记录为成功。

复现入口是项目既有 scripts/build.py 提供的开发环境与正常 CMake/CTest；本轮有界编排及观察留普通 `out/experiments/verify_t5_dependency.py`、上述各目录的 `dependency-result.json` 和构建日志，不新增 docs 附件或原始数据正文。此前输入清单用于区分上游同步与本轮 T5 差异，不再宣称改动后的 E 与未改的 F 生产源码完全相同。

**本轮完成 S02.1 的固定依赖/配置子集，不勾选 S02.1，任务仍为 4/26。** 生产 Registry 此时仍是旧 byte 池，稳定 owner、EnTT 存储与 typed 适配尚待实现；本轮绿色成绩只证明本增量接入、统一 ABI 与选定旧契约回归，不能替代新 Registry 的生命周期、身份、查询、真实 OOM、消费者迁移或整体验收。未构建完整游戏、执行 GPU/安装验证、扩大性能或人工范围；未提交、合并、删除、规格 sync/archive。

收尾 OpenSpec strict validate 通过，apply 为 ready、4/26；T5/关键节点文档 diff --check 无错误，所编写代码的尾随空白检查无匹配。只读文档审计覆盖 49 篇/716 条本地链接，仍报告其他位置的 17 项既有缺失材料/锚点问题，未报告 T5 或本轮关键节点链接错误；不声称全库文档审计通过，不改写历史冻结材料来消除问题。

## 2026-10-20 生产 Registry 替换与调用迁移

用户明确要求继续 T5 正式工作；本轮继续使用 E 作为代码根、F 作为唯一规划根，没有将生产修改回写 F。用户另具名批准“仅 internal.cpp 退出 ECS 构建，保留文件”，不是删除清单或 S05 完成批准。实现复核发现 Session 既有部分提交夹具依赖业务顺序后，先报告冲突，用户同意显式业务顺序键并要求先更新设计；按独立规格预读核对全部九份现役 specs/scenarios、相关 T4 设计/决策及暂停 Fix1 边界后，仅补充[物理步进顺序设计](design.md#物理步进顺序)。现有 A17/A22、D04/D12 语义及阈值保持，未改现役规格、T4 归档或重开其他决策。

### 实现及保证范围

正式 `symocraft_ecs` 已使用唯一 EnTT Registry，旧字节池不再参与此 target。稳定不可复制/移动 owner、RAII、私有批次/实体值的 EntityId、相等/哈希、单调不回绕批次与保留 fresh 高水位均落实；无公开 Free、原生对象、整数/byte/view/iterator 逃生口或旧 AddComponent 行为别名。注册拥有首次名称、独立类型 token 检查默认键碰撞，准备容量/原生空池后才发布元数据；正常对象含空类型、128 对齐资源与不可默认构造类型均走真实生命周期。

读取、严格新增/获取或新增/布尔移除、每实体计数和查询复用分层检查；组件生命周期阶段拒绝重入，查询包含准备与嵌套全程 guard。普通查询在函数内使用局部 typed view，无必需类型走实体存储；显式有序查询自有键、std::less<>、业务 visitor 前平局检查，不改原生池次序。失败判断区分逻辑活对象与可保留容量/空槽：正常 C++ 类型的 `in_place_delete` 名称属性可能让原生回滚留下 tombstone，不能仅因物理 size 变大就假报 Failed；已补相应构造失败、旧值/查询/重试用例，不由此承诺稳定地址或扩大 CT01 类型范围。Failed 只交接自有摘要与析构，测试一致性分支为合法注入，不是实际损坏或任意恢复证明。

玩家/Camera 装配改严格新增；Camera 到玩家工厂的输入先复制 Transform 值，避免跨结构操作保留构造输入借用。character/transform、物理、app 玩家查找及根测试改同步 typed 回调/正确 const。物理集合仍按三类身体组件匹配，按装配方 PhysicsStepOrder 完整键求解；缺键/平局不被过滤掉，准备失败零本步实体提交，单体求解成功才提交，已有前序提交不回滚。Session 的 Failed Registry 诊断先读自有状态而不进入 native 存储，具名 ECS 错误转为明确缺失 motion；未补零、缓存旧成功姿态或扩大公开 Inspect。

极端合法状态适配留正式 ECS 库内统一实现，只有 `registry_contract_tests` 与验证两层 Failed 交接所需的 `session_tests` 可 PRIVATE 消费窄 diagnostics 目录；没有整个 src/PUBLIC 授权、测试布局副本或生产对 test/support 的依赖。关闭测试的游戏构建仍独立成立。独立实验、固定 EnTT 源码/许可和历史产物保留，普通 Debug level 2 的已知终止不改判。

### S05 具名候选与保留

当前源码、正式 target/编译命令、根测试、独立工具和安装规则核对后，准确删除候选仅为下列五个原有 ECS 私有文件；**本轮全部保留，字节与上游同步输入清单一致**：

| 候选相对路径 | 符号/依赖核对 |
| --- | --- |
| `game/modules/ecs/src/internal.cpp` | 旧 ComponentContainer/SparseSetPool、counter 与旧 null_entity；仅获本轮退出编译批准，所有新构建均未编译它 |
| `game/modules/ecs/src/component_container.h` | 旧字节池、raw Add/Get/Remove/Free，只被上行旧 cpp 使用 |
| `game/modules/ecs/src/internal.h` | 旧整数 EntityId、CreateEntityId/GetEntityIndex/GetEntityVersion，仅旧池 include 链消费 |
| `game/modules/ecs/src/legacy/RawMemory.h` | RawMemory 原始序列化；生产 Registry 重写后无 include/调用，不在安装规则内 |
| `game/modules/ecs/src/legacy/MemoryHelper.h` | SizedMemory、Pack/Unpack/SizeOfTypes 及 MemoryHelper 打包函数；生产/根测试/工具无消费者，不在安装规则内 |

旧 RegistryViewer/Iterator、按类型编号 byte 入口与私有 Serialize/Deserialize 已随获授权的 Registry 同文件重写退出公开/实现表面，不存在另一个待删 iterator 文件。保留 `AmoBase.cpp`、foundation memory.h/私有 allocation_probe 及 foundation.memory 测试：app game_entry 和部分回归仍调用初始化/观测，foundation 根测试仍直接验证 Allocate/ReAlloc/Free，且该 cpp 同时拥有实际 Logger；不能整文件删除或声称标准分配已覆盖它们。ECS 新 OOM 依据来自实际全局 new/aligned-new，不再由旧 probe 绿灯证明；原 tracked 命名回归仍按原开关运行，但不构成新池分配跟踪或无泄漏证明。

`simulation/src/Camera.cpp` 仍被 legacy 玩法/渲染转接测试使用，GL 仍为正式保留路径；platform/src/input_state.h 是实际窗口输入归约，不退役。活动 game/tools/cmake 树无旧线程池或旧 Input 入口，不恢复后再删除；lib 的历史二进制、冻结/非活动实验和个人材料不纳入候选。CMake 对非活动 internal.cpp 只有精确保留例外，禁止将其重新编译为活动后备。S05.2/S05.3 仍须准确删除清单的另行确认及删除后的复验，不据本次只读核对勾选。

### 当前候选验证

源码采用前述上游输入加本轮正式改动，4486 项代码/构建/测试与 vendor 字节清单留 E `out/t5-owner-input-20261020-001.json`，紧凑排序清单 SHA256 为 `ab35f498178d211bb3bda97225764da39dc67ff42a43fadc342ca614af74fc3c`。不是 Git commit、冻结验收包或 docs 附件；不包含旧 out、个人文档和第二规划镜像。仍使用 MSVC 19.51.36260.0/tools 14.51.36231、SDK 10.0.22621.0、x64/C++20、固定 EnTT/ETO/DBG01。CPU Debug 140/140、游戏 Debug 82/82 条实际生成 C++ 命令显式 level 0；这是命令覆盖数，不是已构建 target 数。Release 未强制 Debug 宏，所有配置命令均无旧 internal.cpp。

| 最终构建目录（E 的 out 下） | 实际结果与边界 |
| --- | --- |
| `t5-owner-Debug-008` | 正式库及本轮选定消费者构建；最终 `delivery-tests.log` 37/37，通过后仍只称定向 CPU 结果 |
| `t5-owner-Release-008` | 相同生产/测试源码与同一选定范围；最终 37/37 |
| `t5-game-Debug-001` | `BUILD_TESTING=OFF`、D3D12 ON、保留 GL ON、Vulkan OFF，游戏完成构建及最终源码增量重建；没有运行或安装 |

每配置 37 项包含 ECS 正常/原 tracked 名称回归、配置、正式库契约/身份耗尽、编译负例驱动、真实分配/零分配路径、三个独立公开头、player math、simulation contracts/历史物理与玩法组、physics core/独立消费者、Session/D12、app render conversion 及 source/negative boundaries。不是主 CTest 全量或 Vulkan/真实 SDL/GPU/部署成绩。边界为 **113 例，15 接受/98 拒绝**，覆盖两个指定测试的窄 ECS 诊断正例及其他 test/app/PUBLIC/宽路径/genex 拒绝；实际 .inl 模板也纳入生产 include 扫描。

每配置 **29 个对象编译负例**全部因对应项目 C2338 标识或指定 C++ 诊断拒绝，涵盖 const/引用/volatile、类型生命周期、重复/重叠查询、错误回调/键权限、非默认构造、owner 复制移动与私有/整数/排序逃生；合计 58 例，不接受任意报错。正例真实跨 TU 注册/读取、260 类型、空对象/128 对齐资源、末代 4093→4094 拒绝、容量上界及 Clear 保留高水位/注册/名称均执行。身份源耗尽在独立子进程合法前进到末值，不回退源；Clear 准备拒绝实际保留当前 ID、组件值、计数和首次名称。极端索引夹具跳过未使用前缀，不称百万活实体实测。

真实 new/aligned-new 注入每配置覆盖 13 个不同点：owner、首次 fresh、资源挂载准备/构造内部、packed 增长、普通/嵌套有序暂存、component sparse 页、128 对齐 payload 页及注册 #0–#3。12 个点在 CTest，资源构造内部 #1 另跑同一实际可执行程序，均命中一次、捕获原始 bad_alloc、关闭故障后验证旧值/未发布/重试与资源平衡；不是所有序号扫描。注册四点实测请求为 112/96/200/64 字节；资源构造内部确实为 4 字节，区别于挂载准备 #0 的 8 字节。原 `aligned 1` 实际命中 sparse 页（Debug 16431/Release 16423 字节），已据实分名；`aligned 4` 才实际命中 131072 字节 aligned-new payload，未用普通分配假冒该路径。Clear、普通销毁、非尾/稀疏索引 9001 资源销毁与析构四类在下一真实分配已设故障时均实测零分配/零命中，只记“不发生分配”，不计 OOM 恢复。

普通/有序查询覆盖空实体、筛选/反向参数序、const、move-only visitor、显式 Stop、内层异常被捕获后外层保护、键生成/比较/真实暂存失败零业务 visitor 及退出后重试。固定种子 `0x35A91` 的 32 逻辑槽、600 步独立模型逐步核对有效性、组件/值/计数、查询集合与全部旧 ID；首差错路径保留步骤/逻辑行/字段/左右值/编译器，不使用 native 编号或旧存储作 oracle。Session 原 D12 每 Advance/World 提交对照、离散精确/有限浮点和固定绝对阈值保持通过；新增缺键/平局/业务反序/驱动池重排的提交前缀测试，以及 Registry Failed 的只读 Session 诊断测试，未将两层 Failed 混同。

中间失败保留且不计最终通过：`Debug-001` 被原有未编译源码必须有 owner 的规则拒绝，后对具名批准保留文件做精确例外；`Debug-002` 的 260 类型测试触及 COFF section 上限，仅该测试采用 /bigobj；`Debug-004` 的 volatile 负例先实例化上游约束、缺要求的项目原因，后改为先保证项目非法修饰拒绝且合法 const 不变；`Debug-005` 的 uint64 顺序键被 int 转发引发列表窄化，调用方改传明确 uint64；`Debug-006` 为 30/31，旧工厂 32 点上限未走到新六对象池的成功发布，保留故障语义后将既有有限夹具上限调为 64，最终完成失败命中与成功装配。后续各轮仅在源码闭包变化后复验，旧缓存/日志留存；最后两配置均为上述同一候选，未丢弃失败来挑绿。既有 AmoBase localtime C4996、资源对齐 C4324 及 Release 必抛 visitor 实例化 C4702 警告保留，无忽略编译错误。

### 有界成本观察与后续门槛

S04.5 仅执行 1025 实体、一个正常值类型、普通/有序各 16 次查询、3 次短重复的同负载观察；真实链接正式库，显式选择的 `registry_cost_probe` 不参加默认构建/CTest。CPU/配置与上述一致，详细观察留各 `-008/cost.log`，旧 `-007` 试运行保留。下表是本机观测范围，不是预算、速度提升、跨配置倍率或长期稳定性判定；未隔离全部系统负载，无旧实现同条件成本基线。

| 阶段 | Debug / Release 耗时范围（微秒） | 分配请求及累计申请字节 |
| --- | --- | --- |
| 注册 | 5.7–20.9 / 0.2–2.8 | 各 3 次/280 字节 |
| 创建/挂载及跨 packed 页增长 | 633.0–659.2 / 40.6–48.8 | 各 44 次；91482 / 91434 字节 |
| 普通查询 16 次 | 1776.5–1932.5 / 42.2–42.3 | 均 0 次 |
| 有序查询 16 次 | 5534.0–5755.7 / 423.5–492.2 | 各 288 次；1233712 / 1233072 字节 |
| 一半组件移除/重挂 | 379.2–409.1 / 15.8–16.4 | 均 0 次 |
| Clear | 122.8–125.3 / 2.5–2.9 | 均 0 次 |

累计申请字节不是峰值、活跃或保留容量；尚未测对象池/有序暂存峰值及进程峰值，owner 构造在本次计时外。两配置逻辑 checksum 均为 1722000，不以这组小场景推定游戏帧/内存预算、M4 优化或任意工作负载收益。

S02、S03、S04 与 S05.1 的实现/本轮必要有界检查据实完成，进度 **19/26**；S05.2/S05.3 和 S06 保持未完成。五文件本轮保留，删除及其后复验仍等新确认；真实后端运行/安装、剩余构建组合、当前架构收口、人工项目/总时长及 T5/M3 审核仍独立，不继承上游或前一增量成绩。未启动游戏、正式性能采样、修改暂停 Fix1、提交/合并、规格 sync/archive，F 生产代码未被替换。

最终 OpenSpec strict validate 与文档 diff --check 通过，apply 复核 ready、19/26；所编写代码尾随空白检查无匹配。4486 项当前代码重新核对清单无字节变化，新增依赖/必要源码未被 ignore，Git index 未改动。文档审计覆盖 49 篇/722 条本地链接，仍有其他位置 17 项既有缺失材料/锚点问题，未报告本轮 T5 链接错误；全库审计不记通过，历史材料未为消错而改写。

## 2026-10-22 顺序、身份借用与失败保证定向核对

本轮用户仅授权上述三项核对及更新本页，明确暂不删除旧文件或启动游戏。审阅对象为 `E:/Codex/Worktrees/refactor-ecs-memory/Symocraft` 的正式实现，不是 F 的旧 ECS 或独立实验原型；F 继续维护唯一验证正文。本轮未修改生产/根测试源码、设计、规格或任务勾选，未回写 F 生产代码。依据为 [ECS A02-A06、A13-A19](specs/ecs/spec.md)、[A22/A23](specs/module-boundaries/spec.md)、[物理步进顺序](design.md#物理步进顺序)及现役 [Session 部分推进失败](../../../specs/simulation/spec.md#requirement-rejection-and-partial-advance-failure)。

### 物理步进顺序

源码核对 `game/modules/simulation/src/physics_adapter.cpp:17`、`player.cpp:17`、`session.cpp:103` 及 ECS `detail/registry.inl:162`：

- 玩家装配明确赋 `PhysicsStepOrder = 0`，其他刚体的唯一键由装配方负责，不是 Registry 自动签发的身份。匹配集合仍为 Transform/RigidBody/HitBox，不以顺序组件过滤，因此缺键不会静默漏体。
- EachOrdered 在保护内收集自有 uint64 键，以 std::less<> 排序并检查等价键，随后才开始本步业务 visitor；不使用 native ID、地址或池次序补序。每体先复制小值并成功求解，再提交本体的位置/速度/加速度/接地和增加提交数，求解后到该体提交之间没有新的存储查询。
- `session_tests.cpp` 的 ExplicitBodyOrder 四种模式本轮在两配置均执行：缺键、重复键、坏体键在玩家前、坏体键在玩家后；刻意移除/重挂玩家 RigidBody 改变驱动池排列，提交前缀仍分别为 0/0/0/1。PartialFailure 另确认坏体不提交、玩家前序提交保留及 Session Failed 后拒绝继续；原 D12 同生产双装配对照也在同一运行中通过，阈值未改。

**边界：** 缺注册/缺键、平局或键准备失败保证的是“本步物理实体提交为零”，不是整个 Advance 零变化。角色规则、冷却、已消费预算及此前子步可以已经改变；Session 在实际推进阶段报告 AdvanceFailed，不改成预检 Rejected。物理暂存 OOM 的保护机制本轮通过正式 Registry 的有序/嵌套有序故障用例验证，未另对 Session 内该分配点注入；不把分层依据称为该组合路径的直接实测。零物理步也不强行执行键校验或补步。

### 实体身份与借用

源码核对 `game/modules/ecs/include/symocraft/ecs/entity.h`、`registry.h:63`、`src/registry.cpp:11` 及全部活动 simulation/app 的组件读取与结构入口：

- EntityId 按值持有私有批次及 native 值，仅公开默认空值、相等和哈希；不延长 owner 寿命，相等/哈希不读取 Registry。唯一批次源串行、单调且永久耗尽，不按对象地址区分实例，不回收失败构造或清理中已消耗的批次。
- Clear 先取得新批次，再原生清理，成功后才提交批次；保留首次名称、注册及 fresh 高水位。固定 EnTT 的 clear 保留实体回收槽，不能把高水位清零。fresh 成功才推进，容量边界和 4094 末代销毁均在不可逆动作前拒绝；4093 正常销毁后仍能复用为 4094。
- 本轮重跑跨实例/销毁复用/Clear 旧 ID、哈希、重复空 Clear、注册/名称保留、高水位及容量/末代夹具。独立身份耗尽子进程确认准备拒绝后当前 ID、值、计数与名称保留；32 逻辑槽/600 步固定种子模型继续通过。极端夹具不是百万活实体或自然循环到身份耗尽的成绩。
- 结构工作开始即结束此前借用，即使失败保留旧逻辑值；纯预检拒绝和实际未触碰结构的无变化路径不虚构失效。查询回调借用只限当前回调。玩家工厂从 Camera 读取的是自有 Transform 副本，再创建/挂载玩家；其余审阅到的装配均在结构工作完成后重新获取引用，Session 结果和诊断只交接自有值，未发现跨结构保存组件借用的活动调用。
- 所有结构入口先检查 Failed/上下文，再检查 ID/类型/参数，包括预计无变化的重复注册、GetOrAdd 已有及 Remove 缺失。QueryGuard 覆盖筛选、有序准备、全部回调和嵌套；本轮核对内层 Stop/异常不会解除外层保护，全部退出后可以重新执行结构操作。

**边界：** 普通 T&、隐藏别名、带借用成员的所谓“值键”或构造参数不能被模板约束全面证明自有；调用方仍须履行寿命/只读要求。测试不解引用失效借用来证明检查有效，也未验证并发访问、DLL/插件或跨进程身份。不同 Registry 仍要求外部串行，guard 不是锁。

### 失败保证及本轮复验

注册先准备自有名称、元数据容量和 native 空池，再发布注册；Create/Emplace 的异常出口按固定候选实际回滚机制检查是否仍可确认一致。新增失败不发布目标组件，允许保留容量或 in-place tombstone，不要求恢复地址。Remove/Destroy/Clear 若发生无法确认一致性的结构异常则置 Failed 并重抛原异常；不能概括成“任意异常均可恢复”或“一律 Failed”。普通 visitor/键异常只退出对应查询保护，不回滚之前的值更新，也不自动把健康 Registry 判为损坏。

Failed 的 GetStatus 直接返回最小自有摘要，不扫描 native；普通读取、IsEntityValid、注册及 Clear 拒绝，不提供原地修复。Session 诊断先看 Registry 健康状态，无法安全读取时 motion 缺失、不补零；合法受控的一致性失败注入验证了这条交接策略，不是任意内存损坏的恢复实测。Session 可因业务失败进入 Failed 而 Registry 仍健康；工厂多步装配失败也不是共享 Registry 事务，既有用例销毁整份专属装配范围而非在半装配上自动重试。

本轮先逐项重算前节 4486 项源码闭包，摘要仍为 `ab35f498178d211bb3bda97225764da39dc67ff42a43fadc342ca614af74fc3c`；结束时再次匹配。使用原正式 `t5-owner-Debug-008` / `t5-owner-Release-008`，四个受影响 target 的构建均为 no work，不是新的全量构建；四个测试程序前后摘要一致。实际工具链仍为 MSVC 19.51.36260.0/tools 14.51.36231、SDK 10.0.22621.0、x64/C++20、EnTT 4.0.0、ENTT_NO_ETO。生成 Debug C++ 命令 140/140 均设 level 0，配置程序两配置也实报 iterator=0；DBG01 的检查损失和历史 level 2 终止保持，不改判为已修复。

| 本轮直接执行范围 | Debug / Release 结果及限制 |
| --- | --- |
| 正式库契约、独立身份耗尽、配置、Session | 各 4/4 次运行通过；契约内含普通对象、构造抛错、受控 Failed、查询和固定种子模型；Session 内含顺序、前序提交、预检/装配失败、诊断及 D12，不把它们再逐项虚增为独立测试数量 |
| 实际分配/清理场景 | 各 17/17 次运行通过，其中 13 个不同 new/aligned-new 故障点均实际命中一次并捕获原始 bad_alloc；另 4 类清理实测零分配/零命中，不计 OOM 恢复。代表范围及序号与前节一致，包括资源构造内部 #1 与真正 aligned payload #4，不是全面序号扫描 |
| 公开 API 编译负例 | 各 29/29 个因预定项目断言或指定源码诊断拒绝，覆盖 owner/ID 逃生、const、模板及回调误用；是本轮新对象编译，不只检查旧日志 |

合计每配置 **21/21 次 CPU 运行、29/29 个预期编译拒绝**，不是重跑前节 37 项 CTest 或主 CTest 全量。运行命令沿用现有 target/用例，直接执行以不覆盖旧 CTest 日志；本轮脚手架与结果只留 F 的普通 `out/t5-directed-review-20261022-002` 及其驱动，无验证附件或原始日志正文。首个编排在宏拼法检查处误将 CMake 的 `-D` 当成缺少 `/D`，未开始构建/测试；仅修正临时驱动后另存 002 复跑，不记为产品失败或通过。

**结论：** 在上述源码审阅和有界 CPU 复验范围内，三项未发现与冻结契约冲突的阻断性偏差，无需本轮修改生产实现。仍不保证持续/全进程 OOM、违反 noexcept/禁止重入约束的组件、隐藏借用误用或未知内存破坏可恢复；按值 Clear 身份准备本身不分配，不移用旧堆批次实验的 OOM 成绩。五个旧文件前后字节及 E 的 HEAD/index 摘要不变，未删文件、启动游戏或执行 GPU/安装/性能采样。任务保持 **19/26**，不登记人工验收、T5/M3 通过、Git 写入或规格 sync/archive。

## 2026-10-22 S05定向退役与删除后复验

用户本轮明确批准删除[此前列明的五个旧 ECS 私有文件](#s05-具名候选与保留)，继续 S05.2/S05.3，并明确暂不启动游戏、不提交 Git。本次仅在 E 的 `refactor-ecs-memory/Symocraft` 代码根执行，不删除 F 的旧生产文件或同步替换其生产代码。此前各节“保留/未删除”的结果保持原时点含义。

删除前完整复核 4486 项候选闭包与上一节一致，五个文件字节均匹配上游输入清单及 F 现存原文件，无新的活动消费者。实际只删除该表的 `internal.cpp`、`component_container.h`、`internal.h`、`legacy/RawMemory.h`、`legacy/MemoryHelper.h`，不递归清目录、不扩大清单。删除前的绝对路径/摘要留普通 out 的核对结果，既有输入清单、F 原文件和历史产物不变，未删除唯一恢复来源。

同时移除 `cmake/ModuleBoundaries.cmake` 对旧 internal.cpp 的临时非编译保留例外，回归普通源码归属检查；根边界测试增加真实 ECS owner 正例及旧文件名无 owner 反例，后者只在 out 的合成夹具写最小探针，不恢复原旧实现。活动生产/根单测/工具/脚本扫描未发现旧池、旧整数 ID helper、byte/iterator 或旧 AddComponent 兼容调用。foundation 分配器/日志及其测试、Camera、GL、输入路径、vendor、冻结和个人材料不在删除范围。

最终候选相对删除前为准确五项缺失、两项构建/边界测试文件修改；其余 4479 项输入字节不变，正式 Registry 和消费者业务代码未改。4481 项排序紧凑清单摘要为 `57e9c98fe7ad82466189b8e13a07dd6b1c084ee2bd6fb9fc7d23bb41d7663049`，不沿用删除前身份。

首轮 `t5-retired-Debug-001` 构建完成、定向 CTest 36/37：新增边界正例误用了 foundation INTERFACE 壳，被既有真实 STATIC owner 规则正确拒绝。仅修正本轮夹具提供真实 foundation 源/target，不放宽规则；该轮及失败日志保留，不计最终通过。

删除后使用全新构建目录，不复用删除前缓存或成绩；固定 EnTT/ENTT_NO_ETO、MSVC tools 14.51.36231（实际 cl 19.51.36260.0）、SDK 10.0.22621.0、x64/C++20 和 DBG01 不变。CPU Debug 140/140、游戏 Debug 82/82 条生成 C++ 命令均统一 level 0，配置测试两配置实报 iterator=0；命令数量不是构建 target 数或普通 Debug level 2 恢复证明。

| 本轮最终构建目录（E 的 out 下） | 删除后实际结果 |
| --- | --- |
| `t5-retired-Debug-002` | 选定生产库/消费者/公开头从新目录构建，定向 CTest **37/37**；包含 ECS/身份耗尽/配置、真实分配失败、三个公开头、simulation/physics/Session/D12、app 转接及源码/负向边界 |
| `t5-retired-Release-002` | 同一候选及相同受影响范围重新构建、定向 CTest **37/37**；原 D12 阈值与业务断言未改 |
| `t5-retired-game-Debug-002` | BUILD_TESTING=OFF、D3D12/保留 GL ON、Vulkan/benchmark OFF，从新目录完成游戏目标编译链接；没有执行游戏或 GPU 探针 |

每配置 29 个公开 API 编译负例均因预期原因拒绝，共 58 个；边界用例现为 **115 项，16 接受/99 拒绝**，包括本轮新增正反例。另各直接执行资源构造内部 #1，命中一次并传播原始 bad_alloc；结合 CTest，原 13 个代表性真实分配故障点继续成立，4 类零分配清理仍只记零命中，不计 OOM 恢复。测试未使用已删文件、旧 allocator probe 或另一个 ECS 副本。

实际编译/归属清单不含五个退役文件；三个新构建的 `symocraft_ecs.lib` 成员均仅为正式 registry.cpp 对象。两配置库符号检查未发现 ComponentContainer/SparseSetPool、旧整数 ID helper 或旧类型 counter；游戏实际构建完成，并核对其链接命令消费该唯一 ECS target/library，不引用旧 internal 对象或历史 lib 后备。这些是当前构建/链接依据，不宣称已执行最终游戏。

对上述新游戏构建另执行本地安装至 F 的 `out/t5-retire-ecs-20261022-002/local-install`，不启动安装产物。安装的 EnTT-LICENSE.txt 与 vendor 原始 MIT 文件逐字节一致，SHA256 仍为 `73547f6370ee16ea560665329bb62d4e2d8250f41eaa204b47a6c694ca3286ea`；SOURCES.txt 包含固定完整 commit，五个私有文件未被安装。构建/安装的 SymoCraft.exe 摘要同为 `5348ad90faaf27baca24057799ed8512c1e7d6b59b8a24c3f2e5ea3b79360dc1`。这是本地 Debug 安装/许可完整性检查，包含本机调试运行库，不是可再分发 Release 包、无开发环境部署或无关 CWD 启动验证。

本轮编排、源码身份及结果仅留 F 普通 out 的 `t5-retire-ecs-20261022-001`（预检/首轮失败）、`-002`（最终）及驱动，无 docs 附件、源文件备份镜像或原始日志正文。构建保留既有 AmoBase localtime C4996、对齐夹具 C4324、Release 必抛回调 C4702 警告，无忽略编译错误。结束时 4481 项候选再次匹配上述新摘要，F 原五文件及两 checkout 的 HEAD/index 摘要均未改变。

**S05.2/S05.3 已完成，进度 21/26。** 不据此宣布全局内存审计、S06、T5/M3 通过；独立 benchmark 与剩余组合、Vulkan、游戏/窗口/玩法/故障、人工项目/总时长、当前架构收口仍按后续任务处理。没有启动游戏、正式采样、扩删文件、恢复 Fix1、回写 F 生产代码、Git 暂存/提交/合并或规格 sync/archive；已有历史失败和未覆盖项不改判。

收尾 OpenSpec strict validate 通过，apply 复核 ready、21/26；F 本轮文档及 E 受影响代码的 diff --check 无错误，仅既有换行转换提示。未重跑全库文档审计，不将前述其他位置的既有问题改判为通过。

## 2026-10-22 S06.1构建与独立benchmark

用户本轮授权继续剩余构建和独立 benchmark 检查，明确不启动游戏、不提交 Git。执行代码根仍为 E 的 `refactor-ecs-memory/Symocraft`，结果仅更新 F 的本 change；不回写 F 生产代码。八个配置均使用新的 out 目录，从当前删除后候选构建默认 ALL，不沿用删除前二进制。前后逐项复核 4481 项输入，摘要均为 `57e9c98fe7ad82466189b8e13a07dd6b1c084ee2bd6fb9fc7d23bb41d7663049`，五个退役文件仍不存在，生产及测试源码未改。

工具链保持 MSVC tools 14.51.36231 / cl 19.51.36260.0、Windows SDK 10.0.22621.0、x64/C++20、固定 EnTT 4.0.0 和 ENTT_NO_ETO。核对实际生成 C++ 命令：CPU 每配置 140 条、双现代加 GL 游戏 83 条、独立 benchmark 38 条、根入口 runner-only 33 条；全部按配置使用 /MDd 或 /MD，各 Debug C++ 命令统一 `_ITERATOR_DEBUG_LEVEL=0`。CPU 配置测试两配置实际报告 iterator=0。DBG01 的迭代器检查损失及历史 level 2 OOM 终止不改判。

| E/out 下新目录，Debug 与 Release 分别建立 | 本轮结果 |
| --- | --- |
| `t5-s06-cpu-{Debug,Release}-002` | CPU-only、BUILD_TESTING=ON，全目标构建成功；受影响 CTest 各 **83/83**，包含 35 个独立公开头消费者、真实 ECS 多 TU、契约/分配故障、simulation/physics/Session/D12、app 纯 CPU 转接、renderer CPU 门面及三类架构边界 |
| `t5-s06-game-{Debug,Release}-002` | BUILD_TESTING=OFF、benchmark OFF，D3D12/Vulkan/保留 GL 同时 ON；两配置完成全目标构建及最终游戏链接，未执行产物，无测试源码或测试 target 依赖 |
| `t5-s06-benchmark-{Debug,Release}-002` | 直接以 tools/benchmark 为独立 CMake 源根，全目标构建成功，全部 CTest 各 **9/9**；包括发布、协议、进程/执行器、现代/Vulkan模拟及 Python/两个 PowerShell 兼容入口 |
| `t5-s06-root-runner-{Debug,Release}-002` | 根入口仅 benchmark ON，game/CPU modules/三后端及测试 OFF；独立工具两配置全目标构建成功，无测试源码/夹具 target，无游戏模块或 EnTT target |

每个 CPU 配置重新执行 29 个预期 API 编译拒绝；源码归属、负向及 renderer 边界均通过，负向夹具仍为 **115 项（16 接受、99 拒绝）**。Registry 多 TU 消费者与 Session 链接正式 ECS 库，不构建测试专用实现；实际诊断 include 授权仅见 registry_contract_tests 和 session_tests。CPU 依赖图/编译输入无 SDL、原生游戏/后端 target 或 Vulkan SDK，不将 cache 中被 CPU-only 覆盖的默认开关当成实际依赖。

增量检查仅临时改变两个头的 mtime，字节不改且时间均恢复。两配置的私有 diagnostics 头各只触发 **3 个 C++ 对象**：正式 registry.cpp 及两个指定测试，不传播至 simulation/app；公开 registry.h 各触发 **29 个 C++ 对象**，正确传播至 ECS、simulation、app CPU 转接和相关消费者。前后 settled 构建均无 C++ 重编译；增量后再次直接运行正式 Registry 契约和 Session，各配置 2/2 通过。未将链接/资源同步步骤虚计为编译对象或性能成绩。

双现代游戏的真实 source manifest 和最终链接包含 ECS、application、D3D12、Vulkan 及保留 GL，消费唯一正式 ECS target。D3D12 使用 Windows SDK 22621 的 DXC，Vulkan 使用已安装的 Vulkan SDK 1.4.363.0 头与其 DXC；每配置实际生成 4 个 DXIL 和 4 个 SPIR-V，检查文件魔数及生成的源码/编译器/配置摘要。Debug 游戏 SHA256 为 `cb3820f143ad432a737236948a55e15f8850fd4b80b7d1171828579825498da9`，Release 为 `5937af42091ef34a70516ec9b2c3453865980a93e51cffa95c4e2a11e50865f7`。这是编译/链接与 shader 构建依据，不是 GPU 初始化、画面、单/双后端切换或玩法验证。

独立 benchmark 及根 runner-only 的实际 target/编译输入不含游戏、EnTT、SDL 或 Vulkan SDK；独立 benchmark 两配置的 PE 导入也无 D3D12/DXGI/Vulkan/SDL/OpenGL DLL。工具仍依赖 Windows/MSVC、Win32 UI 与匹配 CRT，不宣称无需 Windows SDK 或跨平台。9 项测试只启动受控 fixture/模拟进程，未启动真实 SymoCraft、benchmark GUI、GPU 探针或正式采样；名称含 modern/vulkan 的通过不算真实渲染成绩。各工具 settled 构建无工作，游戏 settled 仅执行既有资源同步。

首轮临时驱动误用了 CTest 不支持的非捕获正则组，测试清单输出正则诊断后 JSON 解析停止；该轮 CPU Debug 构建成功，但没有测试通过结论。仅修正驱动正则，以全新 002 目录重跑；首轮目录/日志保留，不记产品失败或抹去记录。最终构建保留既有 C4996、对齐夹具 C4324、Release 必抛路径 C4702，以及 Vulkan selection 遮蔽 C4458、benchmark 发布测试忽略 nodiscard C4834 警告，未改无关代码或忽略错误。

编排和日志只留 F 普通 `out/t5-s06-builds-20261022-001` / `-002` 及驱动，无 docs 附件或原始日志正文。结束时 F/E 的 HEAD/index 摘要均与开始相同。CPU 只运行所列受影响 83 项，不是全量 107 项；foundation.files 未运行，既有 Win32 1175 风险保留。未进行游戏/窗口/设备、安装后运行/无关 CWD、人工体验或性能复验，未恢复 Fix1、扩删、Git 暂存/提交/合并或 spec sync/archive。

**S06.1 已完成，进度 22/26；S06.2-S06.5 保持未完成。** 上述构建和模拟测试不构成 T5 或 M3 整体验收，也不替代此前要求单独确认的人工项目和总时长。

收尾 strict validate 通过，apply 复核 S06.1 done、22/26；本轮两个文档 diff --check 无错误，仅换行转换提示。未重跑全库文档审计或重置既有未关闭问题。

## 2026-10-25 S06.2真实集成与人工反馈

用户确认按此前列出的范围继续 S06.2 自动检查，并明确“双后端的人工复查通过”。此前已提交自动准备/执行、三路径人工项目及预计总计 35-50 分钟，其中人工约 10-12 分钟；本轮只执行获准自动部分，不重新要求用户操作，不把估计当实际人工时长。正式代码根仍为 E 的 T5 worktree，文档只维护于 F，无其他工作树合并或 F 生产代码替换。

前后重算 4481 项删除后源码闭包，与 [S06.1](#2026-10-22-s061构建与独立benchmark) 的完整摘要逐项一致，五个退役文件仍缺失。原 `t5-s06-game-{Debug,Release}-002` settled 构建未改变其程序摘要，实际运行的双现代加 GL 游戏及安装后的 exe 均匹配该节完整 SHA256；工具链、固定 EnTT/配置与 DBG01 不变。新建 `E/out/t5-s062-d3d12-only-Release-001`、`t5-s062-vulkan-only-Release-001`，分别仅启用一种现代后端、不含 GL/benchmark/测试，全目标构建成功；新建 `t5-s062-native-probes-Debug-001`，只构建真实 application_lifecycle_probe 及依赖，不执行历史全套探针。其生成 Debug C++ 命令统一 level 0，正式库与探针布局/配置相同，不使用测试 ECS 副本。

| 本轮自动项目 | 实际结果 |
| --- | --- |
| 双现代加 GL 启动 | Debug/Release 各显式选择 D3D12、Vulkan、GL，合计 **6/6**；每次 60 帧，完整 441 chunk 固定种子回归场景及预置编辑，均到达 ready、完成循环并正常 Shutdown |
| 单现代启动与拒绝 | D3D12-only/Vulkan-only Release 各 **1/1**；各自另请求两个未编译后端，**4/4** 以 exit=3、SelectBackend/Unsupported 在 ready 前拒绝，无静默回退 |
| 世界/编辑 CLI | Debug/Release 各两个独立无窗口进程，**4/4**；结构化摘要在同配置跨进程及两配置之间一致，均含 441 chunk，不进入图形启动路径 |
| 资源预检及安装运行 | 构建目录及真实本地安装包的 Debug/Release × 三路径预检 **12/12**；安装包从含空格路径、无关 CWD 再运行三路径，**6/6**，每次 60 帧及正常退出 |
| 临时包拒绝 | 仅复制本轮新包且不带 atlas，原包及源码不删改；Debug/Release × 三路径 **6/6** 以 exit=2 指出 texture_atlas.png；非法参数 **2/2** 以 exit=64 输出 Usage |
| 实际数据交接 | 双现代加 GL Release 分别执行 walk/edit，共 **6/6**；正常完成、valid_run=true、无 invalid_reasons，实际移动/编辑及渲染资源交接均有下述有界依据 |
| 装配/故障清理 | Debug 的正式 Application 探针按三路径分别执行，**3/3**；每路径十个取得 owner 后的装配故障点 0-9 均恰好命中一次，部分清理及重复 Shutdown、调用方字符串寿命、Window 次级清理异常后继续释放 Runtime、原始 code=32 保留均通过，export_failures 均为 0 |

实际 GPU 均为 NVIDIA GeForce RTX 5070 Ti。D3D12 报告 `LUID=00000000:00014850 vendor=0x10de device=0x2c05`、驱动 `32.0.16.1664`、FL12_0/SM6_0；Vulkan 报告 `vendor=10de;device=2c05;uuid=d43c9988ca0711129c89be697511f1d0`、驱动 NVIDIA:616.64、API 1.4.351；GL 报告 NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2、4.6.0 NVIDIA 616.64。现代两路实际 4x MSAA、Fifo、validation=1，GL 实际 4x；已输出的 Vulkan 清理摘要均为 warnings/errors=0、native_retired=true。记录的显示条件是本机实际窗口/后端设置，不另宣称线缆、HDR/VRR、其他 GPU 或显示链路覆盖。

walk/edit 是每次 **2 秒、无预热、1280×720、VSync 请求开启、allow-unfocused** 的有限功能诊断，通过现有场景入口执行；不是正式性能采样、预算或 T3 1920×1080 基准复测，不用其计时/帧数推定性能提升。三条 walk 均从 x=-20.5 实际前进约 8.8，y/z 保持，世界摘要不变；末位置因真实时钟推进不同，不要求跨后端相等，也不替代固定 CPU D12。三条 edit 均执行 7 次预定写入，按实际 frames.csv 时间和 0.25 秒调度复算，报告与逐帧编辑总数一致，摘要均由 `fnv1a64:bddd435ea737fa62` 变为 `fnv1a64:88fa563dbb4ac479`；现代两路各有 4,934,664 bytes 资源上传、21 次调用。六幅末帧 PNG 尺寸与像素范围检查通过，另直接查看三路径 edit 图像确认实际地形/方块/选中框非空，不把静态图像当输入人工验收。

真实安装目的地为本轮普通 out 下的 `Debug installed package with spaces` / `Release installed package with spaces`，均从新建的 `unrelated working directory` 执行。对全部安装文件按相对路径排序，以 path/bytes/sha256 行的紧凑 JSON 计算闭包；各包 16 项资源均逐字节匹配相应构建，4 DXIL/4 SPIR-V 魔数和 build.json 二进制摘要再次匹配。

| 安装身份 | Debug | Release |
| --- | --- | --- |
| 文件数及完整闭包 SHA256 | 50；`a4ae2cab6d50d34d6755e0fb8e46bebceb24dc6a4bb67fde8b2e3124ac6b3265` | 49；`42d311203b3e63d82ac08810f4ac86fb7309af2550a12aca2f004a2fbabeb4e2` |
| 16 项资源闭包 SHA256 | `62124c54a1d32d9882fb54531827e3393087ef091c88b6b44f8356a977842ebf` | `f3c69f30e4568cc8795f7c37aa55c57c423f0f2467a17e8a672743fe29e80029` |

两包 EnTT 原始 MIT 字节/摘要及完整 commit 来源通知均通过；配套运行库分别为 9 个 Debug DLL、8 个 Release DLL，包含各自实际 MSVC atomic_wait/runtime 文件。此处是本机安装与无关 CWD 运行，不是无开发环境目标机、完整再分发或干净系统部署认证。原始输入、旧构建和冻结材料均保留。

首轮三个补构建和第一次 D3D12 60 帧运行成功，随后临时编排因未归一 CRLF 而停止解析世界块；仅修正驱动，以新 002 日志目录继续，三个本轮新构建复用其已核对的无变化结果，不冒称再次全量编译。后续只读复核初稿曾误将恰好 2 秒的结束边界也计入编辑；源码在 Advance 前判完成，实际最后帧早于 2 秒，改以逐帧时间复算得到上述 7 次，未重跑游戏、改预期业务代码或放宽契约。两项均为临时脚手架问题，首轮日志/产物保留，不记产品失败或伪造通过。

**人工反馈单独登记：** 用户于本轮明确确认 D3D12、Vulkan 双后端人工复查通过，按此前提供的 T5 Release 启动候选记录；不补造逐项操作轨迹或实际时长，也不是 T5/M3 整体验收批准。GL 的简短人工复查尚无用户结论，自动 GL 启动/图像不能代填，因此 S06.3 保持未完成。

**S06.2 已完成，进度 23/26。** 结果/驱动、临时包及原始诊断只留 F 普通 `out/t5-s062-20261025-001` / `-002`，不新增 docs 附件或长期冻结包。结束时源码闭包及 F/E HEAD/index 均不变，无残留 SymoCraft/application_lifecycle_probe 进程；没有生产/测试源码修改、正式性能预算判定、长时或第二机验证、Fix1 恢复、额外删除、Git 暂存/提交/合并或 spec sync/archive。历史 level 2 OOM、Win32 1175、既有延期及未覆盖项不因本次未复现而改判已修复；受控故障不证明真实设备丢失或任意全进程 OOM 可恢复。S06.3-S06.5 和 T5/M3 整体结论继续独立。

收尾 strict validate 通过，apply 复核 S06.2 done、23/26，S06.3 未勾选；本轮两文档 diff --check 无错误，仅既有换行转换提示。未重跑全库文档审计。

## 2026-10-25 S06.3人工复查收口

用户继前述 D3D12/Vulkan 总体通过后，本轮明确确认“GL 的人工复查也通过了”，并授权更新 S06.3、继续 S06.4/S06.5 文档收口与验收核对。按此前提出的 T5 Release 三路径项目登记，三路径人工反馈均已取得，S06.3 完成，进度 24/26。不补填未提供的逐项轨迹、精确用时、长时或正式性能成绩；此前估计仅为执行前范围，不充当实测时长。本轮不重新启动游戏，不登记 T5/M3 整体验收批准，也不因文档收口取得 Git、代码同步或 spec sync/archive 授权。

## 2026-10-25 S06.4实现与依据对账

本轮重新读取全部九份现役 specs/scenarios、T5 两份 delta 与现有设计/任务，对照 E 的 Registry 公开头/模板/实现、配置及目标、physics adapter、玩家/Session/app 迁移与已记录结果。更新 [ECS](../../../../architecture/ecs.md)、[simulation](../../../../architecture/simulation.md)、[app](../../../../architecture/app.md)、[foundation](../../../../architecture/foundation.md)、[工程](../../../../architecture/engineering.md)和[架构入口](../../../../architecture/Overview.md)，不新建学习版/第二计划。机制说明明确适用于 E 的 T5 实现，F 主工作区仍为 T4/旧 ECS；旧 F 机制及 T4 引用锚点保留，不把相对源码链接指向的旧实现当新门面。仅修正受影响工程页内的 README 失效锚点及已交付 Vulkan 的旧状态，不扩修无关文档。

下表汇总稳定 T5 身份的交付依据；“已实现/有界通过”仅限对应版本、配置和已执行路径，不是本轮重跑或任意类型/故障的证明。早期 4486 项候选与删除后 4481 项候选分开，生产代码及相关测试在 S05 之后未再变化；最近依据优先采用删除后 S05/S06，而不把独立 S01 原型或旧 ECS 绿灯当正式实现成绩。

| T5 身份 | 实际实现、必要依据与限制 |
| --- | --- |
| A01 | 稳定非复制/移动 Registry、唯一正式库、RAII、无 native/byte/view/Free 逃生；真实公开头/多 TU、复制移动负例及资源展开清理，见[正式实现](#实现及保证范围)、[删除后构建](#2026-10-22-s061构建与独立benchmark) |
| A02-A06 | 私有批次/代数、跨 owner/Clear/复用、哈希与默认空、严格销毁、容量/末代拒绝及 Clear 保注册/高水位；[正式边界用例](#当前候选验证)、[身份定向核对](#实体身份与借用)及删除后契约继续成立。极端值用合法窄适配，不称自然创建百万实体；按值 Clear 准备不分配，只测耗尽拒绝，不借旧堆批次 OOM 成绩 |
| A07-A12 | 每实例注册/首次自有名、260 类型、跨 TU token/可控碰撞、空对象/128 对齐资源、不可默认构造、const 和缺失/非法读取/计数；正常与预期编译拒绝见[当前候选验证](#当前候选验证)，删除后各配置 29 API 负例再执行，不靠旧清零池 |
| A13-A16 | 实际结构工作结束旧借用、纯预检/no-op 区分、构造输入先自有化、同步过滤/空实体/嵌套 Stop/异常、无变化结构入口及多错优先级；[定向核对](#2026-10-22-顺序身份借用与失败保证定向核对)与正式契约。不解引用失效对象，不承诺隐藏别名/借用成员自动检测 |
| A17 | 完整自有键、只读 selector、std::less<>、业务 visitor 前平局/准备失败零回调、无 native 补序；[物理顺序](#物理步进顺序)覆盖缺键/平局/反向装配/驱动池重排与前序提交，不把零本步提交当整次 Advance 回滚 |
| A18-A20 | 正常/拒绝/构造异常/可确认回滚与 Failed 分类、无扫描自有摘要、外部串行与 RAII；[失败核对](#失败保证及本轮复验)、删除后真实 new/aligned-new 各 13 个代表故障点及 4 类零分配清理。已批准 DBG01 的 level 0 是正式选择，历史 level 2 terminate 保持失败；不是持续 OOM、并发或未知损坏恢复 |
| A21 | 固定 EnTT/ENTT_NO_ETO、统一 ABI、真实生产 target/公开消费者/两指定诊断测试、CPU-only/测试关闭/独立工具与许可；[S06.1](#2026-10-22-s061构建与独立benchmark) 两配置各 83/83、工具各 9/9、115 边界夹具与私有/公开增量；[S06.2](#2026-10-25-s062真实集成与人工反馈) 实际安装/源包身份。CTest 不是全量 107 项 |
| A22 | T4 上游输入明确同步后，仅迁移必要 caller；业务 PhysicsStepOrder、逐体成功提交及两层 Failed 诊断落实。原 D12 同一生产 CPU 每 Advance/World 提交与固定绝对阈值保持，真实图形/安装及三路径人工另见 S06.2/S06.3，不以实时 GPU dt 互作确定性 oracle |
| A23 | 固定种子 32 逻辑槽/600 步独立模型、真实故障确认命中，原始异常/逻辑值与资源平衡分开；[有界成本](#有界成本观察与后续门槛)记录 1025 实体、普通/有序查询等同负载观察，不预设提升。累计申请不是峰值，暂存/进程峰值和全序号扫描未做，不把旧坏池当正确性基准 |
| A24 | [S05 准确五文件退役及删除后复验](#2026-10-22-s05定向退役与删除后复验)具名授权；唯一 ECS 链接/符号/安装再核。foundation allocator/logger、Camera 兼容、真实 input_state 和活动 GL 按实际消费者保留；无双 ECS 或无用活动后备，不宣布全局内存审计完成 |
| A25-A26 | [S06.1](#2026-10-22-s061构建与独立benchmark)及[S06.2](#2026-10-25-s062真实集成与人工反馈)的当前候选构建、真实单/双现代与 GL、故障清理/部署；[S06.3](#2026-10-25-s063人工复查收口)三路径用户反馈。人工范围已先报，未新增正式性能、长时或第二机；技术交付、预算与节点批准不混同 |

S06.4 的文档及 A01-A26 结果/限制整理完成。所有结果仍在本页原位置及上述链接，不复制原始日志、截图、附件或冻结包，不修改现役 specs、归档 T3/T4、已选预算及旧失败；完整候选身份在本轮结束时再次只读复核。

## 2026-10-25 S06.5验收核对与待审结论

### M3共同标准

对照 [scope 的五类共同标准](../../../../scope/project-scope.md#m3-的共同完成标准)，本次只形成 T5 受影响范围的证据对应，不把局部回归提升为全项目所有类型/操作均无缺陷。

| 标准 | 当前 T5 对应与判读 |
| --- | --- |
| 软边界 | ECS 公共头/detail 与 src/diagnostics 分层，simulation/app 只作必要适配，测试位于根 test；原生 EnTT include 仅属 ECS。已授权旧五文件只在 E 删除，F 旧文件保留有明确源码根含义，不是 E 活动后备 |
| 硬边界 | 真实 symocraft_ecs 唯一编译 owner、受控 symocraft_entt 公开模板依赖，simulation/physics 与 application 保原 target/职责；实际图、源码清单、负向及链接/安装依据成立，不靠全 vendor/global include 隐藏依赖 |
| 生产/测试边界 | 正式库供契约/多 TU/故障/Session/原生 Application 探针共同链接，窄白盒仅指定两 target，BUILD_TESTING=OFF 无测试/fixture 依赖；CPU-only 与根/独立 benchmark 无图形或新 ECS 依赖泄漏 |
| 接口边界 | Registry/EntityId/查询明确职责、生命周期、类型/const、错误/Failed、复制移动、串行与短借用；真实对象/const/身份/编译负例与只读诊断有依据，未开放 native 状态或另建通用反射/事务 |
| 实现边界 | 非空壳门面：对象/身份/注册/查询/失败均有实际实现；业务排序由装配方显式表达并保逐体提交，app 不靠全局查询 native 存储；默认 D3D12、显式 Vulkan 与保留 GL 真实路径分开验证 |

### 上游与继承边界

已核对 [T3 完成批准](../2026-10-19-m3-t3-renderer/verification.md#2026-10-17-r4及t3完成确认)、[T4 完成批准](../2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)及全局[节点表](../../../../verification/VERIFICATION.md#关键节点)。T3 双现代接管于 2026-10-17 获批准，T4 于 2026-10-19 明确保留风险整体通过；其后必要代码已同步 F，T5 于 10 月 20 日明确取得该上游输入、冻结与 E 实施授权。现有 T4 Session/physics/app、Q01-Q04/D01-D13 及 D12 不重选，T3/T4 节点不重新验收。

复用的是既有交接契约、职责和明确节点决定；替换后的 ECS/调用方、ABI、受影响构建及真实集成已经按本页各轮重新取证，不将旧 T3/T4 程序、预算、人工或故障矩阵冒称当前候选成绩。当前源码/双现代游戏/安装包完整身份仍为 S06.1/S06.2 已列候选，本轮仅只读复核，不新增运行成绩。T3 最后局部优化未补完整回归、T4 新工具链未重跑全部旧原生故障的风险，继续随上游保留。

scope 及节点导航仍保留 T0/T1 独立整体验收待确认/未集中核对的文字，现役 world 同样明确不补造 T1 批准；本轮未发现可据以新增登记的明确整体决定。不能因 T2/T3/T4 已批准或本次 T5 技术完成而反推前序通过，也不凭集中表无条目撤销可能存在的历史用户决定。M3 整体审核时仍须用户明确这些前序批准口径；本轮不回填 scope 人工 checkbox 或改写历史报告。

### 保留事项

- [M3-I1/I2](../../../../issue/issues.md) 的 Win32 1175/诊断完整性继续 Open，沿既有“不阻塞”节点决定，不重解释上游批准；[Fix1](../../m3-t2-publication-fix1/proposal.md) 继续暂停，M3-I3 旧日志导航未核范围同样不关闭。本次导出成功不证明根因修复，历史失败/缺码/失焦/尖峰字节不改判。
- T5 正式选择 DBG01 level 0，接受迭代器检查损失；旧 level 2 terminate、未覆盖故障序号/类型、持续/系统 OOM、隐藏别名/违约生命周期及并发/DLL/跨进程不获新保证。保留有消费者的 foundation allocator/logger、Camera 和活动输入/GL，不称已完成全局内存或所有旧 API 审计。
- S06.1 是受影响 CPU 83 项，不是全量 107 项；foundation.files 及其他未选测试不标通过。S04.5 成本为有界观察，峰值/保留容量未测；S06.2 的两秒功能诊断不是 T5 正式性能采样，不能据此给当前候选套用旧 P95/P99/内存预算或宣布无回归。未新跑历史全套后端故障/增量矩阵，也未新增调试器或 CLion 实际工具链选择验证。
- 三路径人工反馈是用户确认的受影响短复查，不补成逐项原 14 项/15 分钟、长时、罕见时序或无资源增长证明。本机含空格安装与无关 CWD 不是真正干净机/M6、任意 Unicode/显示链路或独立整机部署认证，CMake 最低 3.22 的实际旧版本兼容性也未新增实测。
- 树叶剔除/旧视角突变等已延期行为不由 T5 自动修复。GL 仍隔离保留、最终退役另立节点；正式 physics 模块搬迁仍在 T5 审计后、NPC 复用前另行授权，当前只有 simulation 内独立核心。指定中端 Y9000P/低端 GTX1650 整机性能及用户结论仍是下一玩法实施前门槛，不由本机或 T5 收口解除；计算服务/NPC/AI/村落未因本轮获准实施。
- 文档全库既有缺失材料/锚点问题继续具名保留，不为消错恢复或改写冻结材料；受影响文档的当前检查结果单列于本轮收尾，不以 OpenSpec validate 当运行或验收通过。

### 提交审核

**实施方结论：E 的 ECS 技术替换、获准退役、受影响验证及 S06 文档/验收核对已交付，可提交 T5 整体验收审核。** 本次检查未发现需重开冻结设计的新冲突；A01-A26 的证据和限制已分层对账，不新增未确认测试或临时验收预算。

**T5 整体验收尚待用户对本汇总明确批准；M3 整体也不在此自动通过。** M3 审核需结合上述共同标准、T2/T3/T4 已批准事实、T0/T1 待集中确认口径及继续保留事项，由用户作出明确整体决定。当前只提交材料和建议，不改全局节点批准表或 scope 验收勾选；26/26/all_done 只表示本 change 的实施/整理/提审任务完成，不等于这两个整体决定。

F 主工作区生产代码仍未取得 T5。后续代码同步、Git 暂存/提交/推送/合并、主规格 sync 和 change archive 都需独立明确授权，不由本轮人工通过或文档收口推定；也不自动恢复 Fix1、删除 GL/旧材料或启动下一玩法。

收尾只读复核：E 的 4481 项源码、两配置游戏及 50/49 文件安装包再次匹配 S06.2 完整身份；F 核对的旧 ECS 源码、现役九份规格、本次读取的 T3/T4 归档 Markdown/YAML 及两 checkout HEAD/index 前后不变。可维护文档审计为 50 篇/787 条本地链接，本轮无新增错误，修正工程页一个旧锚点后既有 17 项降为 16 项；余项涉及压缩前历史路径/迁移清单及其他入口锚点，全库审计仍 exit=1，不宣称全库通过。受影响九份文档 diff --check 无错误，仅换行提示；T5 strict validate 通过，apply 为 26/26/all_done。只读编排留普通 `out/t5-closeout-20261025-002`，没有修改冻结证据、个人正文/配置或生成标签索引。本轮未重新构建、执行 CPU/SDL/GPU、启动游戏或采样，技术交付提审与真实节点批准继续分开。

## 2026-10-25 T5与M3整体完成批准

用户在本聊天明确决定：“批准 T5 整体完成，保留已列风险并更新验收记录，暂不提交 Git；M3 整体验收也通过，将其他机器的benchmark改到接入现代性能分析工具后”。据此分别登记 **T5 整体完成批准**和 **M3 整体验收通过**，不是从 26/26、validate 或技术交付自动推定。该决定覆盖前节尚待整体批准的状态；T0/T1 独立历史批准未集中核对的事实保留，不再作为当前 M3 整体决定待确认的阻塞，也不倒填其独立批准日期或未测项目。

T5 批准对象为 E 的 `refactor-ecs-memory` 实际生产替换、获准五文件退役及 S06 已列交付，不是 F 的旧 ECS。只读核对仍对应 S06.1/S06.2 的 4481 项源码闭包 SHA256 `57e9c98fe7ad82466189b8e13a07dd6b1c084ee2bd6fb9fc7d23bb41d7663049`、两配置游戏及 50/49 文件安装包；完整游戏/包身份及三路径人工反馈沿用前节，不新增运行。M3 批准按已确认本机 RTX 5070 Ti 功能/兼容范围及上述共同标准、T2/T3/T4 既有决定和 T5 交付记账，未扩大为其他机器、M4 性能、M5 长时或 M6 独立发布通过。

### 风险与后续执行边界

前节[保留事项](#保留事项)继续有效，**仅其中“中低端性能仍是下一玩法实施前门槛”的时间安排被本次明确新决定替代**，不以整体批准关闭问题或增加保证：

- M3-I1/I2 的 1175 与诊断完整性仍 Open，Fix1 继续暂停；M3-I3 旧导航未核、T3 局部优化未补完整回归、T4 新编译器未重跑全部旧原生故障矩阵均保留，历史失败及原字节不改。
- DBG01 level 0 及其检查损失继续具名接受，旧 level 2 terminate 不是修复成功；代表分配故障、部分提交/Failed、身份/短借用保证仍限已核机制，不扩为全序号、持续/系统 OOM、隐藏别名、并发或跨进程保证。
- CPU 83 项非全量 107 项、两秒集成非正式性能、容量/峰值未测、人工短复查非完整旧长时，真正干净机/任意路径/完整工具链及旧 CMake 兼容未测等仍按原范围保留，不据批准补填检查。
- 树叶剔除/旧视角突变仍延期，GL 未退役，foundation 等有消费者路径继续保留；正式 physics 模块搬迁仍须在 NPC 复用前单独授权并验收。未开展计算服务、NPC/AI/村落或现代性能分析工具接入。

指定 Y9000P 中端和 GTX 1650 低端整机 benchmark **改到现代性能分析工具接入后**，不再要求新增玩法实施前先完成两机采样。设备角色、Y9000P 既有目标、GTX 1650 硬阈值待确认以及各机独立版本/后端/负载/协议/原始数据和预算判定要求均不取消；当前两机新候选仍未测，旧冻结基准、本机结果、M3 批准或未来工具接入均不能冒充通过。工具尚未选定或接入，具体接入及采样范围仍需独立确认；长期安排唯一正文见 [scope 最新范围](../../../../scope/project-scope.md#m3-本机-rtx-5070-ti-验收与下一玩法阶段性能门槛)。旧快照及仍引用旧门槛的说明以该明确新决定为准，本轮不改写归档、冻结设计或其他 change 的规划内容。

同步更新全局关键节点、scope 的活动时间规则及架构状态导航；T5 保持 26/26、未归档。F 尚未同步 T5 生产代码，批准不构成代码同步、Git 暂存/提交/推送/合并、主规格 sync 或 change archive 授权。本轮不启动游戏、构建、CPU 测试、性能采样或新功能实现，也不选定性能分析工具。

本次记录更新检查：T5 strict validate 通过，git diff --check 无错误（仅既有换行转换提示）；文档审计 50 篇/796 条本地链接，无新增错误，既有 16 项缺失历史材料/锚点问题仍在，审计 exit=1，不称全库通过。只读前后核对 E 的 4481 项源码、两配置游戏及 50/49 文件安装包、F 核对的旧 ECS 源码、九份现役 specs、T3/T4 归档 Markdown/YAML 和两 checkout HEAD/index 均未变；工作记录留普通 out/t5-acceptance-20261025-001，冻结证据、个人正文/配置及其他 change 规划未改。

## 2026-10-25 主规格同步与归档

用户本轮明确要求归档 T5，并在两份 delta 的合并范围汇总后选择“先同步主规格，再归档”。归档前 schema 为 symocraft-full，proposal/specs/design/tasks 均 done，任务 26/26；T5/M3 整体批准沿前节，不由工具状态推定。配置上下文的早期未授权/旧源码描述按后续明确决定判读，不覆盖当前用户选择。

逐块合并 ecs 的 15 项要求/20 个场景及 module-boundaries 的 5 项要求/6 个场景，无修改/删除/重命名操作；既有边界要求、场景、标题与 Purpose 保留。新 ECS 的 Purpose 按 delta 原文复制，保留编写时点，另加交付源码根/验收说明；复制核对后仅按迁移位置修正 URL，不重写其正文。同步后逐份与全部 delta 再比较，20 项要求及 A01-A26 场景齐全、无 delta 操作标题，无剩余同步项。main-spec 常规校验 10/10，通过的两份受影响主规格及 T5 change 各自 strict validate 通过；全库 strict 因未改动 platform 的原有长段落 WARNING 未通过，不隐去该结果或扩修其他规格。

目标目录预检不存在且处于 docs/openspec/changes/archive 内，八份文件实际移动到 `2026-10-25-m3-t5-ecs-memory`，移动当时全部字节匹配，.openspec.yaml 保留，SHA256 `07a30f3f8a32c99b36c4cd3d71365553a1ae4ef4ddc30a69c98ae2c13525aa78`。随后只修正本次位置变化的 Markdown URL 与当前导航/状态，历史命令、结果、契约和既有批准含义不改，已有冻结档案未重写。其他活动 change 仅必要 T5 链接改址，不改其设计/任务或恢复 Fix1。本轮不启动构建/测试/游戏/采样，不同步生产代码，也不暂存、提交或合并 Git。

归档后再核：活动清单不再含 T5；两份主规格逐项匹配归档 delta、旧边界 Purpose 原样保留，新 ECS Purpose 正文仅 URL 改址。归档内 133 条本地链接/锚点全部可达；可维护文档审计为 44 篇/670 条本地链接，无新增错误，既有 16 项问题及 exit=1 保留。git diff --check 无错误，仅换行提示；E 的 4481 项源码、两配置游戏及 50/49 文件安装包、F 核对的旧生产源码和两 checkout HEAD/index 前后不变，其他八份主规格及 T3/T4 旧归档 Markdown/YAML 原字节不变。普通工作记录位于 out/t5-archive-20261025-001，不新增验证附件、不读取个人正文或更新标签索引。

## 2026-10-25 T5生产交付同步主工作区

用户在归档后独立明确授权“将已验收的 T5 生产代码及必要构建、测试配套同步到主工作区，保留已有未提交改动，同步后复跑受影响检查，暂不启动游戏，也不提交 Git”。本节记录该次源码传播及 F 新检查，不重开归档任务、不改前述历史结果或再次申请节点批准；本页此前“F 未同步”均为该独立授权前的时点状态。

### 输入与保留边界

只读核验 E 的 4481 项已验收源码仍为 SHA256 `57e9c98fe7ad82466189b8e13a07dd6b1c084ee2bd6fb9fc7d23bb41d7663049`，并确认准确五项退役缺失。选择生产、构建、根测试、必要 vendor/资源/脚本及独立 benchmark 配套共 4458 份输入；逐项核对 F 与 T5 原输入，未发现目标源码的独立未提交冲突。实际新增 124、更新 25，随同移除已验收的 `internal.cpp`、`internal.h`、`component_container.h`、`legacy/RawMemory.h`、`legacy/MemoryHelper.h`，均限定 ECS/src 下原五文件，删除前身份匹配且保存原字节备份。没有其他退役或整库删除。

同步后的选定输入逐字节匹配 E，闭包 SHA256 `071cd20bd679a878abddba625cbe2e9de89c8da7cfd41956465e36ce84ed5a14`。F 的 `.gitattributes` 与 E 不同但不属传播范围，保持原样，不全库换行归一化。原有脏文档/归档、其他 change、Git HEAD/index 在源码复制时未改；随后仅定向合并当前源码说明和本节结果，不覆盖其余未提交内容。E 源码/旧产物只读，F 保持 `feature-refactoring`、HEAD `6fea31e4631e267f132aa9b1608dc6538b562e13`，没有 Git 暂存/提交/推送/合并。

### 工具链与首次失败

沿用 VS2026/MSVC tools `14.51.36231`、cl `19.51.36260.0`、SDK `10.0.22621.0`、x64/C++20/Ninja，统一 DBG01。首轮 `out/t5-main-checks-20261025-001` 在 CMake 最小编译器探针报 C1902，尚未编译到项目代码；cl 与配套 mspdbsrv/mspdbcore 的实际版本分别为 19.51/14.51.36260.0，未发现这些文件的版本号不同。保留失败日志/缓存，在正常 Windows 执行环境及全新 `-002` 目录用相同工具链/参数重试成功。不修改系统安装、终止既有进程、改用 /Z7、关闭 /Zi 或削弱 DBG01；此对照不作为 C1902 根因已修复的证明。

### 主工作区本次检查

采用既有 S06.1 的非游戏检查范围，八种全新配置均完成 configure/build/settled，所有结果属于 F 的当前输入：

| 配置 | 实际结果与限制 |
| --- | --- |
| CPU-only Debug/Release | 全部启用 target 构建通过；受影响 CTest 各 83/83，包括 ECS 对象/身份/有序查询、代表分配故障、Session 逐 Advance/World 提交及物理/调用方回归，不是全量 107 项 |
| 公开消费者与边界 | 每配置真实多 TU 消费者、29 个预期 API 编译拒绝及 115 个边界夹具（16 接受、99 拒绝）通过；私有极端状态头只授权两指定测试 |
| ABI 与增量 | 每 CPU 配置 140 个 C++ 编译命令统一 CRT/Debug level 0，无 SDL/GPU SDK；私有诊断头各重编 3 对象，公开 Registry 头各 29 对象，mtime 恢复且源码字节不变，随后契约/Session 再次通过 |
| 游戏 Debug/Release | BUILD_TESTING=OFF、D3D12/Vulkan/保留 GL 同时编入，各 83 个 C++ 编译命令，唯一正式 ECS/Application 及现代后端 owner 齐全；各生成 4 DXIL/4 SPIR-V，头标识和生成元数据有效。只构建，不启动 exe，不新增单后端或真实 SDL/GPU 运行成绩 |
| 独立 benchmark Debug/Release | 各 9/9 fixture/协议测试，无游戏/EnTT/SDL/原生后端输入或图形 DLL 导入；不是实际游戏性能采样 |
| 根 runner-only Debug/Release | 关闭测试、GAME/CPU modules/原生后端后独立构建通过；不运行 runner，不为无测试配置补造 CTest 成绩 |
| 安装 Debug/Release | 安装到全新含空格路径，各 50/49 文件；exe 与本次 build 字节一致，EnTT 原始 MIT 及来源通知匹配源文件。没有启动包或验证真正干净机/M6 |

本次 Debug 游戏 SHA256 `54d76a404937f2ef890720df03d0c35ff2a690b75b396673bcde7e1427c63994`，Release 为 `3662a90e70d00d69abee1a572659278ed0c685a771610dcca50c50f4fb1dc2b7`；不是 E 的旧二进制身份。EnTT 安装许可仍为 `73547f6370ee16ea560665329bb62d4e2d8250f41eaa204b47a6c694ca3286ea`。源码与构建/测试记录留普通 out/t5-main-sync-20261025-001、out/t5-main-checks-20261025-002，旧目录及失败 `-001` 保留，不新增附件或原始数据到本 change。

本轮没有启动游戏、SDL/GPU 检查、人工体验、实际性能采样或新功能；不得继承 E 的历史真实集成作为 F 新运行通过。foundation.files/1175、持续/系统 OOM、罕见并发/故障、峰值/长期稳定性/真正干净机等已列风险保持，Fix1 继续暂停。其他机器 benchmark 仍在现代性能分析工具接入后执行，不因传播恢复旧玩法前门槛。正式 physics 模块搬迁、GL 最终退役及 Git 操作仍须单独授权。

收尾核对：全体主规格常规校验 10/10，两份受影响主规格 strict 通过；platform 原有长段落警告不改，全库文档仍有既有 16 项历史路径/锚点问题，exit=1，不宣称全库全绿。本次未新增文档错误，44 篇现役文档/本地链接检查按最终审计记录；git diff --check 无错误，仅换行提示。4458 份传播输入与 E 逐字节一致，两 checkout HEAD/index、无关脏文档及其他主规格/T3/T4 归档字节不变；本页归档前述正文的原字节前缀仍完整，仅追加本节。个人正文/配置未读写，旧包/失败目录/缓存均保留。
