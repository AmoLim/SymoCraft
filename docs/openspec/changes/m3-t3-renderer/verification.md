---
tags:
  - area/spec
---

# T3 验证记录

本页集中原全局索引的 T3 验证正文，既有结果、失败、用户反馈及结论边界保持；本次只调整记录归属，未重跑验证。关键节点批准仅见 [全局索引](../../../verification/VERIFICATION.md#关键节点)。
既有证据链接指向当前压缩包，不复制、解包或修改旧冻结材料；包内成员身份按原清单解释，本次不证明压缩包恢复可用。后续按 [归档规则](../../../DOCUMENTATION_MAP.md#每个-change-一份验证记录) 更新本页，默认无新附件、无特别要求不附原始数据。

## 未通过与未覆盖

当前默认已按后续用户明确R3批准切为D3D12，GL保留显式过渡，Vulkan未实现；批准与定向切换结果见下节。以下R2/R3对账保留批准前的点时身份及未验证事实，批准不使A01–A19部分项自动变为全绿。Release004人工玩法已有用户通过反馈，受控原生设备移除/真正在途超时及提交注入已有独立证据，不继续笼统记为未测。
原生 Benchmark 已在本轮补充显式 D3D12/现代 v1 消费者，具体版本/自动运行范围见下节；GUI 人工与正式基准不能从 CLI 短测推定。Win32 1175、暂停 Fix1、正式同源 Q06 和首次可操作时间预算继续保留原状态。
本机 RTX 5070 Ti 功能范围不代替下一玩法前的 Y9000P/GTX1650 整机性能门槛，旧 GL 冻结数字不是新后端通过依据。

## 默认切换验证

2026-10-12，用户在已知R3证据对账仍有部分项后明确批准R3并授权切默认；唯一节点记录见 [全局索引](../../../verification/VERIFICATION.md#关键节点)。本轮实施只切游戏默认及首次构建默认，保留显式GL、原validation/能力/失败分类；没有删除GL、开始Vulkan、恢复长采样/Fix1或执行Git/sync/archive。下方历史对账中的A10正常退休、故障关闭整合、新版增量、运行生成身份/适用部署、视觉容差及正式预算仍保持未验证/部分，不反推实测通过。

新包为 [d3d12-default-release-001](../../../../out/m3-t3/package/d3d12-default-release-001/package-manifest.json)，默认 `d3d12`、保留 `opengl`；manifest SHA256 `57b9fbeae40898f84865c4de3e1b1f25d59eb3ce3e1d37c8bf0f92961553599f`，游戏EXE SHA256 `83e084a791754252a43eaf0f6617e7d84058677dcc7ab9e881002e8dab8a3cf8`。新包另建，不覆盖旧Release004、现代独占002或历史候选；旧包保持其原默认选择。

使用MSVC14.38/19.38.33145、CMake4.3.5/Ninja1.13.2/SDK10.0.22621.0。普通 `windows-release` 在全新 `out/build/r3-default-release-001` 默认D3D12+GL+Benchmark配置/完整构建成功，候选专用身份步骤没有阻断普通路径；定向CTest的startup_options、assets.game_package、world.summary_cli及三项边界共6/6。新候选 `out/m3-t3/r3-default-release-001` 使用相同首次默认、Benchmark OFF，实际游戏/启动测试构建及POST_BUILD身份记录/打包成功，startup_options 1/1。另在全新CPU-only Debug目录只构建启动测试，1/1通过，无原生图形target；Python候选脚本单测26/26，兼容旧GL默认与新D3D12默认manifest。没有重跑D/R全套或全部后端矩阵，不以定向绿结果关闭1175。

新包有限运行共6案：默认资产预检exit0；不带 `--renderer` 和显式 `d3d12` 各实际120帧、RTX5070Ti/LUID00000000:000143d6/driver32.0.16.1664、4x/Fifo/validation=1、正常关闭exit0；显式 `opengl` 120帧正常exit0；未交付 `vulkan` 具名Unsupported exit3；隔离副本仅将一份DXIL改名保留，默认预检具名缺DXIL exit2、不回退到已编入GL。原新包56个声明文件运行后逐项hash匹配。有限帧/预检不作新15分钟人工、正式性能或全部原生故障验收；候选未证明无SDK新机器。初始默认不重写已有D3D12=OFF缓存，已有GL-only配置的默认请求仍具名拒绝，此分支未本轮新建GL-only运行证明。

日常构建/CTest/运行日志仅在上述out工作位置保留，本记录默认无新长期附件，不复制旧冻结材料或压缩包，不宣称远端保存/恢复。使用已选公开契约与原工作负载，没有降低区块/图集/画面要求，也不因默认切换改预算值。

默认切换后的目标OpenSpec validate通过，改动范围文档链接检查无错误，diff空白检查通过。全仓文档检查仍有415项既有归档迁移/旧目标链接错误，含压缩后原目录不存在与旧位置映射校验失败；本轮不解包、恢复或改写那些材料，不能报告全仓文档检查通过。

### IDE旧缓存修复

2026-10-12 用户从 `out/build/windows-release/bin/SymoCraft.exe` 报告默认D3D12具名Unsupported；实查该目录GAME ON、GL ON、D3D12 OFF，原因是仅改变option首次默认并未启用旧IDE缓存中的后端。标准Debug/Release游戏preset现明确设置D3D12=ON；根option不使用FORCE，自定义GL-only/无游戏配置仍可命令行显式OFF覆盖，CPU-only局部关闭规则不变。未改运行失败分类或自动回退，也未修改已有候选包。

在上述实际目录重新配置并只完成SymoCraft/startup_options_tests目标构建，最终缓存Release、D3D12 TRUE、GL ON；MSVC19.38.33145/CMake4.3.5/Ninja1.13.2。构建助手首次因编译器路径表示变化触发CMake重新检测并变成Debug，已中止该次构建、重新应用Release预设并核对最终配置，不将中间构建记为Release成功。定向CTest startup_options 1/1；无 `--renderer` 的资产预检exit0，seed424242/regression实际120帧exit0，RTX5070Ti/LUID00000000:000143d6/driver32.0.16.1664、4x/Fifo/validation=1、441区块且正常关闭。未新跑Debug、全部测试、人工15分钟或性能采样，既有R3批准和保留风险不变。

## R2 补充验证

2026-10-12 用户授权收敛 T3-R2 缺口，并选同版本 GL 视觉对照/透明深度单列及隔离进程受控原生故障加有限注入。以下是局部工程依据，不增加节点批准、不切默认、不恢复 Fix1、不替代 R3 或人工检查。本段记录的是本次规则调整前直接保存于 archive 的既有证据，未提交/上传或验证远端恢复。

| 入口 | 本轮实际范围及限制 |
| --- | --- |
| [完整配置矩阵 004](../../../verification/archive/m3-t3-r2-config-004.7z)、[故障入口加入后的图形矩阵 006](../../../verification/archive/m3-t3-r2-config-006.7z) | 全新 MSVC14.38/Ninja D/R：现代独占、GL+D3D12、CPU-only、根/独立工具、关闭测试/工具及游戏全后端关闭拒绝。004 完成18组合；006完成6个受影响图形组合，source_unchanged 为真。004 CPU Debug、006现代Debug/过渡Debug/现代Release的 foundation.files 均实际失败并具名记录1175；普通Debug world.allocations 禁用，不能报全通过或关闭遗留风险。关闭测试实际空测试集，现代包无GLAD/旧GL target/GLSL；未交付Vulkan未测 |
| [首轮夹具失败 001](../../../verification/archive/m3-t3-r2-config-001.7z)、[旧资源假设失败 002](../../../verification/archive/m3-t3-r2-config-002.7z)、[隔离路径失败 003](../../../verification/archive/m3-t3-r2-config-003.7z)、[被中断的 005](../../../verification/archive/m3-t3-r2-config-005.7z) | 保留现代夹具缺字段、世界摘要/资产测试旧GL假设及隔离子进程路径错误的实际失败；修正测试部署与诊断后在新目录复测，不改Files实现/输出位置来关闭1175。005只有部分实际结果，未完成不得当整组通过 |
| [最终Debug受控故障](../../../verification/archive/m3-t3-r2-faults-debug-001.7z)、[最终Release受控故障](../../../verification/archive/m3-t3-r2-faults-release-001.7z) | 各4个独立进程，真实RendererState/D3D12库、RTX5070Ti/32.0.16.1664、validation开启、1x samples。分配 HRESULT 注入保旧画面并能显式空更新；实际Execute后的Signal HRESULT注入停止续交。自有设备API移除产生原生DeviceLost，非自发硬件事故。自有队列gate后的真实texture copy/fence在途，500ms原生Timeout，失败关闭quarantine/重复关闭仍失败，gate解除并确认完成后有限退出；不是正常回收、整游戏/public wrapper 或所有驱动故障验收 |
| [旧数据原生消费者](../../../verification/archive/m3-t3-r2-consumers-001.7z)、[新候选正常运行与快速协议](../../../verification/archive/m3-t3-r2-consumers-002.7z) | 原生Release消费者读取冻结Debug004的GL/D3D12各三场景全部接受，未重写原数据。新现代独占Release完成普通无collector的400帧；同一过渡Release新EXE的GL/D3D12各三场景10+10秒、1080p/4x/VSync-off/441块及原生终态校验通过，旧25列和现代32列/v1身份分开，原包前后身份不变。快速结果与截图是诊断材料，不是正式九轮、视觉容差/性能预算或15分钟玩法通过；裁剪环境仍是有SDK的同一主机 |
| [现代独占Release002包身份](../../../../out/m3-t3/package/d3d12-modern-release-002/package-manifest.json)、[过渡Release004包身份](../../../../out/m3-t3/package/d3d12-transition-release-004/package-manifest.json) | 新独立候选，未覆盖原d3d12-game-D/R-002。现代包须显式选择d3d12，缺省仍请求GL且具名失败；过渡包默认GL。完整运行包留out，archive中的运行/来源/依赖日志不冒称完整可运行包、无SDK新机器认证或R3批准 |

2026-10-12 用户已确认 D3D12 帧时间方案4与两项进程内存相对预算，口径唯一见 [T3 决策](decisions.md#d3d12-已选预算口径)；启动/首次可操作等其他指标仅观测、不新增本轮硬预算，不改变 T2 的历史未决状态。本次仅同步活动规格/计划，未启动正式采集、未标预算实测通过；正式视觉参考的完整场景身份/数值容差、正式同版本同条件 GL 配对采集及预算判定仍未关闭。实际提交故障/真实内存耗尽、自发设备丢失、完整A01–A18与适用人工要求继续按原任务核对；受控与注入不混成硬件事故。shader字节与构建工具/profile在候选来源中绑定，但运行时完整生成身份/适用部署要求不从消费者兼容自动推定。当前任务仍未整项完成。

## R2 短采样收口与保留风险

2026-10-12 用户明确“批准收口，然后进行R3证据对账”：批准的是上述限定范围的 R2 工作收口并保留风险；同时授权只读 R3 证据核对。按现行治理在本节登记，不新增日常子步骤关键节点表、伪造完整技术验收或推定 R3 批准；未授权新测试/长采样、默认切换、Fix1 恢复、Git 或 sync/archive。

2026-10-12 用户报告按本轮提供的过渡 Release004 / 显式 D3D12 命令完成的人工核查全部通过；作为用户反馈记录，不补造逐项日志、具体游玩起止时刻或测量曲线，不将肉眼对照写成数值视觉容差批准。用户随后取消正式九轮，限定 5 分钟改做 GL/D3D12 短对照并收口本轮 R2 工作；该执行范围替代及验收边界见 [追加决定](decisions.md#本轮-r2-短采样收口与风险接受边界)。

证据唯一入口：[短采样比较与身份](../../../verification/archive/m3-t3-r2-short-001.7z)、[GL 原生会话](../../../verification/archive/m3-t3-r2-short-001.7z)、[D3D12 原生会话](../../../verification/archive/m3-t3-r2-short-001.7z)。先 GL 后 D3D12，同一 Release004 EXE/资源及原生执行器；两后端 static/walk/edit 各一轮、10 秒预热+10 秒采样，六轮全部通过原生终态/数据校验及既有报告探针，无重试。实际 RTX5070Ti、GL616.64/D3D12驱动32.0.16.1664、1080p/4x/VSync-off、D3D12 validation 开启；采样后包内声明文件的大小/哈希匹配原 manifest。原始 CSV/summary/PNG/日志保留在同目录，未提交或上传。

| 场景 | GL P95 / P99（ms） | D3D12 P95 / P99（ms） | D3D12/GL 工作集 / 私有提交观测最大值 |
| --- | --- | --- | --- |
| static | 6.861 / 7.581 | 5.570 / 6.141 | 87.95% / 67.85% |
| walk | 6.643 / 7.398 | 5.313 / 5.751 | 88.55% / 67.94% |
| edit | 7.499 / 8.660 | 5.946 / 7.761 | 88.38% / 68.23% |

上述只按短窗口筛查所选数值条件，全部未触线，**不是正式预算验收通过或性能无回归证明**。内存取 `10 <= elapsed_s < 20` 的每项观测最大值，每轮各10次，非正式180秒窗口/生命周期峰值/显存。P95/P99 从 sample CSV 最近秩复算，不删慢帧；D3D12 最大帧 static/walk/edit 为17.817/13.587/19.377 ms，不能由分位数合格声称无卡顿。static 的 GL 采样全程获焦、D3D12全程失焦，严格同条件比较不成立；其余两场景双方全程失焦，allow-unfocused 有效不代表消除后台争用。长采样/重复性/罕见尾帧与持续资源增长未证明，温度/频率未采集。

保留风险：正式视觉完整参考身份/数值容差未闭合；Win32 1175/Fix1 仍开放且暂停；自发设备丢失、真实内存耗尽/原生提交失败、完整 A01–A18 与无 SDK 干净机器部署未由短测覆盖，受控原生/注入证据保持各自边界。Vulkan与下一玩法前中低端整机性能仍独立。按用户限定本轮 R2 工作已收口，不自动恢复长采样；聚合技术验收保持未完成，不登记 R3/默认切换/T3 整体验收批准。

## R3 证据对账

2026-10-12，按用户授权只读核对已有材料，未构建、重测或运行游戏。判定对照 [A01–A19 原目标](specs/renderer/spec.md#requirement-modern-backend-takeover-acceptance)；“已覆盖”只指所列本机 D3D12 条件，“部分”不得变为通过。旧证据仍绑定旧版本，不因本表变为 Release004 新实测，用户人工反馈与自动记录分开。

版本核对：当前 [Release004 manifest](../../../../out/m3-t3/package/d3d12-transition-release-004/package-manifest.json)声明的56个包文件及 [构建来源](../../../../out/m3-t3/package/d3d12-transition-release-004/provenance/candidate-build-identities.json)的3270个源码/资源输入在本轮逐项匹配；短采样44个登记的会话/结果/原始产物、D/R故障8个YAML报告均匹配既存哈希。游戏EXE为 `41d9f5ee928b458fa2b715755465c4e16f30ea7f1075a5858ee378b450cf075f`。Renderer公开三头与历史顺序1–3相同；私有D3D12已增加诊断访问/工厂，旧夹具不能无条件迁成新执行成绩。对照顺序4归档源，新增主要为可选诊断owner、分配/Signal注入、quarantine标记与诊断工厂，普通游戏不创建诊断控制；新006构建/CPU结果和Release004正常短运行作为补充，而不是抹除版本差异。

复用入口：[旧CPU/增量顺序1](../../../verification/archive/t3-r2.7z)、[旧GPU/增量顺序2](../../../verification/archive/t3-r2.7z)、[旧游戏/增量顺序3](../../../verification/archive/t3-r2.7z)、[顺序4候选/原生/窗口/隔离资源](../../../verification/archive/t3-r2.7z)。新配置004/006、故障、消费者及短采样的身份/实际结果仍由上两节链接管理，不另造证据副本或测试矩阵。

| 项目                        | 已有依据                                                                                              | 对账结果及准确剩余范围                                                                                    |
| ------------------------- | ------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| A01 公开消费者/越界              | 006 D/R JUnit实际renderer三头、public_api_consumer、source/negative/renderer boundaries均通过；旧227项门面CPU检查 | 已覆盖公开头/边界，不由此推定GPU运行                                                                           |
| A02 独立/关闭测试               | 004的CPU-only、根/独立工具、关闭测试/工具；006现代无测试D/R空测试集、配置/build及部署预检                                         | 当前交付组合已覆盖；CPU Debug的files失败及world.allocations禁用单列，不报全套绿                                        |
| A03 D/R后端组合               | 004/006真实现代独占/过渡D/R及全关闭配置拒绝；现代包无GL target/GLAD/GLSL                                               | 当前D3D12组合已覆盖；Vulkan单/双组合属R4，GL最终退役条件尚未触发，不作为本轮缺项                                               |
| A04 依赖/增量                 | 新版边界检查/真实link/install；顺序1–3有私有/public/native/app头增量owner记录及失败后另轮复测                                | 部分：新版诊断头及变更后的受影响构建图没有新增增量执行证据；旧成绩可参考，不标本版增量全部验证                                                |
| A05 设备/不支持                | Release004实际RTX5070Ti/driver/MSAA/present；现代独占预检GL具名拒绝，旧隔离包Vulkan Unsupported/非法参数分列              | 本机身份及不可用选择已覆盖；能力不足硬件未实测，其他GPU不增设本节点门槛；缺Vulkan loader组合待R4                                      |
| A06 方块/图集/选框              | 旧原生4x夹具的top-left/多层/深度/alpha/selection像素记录；新完整世界PNG及用户画面核查通过                                      | 功能画面有依据；完整正式视觉参考身份/数值容差仍未定，已批准alpha-depth差异不能解释其他差异                                            |
| A07 固定世界/相机               | Release004六轮原生报告校验：441块/seed424242/固定摘要/1080p/4x/Immediate；用户移动/转向反馈通过                            | 本机固定世界和玩法操作已覆盖；不据短测宣称严格视觉或性能等价                                                                 |
| A08 静止/编辑/空发布             | 新短测static/walk持久几何零重传、edit有真实资源更新；旧原生空mesh与CPU cache/边界编辑，用户人工通过                                  | 现有静止/编辑/空发布契约有依据；不推广为A10的在途回收或无限负载保证                                                           |
| A09 输入寿命/边界               | 新版facade_contract/render_cache/conversion，旧真实上传后CPU容器改变、过期句柄、层/数值等夹具                              | 核心CPU与原生有限契约已覆盖；公开头与相关状态源身份可复用，不冒称每个旧GPU夹具在004重跑                                               |
| A10 在途更新/容量/回收            | 旧夹具无截图submit后立即逻辑destroy；新故障probe证明真实在途texture wait/终止保留                                          | 部分且关键缺口：旧材料明确未强制GPU重叠；正常在途mesh更新/删除、容量失败及退休最终正常回收/峰值尚无完整实证。Timeout quarantine不是正常回收通过          |
| A11 resize/最小化恢复          | 旧D/R完整游戏最小化12.508/12.067秒与原生10秒恢复；Release004用户本轮人工核查全部通过                                          | 有旧自动及当前人工通过依据；新人工无逐操作日志，不伪造fence/Present计数或精确起止                                                |
| A12 VSync/MSAA            | 旧原生Fifo↔Immediate实际Info、1x/4x和覆盖像素；新004真实4x/Immediate                                             | 已有真实模式/样本依据；旧开关夹具保留身份，不从VSync-on推定off预算                                                        |
| A13 资源/分配故障               | 旧D/R隔离缺/坏DXIL、坏atlas/YAML及构造恢复；006同源D/R真实后端分配HRESULT注入保旧资源                                        | 部分：故障类型已有证据，不能称未做分配验证；004完整游戏资产故障退出链未本轮重跑，真实OOM未覆盖，不要求故意耗尽整机内存                                 |
| A14 DeviceLost/提交/Timeout | 新D/R自有设备API移除、真实queue gate/copy/fence未完成时500msTimeout、Execute后Signal HRESULT注入，首错/拒续交/有限退出        | 受控原生加有限注入路线已覆盖，实际在途不再记未测；public wrapper/整游戏异常关闭仍无同场景集成实证，自发事故/实际Signal API失败非注入成绩，不新增整机TDR必验要求 |
| A15 正常/异常/重复关闭            | 旧原生正常drain/重复Shutdown/缺坏shader后恢复；新版CPU状态/cleanup、故障probe首错/失败重复关闭；用户Esc/关闭/重启反馈                  | 部分：正常关闭与受控失败各有依据，完整应用在同一原生故障下的关闭聚合及长期重复启动资源增长未证明；quarantine不当正常清理                              |
| A16 诊断                    | 旧D/R原生及新D/R故障/004采样均validation开启；当前InfoQueue将warning/error/corruption具名fatal，正常采样与关停成功            | 已有D3D12 Debug Layer依据，未发现所查记录的未解决原生同步错误；不是所有驱动问题证明，旧GL适用诊断仍绑定旧Debug身份，Vulkan待R4                |
| A17 截图/异步样本               | 新6轮PNG/精确frame ID/CPU-GPU字段由原生消费者与报告probe校验；旧无截图异步稀疏ID、CPU缺失/丢弃原因夹具                               | 所列截图/样本契约已覆盖；256就绪队列溢出没有新的原生专项实测，不据正常全部ready承诺无丢失                                              |
| A18 协议/安装/部署              | 新旧GL25列/现代32列v1原生消费者全部接受；Release004安装包真实运行，CRT/notices/imports/source/DXIL hash+SM6.0工具身份在包来源     | 部分：运行报告未自带完整shader生成身份，包外关联不等于运行时闭合；新版无关/中文空格CWD/干净无SDK机器、现代GUI人工未全部留证。GL退役条件未触发，不要求提前删GL      |
| A19 本机人工玩法                | 2026-10-12用户报告本轮提供的Release004显式D3D12人工核查全部通过，结合先前15分钟无异常反馈及当前命令/候选身份                              | 用户确认通过，非自动游戏回放；不重开要求再玩15分钟，不补造起止时刻/帧内存曲线，不继承M5/其他设备成绩或R3节点批准                                   |

结论：不是“所有项目未测”，也还不是“全部接管门槛通过”。优先需要补证的是A10的正常在途资源退休，以及A13–A15的真实生产调用方故障/关闭整合；A04新版增量、A18运行身份/适用部署与正式视觉参考/容差仍部分。正式长采样用户已取消且R2工作收口获批，保留其预算未验收风险，不自动恢复；是否将这些剩余项作为R3延期/例外须用户另确认，不能由R2批准继承。1175/Fix1保持原风险，不以修复它作为本轮重启暂停任务的前置；Vulkan/其他GPU/GL最终退役不提前列为D3D12接管阻塞项。本轮只完成证据对账，不提出或执行未经确认的新测试矩阵，不登记R3批准/切默认。
