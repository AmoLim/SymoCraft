---
tags:
  - area/spec
---

# 验证记录

> 此前验收文档曾同步至 `G:/Projects/Symocraft` 审阅；实现与最新实测/节点记录位于 `G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft`，不表示主工作区源码已集成。N1/N2/N3 与整体验收均已获明确通过，依据及保留限制见 [2026-11-07 整体验收通过与主规格同步](#2026-11-07-整体验收通过与主规格同步)。下列构建/运行命令以实现工作树为执行目录。

## 2026-10-28 N1 局部交付

本轮源码为 `apply-renderdoc-tracy` 托管工作树，起点 `fd09c947106c686755c269eebd4e4e504ee870d8` 加本轮未提交改动。用户已确认 `SYMOCRAFT_PROFILE_PLOT` 命名、退出配置方案 A 并授权继续 N1；未启动 N2/N3，不代表 S1 全部前提、整个 change 或人工验收通过。Tracy/退出配置与接口决定见 [proposal](proposal.md#short-approach)，本页不复制活动任务清单。

### 交付与身份

- 固定官方 Tracy v0.14.1、commit `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c`；vendor 中保留的 92 个文件与该 commit ZIP 对应成员逐字节一致，源包 SHA256 与原始许可见 [来源记录](../../../../cmake/licenses/SOURCES.txt)。未修改上游源码，未安装或运行 Viewer/RenderDoc。
- 已提供四个薄宏、唯一静态 client、默认关闭开关、Release+PDB preset、私有消费者接线和边界检查。生产 CPU 区域、实际帧边界及 GPU 注释尚未接入；宏测试不证明生产轨迹存在。
- YAML 编解码迁至 foundation 所属独立 `symocraft_document_io`，实现相对旧 Git blob 仅改变包含路径，基础 foundation 与独立 physics 不反向依赖它。11 个包含消费者完成迁移。先完成新适配定向验证，再按用户对准确清单的确认删除原 telemetry `include/symocraft/telemetry/document_io.h` 和 `src/document_io.cpp`；没有删除其他旧链文件。
- 新检出中的 EnTT LICENSE 曾被换行转换，导致既有哈希检查拒绝配置。经用户单独确认，恢复为 Git 原始字节，并仅为该许可证及 Tracy 原始文件增加 `-text` 保护；未放宽身份校验、全库规范化换行或改全局 Git 配置。

### 实际检查

工具链为 VS2026 x64、MSVC tools 14.51.36231（该目录实际 cl 版本 19.51.36260.0）、Windows SDK 10.0.22621.0、CMake/Ninja。下列均在本工作树新建的 `out/build/` 子目录执行，不复用其他候选产物。

| 构建目录 | 配置与结果 |
| --- | --- |
| `n1-yaml-migration-debug` | Debug、CPU_ONLY=ON；先完成迁移前定向 5/5。最终显式请求 Tracy/marker ON 后检查局部强制 OFF，定向 6/6 通过；实际生成图与独立 physics 编译闭包无 client/图形依赖，不以仍为 ON 的缓存值推定实际启用 |
| `n1-tracy-debug` | Debug、GAME/BENCHMARK OFF、CPU_MODULES ON、CPU_ONLY OFF、Tracy ON；正式 simulation/physics 闭包及宏/YAML/公开头构建成功，最终定向 6/6 通过 |
| `n1-profiling-release` | profiling preset 的 Release 配置，额外 GAME/BENCHMARK OFF、CPU_MODULES ON、原生后端 OFF；定向 6/6 通过。实际 C++ 编译保持 `/O2` 并增加 `/Zi`，链接含 `/DEBUG /OPT:REF /OPT:ICF`，匹配 PDB 存在；不是完整游戏 profiling 构建通过 |
| `n1-default-debug-no-tests` | 普通 Debug 默认 Tracy/marker OFF、GAME ON、BENCHMARK OFF、BUILD_TESTING OFF；成功构建真实 `symocraft_application`，覆盖 application/GameRenderer/旧 telemetry 的 YAML 迁移，不运行游戏 |

最终定向选择为 `ctest --test-dir out/build/<目录> --output-on-failure --no-tests=error -R "foundation.document|profiling."`，包含两个 document 公开头消费者、文档往返及错误用例、宏/多翻译单元、关闭态参数零求值、无 vendor 配置测试。无 vendor 测试在临时独立源码夹具中消费真实适配副本：OFF 可配置/编译/运行，ON 具名拒绝缺源码，不搬动实际 vendor。最新 Debug/Release 检查还覆盖基础 foundation/独立 physics/业务公开头/直接 Tracy include 的四个拒绝及 world 私有包含的一个允许用例。

启用态测试前核对实际子进程环境 `TRACY_NO_EXIT` 未设置；测试程序在没有 Viewer 的条件下有界退出。未测试启用不支持覆盖时的等待，也不把限制配置视为已实现强制拦截。Debug 实际 C++ 编译均含 `_ITERATOR_DEBUG_LEVEL=0`，独立 physics 不获 Tracy 编译定义/头根；client 编译项及宏测试最终链接库各为一份。多翻译单元链接成功不等于已完成所有最终产品的符号审计。

首次定向测试的正则额外选中两个尚未构建的公开头消费者，出现 Not Run；补建后通过，保留该次状态，不计为代码运行失败或最初全绿。初始 EnTT 身份拒绝也保留为配置失败，不冒称初始环境直接通过。

集成侧复核改动范围、公开/私有接线及 YAML 旧路径残留；`git diff --check` 与 `python tools/openspec/run.py validate integrate-renderdoc-tracy --strict` 通过。OpenSpec 校验只证明规划格式，不证明运行正确性。

### 未执行与限制

未运行 world-summary、游戏、原生 GPU 探针、Viewer 连断/晚连接/重连、RenderDoc 捕获回放或人工体验；五个已迁 include 的原生探针未编译。未执行完整 Debug/Release 后端矩阵、全部 CTest、实际安装或干净机器部署。只核对默认关闭构建生成的安装规则未包含 Viewer/RenderDoc 安装项，不声称安装产物验证通过。CPU 序列开启/关闭业务等价与生产帧/计数接线留后续节点，尚无正式性能结论。

S3 的 world-summary/探针验证与 S4 的完整最终产品/安装检查尚未全部闭合，因此本轮不勾选整项 S3/S4。旧 benchmark/Session/导出仍活动，未执行 S8 退役；Fix1 继续暂停，历史冻结材料、旧缓存、个人文件未动，无 Git 提交/推送、主规格同步或归档。


## 2026-11-02 当前托管 worktree：SDK28000 与 N1

本轮代码根为 `G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft`，起点 `fd09c947106c686755c269eebd4e4e504ee870d8` 加复制的本 change 文档及本轮未提交实现。上节 2026-10-28 的 E 工作树成绩保持历史身份；其源码不在本次新检出中，因此本轮重新交付及检查，不继承旧成绩。用户已授权当前代码根继续实施、改用 SDK28000、更新构建说明，并确认双现代 Debug/Release 单/双组合、受影响自动检查、CPU-only/关闭测试、安装、GL 回归与工具验收范围；人工操作每后端最多五分钟、总计十分钟，不恢复历史长测或其他机器采样。

### 本轮交付与实际身份

- VS2026 位于 `F:/Toolchains/Cpp/Microsoft VS/2026`，MSVC tools `14.51.36231`（实际 compiler `19.51.36260.0`），Windows SDK `10.0.28000.0` 位于 `F:/SDK/Windows Kits/10/WindowsSDK`；Python 3.12.14、CMake 4.4.5、官方 Ninja 1.13.2。Ninja 仅置于本树 `out/tools`，ZIP 与官方发布 SHA256 `07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65` 一致。OpenSpec CLI 1.16.0 已按单独授权全局安装。
- 根 Agent/README、脚本说明及构建/原生验证的共享 SDK 默认值已切为 28000；示例使用新的 `msvc51-sdk28000-*` 目录。共享 helper 仅向子进程传递实际选中 SDK 的 `CMAKE_WINDOWS_KITS_10_DIR`，使自定义安装位置的 UCRT 可被 CMake 发现；不修改全局环境。保留显式旧 SDK 参数检查，更新默认值回归与自定义根传播检查。
- 重新提供独立 `symocraft_document_io`，新 header/cpp 只改变编译 owner/include 路径；编码、加载和错误机制未改。11 个包含消费者已迁移，补充文件往返、坏 YAML 与缺文件回归。经本轮准确两路径清单及用户确认，新适配三项定向检查通过后，删除原 telemetry `include/symocraft/telemetry/document_io.h` 与 `src/document_io.cpp`；其他旧链文件未删除。
- 固定 Tracy v0.14.1 / `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c`，本次保留完整 `public/` 88 文件及 LICENSE，共 89 文件，逐文件与固定 commit ZIP 原字节一致；来源摘要/许可见 SOURCES，不把上节历史 92 文件清单改为本次清单。唯一静态 client、四个薄宏、默认 OFF 开关、私有接线、profiling preset 与许可/PDB 安装规则已接入。ZONE 使用计数唯一局部名，支持同一行多个区域且寿命至调用方词法块末尾；关闭态不求值参数。
- 本次检出的 EnTT LICENSE 被 CRLF 转换，已恢复成 Git 原始 blob 字节并通过原有 SHA256，不放宽校验；仅为该许可证和新的 Tracy 原始文件增加 `-text`，未规范化其他文件或改变全局 Git 配置。

### 实际检查

全部使用本轮新建的 `out/build` 子目录，不复用 SDK22621/旧编译器产物。按最终代码的相关部分选择定向检查；以下不是整个 change 或双后端矩阵通过声明。

| 目录 | 已执行结果 |
| --- | --- |
| `n1-sdk28000-cpu-debug` | CPU-only Debug；实际构建 simulation/physics 闭包、YAML/公开头与关闭态宏。显式请求 Tracy/marker ON 后，本地仍强制 OFF；实际全部编译命令无 Tracy 定义/头根/client。最终定向 8/8 通过，含 YAML/宏/无 vendor/源码边界及构建 helper |
| `n1-sdk28000-tracy-debug` | GAME/BENCHMARK OFF、CPU_MODULES ON、Tracy ON；YAML/宏/两个文档公开头消费者最终 6/6 通过。宏覆盖同一行多区域、多翻译单元、一次求值与关闭态未定义参数不求值 |
| `n1-prof-rel` | Release、游戏/后端 OFF、Tracy/marker ON；同一组最终 6/6 通过。实际编译保持 `/O2` 并有 `/Zi`；链接含 `/DEBUG /OPT:REF /OPT:ICF`，匹配 PDB 存在。此目录安装规则的旧 CRT 自动发现有警告，不称其为安装验证 |
| `n1-game-dbg` | 默认关闭 profiler 的 D3D12+GL Debug 真实游戏、应用生命周期 probe、报告消费者及三份已迁 include 的 D3D12 probe 编译成功；world-summary、资产预检、源码/负向/renderer 边界及构建 helper 共 6/6 通过。未运行 GPU probe 或普通窗口玩法 |
| `n1-prof-game-rel` | 无测试、D3D12、Tracy/marker ON 的真实 Release 游戏构建成功；实际 client 编译条目及最终游戏链接各一份。安装至 `out/install/n1-profile-release`，exe/PDB 与当前构建字节一致，原 Tracy 许可存在、无 Viewer/RenderDoc 可执行文件、无测试注册/fixture。安装包在无关且含空格的 CWD 完成 `--check-assets --renderer d3d12` 和固定 seed 的 441 区块 `--world-summary`，退出均为 0；不称干净机器或真实图形部署通过 |

无 vendor 检查在临时独立源码夹具复制实际 profiling 适配，OFF 可配置/编译/运行，ON 缺源具名拒绝，不搬动真实 vendor。负向包含检查拒绝基础 foundation、独立 physics、业务公开头及直接 Tracy include，允许 world 私有适配调用。实际 Debug C++ 全部保持 DBG01 与 Debug CRT；独立 physics 编译及目标闭包不获 YAML/Tracy。启用态宏检查核对 `TRACY_NO_EXIT` 未启用，在无 Viewer 环境下有界退出；不据此宣称 Viewer 连/断已测。

首次新适配测试因匹配的 Debug CRT/UCRT 不在子进程查找路径，两个公开头消费者为 `0xc0000135`、文档测试超时；为本轮测试子进程提供匹配运行库路径后通过，不把首次结果改成成功。首次 D3D12 Debug 配置因自定义 SDK 根未传给 CMake 而缺少 `ucrtbased.dll`；修复 helper 子进程根传播后，同一 SDK28000 新目录重新配置并构建成功。原始失败和目录保留。

### 尚未闭合

S4 的可选依赖/构建任务已完成并回读 apply 指令确认进度为 1/10。S3 还缺 Vulkan 两份受影响探针的编译/适用检查，因此不勾选；`n1-vk-dbg` 的真实配置在 `RendererVulkan.cmake` 找不到 `vulkan/vulkan_core.h` 处失败，当前无 `VULKAN_SDK`，已向用户询问安装路径。S1 的验证范围已确认，但 RenderDoc/Viewer 实际安装身份未核对；N2/N3 生产埋点、场景入口/轻量快照、GPU 注释与真实工具捕获尚未实施/执行，不把 N1 计作整体验收。旧 benchmark/Session/GPU query 仍活动，S8 的其他退役没有授权或执行。

`git diff --check` 通过。新版 OpenSpec 1.16.0 的 `validate --strict` 因现有七份增量规格在首个二级标题前的前言/元数据产生七条 WARNING 而返回非零；未改写这些规格或将格式告警冒称运行失败，也不继承上节旧 CLI 的严格通过结果。未进行规格 sync/archive、Git 暂存/提交/推送、历史证据修改或其他 change 内容迁入。


### 同日后续：Vulkan SDK 安装识别与 S3 收口

用户已安装官方 Windows x64 Vulkan SDK 1.4.363.0，实际根为 `F:/SDK/VulkanSDK/1.4.363.0`。系统已登记 VULKAN_SDK；当前应用进程尚未继承新变量，本轮仅向构建子进程显式提供该根及 Bin，不更改全局配置。核对头文件 revision 363、DXC、dxcompiler.dll 及 Khronos validation DLL/manifest 均存在。`vulkaninfoSDK --summary` exit0，枚举到 RTX5070Ti、NVIDIA621.01、设备 API1.4.357，且可见 VK_LAYER_KHRONOS_validation 1.4.363；另有三条显示扩展枚举相关 loader 警告，工具枚举成功不等于游戏 validation 零警告。

保留先前 `n1-vk-dbg` 配置失败；新建 `out/build/n1-vk-d2`，仍用本轮 MSVC/SDK28000。CMake 实际选中该 SDK 的 Include 与 Bin/dxc.exe，真实 SPIR-V 编译 probe 通过并核对产物 magic。Vulkan Debug 游戏、renderer_vulkan_wsi_probe、renderer_vulkan_lifetime_fault_probe 均编译/链接成功，四份生产 HLSL 生成 SPIR-V 成功；资产预检、world-summary、源码边界与 renderer 边界 4/4 通过。

结合前述 YAML 往返/错误测试、公开头消费者、D3D12/应用调用方及实际 CPU/physics 依赖图检查，S3 的迁移与受影响消费者编译检查已收口，回读 apply 进度为 2/10（S3、S4）。本轮没有运行两份 Vulkan GPU 探针的窗口/故障场景；其仅 include/链接归属受本次 YAML 迁移影响，不把编译通过改称真实图形/故障或新候选整体验收通过。S1 工具身份及其余 N2/N3、捕获/回放与 S8 退役仍待推进。


### 同日后续：DirectX/D3D12 可用性

按用户要求检查已构建的 `n1-game-dbg/bin/SymoCraft.exe`，固定 seed424242、regression/four-chunk、120 帧、单进程上限60秒。启用 validation 的首次启动在 D3D12GetDebugInterface 失败，HRESULT `0x887A002D`（本机 SDK 定义为 DXGI_ERROR_SDK_COMPONENT_MISSING），正常关闭、exit3；调试层环境未就绪，不冒称 validation 验证通过。

随后显式设置 `--validation 0` 单独检查基础 API，实际选中 D3D12 / RTX5070Ti，driver32.0.16.2101、4x MSAA、Fifo；完整441区块真实绘制120帧，约3.26秒正常退出，exit0、stderr为空。基础 D3D12 渲染可用与调试验证受阻分别记录；不将这次短启动视作 RenderDoc、画面对照、全矩阵或阶段验收通过。未安装系统组件或改动渲染配置默认值。

### 同日后续：调试层就绪、工具身份与独立场景

用户安装 Graphics Tools 后，在同一 D3D12 Debug 目录重新启用 `--validation 1`，固定场景真实绘制120帧，约3.44秒，exit0、stderr为空；先前缺 SDK component 的失败仍保留。后续新场景代码在 D3D12/Vulkan 各执行一次普通120帧 validation-on 回归，均正常退出。短运行只证明受影响路径，不能推定全矩阵或人工验收。

本树 `out/tools` 使用官方 RenderDoc 1.47 x64，CLI 报 commit `b0e1370309f7d7bf986a73830446ca39d9b8c1fd`，官方 ZIP SHA256 `b51712e58d40066b34ca30c7869d15f72e4b5c001b2d92232f6783a636d5a3c1`；匹配 Tracy 0.14.1 Windows Viewer 工具包 SHA256 `f7499d74914aa3ba94a2c1ce72f36477d7b61d9d0f7c9790e05274c258c97fb5`，capture CLI 报 `0.14.1 / 30997d5`。工具身份与本轮已确认执行范围齐备；安装/版本检查不冒称捕获或 Viewer 连断已通过。数值快照本轮不新增 HUD/面板，代码根授权和人工时长沿本节已记录的当前用户确认。

独立入口已支持 `--profile-scenario static|walk|edit` 及整数1..1200秒（默认60），与预检/摘要/有限帧和冲突场景参数互斥；旧 benchmark/输出/预热采样/焦点参数在窗口初始化前具名拒绝、exit64。真实装配分别核对 walk→spawn、static/edit→four-chunk 的原 position/yaw/pitch 和45度初始 FOV。新场景不创建 Performance::Session；仍未退役的内部旧 probe/旧产品接线不作为新入口。

D3D12/Vulkan Debug 的 static/walk/edit 各独立运行一秒，共6/6正常限时完成；保持1920×1080、4x、请求VSync-off、validation-on及完整世界，不给输出目录、不连接工具，edit 按0.25秒节奏执行。原生 WM_CLOSE 正常取消。测试最初用 PostMessage 的 Escape 未到达SDL，游戏按十秒时限正常完成而 harness 断言失败；第二次前台验证拒绝后未发送全局输入并关闭自有子进程。这两次不算 Esc 通过。

随后改用链接相同 Application 的显式原生 probe，仅向自有窗口队列投入 SDL 合成事件：每后端各9项，共18/18通过，覆盖 Esc/同轮失焦Esc、resize、minimize、失焦继续、玩法输入隔离和三初态。最小化/尺寸失效具名失败、清理有界；取消不再推进；重复 Shutdown 事实一致，旧快照持有仍有效。合成事件/自动探针与人工体验分别记账，不填人工批准。CPU 场景测试覆盖32次正反编辑在8秒恢复原digest；参数、场景及轻量快照定向5/5（含快照公开头）通过。首次额外选中尚未构建的公开头消费者为Not Run，补建后通过。

轻量 DebugSnapshot 已接已有 Advance/Present 结果，不增加ECS遍历；一秒固定计数覆盖成功呈现、零/多物理步、暂停/跳过/恢复、缺值与旧快照所有权，50万次合成观测仍为常量存储。S7 尚未完成：旧 Session/query/系统采集及旧CPU耗时字段仍待移除，不能将快照测试作为旧链退役证明。S5 生产CPU区域/计数正在接入，Viewer与RenderDoc实际验收尚未完成。

### 同日后续：S5 生产时间线

实际接入 app 初始化/事件/输入/场景/帧/关闭、World准备/编辑/网格、SimulationSession/角色阶段/物理适配、renderer缓存上传及 D3D12/Vulkan prepare、submit、Present CPU调用和D3D12呈现节流等待；Present CPU调用不标为GPU执行时间。独立physics无埋点，不新增业务线程、异步对象引用或逐体素区域。固定名plots区分每帧物理步/重建数/draw数及字节；资源Create/Update上传与Render-local上传分别表达。APP_INFO发布真实构建/后端/设备/驱动/API/extent/samples/present/validation及seed/workload身份；共同帧边界仅app发出一次。

相同 MSVC/SDK 的关闭态 `n1-sdk28000-cpu-debug` 与开启态 `n1-sdk28000-tracy-debug` 分别对参数、独立场景正反编辑、真实生产SimulationSession及快照执行4/4，均通过，物理步/World提交及既有数值检查保持。开启态不含独立physics的Tracy定义；Release真实游戏保留优化及匹配PDB。接入阶段编译曾因Vulkan局部 `acquire` 作用域被缩小而失败，提升至共享帧上下文后编译通过，未改变acquire/提交顺序。

`n1-prof-game-rel` D3D12 validation-on/edit16秒：匹配0.14.1 capture客户端连接3秒→断开→重连3秒→断开，游戏持续运行且最终exit0。两份真实轨迹均由官方csvexport检查到上述生产CPU区域及计数，Viewer成功打开轨迹并显示帧/plots；导出仅为本地检查工具轨迹，不恢复游戏旧CSV协议。Viewer实时连接127.0.0.1可见prepare/submit/Present，主动Stop后仍运行。首次后台试验未由父进程记录退出码，因此只记录其连接/停止观测，不据缺失尾日志推定崩溃或正常退出；另一次有父进程等待的无Viewer场景40秒exit0。随后walk18秒用Viewer自动连接并一直连至游戏结束，Viewer收到9127个帧边界、显示断开，游戏实际exit0、完成诊断及Shutdown成功。

初次Viewer启动出现Windows防火墙请求，由用户明确回复“已处理”后继续；Agent未操作安全权限。支持的子进程环境始终未启用TRACY_NO_EXIT覆盖。以上为短诊断连断/数值保持依据，不是低中端机器、旧正式预算、长期稳定性或整个change验收；旧Session/query未在S5中宣称退役。

### 同日后续：S6 原生注释与 RenderDoc

D3D12/Vulkan私有后端增加独立注释开关：持久world mesh/atlas、present/depth/MSAA、upload及command对象在新建资源时具名；world/selection/upload/resolve/readback按实际工作分组，RAII在命令关闭前配对。关闭宏不求值对象、动态名称及无工作条件，不产生额外mesh遍历；无RenderDoc SDK/链接依赖。Vulkan可选debug_utils独立于validation，缺扩展或缺任一注释函数明确诊断后关闭注释。真实helper的缺扩展/缺函数、临时字符串、嵌套异常配对及无工作区域单测通过；这是mock能力缺失，不称本机实际缺扩展。CPU-only关闭态未定义参数编译/运行通过；模块三项边界3/3通过，diff whitespace检查通过。

新目录 `n3-dual-rel` 使用本轮相同工具链/SDK、两现代后端、Tracy OFF/marker ON/BUILD_TESTING OFF，完整游戏构建成功。两后端validation-on/edit两秒均exit0（各7次实际编辑），Vulkan报告0 warnings/0 errors、native_retired=true；validation-off/static一秒也各exit0，核对两开关独立。保留GL Debug路径重编译后在RTX5070Ti/NVIDIA621.01/OpenGL4.6实际绘制120帧、退出0；没有要求GL注释等价。

RenderDoc CLI可运行，但最初数个GUI/脚本启动未出现可操作窗口，20/60秒等待超时，脚本未执行；日志已完成GPU枚举，未确认根因，不归因于游戏。仅结束本轮准确PID/路径匹配的自有超时探针。用户手动打开工具并回复“出现窗口了，请继续”后，同一工具脚本正常执行；未改写工具配置或操作其安全/隐私权限。首次D3D12 frame30实际捕获和回放初始化通过。

随后对候选exe SHA256 `cd7d379214d4487dc24ea7c28200ee1510737b53a568d7be7a3873b34b0a3d95` 用外部RenderDoc ExecuteAndInject分别运行三个十秒场景、就绪后请求一帧：D3D12 static/walk/edit frame967/1347/974，Vulkan frame782/982/884，六份均实际OpenCapture成功。回放核对对应API、nVidia且无degraded，world中超过100个非空draw、实际post-VS网格及buffer数据、绑定的64×64×64层atlas及非空纹理像素；world/resolve.msaa事件及持久mesh/atlas/present资源名均可辨。static/edit各有24顶点selection，walk当前视线无selection，不伪造区域。六份回放导出的真实颜色图像已查看：对应固定场景/移动视点/编辑材质及适用选框，未发现明显几何、层号或方向错误；不冒称逐像素容差或用户人工批准。

捕获时使用各后端生成shader、实际1920×1080/4x/VSync-off请求/validation-on，Release CPU优化与shader O3未为捕获切换；RenderDoc日志核对RTX5070Ti/driver32.0.16.2101。Vulkan仅通过子进程VK_ADD_IMPLICIT_LAYER_PATH发现工具原manifest，由外部启动启用捕获，不注册全局层、不启全局注入/child hook。捕获/回放和临时PNG/检查数据只留out工作存储，不新增验证附件。工具扰动、实际检查范围与旧预算不等价；旧Session/query尚存，六份为S6接入点候选，不称S8退役后最终候选。工具操作说明已随实际接线更新，S7/S8及最终矩阵/S9/S10仍未完成。

### 同日后续：S7 app 观测替换与 query 移除

Application不再拥有Performance::Session，不创建输出目录/逐帧记录、周期进程/GPU内存采样或Seal/Export；Run仅保必要场景实际时间、已有Advance/World/Present顺序、固定计数plots及DebugObserver。main保持参数64、资产2、运行/清理3与正常/取消0分类，关闭报告仍自有、幂等并保首因/次错诊断，无旧退出4。GameRenderer截图不再要求Session；失败后的operations_allowed仍阻止资源续交并保留完整Renderer关闭事实，未因删除采样删掉清理约束。

现代公开PollGpuSamples、GPU样本类型/能力和旧CPU耗时结果已移除；两后端不再创建timestamp heap/pool/readback、写入/解析计时query或维护返回队列。D3D12实际提交/Present信号、Frame fence、呈现节流、设备移除检查，Vulkan submit/present fence、实际窗口失效/重建、资源退休和截图readback仍保持。迁移时曾因删除耗时累加留下孤立语句而编译失败，完整移除仅属于耗时溢出的分支后，两后端重新构建成功；普通错误检查未借此放宽。

CPU定向6/6通过：快照、幂等关闭报告、正反场景编辑、renderer层号缓存、公开API消费者及门面有效契约。旧耗时/采样专属断言移出已退休接口，继续覆盖自有计数/截图、非法结果、实际失败停止续交、线程/重入、资源和实例寿命，不靠删除有效契约变绿。两后端新app原生场景合成事件18/18再次通过。原生应用装配/清理probe已改为不导出，分别覆盖十个取得后失败点、重复关闭、调用方字符串寿命及窗口次错仍继续Runtime清理；D3D12/Vulkan各exit0，并断言probe父目录未产生旧文件。

共享绘制/截图probe移至两现代任一后端可注册的位置，不再只在Vulkan构建提供。D3D12/Vulkan各用实际4x/validation-on执行drawing模式，均exit0，覆盖原透明/深度/面向/纹理层/选框、空mesh恢复、非法层提交前拒绝、VSync与原生尺寸变化；截图直接消费自有FrameResult，不经过Session/query。只执行drawing模式，不把未运行的world-window最小化/输入组记通过。旧Session源码、独立产品、专属测试和脚本尚待S8准确删除清单确认，旧故障/寿命probe中采样相关消费者迁移及最终矩阵仍未完成，不声称整个change或S9通过。

### 同日后续：S8 消费者迁移与删除前检查

保留并迁移 D3D12/Vulkan 原生故障、寿命、WSI、backend smoke 及 frame-bindings 消费者：移除旧 Poll/query 专属分支，frame ID 直接核对 Render 返回；D3D12 真实设备移除由后续实际 CreateMesh 操作传播，不再以 Poll 触发。提交/呈现 fence、更新/删除版本的在途保留、容量/回滚、设备失效后拒绝续交及不确定清理的终止保留均保留。D3D12 lifetime、allocation、submission、device-removal 与 Vulkan lifetime、submit、device-lost 原生运行各 exit0；四份寿命/Vulkan故障自有报告通过迁移后的校验器。测试启动曾把 D3D12 模式误写为 device-lost，返回64；更正为原有效 device-removal 后通过，不把参数错误记为设备故障。另保留的 D3D12 in-flight-timeout/Vulkan 其他故障与完整 WSI 模式尚未在此点重跑，留最终 S9。

生产调用方已移除 FrameDiagnosticScope/WorkScope 的旧 CPU 分段采集；Vulkan 白盒诊断只保留有效 native allocation/bind/call 事实，删除 wall_ns 与 query-read 记录，不恢复耗时分析。更新后 Vulkan frame-bindings/buffer-policy 原生 probe 实际运行51帧、保存17份截图并执行其像素/资源断言，exit0、validation 0 warnings/0 errors、native_retired=true；这是当前保留测试的正确性依据，不是正式性能成绩。

verify-d3d12-candidate.py 保留包清单/字节/规范路径及 reparse 拒绝、系统路径运行、隔离资产故障；verify-d3d12-game.py 改为直接场景并核对实际后端、完整441区块、初态映射、完成/关闭及无旧输出，不再读取 CSV/probe。部署脚本保留游戏/中文空格路径/CRT观察及资产故障，撤仅旧 runner 的GUI取消/协议检查；输入/取消有效契约由已迁移app原生probe覆盖。共享文件及非性能检查未删除；历史 experimental/Fix1 采集器不改写。

迁移后 Python 检查：共享脚本37/37、candidate包/路径26/26、developer脚本19/19；原生寿命/WSI校验器的CPU测试此前对应5/5、7/7、8/8通过。candidate首次因系统临时目录写入权限在setUp失败，改用本工作区out下独立临时目录后26/26通过，未修改产品来绕开测试。developer初次仍按旧benchmark盘点数断言失败，改为显式排除旧产品输入并保留逐文件字节/脏diff检查后通过。新游戏验证脚本实际D3D12 static2秒/walk3秒/edit9秒和资产/不可用Vulkan/非法后端6/6通过，无旧CSV或输出目录；实际初态/移动/编辑完整规则仍由原生/CPU测试覆盖，不从控制台完成标志推定。

更新 StartupOptions 去除旧内部占位字段后，D3D12/Vulkan 游戏及受影响原生 probe 均重建成功；参数与三项源码/负向/renderer边界4/4通过，diff whitespace检查通过。根构建默认不提供benchmark，开启旧缓存选项在CPU-only局部关闭前明确失败；两份全新目录分别验证普通/CPU-only的旧选项请求均被具名拒绝且不生成Ninja图。构建矩阵入口同步改为预期拒绝，不静默配置空工具；尚未执行最终矩阵，不称 S9 通过。

当前仅撤调用、API和注册，54份旧链文件仍待下列准确清单确认；旧GL timer/显存实现与旧telemetry编译清单将在确认后成组移除并重建。S8不勾完成，S9/S10仍未完成，未触碰历史证据、旧缓存、原工作区或Git历史。

### S8 准确删除清单（待确认）

以下为本轮依赖复核后的准确 **54 份现有文件**；本清单尚未获确认，尚未执行删除。此前获准的两份 document_io 旧文件不在本清单。只对本托管工作树生效，不作用于原工作区、历史包、缓存、实验冻结源、Fix1 或个人文件。

迁移依据：新场景/快照/Tracy/原生注释已接生产路径；app 不再构造旧 Session，现代时间戳 query 已撤，截图与提交/呈现 fence 保留。旧 GPU timing/CSV/runner 断言随产品退役；仍有效的输入/位姿、文件可靠性、包身份/资产、截图、寿命/故障测试已保留并迁移，删除后仍须重建及执行 S9，不能因清单成立而称最终验收通过。

共享的 verify-d3d12-candidate.py、verify-d3d12-game.py、verify-sdl-delivery.py、VerifyMigrationRuntime.py、windows_verification.py、构建/打包脚本及其非性能测试均保留；只迁移旧协议部分。world/benchmark_workload.h 保留新场景所需动作。test/experimental/ 中旧候选/暂停采集器原样保留，不承诺其继续适用于新产品，不增加兼容壳。

**旧独立产品与专属 runner 测试（16）**

- `test/benchmark/CMakeLists.txt`
- `test/benchmark/benchmark_script_tests.ps1`
- `test/benchmark/benchmark_script_tests.py`
- `test/benchmark/capture_probe.cpp`
- `test/benchmark/publication_tests.cpp`
- `test/benchmark/runner_tests.cpp`
- `tools/benchmark/CMakeLists.txt`
- `tools/benchmark/README.md`
- `tools/benchmark/include/benchmark/runner.h`
- `tools/benchmark/runner.manifest`
- `tools/benchmark/src/gui.cpp`
- `tools/benchmark/src/gui.h`
- `tools/benchmark/src/main.cpp`
- `tools/benchmark/src/platform.cpp`
- `tools/benchmark/src/platform.h`
- `tools/benchmark/src/runner.cpp`

**旧采集、系统内存与 GL timer 实现/接口（10）**

- `game/modules/telemetry/include/symocraft/telemetry/performance.h`
- `game/modules/telemetry/src/performance.cpp`
- `game/modules/telemetry/src/export_diagnostics/export_probe.h`
- `game/modules/platform/include/symocraft/platform/process_memory.h`
- `game/modules/platform/src/process_memory.cpp`
- `game/modules/renderer/legacy/include/symocraft/renderer/gpu_timer.h`
- `game/modules/renderer/src/gpu_timer.cpp`
- `game/modules/renderer/src/performance_memory.cpp`
- `game/modules/renderer/src/frame_diagnostics.h`
- `game/app/src/foreground_readiness.h`

**仅服务旧性能协议的测试与 fixture（13）**

- `test/unit/telemetry/performance_tests.cpp`
- `test/unit/telemetry/renderer_performance_tests.cpp`
- `test/unit/telemetry/lifecycle_tests.cpp`
- `test/unit/renderer/gpu_timer_tests.cpp`
- `test/unit/renderer/frame_diagnostics_tests.cpp`
- `test/unit/app/foreground_readiness_tests.cpp`
- `test/support/benchmark_fixture.cpp`
- `test/support/benchmark_process_fixture.cpp`
- `test/support/sealed_export.h`
- `test/integration/inspect-sdl-delivery.cpp`
- `test/integration/renderer/game_report_probe.cpp`
- `test/integration/renderer/d3d12_frame_timing_probe.cpp`
- `test/integration/renderer/vulkan_frame_timing_probe.cpp`

**旧协议专属入口、分析器及其测试（14）**

- `scripts/benchmark.py`
- `scripts/benchmark.ps1`
- `test/integration/renderer/verify-d3d12-frame-timings.py`
- `test/integration/renderer/verify-vulkan-frame-timings.py`
- `test/integration/renderer/test_d3d12_frame_timings.py`
- `test/integration/renderer/test_vulkan_frame_timings.py`
- `test/integration/renderer/frame_timing_schema.py`
- `test/integration/renderer/vulkan_native_observation.py`
- `test/integration/renderer/verify-vulkan-async.py`
- `test/scripts/test_renderer_vulkan_async.py`
- `test/integration/renderer/verify-d3d12-budgets.py`
- `test/integration/renderer/verify-renderer-comparison.py`
- `test/integration/renderer/test_budget_scripts.py`
- `test/integration/renderer/test_foreground_comparison.py`

**旧产品当前架构说明（1）**

- `docs/architecture/benchmark.md`

确认后的配套编辑范围：移除这些文件的 CMake/公开头测试注册与旧产品安装导航，收窄 telemetry/platform/GL 实现清单及允许 owner；GL batch/Renderer 去掉 timer 参数与耗时/显存采集字段，但保留顶点/上传字节/draw 事实和全部实际绘制/截图。既有架构内容随交付更新，不复制成新的旧链文档。

### 2026-11-02 节点验收交接状态

用户明确要求按N1、N2、N3三个交付节点分别验收。此前按S任务汇报实现/检查进展，没有向用户逐节点提交验收包，已经提前开展部分N4集成检查；这些检查不代替用户验收。当前S1–S8的勾选只表示对应实施任务及所述检查已完成，**N1/N2/N3均未记为用户验收通过**，S9/S10仍不勾选。后续按N1→N2→N3交接，获得对应明确结论后再收口N4，不自动同步规格、归档或执行Git写操作。

用户已在准确54份清单之后明确回复“允许按准确54份清单删除并继续验证”。随后仅删除上述54份文件，连同此前另获准的两份document_io共56份；核对Git删除集合完全匹配，历史包、归档change、原工作区和Files::Publish实现未改。GL截图函数原义迁到legacy_renderer.cpp，修正残留timer调用参数；真实GL回读像素/上下方向及清理、普通120帧和shader/batch两项CPU回归通过。首次编译/链接暴露这两处迁移遗漏，修正后通过，不隐去失败。

#### N1 验收包：公共适配与构建（原候选，追加退役后重交）

2026-11-03 范围更新：下列构建/测试与操作保持原候选身份。用户要求追加退役 Files::Publish 后再验收 N1，原 N1 结论请求暂缓；新增3份准确清单及当前状态见本页末节，不从旧候选结果推定新候选验收。

交付范围为S3/S4及共享退役接线：非性能YAML由foundation所属独立symocraft_document_io唯一编译；基础foundation/独立physics不反向链接YAML或Tracy。Tracy固定0.14.1官方源码、唯一STATIC client及四个薄宏，默认OFF、关闭不求值；profiling保Release优化/匹配PDB，CPU-only局部关闭client/注释，旧benchmark开启缓存明确拒绝。实现入口：[foundation构建](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/game/modules/foundation/CMakeLists.txt)、[profiling构建](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/game/modules/profiling/CMakeLists.txt)、[宏头](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/game/modules/profiling/include/symocraft/profiling/profiling.h)、[presets](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/CMakePresets.json)、[来源许可](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/cmake/licenses/SOURCES.txt)。

最终普通构建使用新n9目录、MSVC14.51.36231/SDK10.0.28000.0，Tracy/marker OFF；D3D12、Vulkan、dual的Debug/Release六组合均完整构建成功，并在GL局部迁移后刷新构建。实际CTest结果如下，不称全绿：

| 配置 | 通过 | 未通过 | 禁用 |
| --- | ---: | --- | --- |
| D3D12 Debug | 108 | foundation.files超时，日志已捕获1175 | world.allocations 1项 |
| D3D12 Release | 109 | foundation.files超时 | 0 |
| Vulkan Debug | 110 | foundation.files失败，1175 | world.allocations 1项 |
| Vulkan Release | 111 | foundation.files超时 | 0 |
| dual Debug | 110 | foundation.files超时 | world.allocations 1项 |
| dual Release | 111 | foundation.files超时 | 0 |
| CPU-only Debug | 102 | foundation.files失败，1175 | world.allocations 1项 |

CPU-only新目录即使请求Tracy/marker ON，实际图仍不含client/SDL/platform/原生后端/旧产品实现。另建dual profiling Release、BUILD_TESTING OFF：完整构建/安装成功，CTest库存0，TracyClient.cpp实际编译owner为1，包带匹配PDB/原Tracy许可，不含旧工具、Viewer、RenderDoc、fixture、库或对象。两个后端均从中文空格包路径、无关CWD、系统PATH且移除SDK环境提示运行并正常退出；这仍是开发主机，不能称真实无SDK/干净机通过。普通Release另已安装，当前不以安装动作本身宣称额外运行检查通过。

**验收需知：** Files::Publish未修改；1175及超时保持未通过，暂停Fix1不恢复、不声称根因修复。文档审计另有38项未通过：37个既有问题/验证索引中的缺失out目标，及归档迁移元数据摘要不符；后者Git原始60字节摘要匹配登记，工作树63字节仅CRLF差异，未改冻结字节或全局换行配置。OpenSpec1.16严格校验仍因7份增量首个二级标题前的既有文字产生警告而非通过。以上都不由节点批准自动改绿。当前材料足以审阅交付与保留问题，未替用户接受这些风险。

##### N1 用户检查操作

本节点由用户审阅交付范围、可复查产物和保留问题后给出结论，不要求逐份复算哈希或手动重跑六种完整构建。以下命令使用已有候选，不重新配置/覆盖缓存；文档在主工作区阅读，代码/产物仍在上文列明的实施worktree。Viewer实时连线、CPU区域业务含义和RenderDoc捕获分别留N2/N3，不把它们列为本节点新增人工工作。

1. **确认交付边界。** 阅读本节交付范围及六组合结果表；按实现链接查看foundation目标的独立document_io、profiling目标的唯一STATIC client与普通/专用preset的开关。应看到基础foundation不反向链接document_io；独立物理不引入YAML/Tracy；普通Debug/Release的两个开关OFF，profiling显式ON且保Release优化/符号。CPU-only允许缓存显示请求ON，但配置作用域强制关闭，必须以实际target/编译图而非单个缓存值判定。
2. **检查安装目录。** 用资源管理器打开 `G:\Sandbox\Worktrees\integrate-renderdoc-tracy\Symocraft\out\install\n9-profile-dual`。应有SymoCraft.exe、SymoCraft.pdb、assets、licenses及app-local CRT DLL；licenses中有Tracy-LICENSE.txt。预编译shader在assets/shaders/d3d12和vulkan。应无旧Benchmark程序、Viewer、RenderDoc、fixture、测试exe、.obj或.lib；本包是profiling Release，PDB是有意交付。
3. **运行一次资产检查。** 在普通PowerShell执行：

   ```powershell
   $worktree = 'G:\Sandbox\Worktrees\integrate-renderdoc-tracy\Symocraft'
   $package = "$worktree\out\install\n9-profile-dual"
   & "$package\SymoCraft.exe" --check-assets --renderer d3d12
   $LASTEXITCODE
   ```

   预期输出 `All required assets are present.`、退出码0；不需要启动Viewer/RenderDoc。它只证明资产预检与这次启动退出，不替代已有图形/场景/干净机检查。
4. **可选：亲自复查三项N1自动测试。** 沿用上一步变量，在同一PowerShell执行：

   ```powershell
   $env:PATH = "$package;$env:PATH"
   & 'F:\PackageManagers\Scoop\apps\cmake\current\bin\ctest.exe' --test-dir "$worktree\out\build\n9-dual-release" --output-on-failure --no-tests=error -R '^(foundation.document|profiling.macros|profiling.disabled)$'
   ```

   预期3/3通过。这里使用安装包CRT补充当前终端的子进程搜索路径，不改系统PATH，关闭终端即结束。这三项复查覆盖YAML往返/错误与普通Release关闭态宏；开启态、缺依赖、唯一client、CPU-only/physics图隔离及六组合以本节已有实施方证据为准，不从3/3推定全矩阵全绿。
5. **给出验收结论。** 必须结合上方验收需知判断：foundation.files的1175/超时仍未通过；Debug禁用world.allocations不计通过；文档全库审计及OpenSpec严格警告尚未解决。可以给出“N1通过，保留已列问题”，也可以列出具体需修正/补证项并暂不通过。未收到明确结论前不登记N1通过，不以此自动批准N2/N3、整体收口、行为规格同步或归档。

#### N2 验收包：应用与CPU观测（待用户验收）

交付为S2、S5业务CPU部分、S7应用/telemetry部分：直接static/walk/edit有界场景，无输出目录或工具前提；walk→spawn、static/edit→four-chunk及45度初态，保原实际时间/0.25秒编辑与反向恢复；隔离玩法输入，窗口局部Esc/关闭取消，尺寸失效具名失败。Application不再拥有性能Session或导出，保持0/64/2/3分类及有界幂等清理。DebugSnapshot仅FPS/间隔/实际物理TPS/坐标/后端，自有值、常量存储，无新HUD/面板。

实现入口：[参数](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/game/app/src/startup_options.cpp)、[Application](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/game/app/src/application.cpp)、[快照](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/game/modules/telemetry/include/symocraft/telemetry/debug_snapshot.h)。依据包含前述参数/初态/隔离原生18项、正反编辑与50万次快照观测、应用装配/清理，以及本次最终Tracy开启态参数/场景/SimulationSession/快照4/4；关闭态CPU-only对应项亦通过。最终安装包D3D12/Vulkan各完成capture连接→断开→重连→断开，真实CPU区域/plots可读，游戏正常完成退出；另各执行无Viewer运行正常退出。支持环境未启用TRACY_NO_EXIT覆盖；不将合成事件或自动结果写成人工玩法验收。

##### N2 用户检查操作

以下步骤供N1取得明确结论后交接N2使用；当前不登记N2通过。N2/N3共享已确认的人工上限：每后端合计不超过5分钟、两后端总计不超过10分钟，不按节点或场景重复追加。自动结果已列在上文，用户不需重跑全部夹具或审阅工程日志；达到上限时记录未完成项，不默认延长。

1. **确认场景与普通运行的边界。** 本轮只有数值快照接口，没有新HUD/面板；看不到FPS面板不是缺失交付。直接场景应无需输出目录，保留实际世界、视点和编辑动作。可以在普通PowerShell对已有包作一次短复查：

   ```powershell
   $worktree = 'G:\Sandbox\Worktrees\integrate-renderdoc-tracy\Symocraft'
   $package = "$worktree\out\install\n9-profile-dual"
   & "$package\SymoCraft.exe" --renderer d3d12 --profile-scenario walk --scenario-seconds 5
   $LASTEXITCODE
   ```

   预期完整世界沿脚本运行，5秒后打印场景completed和正常shutdown、退出0。若亲自检查取消，可另启动同一场景并按Esc；预期打印cancelled、正常退出0，不冒称完成。Vulkan使用同一包、将renderer改为vulkan，不更换资产；既有自动检查已覆盖两后端，亲自复查仍共用上述时限。
2. **查看真实CPU轨迹。** 现成轨迹来自上文同一profiling安装候选，可使用匹配Viewer打开：

   ```powershell
   & "$worktree\out\tools\tracy-viewer-0.14.1\tracy-profiler.exe" "$worktree\out\runtime\n9-tracy-install-001\d3d12-connected.tracy"
   ```

   在帧/CPU区域中查看app.frame、simulation.advance、world.mesh及renderer/cache/upload和对应后端区域，Plots查看实际物理步、重建和draw计数。Present区域表示CPU调用/等待，不是GPU执行时间。另一后端的现成文件为同目录vulkan-connected.tracy；断开/重连和无Viewer正常退出已有实施方证据，不要求为验收再次采集整轮。轨迹位于out工作存储，不作为长期附件。
3. **给出N2结论。** 结合交付范围、已有自动依据与实际观察，明确通过且保留哪些问题，或列出需修正/补证项。参数/输入/编辑、快照精确数值与常量存储以正式库自动测试为依据，短操作不代替长期稳定性或旧性能预算；N2结论不自动批准N3或整体收口。

#### N3 验收包：渲染观测（待用户验收）

交付为S6、S5渲染CPU部分、S7渲染部分及受影响消费者迁移：两现代原生资源名/成对事件、独立注释开关，Vulkan注释缺能力只降为无注释、validation独立；GameRenderer/截图与旧Session解耦，旧GPU计时query/队列已移除，提交/呈现fence、资源退休、设备/提交/超时事实及截图readback保留，GL继续作为受影响回归路径。

最终工具候选为dual profiling Release安装exe，SHA256 `eebedcb0511ce6c5e64e18b8a54db175f8bad79705ab1031560008790926ee33`。RenderDoc1.47对D3D12 static/walk/edit捕获frame677/1291/901、Vulkan frame818/834/754，六份均实际回放成功；核对nVidia/对应API/非degraded、真实world draw与post-VS网格、64×64×64层atlas、适用24顶点selection及world/resolve事件和资源名。六份回放PNG已查看，未发现明显几何/层号/方向错误，不称逐像素等价或用户视觉批准。实际1920×1080、4x、Release优化、validation-on，Tracy/marker开启，工具成本不与旧预算直接比较。

当前检查还包括同一生产库的26项完整原生批：GL回读/普通循环、双后端drawing/world-window及应用生命周期、D3D12四种故障与在途寿命、Vulkan十三种寿命/故障，全部exit0；迁移后的报告校验器通过。最终刷新后D3D12 drawing/GL再次通过，world-window曾在恢复后失去前台焦点而失败；保留该失败，经桌面工具聚焦自有窗口后独立有界复查，报告成功且父进程保存exit0，完整像素恢复/输入断言未放宽。Vulkan六种能力/WSI分支也以最终构建全部通过。shader/image故障要求实际GL初始化，坏配置按CPU预检早于窗口拒绝；纠正旧脚本前提后3/3真实检查通过，共享脚本38/38、developer19/19。没有用删除有效测试使结果变绿。

操作与复核入口：[工具说明](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/scripts/README.md#profiling-build)、[renderer机制](../../../architecture/renderer.md)、[app机制](../../../architecture/app.md)。原始工具轨迹、回放图片和临时检查只存out工作存储，本记录不新增附件链。尚未获得N1/N2/N3人工验收，也未补填人工框、恢复长测/其他机器采样或执行整体验收、行为规格同步与归档；三份主规格只修复已删除源码/架构的导航目标，未同步本change的行为增量。

##### N3 用户检查操作

N2取得明确结论后交接本节点。沿用N2/N3共享的人工时限；可直接审阅已捕获候选，避免为读取结果重复启动采集。打开现有工具 `G:\Sandbox\Worktrees\integrate-renderdoc-tracy\Symocraft\out\tools\renderdoc-1.47\RenderDoc_1.47_64\qrenderdoc.exe`，通过Open Capture依次选择实施工作树的 `out\runtime\renderdoc-003` 下文件：

| 后端 | static | walk | edit |
| --- | --- | --- | --- |
| D3D12 | d3d12-static_frame677.rdc | d3d12-walk_frame1291.rdc | d3d12-edit_frame901.rdc |
| Vulkan | vulkan-static_frame818.rdc | vulkan-walk_frame834.rdc | vulkan-edit_frame754.rdc |

在Event Browser选择world中的实际draw，检查Texture Viewer的atlas及适用层、Mesh Viewer的真实网格和颜色输出，确认对应场景没有明显缺几何、错材质或翻转。static/edit捕获有selection事件，walk本次视线未选中方块，缺少selection不是伪造空工作组的理由；world和resolve.msaa及持久mesh/atlas/present资源名应可定位。两API必须分别回放，不能以文件存在或另一API代替。

关闭当前捕获即可结束本次只读复查，不需要全局注入、改loader注册或重新构建。新捕获的启动参数、工作目录和可选Vulkan子进程层路径见[工具操作](G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft/scripts/README.md#profiling-build)。未完成、异常和时限到达均如实反馈；明确给出N3通过并保留问题，或列出修正/补证项。工具画面观察不构成逐像素容差、正式性能预算或整体用户批准。

### 主工作区文档审阅位置

2026-11-02 用户要求验收文档在主工作区审阅；本change的proposal、七份delta specs、验证记录及配套架构/操作说明已同步至 `G:/Projects/Symocraft`，保留该处原有N1记录和其他change。N1/N2/N3验收结论仍待用户明确给出，S9/S10未收口。源码、构建/运行包、主工作区现役行为规格、其他change、个人文件与冻结材料不随本次文档同步迁移；文档中的本轮源码链接指向实际实施工作树，不能把主工作区的旧代码当作本候选。主工作区保留的旧benchmark文档/代码待另行代码集成处理，本次没有执行文件删除或OpenSpec行为同步/归档/Git写操作。

### 2026-11-03 规格依据补齐与待验收状态

本轮对已安装普通Release双后端包补做独立运行核验：exe SHA256 `9c16418d400b7b08e8c15712cd2ae56ca3a6ad93d10aeda0f10ae2fdb16c3d38`，Tracy/marker均OFF。包内无Tracy许可/PDB、Viewer、RenderDoc或旧产品/测试/库/对象；在中文无关CWD、仅系统PATH且移除SDK环境提示的子进程中，D3D12/Vulkan static各1秒正常完成、退出0，实际设备均为RTX5070Ti。关闭态额外设置TRACY_NO_EXIT=1仍正常退出；该变量不扩展开启态支持边界。相同seed424242、regression/four-chunk/test-edits的普通包与既有profiling包world-summary原始stdout逐字节一致，四次运行均exit0，CWD未产生游戏输出文件。此对照只证明该固定输入的摘要及关闭，不推定任意工作负载、正式预算或干净机部署通过。

S9对原能力继续有效契约的对应依据如下，沿用本记录已说明的具体候选和CPU/原生范围，不继承历史成绩：

| 能力增量 | 本轮依据 | 保留限制 |
| --- | --- | --- |
| application-lifecycle | 参数/初态/场景18项，真实装配失败与幂等关闭probe，普通/专用安装包正常退出 | 合成输入不代替用户玩法批准；任意OOM不保证完整诊断 |
| telemetry-lifecycle | 正式库快照测试含缺值、零/多物理步、暂停/恢复、自有旧值及50万次常量存储，正反编辑与开关对照 | 无新面板，无逐帧文件；固定输入对照不覆盖任意运行 |
| platform | 本窗口取消/失焦/尺寸probe及真实world-window恢复；公开头/CPU-only图隔离 | 前台焦点受限失败及聚焦复查均保留；开发主机不是干净机 |
| renderer | 自有帧/截图门面测试，26项原生批、六种Vulkan WSI分支、双API六份真实回放与GL受影响回归 | 注入故障与自发故障分列；旧GPU计时和预算仅适用于历史候选 |
| module-boundaries | 六组合构建/边界、独立头、缺依赖及开关、唯一client、CPU-only/关闭测试和安装库存 | Debug禁用扫描不计通过；不以CMake配置代替运行 |
| foundation-files | 文档往返/错误与保留的正式文件测试，非性能YAML消费者迁移 | foundation.files仍有1175/超时；Files::Publish未改，Fix1继续暂停 |
| development-profiling | 同版Tracy连接/断开/重连/无Viewer退出，六份RenderDoc实际回放，准确54份删除及历史保护核对 | 工具成本不同于旧CSV，未恢复长测/其他机器，也未取得用户节点验收 |

N2/N3只补齐现有交付的用户检查入口，共享此前人工上限，不新增实施范围或另一任务清单。当前仍为8/10，S9/S10未勾选；本轮已向用户提交N1结论请求，等待明确答复后按N2/N3交接。文件发布未通过、文档审计问题及OpenSpec严格警告保持原状态，不借补充记录接受风险或补填批准。本轮新增记录位于本实施工作树，未再次传播主工作区文档副本。

## 2026-11-03 Publish 追加退役范围与准确清单

用户确认先更新规格，将 Files::Publish 及其专属测试纳入退役，保留历史故障与 Fix1 记录，调整后再做 N1 验收。生产调用复核：旧 benchmark/Performance::Session 移除后，game 中只剩 Publish 自身实现，调用仅见专属单元测试和暂停的 Fix1 mapping_probe；独立 YAML Load/Dump、world-summary 和 PNG 输出不调用它。目标增量见 [foundation-files](specs/foundation-files/spec.md)，本节不是实际删除或 N1 批准。

### 新增准确3份删除清单（已确认并删除，验证结果见后文）

全部路径相对本实施工作树 `G:/Sandbox/Worktrees/integrate-renderdoc-tracy/Symocraft`。本清单独立于已批准并执行的原54份及两份 document_io，不扩大旧删除授权；本次列出时3份均仍存在。

| 序号 | 准确路径 | 退役理由 |
| --- | --- | --- |
| 1 | `game/modules/foundation/include/symocraft/foundation/files.h` | 无生产调用方的 Files::Publish 公开 API |
| 2 | `game/modules/foundation/src/files.cpp` | 仅实现上述 API 的 Windows 发布代码 |
| 3 | `test/unit/foundation/files_tests.cpp` | 仅验证上述 API 的正式库专属测试 |

配套实施仅修改现有 `game/modules/foundation/CMakeLists.txt` 的 `src/files.cpp` owner 和 `test/unit/foundation/CMakeLists.txt` 的 files_tests/foundation.files 注册，核对公开头自动消费者及安装图不再包含退役入口；这些是接线编辑，不是新增文件删除。当前接线仍保留，本轮尚未实施，也未仅禁用失败测试来制造通过。

**明确保留：** `test/support/publication_diagnostics.h` 仍被 Fix1 mapping_probe 使用；`test/experimental/publication-fix1/` 全部材料、历史运行包/失败/日志/冻结证据、旧缓存及原工作区均保持。Fix1 文档是否作为取消计划归档单独依用户决定处理，移动归档不删除实验现场，不把原14项未完成任务打勾。未来重用 Publish 需独立设计和授权，不要求新游戏编译旧探针来维持历史运行兼容。

**验收状态：** S8 因新增范围重新打开，当前7/10，S8–S10未完成。原7组 CTest 的 foundation.files 1175/超时和 Debug world.allocations 禁用仍按原候选保留，未改成通过；原文档审计38项与严格校验7份前置文字警告也未由规格修改消除。本轮仅调整目标规格与清单，不改现役行为主规格或生产代码，不运行新游戏测试、不登记人工验收。实际退役后须核对新候选的生产/公开头/构建/CTest/安装图与有效非性能文件行为，再重新提交 N1。

本次规格检查：apply 重新读取为7/10；strict validate 无 ERROR，仍有7份增量首个二级标题前文字警告，因此严格检查未通过。全库文档审计仍为38项错误，本 change 未新增链接错误；`git diff --check` 通过。Fix1 归档输入核对为 symocraft-full，规划产物齐全但功能任务仅1/15；其3条 ADDED requirement（全阶段诊断、因果可靠性、终态有界验收）均非现役已实现保证，若取消归档应跳过合并。已向用户单独提出“14项未完成仍归档且不合并规格”和“准确新增3份删除”确认，收到前均不实施；原 Process Monitor/采样暂停继续生效。

### 本轮明确授权与实施

随后用户分别明确选择“确认，取消计划并归档，不合并规格”和“允许按准确3份清单删除并继续验证”。已通过项目入口将 Fix1 归档至 [2026-11-03-m3-t2-publication-fix1](../archive/2026-11-03-m3-t2-publication-fix1/verification.md#2026-11-03-因能力退役取消计划并归档)，`specsUpdated=false`；任务仍1/15，14项未完成未勾选。移动时6份原文件大小/SHA256一致；之后仅修正该归档的导航、追加取消状态及更新当前入链，原任务/命令/结果含义不变，旧冻结归档未改。

准确新增3份已删除，foundation owner 与 foundation.files/专属测试注册已移除，未引入兼容壳、仅禁用失败用例或替代发布实现。当前产品删除共59份（原54+2+本次3）；Fix1另有6份原文件只是完整移动到归档，不计为产品清理。暂停诊断目录、辅助头及其余旧归档快照已核对保持，所有现役主规格的行为 Requirement blocks 未在本轮合并退役或 Fix1 增量；其中两页只更新退役源码/归档导航及注明待同步。

新的验证使用独立 `n11-*` 构建/安装与运行目录，保留全部 n9/n10 候选、缓存及失败记录。首次普通权限初始化 Visual Studio 返回拒绝访问，尚未进入配置/编译；经构建权限继续后实际配置/编译正常，此启动失败不当作代码回归或隐藏。最终结果与新 N1 入口在后文补齐，完成前不登记人工验收。


### 退役后最终自动检查（n11 候选）

工具链保持 MSVC 14.51.36231、Windows SDK 10.0.28000.0、Vulkan SDK 1.4.363.0，全部新建 n11 构建目录；不复用旧候选成绩。六种普通游戏组合、CPU-only Debug 和测试关闭的 profiling 双后端 Release 均配置/编译成功。普通开关 OFF；profiling 两开关 ON；CPU-only 即使请求 ON 也无 Tracy/native target。

| 新配置 | 启用通过 | 失败 | 禁用 |
| --- | --- | --- | --- |
| cpu-debug | 101 | 0 | world.allocations 1项 |
| d3d12-debug | 107 | 0 | world.allocations 1项 |
| d3d12-release | 108 | 0 | 0 |
| dual-debug | 109 | 0 | world.allocations 1项 |
| dual-release | 110 | 0 | 0 |
| vulkan-debug | 109 | 0 | world.allocations 1项 |
| vulkan-release | 110 | 0 | 0 |

合计754项启用通过、0失败，4项既有 Debug world.allocations 禁用不计通过；profiling 关闭测试配置为0项注册。每个启用测试的配置与各自 n9 原清单严格对照，仅减少退役的 foundation.files 及其公开头独立消费者两个注册，未删除/禁用其他用例。foundation.document（含 YAML 往返、类型/精度、畸形/缺失文件）、内存、图像读写、场景及边界检查均有本候选实际结果。

生产源码/编译命令、公开头消费者、目标图、CTest 和安装库存均无 Files::Publish 实现/入口或专属测试；两种新双后端 Release 的 foundation 正式库中也无 Publish 符号。profiling 仅一份 TracyClient.cpp 编译、部署同次生成的 PDB/许可；普通包没有 Tracy/旧工具，两个安装包均无 Viewer、RenderDoc、benchmark、fixture 或测试二进制。

普通与 profiling 新安装包各自从空中文无关 CWD、仅系统 PATH 且移除 SDK/开发环境变量运行：两 API 资产预检、两 API 各1秒 static 场景和固定输入 world-summary，共10次均退出0，实际适配器为 RTX 5070 Ti，正常 shutdown。普通包子进程设置 TRACY_NO_EXIT=1 仍退出；profiling 子进程不启用该不支持覆盖。两包 world-summary 原始 stdout 字节一致，并与先前 n10 对照输出一致；CWD 无旧性能文件或其他输出。此处是自动短启动/关闭与非性能文件检查，不冒称新一轮 RenderDoc/Viewer 人工验收、长测或干净机部署。

新普通安装目录 `out/install/n11-dual`；新 profiling 安装目录 `out/install/n11-profile`。exe SHA256 分别为 `fc2de0d6a36d58e1cb5b112e0813efb2b42dca61134eca13b9ba6df16f1b3fd7`、`269b932b257de28c7d7d88c318ab6de43a24fd6f799238103d740ce79bf6f88d`。n9 两个安装候选 exe 摘要均核对未变，历史1175/超时、失败/禁用及原工具捕获继续保持其原候选身份；移除能力不证明历史故障修复。Fix1 现场/辅助头、旧冻结归档和未授权删除集合未变；现役主规格仅调整导航，未合并行为增量。

文档检查仍未全绿：strict validate 有7份既有前置文字警告、无 ERROR；全库审计36项错误（35个既有 out 目标缺失及1个旧归档元数据摘要差异）。本轮将4个已退役源码链接改为准确退役记录导航，其中2项原有失效入链得以修正；未改旧包或补造文件来消除错误。本 change 的链接和 `git diff --check` 通过。具体原始检查只存 out 工作目录，本页不新增附件。

### N1 重新交接（n11 原入口，已由下方 Debug 补测交接替代）

以下替代前文 n9 原候选的 N1 请求。N1 的独立 YAML 适配、薄宏/唯一 client、默认关闭/缺依赖隔离、CPU-only、安装及新增 Publish 退役已具备实际依据；S8 已按准确清单完成并复核。用户只需审阅上述交付和保留限制，不需要手动重跑完整矩阵。

可选快速复查：打开 `G:\Sandbox\Worktrees\integrate-renderdoc-tracy\Symocraft\out\install\n11-profile`，应见游戏、匹配 PDB、assets、licenses/Tracy-LICENSE.txt 和 app-local CRT，无旧 benchmark、Viewer、RenderDoc 或测试程序。普通 PowerShell 可执行以下资产预检，无需连接 Viewer 或启动捕获：

```powershell
$n1Package = 'G:\Sandbox\Worktrees\integrate-renderdoc-tracy\Symocraft\out\install\n11-profile'
& "$n1Package\SymoCraft.exe" --check-assets --renderer d3d12
$LASTEXITCODE
```

预期 `All required assets are present.`、退出0。需要复查自动项时，使用 `out/build/n11-dual-release` 的 foundation.document、profiling.macros、profiling.disabled；完整范围以本节实际结果为准，不从三项推定其余通过。

当前重新提交 N1，尚未登记用户通过；用户可明确“N1通过，保留已列历史及文档限制”，或提出需要修正的具体项。N2/N3 的 Viewer/RenderDoc 检查及用户结论仍按原节点与共享人工上限分别处理，本轮未重做工具捕获或继承原帧结果为 n11 已验收。当前8/10，S9/S10保持未勾选；本 change 未 sync/archive，也无 Git 写操作。

### Debug world.allocations 注册修正与补测

2026-11-03 用户追问原4项禁用的含义后，核对发现它们是同一个 `world.allocations` 在4种 Debug 配置中的注册。此前仅称“原有禁用”未识别出判断过时，现予纠正：根 `cmake/MsvcAbi.cmake` 已通过目录编译定义对所有 Debug C++ 统一 `_ITERATOR_DEBUG_LEVEL=0`，旧测试条件却只查看 `CMAKE_CXX_FLAGS`/`CMAKE_CXX_FLAGS_DEBUG`，因此错误禁用。用户明确同意修正条件、补跑四种 Debug、更新记录后再提交 N1；前次 N1 请求由本节后的新交接替代，不视为用户已接受。

仅移除 `test/unit/world/CMakeLists.txt` 中过时的 flags 字符串判断，沿用根 DBG01 的统一配置，不改测试实现、故障注入、断言或240秒超时，也不改变生产 ABI/行为。该用例检查配置解析、World/Chunk/生成器创建及网格 scratch/输出分配失败后的资源释放、残缺网格不发布、同一 mesher 恢复和另一世界不受污染；它继续作为正确性测试保留。

保留 n11 全量结果，在相同四个 Debug 构建目录重新配置并构建 `world_allocation_tests` 的必要依赖，然后只运行 `ctest -R '^world\.allocations$' --output-on-failure --no-tests=error`。工具链/SDK未变，不需要变更 ABI 或新建全闭包目录。补测工作日志/JUnit/前后库存只存 `out/runtime/n12-world-allocations-001/`，未覆盖旧日志或增加文档附件。

| Debug 配置 | 补测通过/执行 | 失败/禁用 | 生成 C++ 命令统一 level 0 | 实际测试时长 |
| --- | --- | --- | --- | --- |
| D3D12 | 1/1 | 0/0 | 162条 | 3.85秒 |
| Vulkan | 1/1 | 0/0 | 162条 | 4.20秒 |
| 双后端 | 1/1 | 0/0 | 169条 | 4.82秒 |
| CPU-only | 1/1 | 0/0 | 138条 | 3.37秒 |

四个测试均真实执行并退出0，未跳过；各自输出均确认 definition=1747、factory=57、generator/noise=1、scratch/output=28 的故障扫描到达成功。生成命令数量不是本次重新编译数量：目标与依赖构建均无需重新编译，四种配置测试 exe 和 world 正式库的前后 SHA256 各自一致。CTest 前后测试名称集合完全相同，除该用例的 DISABLED 属性被移除外，其余用例注册/属性不变。全闭包生成的 C++ 命令均有 level 0，测试继续使用 Debug CRT。

**当前累计依据为758项启用通过、0失败、0禁用：前一批相同二进制的754项完整检查，加本次4项定向补测。** 这不是重新执行一次758项全量 CTest；注册修正不影响其他生产/测试二进制，未据此重复全部矩阵、安装运行或 GPU 捕获。n11 的安装包/运行身份保持，原754+4禁用及更早1175/超时记录保留各自点时含义。旧普通 Debug level 2 的终止、持续/系统 OOM、未知分配路径或完整内存安全未被本次补测证明修复。

### N1 补测后重新提交（已于2026-11-03获用户通过）

N1 当前交付在前文 n11 公共适配/构建/安装与 Publish 退役的基础上，已修正并补齐四种 Debug 的 world.allocations；此前4项禁用不再作为当前待接受限制。需要复查时，在原任一 `out/build/n11-*-debug` 目录只运行该用例即可；新的实际结果见上一节，不要求用户手动重跑全部检查。

仍保留全库文档审计36项既有错误与 OpenSpec strict 的7项既有警告，历史故障及 Fix1 取消归档不改判修复。本轮 `git diff --check` 与受影响文档链接复核通过，无现役行为规格合并、Git 写操作或人工批准。当前8/10，S9/S10仍待节点结论及后续收口；重新提出 N1 验收请求，未登记通过，不自动批准 N2/N3 或整个 change。

### 2026-11-03 N1 用户明确通过

用户针对上述补测后交付明确回复：“N1通过，保留已列历史及文档限制”。据此登记 N1 公共适配与构建通过，依据为 n11 候选及同二进制四种 Debug world.allocations 补测；758项累计通过的分批口径、文档36项错误/strict 7项警告、历史1175/超时及 Fix1 未修复等限制保持。前文“待N1明确验收”是该决定前的历史状态，不覆盖本次用户结论。

本次仅为用户要求的 N1 节点结论，不等于整个 change 的阶段整体验收，不新增全局关键节点批准或复制验证正文。N2/N3尚待分别交接与用户结论，S9/S10保持未勾选；无整体完成、行为规格同步、归档、Git写操作或额外人工预算授权。

### N2 当前候选交接（待用户明确验收）

N1 通过后，将 N2 的工具依据刷新至当前 `out/install/n11-profile/SymoCraft.exe`，SHA256 `269b932b257de28c7d7d88c318ab6de43a24fd6f799238103d740ce79bf6f88d`，不把原 n9 轨迹直接改记为当前包结果。N2 交付范围仍为前文场景/应用、轻量快照及 CPU 观测；n11 正式库自动检查覆盖参数、初态、场景/编辑、快照及模拟，n12 补测仅修正 world.allocations 注册，不改变这些二进制或行为。无新 HUD/面板、GPU 时间线或自有性能导出。

本次在原已确认范围内，用匹配 Tracy 0.14.1 的 capture/csvexport，经127.0.0.1分别对两后端执行16秒 edit 场景：连接采集3秒→断开→重新连接采集3秒→断开，场景完成且正常退出0；另各执行2秒 walk、完全不运行 Viewer/capture，正常退出0。四份轨迹均能真实导出 app.frame、simulation.advance、world.mesh、renderer.cache_upload_cpu、各自 prepare/submit/present_call CPU 区域及 physics.steps、mesh.rebuilt、renderer.draw 计数。Vulkan 本轮关闭日志 validation warnings/errors 均0；Present区域仍只表示CPU调用/等待，不冒称GPU执行时间。

使用当前安装包、空中文无关 CWD、仅系统 PATH 且移除 SDK 开发环境变量，未启用 TRACY_NO_EXIT 覆盖；运行后 CWD 无旧性能文件。新轨迹及临时导出位于 `out/runtime/n13-tracy-n2-002/`，不新增长期附件或第二验证报告，不覆盖 n9/n11/n12 记录。该目录的 `d3d12-connected.tracy`、`d3d12-reconnected.tracy`、`vulkan-connected.tracy`、`vulkan-reconnected.tracy` 均成功采集和读取。

保留失败：首次普通权限批 `n13-tracy-n2-001` 的 D3D12 capture 在15秒连接超时，未取得可用轨迹，夹具清理了自己启动的游戏进程；该轮不计正常退出或通过。使用相同安装包/参数，在已授权采集权限下的新编号002批成功；未修改防火墙、全局环境、vendor或业务代码，未查明首次超时的底层原因，不将权限复查成功当作已定位或修复。无需因此重做已通过的 N1。

**用户复核入口：** 下列以当前包/轨迹替代前文 N2 的 n9 操作路径，不要求再次采集。N2/N3仍共享每后端最多5分钟、总计最多10分钟的已确认人工上限，不按节点重新计算或自动延长。可选直接运行一次5秒 walk，预期 completed、shutdown和退出0；若复查Esc取消，预期 cancelled、正常退出0。

```powershell
$worktree = 'G:\Sandbox\Worktrees\integrate-renderdoc-tracy\Symocraft'
& "$worktree\out\install\n11-profile\SymoCraft.exe" --renderer d3d12 --profile-scenario walk --scenario-seconds 5
$LASTEXITCODE
& 'F:\Toolchains\Tracy\tracy-profiler.exe' "$worktree\out\runtime\n13-tracy-n2-002\d3d12-connected.tracy"
```

另一后端改用 `--renderer vulkan`，轨迹查看 `vulkan-connected.tracy`。在 Viewer 的 Frames/Zones/Plots 查看真实应用、模拟、网格与后端 CPU 区域及计数，未经过的工作不应伪造；自动导出核对不替代用户对可读性/交付范围的结论。快照精确值/存储及输入故障等以既有正式库检查为依据，不要求短人工操作重现全部夹具。

现提交 N2，尚未登记通过。用户可明确“N2通过，保留已列历史、文档及首次连接超时限制”，或提出具体需修正项。此前文档36项错误/strict 7项警告、历史故障、未测性能/长时/其他机器范围继续保留；N3及整体收口另待结论，任务仍8/10，无行为规格同步、归档或Git操作。

### 外部工具位置以 Dev 盘 Toolchains 为准

2026-11-03 用户指定使用 Dev 盘已安装工具。当前操作入口改为 `F:\Toolchains\Tracy\tracy-profiler.exe`，采集/导出使用同目录 `tracy-capture.exe`/`tracy-csvexport.exe`；RenderDoc 使用 `F:\Toolchains\RenderDoc\qrenderdoc.exe`，命令行工具、`renderdoc.dll` 和 `renderdoc.json` 位于同目录。后续 N2/N3 操作以这些路径为准，前文 out/tools 路径仅保留当时的执行/复核身份，不批量改写历史命令，也不删除旧工具。

核对 RenderDoc 可执行文件/DLL 版本为1.47.0.0；其 qrenderdoc/renderdoccmd/renderdoc.dll 三份以及 Tracy Viewer/capture/csvexport 三份 SHA256 均分别与此前实际验证的1.47/0.14.1工具一致。Tracy exe 没有版本资源，通过与已知固定版本逐字节一致确认身份，未据空元数据猜测版本。游戏内 vendored client、开关、依赖和安装包不因此变更；不需要更换编译工具链或重建游戏。RenderDoc 的 Vulkan manifest 使用同目录相对 `renderdoc.dll`，若启动需进程级 layer 路径则指向 `F:\Toolchains\RenderDoc`，不修改全局注册/环境。

当前 N2 仍由用户查看轨迹及操作后再给结论，不因切换工具位置登记通过。人工时长、已列限制与 N3/整体待验收状态保持。

### N2 轨迹 Frame 540–580 的现场核对

用户报告 d3d12-connected.tracy 的 Frame 540–580 draw 为0。本次核对当前打开的 n13-002 采集时间身份、原始逐样本导出及 Viewer 视图：1384个 renderer.draw 样本全部为361或362，零值样本数为0。图表自动缩放后 draw 的纵轴最低是361；其下方 renderer.upload 才有0，不能将两条曲线或图底等同于零次绘制。

已用 Viewer 显示帧号对齐：Frame540起点2.589609872秒，Frame580起点2.676297538秒；该区间含41个 draw 样本，每帧均为362，renderer.upload均288字节。Viewer 当前采集内跳帧输入255/295对应显示540/580，含采集前285帧偏移，未把导出行号直接当显示帧号。对应 app.frame、prepare/submit/present_call CPU 区域存在，没有观察到该区间绘制停止。

真正低谷是显示 Frame1005–1153，149个样本 draw=361、renderer.upload=0；首/末样本为3.651513727/3.895900894秒，3.902497476秒恢复362/288。D3D12 正式实现对非空世界网格逐项计 draw，选框另加1次draw，并为24个12字节的线顶点计288字节upload。当前后端该 upload 统计只在选框上传处累加，世界网格更新另见 mesh.upload；0不表示世界渲染或全部上传停止。

按采样参数/场景日志及生产脚本重建：本次为 D3D12 edit 场景，seed424242、regression/four-chunk，初始位置(-2.5,161.9,-2.5)、yaw45/pitch-20.4/FOV45，1920×1080、4x MSAA、validation-on；脚本每0.25秒编辑固定方块且不接收玩家移动。低谷与脚本在场景2.25秒移除(-1,161,-1)石块、2.50秒放入橡木板的阶段对应：世界仍361次draw，射线无选中目标时暂不提交选框，因此少1次draw及288字节选框上传，随后恢复。此段两端均有3个网格重建、约668KB世界网格上传，与每0.25秒的编辑节奏吻合。

以上计数/时间来自原轨迹，画面状态是结合源码与脚本的逻辑还原；该Tracy采集没有逐帧图像，不能冒称同一历史帧的截图或RenderDoc回放。未改原轨迹、产品代码、采样口径或批准；调查工作只存 out/runtime/n14-draw-analysis-001，N2仍待用户结论。

### 2026-11-03 N2 用户明确通过

用户在查看轨迹/操作及 Frame540–580 现场核对后明确回复：“明白了，N2通过，继续推进N3验收”。据此登记 N2 应用与 CPU 观测通过，依据为当前 n11 安装候选、n13-002 两后端实际轨迹/退出及既有正式库检查；n13-001 首次连接超时、文档36项错误/strict 7项警告、历史故障和未测范围继续保留，不因批准改变其点时结果。

开始 N3 渲染观测交接；N3及整体 change 尚未批准，S9/S10仍未勾选。沿用此前已确认的双现代后端/三场景捕获回放范围及 N2/N3 共享人工上限，不增加新后端、长测或其他机器采样，不执行行为规格同步、归档或 Git 写操作。

### N3 当前候选捕获与回放（已于2026-11-03获用户通过）

使用用户指定的 `F:\Toolchains\RenderDoc` 1.47（renderdoccmd报告完整来源 `b0e1370309f7d7bf986a73830446ca39d9b8c1fd`），对当前已通过N1/N2的 `out/install/n11-profile/SymoCraft.exe` 刷新六个捕获。游戏 SHA256 仍为 `269b932b257de28c7d7d88c318ab6de43a24fd6f799238103d740ce79bf6f88d`，旧 renderdoc-003 捕获不改名/覆盖，也不改记为当前包结果。每场景使用原10秒有界入口，validation-on；Vulkan只为目标子进程设置 `VK_ADD_IMPLICIT_LAYER_PATH=F:\Toolchains\RenderDoc`，无全局hook/子进程hook或loader注册修改。

六份新文件均位于实施工作树 `out/runtime/n15-renderdoc-001/`，各自实际打开并回放成功，而非仅核对文件存在：

| 后端 | static | walk | edit |
| --- | --- | --- | --- |
| D3D12 | d3d12-static_frame611.rdc | d3d12-walk_frame1134.rdc | d3d12-edit_frame763.rdc |
| Vulkan | vulkan-static_frame662.rdc | vulkan-walk_frame650.rdc | vulkan-edit_frame665.rdc |

六次回放均为对应API、nVidia且非degraded，world内有361次非空绘制；首个实际world draw的post-VS输出26010顶点且能读取真实buffer。World.Mesh/Buffer.mesh、World.Atlas及Present.Color逻辑资源名存在，world与resolve.msaa事件可定位；atlas为64×64×64层，实际绑定纹理层可读且有有效内容。static/edit另有24顶点selection draw，walk这次视线未选中目标，不创建伪selection空组。六张真实颜色输出PNG均1920×1080并已查看，场景与视点相符，未发现明显缺几何、错材质或翻转；不声称逐像素等价、用户视觉批准、性能预算或完整GPU时间线。

本次只刷新外部捕获/实际回放及可读性依据，不改生产源码或重做已通过的构建/CPU/故障矩阵。原GL回读、窗口/输入/寿命/故障依据及限制保留原候选身份，不将新捕获成功当作它们被重复执行。新RDC、PNG及最小检查仅存out工作目录，本页不新增附件链；新源包/安装身份、源码行为与N1/N2已验收内容保持。

保留失败：最初隐藏后台 `qrenderdoc --python` 尝试180秒内未生成脚本结果，被启动夹具超时清理，不能算已采集/回放通过。之后显式打开安装的GUI，载入使用明确绝对路径的脚本，在官方Python Scripting面板执行，完成全部六场景检查。脚本包装已去除对GUI上下文 `__file__` 的依赖；后台入口的具体原因未得到独立验证，不从GUI成功倒推首次尝试通过或工具所有启动方式可靠。未修改安全/隐私设置、驱动或工具vendor。

### N3 用户复核入口与结论

本节替代前文N3的旧安装候选/工具路径。打开 `F:\Toolchains\RenderDoc\qrenderdoc.exe`，用 **File → Open Capture** 选择上述新目录中的捕获。N2/N3仍共用已确认的每后端最多5分钟、总计最多10分钟人工上限，不重置预算、不要求重新采集；达到上限时列未完成项，不自动延长。

1. **画面和场景。** 先看帧末颜色输出：static应是four-chunk石块与选框；edit本次前方块已换为木板且选框可见；walk为移动后的地面/远处石块且本帧无选框。分别检查D3D12/Vulkan，没有明显缺块、错纹理或上下翻转。相应PNG可作为快速画面入口，不能代替RDC回放。
2. **真实事件与数据。** 在Event Browser展开world，选实际draw，在Mesh Viewer查看网格；在Texture Viewer查看World.Atlas的适用数组层，确认64×64、64层。核对world、resolve.msaa事件及World.Mesh/Buffer.mesh、World.Atlas、Present.Color资源名；static/edit可查看selection的24个线顶点，walk缺selection不表示世界未绘制。
3. **明确结论。** 结合已列自动依据与自己的观察，给出“N3通过，保留已列限制”，或列出具体异常/未完成项；不需要重跑完整自动测试或恢复历史长测。截图不是GPU耗时证据，注入捕获成本不与旧CSV预算直接比较。

当前N1/N2已通过，N3尚未登记通过；文档36项既有错误/strict 7项既有警告、历史失败与未测范围继续保留。任务仍8/10，S9/S10待节点与整体交付收口；无本 change 的行为规格同步、归档、Git写操作或整体批准。

### 2026-11-03 N3 用户明确通过

用户查看 N3 复核入口后明确回复：“N3通过，保留已列历史及文档限制，继续推进剩余收口工作，完成后提交整体验收。”据此登记 N3 渲染观测通过，依据为当前 n11 安装候选、n15 六份新捕获及实际回放/画面检查。未补造逐项人工结果或时长；首次后台脚本启动超时、文档36项错误/strict 7项警告、历史失败及未测范围保持。N1/N2/N3均已取得明确结论，前文待验收文字保留各自点时含义，不覆盖本次决定。

本轮获准继续 S9/S10 的剩余收口并提交整体审阅，尚未获得整个 change 的阶段整体验收通过、主规格同步、归档或 Git 写授权。全局索引仅补当前 change 导航，不新增日常子节点批准表或复制验证正文。


## 2026-11-03 收口与整体验收提交

用户已分别明确通过N1、N2、N3，并要求完成剩余工作后提交整体。本节是当前整体审阅入口；前文各轮结果、未通过/禁用及待批准状态保留原候选/时点身份，不覆盖后续已明确的节点结论。此处尚不登记整个change通过。

### 当前交付与验证依据

| 能力路径 | 最终交付与适用依据 |
| --- | --- |
| development-profiling | 当前n11包通过n13两API Tracy连接/断开重连/无Viewer退出；n15两API三场景六份真实捕获及回放通过，N2/N3已获用户明确结论。工具输出不等价旧性能预算 |
| module-boundaries | n11八种新目录配置/构建、唯一client与开关/公开头/CPU-only/关闭测试/安装隔离；n16再次核对8种构建、目标图与758项启用注册，产物摘要不漂移 |
| application-lifecycle | 当前参数/初态/场景/装配/关闭正式库检查及当前安装包场景运行；n13/n15覆盖当前三种场景实际路径；无旧Session/导出要求 |
| telemetry-lifecycle | 自有轻量快照、实际Presented/物理步速率、缺值/暂停恢复及常量空间检查；无新HUD、逐帧历史、系统内存采样或业务遍历 |
| platform | 玩家输入与场景隔离、允许失焦/局部取消和失效尺寸规则的正式库/原生专项依据；当前CPU图与安装场景独立成立，不将旧原生批改记为新候选重跑 |
| renderer | 持久资源命名/成对原生事件、图形SDK边界与原有正确性同步；当前n15回放361次world draw、真实post-VS网格/atlas/选框/颜色输出，旧原生故障/GL回归保持原身份及用户保留限制 |
| foundation-files | Publish头/实现/专属测试及注册已退役；YAML适配独立owner、文档往返/错误、world-summary和PNG有效行为保留；Fix1仅取消归档，历史故障未改判修复 |

自动结果累计758项启用通过、0失败、0禁用，仍是n11原754项加n12四种Debug world.allocations补测，未虚称本轮重新执行一次758项全套。n16八种配置完成settled构建、注册和源码/依赖图核对，测试exe/world库/游戏exe摘要保持各自原值；没有为收口重复全套测试、GPU故障批或新增人工体验。普通和profiling安装包摘要也与N1/N2/N3审阅身份一致。

本次最终确认生产树/构建图没有旧Session、PollGpuSamples、GPU timing query、Publish及专属测试/旧benchmark产品；当前包不产生旧性能输出，不安装旧工具、Viewer或RenderDoc。准确产品删除仍为59份（54+2+3），Fix1原6份完整移入取消归档，14项未完成不勾选。保留的诊断现场/辅助头及旧归档快照匹配，EnTT原许可字节匹配HEAD，主规格行为Requirement未同步，原工作区未写入。

交付位置：普通包 `out/install/n11-dual`，profiling包 `out/install/n11-profile`。exe SHA256分别为 `fc2de0d6a36d58e1cb5b112e0813efb2b42dca61134eca13b9ba6df16f1b3fd7`、`269b932b257de28c7d7d88c318ab6de43a24fd6f799238103d740ce79bf6f88d`。最新Tracy轨迹在 `out/runtime/n13-tracy-n2-002`；最新RenderDoc捕获在 `out/runtime/n15-renderdoc-001`，均为out工作存储，不附入文档。外部工具以 `F:\Toolchains\Tracy` 0.14.1、`F:\Toolchains\RenderDoc` 1.47为准；游戏vendor client未变。工具链保持MSVC tools14.51.36231、Windows SDK10.0.28000.0及Vulkan SDK1.4.363.0。

### 文档收口与保留限制

主入口、工具README、受影响架构与共享验证方法均已描述独立场景、薄宏/依赖隔离、轻量观测、准确退役及历史恢复。移除13处过时“全部节点待验收”的当前页提示，统一链接本页判断结论；未批量改写历史命令或冻结证据。补齐Dev盘工具路径、图表自动纵轴/选框draw及mesh上传口径，修正docs入口对已退役旧文档目录的残留描述。AGENTS仍使用批准的SDK28000与DBG01构建规则；全局VERIFICATION只新增当前change导航，没有补造整体验收批准。

最终文档审计仍有36项错误：35个原有out目标缺失、1个旧归档元数据摘要差异；OpenSpec strict仍有7份增量首个二级标题前文字的警告、无ERROR，因此这两项检查不称全绿。它们未随本次收口被修复/豁免，冻结材料与CLI规则未为消警告而改写。受影响活动文档的文件/锚点链接及 `git diff --check` 另行通过，非这些历史缺口的新增链接错误没有引入。

保留n13首次普通权限Tracy连接超时及n15首次后台脚本启动超时，后续成功批分别记录，不改写首轮为通过或声称底层原因已定位。历史1175/1176/1177、缺码/超时、旧Debug level2终止和冻结预算/未测范围保持；API退役不是根因修复。新工具不是完整GPU时间线、进程/显存分析或性能无回归证明。长时/其他机器/真正干净机及正式性能预算未因本change通过，未追加采样或人工预算；SDK/驱动/窗口时序与普通CRT部署等仍受各自已记覆盖范围限制。

### 整体验收请求

S1–S10的实施与适用检查已完成，N1/N2/N3已按用户明确结论通过；CLI任务完成只表示清单闭环，不代表本段已获人类整体批准。请用户结合本页当前交付与上述保留限制明确“integrate-renderdoc-tracy整体验收通过，保留已列限制”，或提出具体未满足项。无需再次逐项复跑三个节点或扩大人工时长。

此处整体批准仅针对本工作树当前交付，不包含将生产改动传播原工作区、合并行为主规格、归档本change或任何Git提交/推送/合并。Fix1已取消归档是此前单独授权的操作，不据此推定当前change的生命周期授权。收到明确整体结论前，全局关键节点表与本页均不登记整体通过。


## 2026-11-07 整体验收通过与主规格同步

2026-11-07 用户明确回复“好的，验收通过，同步主规格”。据此登记当前工作树的整体验收通过，依据为上节交付及 N1/N2/N3 的明确结论。普通及 profiling 安装包的可执行文件 SHA256 复核与上节一致。累计 758 项测试通过沿用此前验证，工具首次连接或启动超时、历史发布故障、文档审计与未测范围仍按原记录保留。

本轮同步七项能力至 `docs/openspec/specs/`。新增 [development-profiling](../../specs/development-profiling/spec.md)，更新 application-lifecycle、foundation-files、module-boundaries、platform、renderer、telemetry-lifecycle。合并 14 项新增、12 项修改和 6 项移除，保留继续适用的场景标识。旧独立 benchmark、性能导出和 `Files::Publish` 的现役条款随对应实现退役。Fix1 的取消归档及原失败结论沿用历史记录。

批准和规格同步适用于当前工作树。当前 change 目录保留在活动区。本轮执行文档及规格核对，构建、游戏运行、GPU 回放和性能采样沿用既有候选记录。

同步后的 OpenSpec 常规主规格校验 11/11 通过。严格校验仍报告 platform 既有的一项条款过长警告，change 增量仍有七项既有格式警告。七份同步主规格内的 49 个本地路径及锚点均有效。全库文档审计与收口记录逐项相同，仍有 36 项既有错误，无新增，标签索引已刷新。

## 2026-11-07 Git 提交核对

按 Git 计划在隔离检出中逐笔核对。G1 的 Tracy 原始源码和许可证按原字节纳入。G2a 双后端 Release 构建保留旧 benchmark，11 项定向测试通过。G2b 双后端 profiling Release 构建通过，首轮定向测试 6/7 通过。无 vendor 的小型配置夹具启动失败，固定路径诊断得到 Windows 0xC0000135。确认依赖 Debug CRT 和 Debug UCRT 后，仅给验证进程补充 DLL 路径，该项重跑通过，CPU-only Debug 的禁用态宏测试也通过。

G2c 双后端 profiling Release 构建、112 项 CTest、安装和旧 benchmark 配置拒绝均通过。G2d 的 D3D12、Vulkan、双后端及 CPU-only Debug 均实际通过 world.allocations。构建使用 MSVC 14.51.36231、Windows SDK 10.0.28000.0 和 Vulkan SDK 1.4.363.0。此前实际捕获和用户验收仍绑定原安装候选。

提交候选的主规格常规校验 11/11 通过。严格校验保留 platform 的 1 项警告和增量的 7 项警告。隔离检出的文档审计保留 35 项原有缺失目标，无新增。隔离检出读取的旧归档元数据字节符合其清单，原实施工作树先前报告的字节差异仍保留原记录。
