---
type: 阶段记录
status: R1已冻结，R2顺序4候选已交付，人工及R3整组验收未完成
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-08
updated: 2026-10-09
tags:
  - area/milestones
  - topic/rendering
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# M3 T3 D3D12 前置实验

## R1 冻结与 R2 启动

2026-10-09 用户完成 R1 审阅、勾选 T3-M01–T3-M07，并明确决定：“我已经审查了R1了，可以冻结R1，批准进入R2”。**R1 已关闭，Renderer v1 公开契约/target 设计基线冻结，R2 生产实施获准开始。** 见 [冻结记录与范围](../../verification/archive/t3-r1/freeze-20261009-001/README.md)、[冻结身份](../../verification/archive/t3-r1/freeze-20261009-001/freeze-manifest.json) 和 [R2 实施入口](r2-entry.md)。批准前 12 份原字节来源已固定，不覆盖既有查验归档、运行成绩或便携包。

上述冻结落档当时只完成节点冻结和实施交接，没有新CPU/GPU运行或生产代码改动。后续顺序1至4实施与验证另见下节；真实D3D12后端、统一完整游戏与Debug/Release `002`候选现已交付，默认仍旧OpenGL，人工及R3整组未验收，R3用户批准后才切默认，Vulkan和双现代整体验收属于R4。历史实验声明消费者不改称生产API，旧JSON中待批准/未冻结字段不回写。

R2 要重新核对最新同源依赖：`foundation/src/files.cpp` 已不同于 R1 运行来源，差异记录在冻结身份中。生产资产/采样/颜色、相机 GPU 画面、真实 target 图、混层三角形及故障边界继续验证，不静默收窄输入或把旧成绩扩为生产通过。

Obsidian 库根为 `docs/`；R1 查验文件已按节点复制至 [R1 查验归档](../../verification/archive/t3-r1/review-20261009-001/README.md)，本页的产物链接使用库内相对地址。原始 `out/` 材料和可运行包不移动，归档不作为新运行成绩或可运行包；`test/` 源码仍是本机外部链接。查看命令见 [人工核查附录 A](manual-verification.md#附录-a-人工核查命令)。

固定人工核查入口：[manul-verification.md](manual-verification.md)。R1人工完成结论已移入无checkbox历史表；顺序4交付后当期主表换入T3-M08至M15与逐项Python命令，全部待人工，不以独立实验代替生产接管，也不以自动结果代填用户勾选。

2026-10-09 较早决定中，用户明确批准 **T2 节点正式通过**，权威记录见 [正式节点批准](../m3-t2/README.md#t2-正式节点批准) 和 [批准身份](../../verification/archive/t2-approval-20261009/approval.json)。MB01 方案2同时确认 working set ≤ **704 MiB**、private bytes ≤ **1408 MiB**，FB01 方案2保持。T3 的 T2 节点批准前置已解除；这不代表 Win32 1175 已修复、暂停的 Fix1 已完成、正式同源 Q06 已实测通过，或首次可操作时间预算已敲定；这些继续作为遗留风险跟踪，不再阻断 T2 节点。该次 T2 决定本身未批准 R1；后续 R1 的独立批准以本页上方新记录为准。

2026-10-08 范围确认：T2/T3/T4 当前功能/兼容验收只限定本机 RTX 5070 Ti，其他 GPU 或第二机器不阻塞节点；中低端整机性能验证仍为下一玩法功能开发前门槛。以 [[project-scope#M3 本机 RTX 5070 Ti 验收与下一玩法阶段性能门槛|当前验收范围与后续性能门槛]] 和 [T3 spec](../../openspec/changes/m3-t3-renderer/proposal.md) 为准。用户随后明确要求撤回核显验证：已删除专项代码、验证包及该轮生成输出，原独显 R1 实验和证据保留，当时 R1/v1 尚未冻结。

本阶段依据 [T3 spec](../../openspec/changes/m3-t3-renderer/tasks.md#1-r1-契约与关键实验既有节点) 推进 R1。2026-10-08 用户授权与 T2 的 S3–S5 并行开展 D3D12 前置实验；该准备阶段不修改生产 Renderer、不删除 OpenGL、不采用第三方 RHI。2026-10-09 已按独立用户批准转入 R2，生产实施顺序见新入口。

上一轮撤回不修改生产 Renderer 或 T2 目录，也未重新运行 GPU。用户随后授权继续 R1 准备，本轮双配置复核另见下文，不重新引入核显专项。Y9000P / GTX 1650 整机性能仍按 [[project-scope#M3 本机 RTX 5070 Ti 验收与下一玩法阶段性能门槛|中央规范]] 在下一玩法开发前验证，不由删除专项验证包改变。

实验入口为 [独立夹具](file:///F:/GameDevelop/OpenGLProject/Symocraft/test/experimental/d3d12-r1/README.md)。它导入实际 platform、foundation、SDL 静态库，使用 Native 窗口与私有 HWND 桥接；不重新实现生产 Window，也不成为生产依赖。构建和原始运行材料只写入独立 `out/m3-t3` 目录，不复用或覆盖 T2 的构建与证据。

## R2 顺序1交付

按用户授权，已实现冻结公开三头、真实 CPU PImpl 门面、资源所有权/句柄事务、全帧与生命周期验证，以及私有后端契约/工厂和实际构建边界。验证与原始材料入口见 [顺序1实施报告](r2-step1-implementation.md)。顺序1当时未运行 GPU、未交付原生后端/现代包，工厂明确 Unsupported；这些历史成绩不扩为后续 GPU 成绩，旧 OpenGL 默认不变。

## R2 顺序2交付

已落实真实生产 D3D12 设备/呈现、持久资源、生产 shader、严格 MSAA、纹理/深度/剔除/选择框、尺寸恢复、query/readback 和有界故障所有权策略。用户明确批准现代透明 discard 不写深度，旧 GL 不改。最终 RTX Debug/Release 各 4/4 原生进程、原生边界各 11/11、CPU-only 边界各 8/8；真实最小化均超过十秒。完整记录、原失败、数值极值与边界见 [顺序2实施报告](r2-step2-implementation.md) 和 [节点归档](../../verification/archive/t3-r2/step2-20261009-001/README.md)。

顺序2当时 CPU 全量并非全部绿：Release 最终仍复现 foundation.files Win32 1175，T2/Fix1 状态不改。实际设备丢失/真正在途 timeout 未动态验证；该轮固定几何 GPU 检查不是完整世界/玩法，没有现代运行包、R3 批准或默认切换。后续顺序3成绩独立见下节，不扩大旧轮范围。

## R2 顺序3交付

已接入真实完整游戏循环、World/Chunk/发布revision持久缓存、Camera/图集/UV转接、唯一Present、中立采样与截图、图形先于Window清理。最终RTX Debug/Release各static/walk/edit三组通过，Release旧GL对照三组通过；静止/移动无区块几何重传，连续编辑只更新发布网格。详见 [顺序3实施报告](r2-step3-implementation.md) 与 [节点查验归档](../../verification/archive/t3-r2/step3-20261009-001/README.md)。

四套构建完成，重点与公开边界回归通过；该轮native Debug全量仍有Win32 1175，原失败保留。完整世界短测不是人工14项玩法/15分钟或设备故障证明。当前默认GL、R1冻结及T2遗留事项不改；随后顺序4交付独立见下节，不扩大顺序3旧成绩。

## R2 顺序4交付

已交付真实 `d3d12-game-debug-002` / `d3d12-game-release-002` 生产过渡包，实际install、3282项同源输入、工具/exe/DXIL/imports/匹配CRT/notices与精确包hash均留证。Python候选单测24/24；最终native Debug69/69启用项、Release70/70，CPU-only Release61/61、Debug59/60仍复现foundation.files Win32 1175。两个实际包的完整441区块static/walk/edit、普通400帧及隔离资源故障通过，真实游戏最小化12秒/恢复/缩放/目标正常关闭也有独立记录。

见 [顺序4报告](r2-step4-implementation.md) 与 [库内查验归档](../../verification/archive/t3-r2/step4-20261009-001/README.md)，原始失败和旧001包保留，未重新引入核显或覆盖R1。人工主表已提供T3-M08至M15与各项Python命令，全部未勾选。当前候选仍含隔离旧GL；实际DeviceLost/提交/在途故障、现代独占/全关闭、部分补充配置、GUI现代协议及正式视觉/性能标准未完成，14项/15分钟未执行，不能申请整组通过或切默认。

## 冻结门槛

| 条件 | 当前状态 | 证据要求 |
| --- | --- | --- |
| T0 / T1 交接 | 已阅读，继续保留既有语义 | [CPU 契约清单](../../openspec/changes/m3-t3-renderer/design.md#architecture)，28 字节非索引三角形顶点，callback 借用不跨调用保存，更新成功后才记发布版本 |
| T2 S3–S5 与节点批准 | S4 交接已交付；2026-10-09 用户正式批准 T2，节点批准前置已解除 | [正式批准与遗留事项](../m3-t2/README.md#t2-正式节点批准) 区分用户决定与实测结果；FB01/MB01 已确认，发布稳定性、正式同源 Q06 及未定首次可操作时间预算继续跟踪，不冒称技术问题已关闭 |
| D3D12 能力与工具链 | 本机 RTX 5070 Ti Debug / Release 已核实 | 实际 device LUID 与 DXGI adapter 一致、PCI 10de:2c05、FL12_0 / SM6.0；查询交换链配置并核对，无软件设备/能力降级 |
| R1-E01 纹理绘制 | RTX 5070 Ti Debug / Release 通过 | 两层实际采样、像素方向/UV/层号/Linear 与 sRGB、28 字节布局、截图和 Debug Layer；生产资产画面不因此通过 |
| R1-E02 更新与删除 | RTX 5070 Ti Debug / Release 通过 | CPU 自有化、成功更新、空网格、过期句柄；fence 3 未完成时替换、fence 5 未完成时删除，之后正常回收 |
| R1-E03 呈现资源重建 | RTX 5070 Ti Debug / Release 通过 | 三次实际像素缩放、真实最小化至少 10 秒、显式零尺寸契约暂停与恢复；不重建 Window 对象 |
| 公开 Renderer v1 / target 图 | 2026-10-09 用户批准冻结设计；顺序1至3已落实CPU门面、原生backend与统一游戏调用 | 历史批准依据不回写；见 [顺序1](r2-step1-implementation.md)、[顺序2](r2-step2-implementation.md)、[顺序3](r2-step3-implementation.md)；生产候选与完整人工验收仍待后续 |
| R1 人工审阅与节点决定 | T3-M01–T3-M07 用户审阅通过，明确批准冻结及进入 R2 | [人工历史记录](manual-verification.md#r1-已完成项目)、[批准身份](../../verification/archive/t3-r1/freeze-20261009-001/freeze-manifest.json)；不推断新动态运行 |
| R2 生产接管 | 顺序1至4已实施，实际生产候选及有限复核已交付，人工与技术缺口未关闭 | 按 [R2入口](r2-entry.md) 与 [顺序4报告](r2-step4-implementation.md) 收尾，短测不替代14项/15分钟、A01–A19或R3批准 |

R1 历史范围：候选 CPU 数据和句柄校验在实验目录中独立测试，名称和实现不作为生产 API 的稳定承诺。纹理采用 RGBA8、层优先、紧密行、左上原点与显式 Linear/SRGB；当时相机转接有 CPU 数值依据，但生产 GPU 画面尚未核对。实验的局部正交屏幕坐标不等于冻结后的游戏相机契约；后续顺序2画面验证另见新报告。

## R1 转接准备与双配置复核

用户已授权继续 R1 准备；接收 [T2 S4 交接](../m3-t2/s4-platform-handoff.md) 和 [S5 清理回归](../../verification/archive/milestone-evidence-20261011/m3-t2/evidence/s5-cleanup-validation.json)。这些是原准备轮次的证据，当时不构成 T2 最终批准；T2 已于 2026-10-09 获用户正式节点批准，当前遗留风险按 [权威记录](../m3-t2/README.md#t2-正式节点批准) 跟踪，不回写旧实验成绩。完整来源、JUnit、输入/资产与 GPU SHA 见 [本轮验证身份](../../verification/archive/t3-r1/handoff-validation.json)。

| 本轮交付 | 实测范围 | 边界 |
| --- | --- | --- |
| [声明参考](file:///F:/GameDevelop/OpenGLProject/Symocraft/test/experimental/d3d12-r1/contract/candidate_renderer.h) 与独立消费者 | 双配置编译签名、不可复制/移动 PImpl 形状、无 SDK 头/后端宏；仅链接隔离 CPU API target | 只有声明，没有生产或 mock Renderer；按已批准 v1 基线在 R2 实施错误分类、全帧/frame ID、状态/线程语义与公开/私有拆分 |
| CPU 契约与生产转接 | Debug / Release 各 CTest 4/4，原 70 检查 + 新增 82 检查；真实 assets/simulation/world target，不重复编译生产替身 | [Debug](../../verification/archive/t3-r1/review-20261009-001/log/r1/build/r1prep-cpu-debug-ctest-final-003.log)、[Release](../../verification/archive/t3-r1/review-20261009-001/log/r1/build/r1prep-cpu-release-ctest-final-004.log)；不初始化 SDL/GPU |
| 纹理资产与层号 | 实际 512×512 RGBA 图集、64 个 64×64 层全像素映射；原 PNG 底行优先层号保持、候选层存储为 top-left，旧 UV 采样转换提案为一次 `v'=1-v` | 旧 GL 为 NEAREST/REPEAT、Linear 存储；当前 GPU 夹具 POINT/CLAMP、sRGB RTV，不能将生产颜色/边界采样等价标通过 |
| 相机与网格发布 | 真实 Camera 的度→弧度、near 0.1 / far 2000；显式 RH_NO/RH_ZO 数值对照。真实 World callback 内自有化，按身份映射，更新成功才记 `record.revision`，失败/空结果/静止/显式销毁均检查 | CPU World 使用 radius 2 / seed 424242 / vegetation false，非完整世界 GPU 验收；已为 world-space 的顶点不得再次按 chunk 平移 |
| D3D12 Debug / Release | 新建独立 platform/foundation/SDL 构建；两配置正常/合成超时各 2/2，E01–E03、Debug Layer 0 错误/警告、真实设备/呈现查询核对 | [Debug](../../verification/archive/t3-r1/review-20261009-001/evidence/r1prep-debug-001/verification.json)、[Release](../../verification/archive/t3-r1/review-20261009-001/evidence/r1prep-release-001/verification.json)；超时仍非真实设备丢失/在途 GPU 停滞 |

本轮五张 Debug 截图已逐张视觉复核；五张 Release 截图逐字节 hash 与 Debug 及原独显参考一致。实际交换链为 UNORM 存储、三缓冲、1x/quality 0、FLIP_DISCARD、windowed、flags 0；Present sync interval/flags 均为 0。每次创建/resize 查询实际配置，不以请求值替代；没有测试 HDR 或声称物理输出设备由此得到验证。

Debug 沙箱内再次出现 C1902，经用户批准在沙箱外成功构建/运行，没有改 PDB 选项规避。CPU 转接首次整数叉积和复用缓存变量错误已修正，原失败日志保留。Release 第一次运行被源码 hash 守卫在开窗前拒绝，重建来源记录后才实际运行；不把拒绝算作 GPU 成绩。新导入与打包校验同时记录 platform 和 foundation 源码，拒绝旧 foundation 库冒充当前同源。

上述准备轮次不制作新验证包、不修改生产 Renderer/T2，不代填 [人工核查表](manual-verification.md)；当时 R1/v1 未冻结、R2 未开始。之后用户于 2026-10-09 完成独立审阅并批准冻结，当前状态以上方新记录为准。生产采样/颜色、混层三角形和真实门面语义仍需在 R2 按基线实施与复核，不能因节点批准改写历史实验范围。

混层三角形作为 R2 实施复核项保留：当前 World 面与实验夹具均同层，shader 使用不插值的整数层；不能将该实验推广为任意混层三角形的已验证行为，也不能静默增加输入拒绝限制。若需改变公开语义须按受控变更规则确认。点采样/Clamp 的夹具不替代生产采样器、mip、资产归一化与相机迁移核对。

## 保留的本机 RTX 记录

下表只使用撤回前已保留的原独显便携包及其证据。当时只有 Release GPU 成绩，不能将 CPU Debug 或 Release 下启用 Debug Layer 等同于 Debug GPU 配置；本轮真正 Debug 成绩另见上节，不回写原身份。

| 项目 | RTX 5070 Ti |
| --- | --- |
| 实际 D3D12 硬件与工具链 | FL12_0 / SM6.0，LUID 00000000:000144d0，driver 32.0.16.1664 |
| 双层纹理与颜色/方向截图 | 像素检查与原五张实际截图逐张视觉复核通过 |
| 在途更新/删除及 fence 回收 | 真实 GPU gate：更新时 submitted 3 / completed 2，删除时 submitted 5 / completed 4；后续正常回收 |
| 缩放/最小化至少 10 秒/恢复 | 800×480、512×384、720×540；真实最小化与显式零尺寸暂停无 GPU 提交，恢复通过 |
| Debug Layer 无未解决警告/错误 | 初始化、帧、重建及清理后均为 0 / 0 |
| 有界等待故障与正常逆序清理 | 正常 exit 0；合成超时 exit 1，200 ms 事件等待，进程总耗时约 503 ms |

实验不证明 D3D12 完整玩法接管、Vulkan 接管、A01–A19 全组或 T3 通过。超时注入仅证明指定失败路径，不宣称实际设备丢失。截图和 fence 记录分别用于视觉与生命周期判断。

故障边界：本夹具的超时注入是在已完成读回后让 fence 事件故意不触发，证明原始错误保留、有限等待和不二次等待；不是 GPU 实际停滞时的安全资源回收证据。真实提交/Signal 失败、设备丢失和仍在途的读回/纹理上传等待失败，需要在 R2/R3 的生产生命周期与 A14 故障设计中另外落实。失败态的对象释放按放弃当前设备路径记录，不算 fence 确认完成的正常延迟回收。

## 本轮验证

本节为原 Release 便携包交付记录，汇总与 SHA-256 见 [R1 前置验证清单](../../verification/archive/t3-r1/preparation-validation.json)，原始材料保留在 `out/m3-t3`，不覆盖早期失败。本轮双配置与转接身份见上节。

| 验证 | 实际结果 | 证据 |
| --- | --- | --- |
| 独立 CPU 契约 | Debug / Release 各 70 项检查、CTest 各 1/1；独立配置不发现 DXC、SDL、D3D12/DXGI，不创建窗口 | [Debug](../../verification/archive/t3-r1/review-20261009-001/log/r1/build/cpu-debug-ctest.log)、[Release](../../verification/archive/t3-r1/review-20261009-001/log/r1/build/cpu-release-ctest.log) |
| GPU Release 构建 | 实际平台/SDL archive 导入、SM6 DXIL 生成和实验构建通过；只构建 platform 依赖，不构建生产游戏/renderer | [最终构建](../../verification/archive/t3-r1/review-20261009-001/log/r1/build/release-build-005.log) |
| 编译工具链 | MSVC 19.38.33145 / tools 14.38.33130、Windows SDK 10.0.22621.0、DXC 1.6.2112.16、CMake 4.3.1、Ninja 1.13.2 | `build-identities.json`；这里只证明实际版本，不冒称最小 CMake 3.22 兼容验证 |
| 本机前置实验与重复运行 | 修正后独立运行 `003`、`004` 均为正常/注入两项 2/2；最终再补清理后诊断，用便携包复跑 2/2 | [003](../../verification/archive/t3-r1/review-20261009-001/evidence/desktop-release-003/verification.json)、[004](../../verification/archive/t3-r1/review-20261009-001/evidence/desktop-release-004/verification.json)、[最终便携包实测](../../verification/archive/t3-r1/review-20261009-001/package/d3d12-r1-release-001/evidence/desktop-smoke-001/verification.json) |
| GPU 与图片 | RTX 5070 Ti / driver 32.0.16.1664；真实硬件创建 FL12_0，SM6.0 经 CheckFeatureSupport；Linear 灰 128 → 188，sRGB 灰 128 → 128，alpha 孔为黑；截图逐张复核 | 最终报告及五张 PNG |
| Debug GPU 配置 | 未完成：原沙箱编译器检查触发 C1902，提权构建请求被取消；未通过修改 PDB 选项规避，原现场保留 | `out/m3-t3/platform-debug/CMakeFiles/CMakeConfigureLog.yaml` |

前两轮 `desktop-release-001` / `002` 在局部绘制之后异常退出，exit 2173、缺最终报告，均判失败。审查修正了 Present 之后再 Signal 的覆盖范围、重建前重置已完成命令列表、清理时先释放命令列表/allocator 再释放引用资源，并保留逐行日志。后续结果通过，但没有 native crash dump，不能将原 2173 的精确原因单独归于其中某一处。另修正了构建脚本复用缓存时误用 PowerShell 只读 `$HOME` 变量的问题；该次失败只留有历史终端记录，不声明磁盘存在 `release-build-002.log`。

## 历史便携包

便携实验目录：[d3d12-r1-release-001](../../verification/archive/t3-r1/review-20261009-001/package/d3d12-r1-release-001/README.txt)。包含实验 exe、两份 shader、app-local Release CRT、SDL 许可证、验证脚本及原样来源记录；不需要开发 SDK / DXC / 游戏资产，不包含第二台机器成绩。运行机器仍须提供 D3D12 Debug Layer（Windows Graphics Tools），缺失即记录受阻，不静默关闭验证。

验证脚本的便携模式只校验包内相对路径与 SHA-256，原构建绝对路径仅作来源记录；shader 路径显式指定到包内。已经从 `C:/Windows/System32` 启动便携验证，程序在自己的证据目录运行，正常/注入均通过。第二台机器运行说明在包内 `README.txt`；采用新的 `evidence/y9000p-release-001`，回传完整结果与截图后再复核。已有 `desktop-smoke-001` 只属于台式机，不能改名作为 Y9000P 结果。

上述包和双机交接文字是前一轮交付事实，不代表当前还以 Y9000P 为 R1 进入条件；当前只按本机 RTX 5070 Ti 新 spec 执行。历史包及证据不改名作为另一设备成绩，其他设备的专用验证能力不作为当前节点前置。

## 清理与交接

已移除核显专项 adapter 选择/库存 CLI、定向 runner、专项测试及打包分支，恢复原高性能默认 R1 夹具。保留候选 CPU 契约和原实验的 GPU gate、在途回收、Present/fence 覆盖及逆序清理修复。

删除范围仅限 T3 本轮 `platform-release-002`、`r1-release-002`、核显包及 ZIP/解压复核目录、同版显式 RTX 回归输出和 `integrated-*.log`。原独显构建、便携包、CPU 证据与早期失败现场不覆盖；T2 产物与生产代码不动。本次没有新的 GPU 验收成绩。

清理后临时 Release 构建通过，CTest 1/1 与 CPU 70 项检查通过；已撤回的 CLI 均在创建窗口前拒绝，三个构建/打包/验证脚本语法无误。本次临时检查目录也已删除；原独显包清单、exe、验证报告及五张截图的 SHA-256 均未改变。

上述为上一轮清理事实；新的 CPU 转接与双配置 GPU 证据见上节。T2 与 R1 已于 2026-10-09 分别获用户正式批准，当前已冻结 R1/v1 设计基线并进入 R2；原包/成绩不回写。生产转接画面、真实 target/门面与在途超时/设备丢失边界继续按 R2/R3 实施与验证。

## 实现依据

纹理上传/读回使用实际 copy footprint，读回前等待相应 fence；`Map` 本身不提供 GPU 同步。依据：[Microsoft 读回说明](https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps)、[资源状态转换](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12)。能力检查通过实际设备的 `CheckFeatureSupport`，不从显卡名称推定，参见 [Microsoft 能力检查说明](https://devblogs.microsoft.com/directx/introducing-a-new-api-for-checking-feature-support/)。

本轮实际设备身份来自 [ID3D12Device::GetAdapterLuid](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-getadapterluid)，并与创建时的 DXGI adapter 硬校验；实际交换链参数通过 [IDXGISwapChain1::GetDesc1](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiswapchain1-getdesc1) 查询，不从默认优先级或显示器连接推定。
