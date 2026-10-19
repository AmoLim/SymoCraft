---
tags:
  - area/spec
---

# T5 验证

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

已核对[T4 完成批准](../archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-19-t4整体完成批准)、[scope](../../../scope/project-scope.md#m3-t4-完成应用平台与模拟模块接口)、[工程状态](../../../architecture/engineering.md)、当前 AGENTS/config 及现役规格：T4 于 2026-10-19 获本机 RTX 5070 Ti 范围整体验收，五份增量已同步为现役契约，change 已归档；后续独立明确授权又将生产源码和必要构建/测试配套带入 F 的 feature-refactoring。T3 同样已批准归档，D3D12 默认、Vulkan 显式可选、GL 保留。归档或旧验证中“F 仅 T3/仅审核 docs”的文字是当时事实，本轮不改写它们，也不再将它们当当前未交付状态。

只读源码确认 F 已有 `SimulationSession::Create/Advance/Pause/Resume`、`physics_adapter.cpp` 的按实体取小值/求解成功后提交，以及 simulation/physics 内真实 STATIC `symocraft_physics`；app 的真实 owner 为 `symocraft_application`，旧 runtime 名仅为 alias。F ECS 仍是 T4 限定修补后的旧 typed/byte 池，不是新 EnTT Registry。原 `E:/Codex/Worktrees/refactor-ecs-memory/Symocraft` 实验树没有 `game/modules/simulation/src/session.cpp`，其原实验身份不变，不能直接充当最新 T4 生产输入；本轮没有将其源码同步到 F 或反向合入 T4。

主工作区 CLI 基线已是 VS2026、MSVC tools `14.51.36231` / cl `19.51.36260.0`、SDK `10.0.22621.0`，与 T5 已验证工具集一致；这不证明 T5 DBG01 的 level 0 已在生产库/消费者全链路统一或完成重建。S01.1 仍须确认 T5 实际采用源码/构建身份、统一 ABI 和代码根/生产范围，S01.4 仍待正式冻结/实施入口确认，任务保持 2/26。本次同步只纠正输入状态，不新增行为/能力、重选决策或授权生产替换。

T4 的 CPU Debug foundation.files/1175、历史导出/缺码失败与未覆盖风险仍按归档保留，Fix1 继续暂停；上游整体批准不等于全部测试全绿，也不将旧成绩转成 T5 替换后的成绩。本轮仅静态/文档核对，未构建、测试 CPU/GPU、启动游戏、采样或追加人工验收，未提交、合并、删除、规格 sync/archive。

主工作区 T5 strict validate 通过，apply 复核仍为 ready、2/26，S01.1/S01.4 未勾选；本轮四份文档的尾随空白检查无匹配、已跟踪文档 diff --check 无错误，仅 Git 换行提示。只读文档审计覆盖 49 篇/705 条本地链接，T5 文件未报告链接或锚点错误；全库仍有其他位置的 17 项缺失材料/锚点问题，未扩修或宣称全库通过。
