---
tags:
  - area/spec
---

# T4 验证记录

## 2026-10-19 T4整体完成批准

本聊天用户明确决定“批准T4整体完成，保留上述风险，进行change archive”。据此本机RTX5070Ti范围的T4整体验收通过，S01–S06保持6/6，候选仍为下节绑定的MSVC14.51 Release游戏13c060179f15e03c7376ab606f308a3d4fb8a819ec5aaa74c5f6310b3ca4f7e6及对应48文件包；本次只登记用户决定，没有重新构建、测试或采样。唯一关键节点登记在[VERIFICATION](../../../../verification/VERIFICATION.md#关键节点)，下文“待整体批准/未通过/未完成”等保留各自历史时点。

批准明确保留[S06对账中的风险](#保留风险与批准边界)：CPU Debug 83/84及foundation.files/1175、旧导出/缺码失败和失焦/尖峰均不改判；M3-I1/I2继续开放、Fix1暂停，长时/中低端/真正干净机、跨工具链未重跑范围与CLion实际选择等不新增通过结论。A23正式physics搬迁、T5全面重构、GL退役、计算服务、Git提交/推送/合并均不由本节点授权。

用户随后明确选择“同步规格后归档（推荐）”。2026-10-19已将platform/simulation的7项修改及application-lifecycle/physics-core/telemetry-lifecycle等13项新增要求同步到现役规格；逐份对账全部已同步，原标题/Purpose、未涉及条款及继续有效场景保留。E/F标准规格校验各9/9，额外strict检查仅platform首条正文长度超过500字符警告，不拆改稳定条款；不将格式校验当运行成绩。Purpose中的旧目标/未实现字样保留原编写时点，当前契约读Requirements、实际交付读本记录。

T4 change已移入`docs/openspec/changes/archive/2026-10-19-m3-t4-application-simulation/`，同批文档与必要链接同步F审核目录；不再是活动计划。归档保留各自.openspec.yaml与原始证据，不删除原out、包、失败或暂停Fix1现场，也不归档T3/Fix1。F代码仍为T3，未合并T4；不授权Git或T5。

归档后E/F各核对118条归档相对链接，均无缺失目标/锚点；五份现役规格和十份归档Markdown字节一致，各自.openspec.yaml保留移动前原字节。生产/测试/工具源码、既有冻结归档及当前游戏包身份未变，未重跑运行测试。全库文档审计仍有E53/F17项既有缺失材料/锚点问题，本次不扩修或宣称全库无错误；本批差异空白检查通过。

## 2026-10-19 S06收口与T4整体验收申请

用户在收到新编译器Release安装包的D3D12/Vulkan普通入口命令后明确反馈“可以，人工验收通过，接下来汇总S06并进行T4整体验收”。据此登记本次A22四组短体验人工部分通过，绑定下述同一候选及seed424242/regression；不是沿用安装004的旧反馈，不补造逐组操作、精确用时、15分钟/长时或额外人工checkbox。整体T4节点在本次汇总后提请用户明确批准，当前不先登记为整体已通过。

S06实施方交付与适用证据已完成对账，任务S01–S06为6/6；这是技术交付完成并申请整体验收，不把全部历史测试改判全绿。候选仍只位于E:/Codex/Worktrees/refactor-application-simulation/Symocraft，整合上游已批准T3 604c2b58并保Q01=2/Q02=1/Q03=2/Q04=3及D01–D13；本轮不重选接口、不改生产或测试源码、不新增运行矩阵、不Git写入/规格sync/archive或启动T5。下文未完成/未人工/未采样等旧文字保留各自时点，以本节为当前交付状态。

### 当前候选与适用结果

正式CLI工具链为MSVC14.51.36231（cl19.51.36260.0）、SDK10.0.22621.0，Release安装为out/tc51-002/deploy space 中文，游戏SHA256为13c060179f15e03c7376ab606f308a3d4fb8a819ec5aaa74c5f6310b3ca4f7e6。此次只读身份/结果核对out/m3-t4/s06-closeout-001/result.json通过：3296项游戏生产输入及48个包文件仍匹配编译器切换的delivery-003，正式性能27轮/18组配对、原CSV及此前失败/重采字节均保持。不是新构建/测试成绩或完整可重现仓库冻结。

| 交付范围 | 汇总结论与必要边界 |
| --- | --- |
| 当前新工具链构建/CPU | 六个新目录构建成功；全后端游戏Debug启用95/95、Release96/96，CPU Release85/85。CPU Debug83/84，foundation.files closed_target实际1175失败，Debug world.allocations另禁用1；保留失败，不称全套自动测试全绿。D12/公开消费者/100例正负边界及最小ECS/分配故障随适用集合通过，见[工具链记录](#2026-10-18-正式编译器切换) |
| 独立工具/关闭测试 | 限定发布兼容修补有修补前错误5红灯；最终Debug/Release各9/9，工具Release关闭测试构建成功/CTest空集，游戏Release关闭测试同样成功/空集。原两项工具故障的历史目标持有者未知，原6/8保持，不以此次通过证明1175已解决，见[限定修补](#2026-10-18-工具发布兼容性限定修补) |
| 配置/增量/真实故障 | 10月17–18日的扩展单/双现代/GL/CPU/根及独立工具矩阵、每配置8项mtime探针、真实SDL/窗口/GPU/在途/异步与受控故障已有各自证据；部分整体结果因发布/导出失败为非零，正常清理与导出失败分开。原MSVC14.38身份不变，新工具链只刷新用户确认范围，不冒称全原生故障/全配置增量均在14.51重跑，见[S06续行](#2026-10-17-s06组合验证续行) |
| 协议/安装 | GL25列与现代32列、seed/441区块/361网格、生成/摘要/终态及两个原生消费者均有实证。新包无测试/开发库，八份VC145 CRT匹配14.51.36247.0；无关CWD/System32-only PATH的三后端预检及两配置30帧启动共9项通过，不是真正干净机器 |
| 正式性能 | 固定最新明确重采003为walk-3，所选27/27有效、两现代各9/9组满足原绝对/GL110%帧时间及两项内存门槛；原CSV独立复算一致、旧失焦及002不覆盖，GL81.2144ms尖峰完整保留。不是跨编译器T3迁移净收益或长时证明，见[正式采样](#2026-10-19-t4新工具链正式前台采样) |
| 新候选人工体验 | 用户明确通过所给D3D12/Vulkan普通入口的四组短体验，按既定每后端最多300秒范围登记总体反馈；精确操作/用时未提供，不扩成M5或逐项原14项表 |

### A项交付对账

| 稳定身份 | 已交付依据及验收限定 |
| --- | --- |
| A01–A03 | 真实公开头/唯一owner、独立physics消费者与负向边界、CPU/独立工具/关闭测试；同一生产库而非复制实现 |
| A04–A06/A09/A20 | 分段装配/重复关闭与首错报告、自有输入/pending/Wait/失败锁存、暂停恢复/窗口事实和场景隔离；真实SDL/GPU受控证据与新候选人工反馈分开，非任意OOM/时序保证 |
| A07–A08/A10–A16/A24–A26 | 双会话预算、自有结果、物理/World适配、预检/Failed/部分提交；D12每Advance及World提交固定阈值对照、视图省略/重复及旧结果寿命，未改为CCD或共享Registry事务 |
| A17–A19 | 独立Scenario、GPU关联/明确缺失、Sealed不可变、关闭报告汇合、显式导出重试及Saved无重写、退出0/4/3；允许并保留真实发布失败，不据此声称无故障文件事务 |
| A21 | 普通/benchmark协议、资源/CWD/中文空格安装、生成身份与新工具链包绑定；本机受控部署不代替M6干净机器 |
| A22 | 本机RTX5070Ti上D3D12/Vulkan组合自动证据、正式预算及本次四组人工总体通过；不是另一GPU、中低端或长期稳定性结论 |
| A27 | 源码再次核对session.cpp自有结果及D09实测成本触发注释；simulation/app/platform/telemetry机制已落文，修正三处过时状态说明，不新增公开Inspect |
| A23 | 本次不适用、不勾通过；正式physics所有者搬迁仍在T5审计后/NPC复用前另立授权节点 |

### 保留风险与批准边界

- M3-I1/I2按既有问题表保持Open不阻塞、Fix1继续暂停；当前CPU Debug及此前导出1175/缺码失败真实保留。工具兼容缺口修补、正式采样/人工通过都不证明发布稳定性根因闭环。
- 本轮采样的失焦来源及GL尖峰根因未知，不删除或重判原失败；短体验/90秒窗口不证明罕见卡顿、持续资源增长或M5长时。完整原生资源耗尽、自然设备事故/系统OOM未由受控注入保证。
- 指定中低端整机仍为下一玩法功能实施前门槛；真正干净机/M6、CLion共享toolchain实际选择未实证，正式同源Q06及启动/首次可操作预算保持原边界，不新增其为T4必须修复门槛。
- F仍仅含已批准T3生产代码与本任务审核docs；T4验收不等于代码合并、Git提交/推送、current specs同步/归档、GL退役、全面ECS/内存T5或服务化授权。

实施方结论：按已确认本机/T4范围建议进入整体批准，未发现需重开Q/D或扩大迁移的新冲突；既有失败与未覆盖项按上述边界透明保留。人工部分已获用户明确通过，当前仍等待用户对本汇总的整体节点决定，不以6/6或文档校验代替批准。原生apply确认6/6/all_done，E/F严格validate、受影响T4及三份架构文档链接/空白检查通过；全库audit仍有E53/F17项既有错误，不称全库通过。五份本轮docs已按同步身份保护写回F审核目录，不改全局批准表或现役spec。

## 2026-10-19 T4新工具链正式前台采样

当前结论：所选27/27轮有效，D3D12/Vulkan各9/9组同时满足原帧时间及两项内存预算，独立原始CSV复算一致；这是T4正式性能检查完成，不是S06或T4整体验收，也不关闭Fix1/1175。原失焦轮及两次明确授权重采均保留，下列分段说明本轮执行来源。

用户在聊天确认27轮范围并授权Computer Use仅将采样窗口置前；主agent独自执行，固定同一MSVC14.51 T4 Release候选。顺序GL九轮→D3D12九轮→Vulkan九轮，static/walk/edit各三轮，30秒预热+90秒采样，1920×1080/4x/VSync-off/seed424242，现代validation-on；严格前台及有界准备保留，原P95/P99绝对/同批GL110%相对及两项进程内存窗口最大值110%预算不变。不重建同工具链T3旧候选或计算跨编译器迁移净收益，不恢复Fix1、追加人工体验或作Git/规格sync/archive。

本轮新目录为out/t4-formal-001，既有采集器基本检查7/7通过；游戏源码/资产等3296项production输入及48文件安装包与tc51-002/delivery-003一致，Release游戏SHA256仍13c060179f15e03c7376ab606f308a3d4fb8a819ec5aaa74c5f6310b3ca4f7e6，消费者使用本次限定修补后的独立工具Release。北京时间15:07:56–15:20:14执行六轮后按约定停止：GL static三轮及walk前两轮有效，首个walk-3正常完成导出/关闭但exit4，summary明确window-not-focused，不是发布1175或GPU预算失败。其17723总帧中1454个预热帧失焦（elapsed 5.0982306至17.4820595秒，帧间隔累计约12.390524秒），13910正式帧全部获焦仍不能截掉预热判有效。原批有效5/27，剩余21轮未启动，两现代本批尚未采样；没有超时/强杀、自动重采或删帧。失焦时究竟哪个窗口取得前台没有记录，不归因为用户操作、IDE或锁屏。

Computer Use首次置前调用报foreground window did not report a process id；刷新返回窗口后的一次恢复置前成功并实际查看GL画面，不据此推断每轮均由桌面工具激活。游戏已有SDL显示请求/有界准备及执行器各轮只读前台确认仍有效；前台确认成立不保证整个预热和采样持续获焦。原批postcheck.json独立复算五个有效轮的nearest-rank、内存窗口/最大值及原文件hash，保留GL static第2轮两个>20ms帧（其中一个>50ms，最大81.2144ms），没有剔除尖峰。该离线核对通过仅是部分数据核查，不是采集完整或预算通过；生产/包/采集输入三项unchanged均true。

用户随后明确要求仅重新采样walk-3，于独立out/t4-walk3-002使用同一候选/参数执行一次，不自动续跑其他21轮。北京时间15:23:16–15:25:19成功，Computer Use对该唯一游戏窗口置前并查看，19697总帧（含预热）全部获焦，正式14702帧；游戏/两个原生消费者均exit0，finished/valid_run=1，诊断为空且无超时/强杀。P95=7.2377ms、P99=7.9700ms、max=12.9042ms，无>20ms正式帧；90次正式内存观测，工作集最大515620864字节、私有提交最大1130909696字节。独立postcheck复算一致，原失败result及六份capture产物hash未变，production_sources/package/inputs_unchanged均true。

用户再次要求walk-3重采，独立out/t4-walk3-003执行一次成功：19619总帧（含预热）全部获焦、正式14657帧，P95=7.3436ms、P99=8.0277ms、max=10.1614ms，无>20ms正式帧；90次内存观测，工作集最大519569408字节、私有提交最大1134026752字节。真实游戏/两个原生消费者检查及独立复算均通过，生产/包/输入及上轮产物字节保持。002成功与原失焦轮都保留，不以重采消除原失败。

随后用户明确要求walk-3后继续其他21轮；out/t4-remaining-001于北京时间15:31:51–16:15:10完成GL edit三轮→D3D12九轮→Vulkan九轮，新增执行器仅组合既有真实采集/原生消费/验证函数，不改生产/测试源码、焦点或预算语义。主配对固定原五个有效GL轮加最新明确重采003及本批21轮，不在002/003间挑最好或平均；原批failed状态及002独立成功保留。本次续行没有新增无效轮、重采、超时或强杀。

最终result.json仅引用原产物、不改写前两批；27个主轮次共1249451帧（正式936763帧），含预热全部focused=1，游戏/各自两个原生消费者全部exit0、正常导出关闭，GPU缺失样本0。按场景/轮次逐一核对18组workload/digest/seed/4x等身份及预算，不混池、择优或删慢帧。正式窗口仍30<=elapsed<120秒，P95<=10ms、P99<=12ms且各自<=对应GL的110%；工作集和私有提交分别以约每秒观测的正式窗口最大值<=对应GL的110%，不叠加T2绝对内存上限。独立postcheck.json从原CSV复算nearest-rank、全帧焦点、内存覆盖/间隔/最大值及18组预算全部一致。

| 后端 / 九轮范围 | P95 ms | P99 ms | 工作集 MiB | 私有提交 MiB | 逐对结果 |
| --- | --- | --- | --- | --- | --- |
| GL | 6.8913–7.7876 | 7.5599–8.6672 | 486.590–517.559 | 1074.363–1102.961 | 本批基线 |
| D3D12 | 1.8793–2.7019 | 2.1869–3.8354 | 451.996–462.125 | 725.586–732.566 | 9/9全部门槛通过 |
| Vulkan | 2.6332–2.8784 | 2.9677–3.8950 | 461.648–464.617 | 753.922–760.035 | 9/9全部门槛通过 |

表中为各轮范围，不用范围端点交叉配对；逐对应轮次最不利工作集/私有提交比值为D3D12 94.18%/67.70%、Vulkan 94.88%/70.26%，均低于110%。两现代正式帧均无>20ms或>50ms帧；GL原static第2轮81.2144ms尖峰仍计入统计，max不是新增预算。实际设备仍RTX5070Ti/vendor10de/device2c05、D3D12 driver32.0.16.1664、Vulkan NVIDIA:616.64，现代4x/Immediate/validation-on；Vulkan九轮均0警告/0错误、native_retired=true。没有绑定私有热观察器、关闭验证层或降低负载来取得结果。

续行production_sources/package/inputs/originals_unchanged均true，包含此前失败、002和003全部原文件hash；原包及源码身份保持上述新编译器候选。所选27轮协议时间54分钟，明确重采/停止及跨批准备间隔单列，不能冒称27轮无中断连续运行、同源T3迁移净收益或罕见尖峰/长时内存增长已证明。中低端、干净机器和新编译器候选的额外人工体验未执行，Fix1仍暂停；S06未完成，不自动发起人工或整体验收。采样已结束、无遗留游戏窗口，docs同步F仅供审核，不表示代码合并或规格sync/archive。

## 2026-10-18 工具发布兼容性限定修补

用户明确授权“限定修补工具侧发布兼容性并补回归，不扩大到暂停的Fix1”；主agent独自实施，仅E候选的tools/benchmark/src/platform.cpp、私有platform.h及test/benchmark的测试/CMake。独立工具仍链接自身benchmark_core，不引入foundation；runner.cpp及game/modules/foundation/src/files.cpp保持逐字节不变，不改变终态协议、游戏算法或故障判断，不作Git写入/规格sync/archive。

真实WriteYaml保持写完close后发布，已存在目标改用ReplaceFileW(flags=0)；仅ERROR_FILE_NOT_FOUND进入一次MoveFileExW(flags=0)，首次创建不覆盖竞争目标。两次API均立即保存原生错误，再构造包含api/flags/temporary/destination/初次replace_error的system_error。不预查目标存在、不自动重试/sleep/先删目标或清理失败tmp。兼容读者保持旧快照，新路径读取新内容；真正不兼容的读者仍失败。ReplaceFileW晚期失败可能改变文件状态，本修补不是事务/崩溃恢复或1175修复，不声称任意失败都保留旧目标。机制见[benchmark架构](../../../../architecture/benchmark.md)。

新增benchmark.publication链接同一真实生产库，私有函数绑定仅用于确定性竞争/错误分派，不复制实现、不向runner.h增加公开接口。覆盖首次发布、自有share=7读者旧快照/新路径、真实share=1阻塞错误32、自建只读目标错误5、缺失源错误2与首次竞争不覆盖；关闭阻塞后的再次写入是调用者显式操作，不是生产重试。真实竞争记录并比较当次Win32错误80或183；注入5/1175/1176/1177只验证原码/路径传播及无fallback/重试，不冒称复现其真实文件系统后果。首次Move失败另验证32及replace_error=2上下文。

| 本轮新目录 / 同一新工具链 | 实际结果 |
| --- | --- |
| out/tc51-pub-red-001，修补前Release新兼容读者回归 | 构建成功，benchmark.publication按预期失败，system/native_code=5；保留红灯现场 |
| out/tc51-pub-fix-001/debug | 完整独立工具构建成功，CTest一次9/9通过，含publication/executor/vulkan，38.12秒 |
| out/tc51-pub-fix-001/release | 完整独立工具构建成功，CTest一次9/9通过，含publication/executor/vulkan，34.31秒 |
| out/tc51-pub-off-001/release | BUILD_TESTING=OFF全新构建成功，CTest0项；不依赖新增测试 |

使用固定MSVC14.51.36231/cl19.51.36260.0及原SDK10.0.22621.0，不借切换编译器掩盖失败；绿灯与OFF各自result.json记录四份修补输入且inputs_unchanged=true。受保护foundation files.cpp SHA256为179690ba5f6a3232968ffb791851f4617049ef10c821caff02cb9c05e12e547b，工具runner.cpp为0044bf899ae10d2a5dbf53c90748ce21fdb0d11186f43241a894901ec62d240b，两轮均一致。未循环补跑失败套件，原tc51-002的6/8及tmp、诊断目录原样保留。

本轮闭环的是可控复现的工具兼容性缺口及其受影响回归；原两次故障当时的目标持有者仍未知，不据此宣布历史原因完全查明。foundation/导出1175风险及Fix1暂停状态不变，S06不勾选；没有新GPU、安装包、人工或正式性能成绩，旧游戏包身份不变。docs同步F仅供审核，不代表F已应用工具代码。E/F严格validate及受影响diff空白检查通过；全库文档audit仍分别53/17项既有错误，本change错误0，不称全库通过。

## 2026-10-18 工具发布失败定向核查

用户本轮仅授权定向核查工具两项失败原因；主agent独自进行源码/原现场只读核对及有界CPU对照。未改生产/测试源码或发布机制、不恢复Fix1全面诊断、不启动Procmon/正式性能、不作Git操作，原始失败及新诊断输出保留，docs同步F审核目录。

原executor的instant-failure和vulkan的modern-bad-generation均残留旧session.yaml（attempts=0）与新session.yaml.tmp（首static attempt=1/state=running），尝试目录尚无stdout/stderr/capture。两者可定位到runner.cpp首次将轮次标running后的WriteYaml，发生在创建模拟子进程之前，而非GPU/Vulkan绘制、游戏终态或取消。当前原文件属性均为Archive而非只读；这些点时观察不补造故障当时全进程句柄/ACL事实。

实际工具WriteYaml在MoveFileExW之前已显式close写流；ReadYaml用GENERIC_READ/share=7且局部RAII结束句柄，当前源码没有这条新会话路径持有session.yaml读者的依据。工具仍用MoveFileExW(REPLACE_EXISTING|WRITE_THROUGH，flags=9)，与foundation已使用的ReplaceFileW/首次不覆盖Move分支不同，不能因为foundation有窄修补就认为独立工具也已修补。

在 `out/tc51-diag-002` 直接链接真实benchmark_core/symocraft_yaml，以MSVC14.51原库及相同当前源码全新MSVC14.38 Release库对照，未复制生产cpp或改变其实现：两版无读者的33次写入均成功；持有本测试自己的share=7读者时，第二次WriteYaml均具名system错误5，旧target和新tmp都保持；真正禁止删除共享的读者同样拒绝。数值码来自本次catch的system_error.code，不回填原两项缺码记录。新旧编译器的executor/vulkan套件各独立执行一次于新目录，四次均通过，仅表明本轮未复现原瞬时故障，不覆盖原失败/判修复/新增性能成绩；未循环补跑。

另以ctypes直接调用Win32、显式成功WriteFile及CloseHandle，33次无读者发布完成；原生三项独立控制得到：

| 自有目标读者 / 发布API | 本次实际结果 |
| --- | --- |
| share=7 / MoveFileExW flags=9 | 立即错误5，旧读者及目标仍旧内容，tmp保留 |
| share=7 / ReplaceFileW flags=0 | 成功，旧读者读旧内容、新路径读新内容，tmp已消费 |
| share=1 / ReplaceFileW flags=0 | 立即错误32，旧内容与tmp保留，没有吞掉真正不兼容的读者 |

这证明了工具现有发布方式不兼容目标仍被打开的具体缺口，不依赖MSVC2026/STL、游戏/SDL或真实GPU。Windows普通rename对打开目标的限制及可选POSIX替换语义可参照[Microsoft FILE_RENAME_INFORMATION](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information)；未据此选用新机制。初始 `tc51-diag-001` 只因临时探针使用错误YAML include路径而未编译，002沿正式生成的窄头视图后运行，保留前者，不算一次发布尝试。

根因结论分层：发布兼容缺口已有可控因果复现，且新旧编译器/纯Win32均有对应证据；但原两次失败发生时究竟谁打开目标、是否确由同一条件触发，现有记录仍不足。不得归因为杀毒/IDE/SDK，不能宣布原两次根因完全闭环，也不把错误5和foundation的1175混为同因。生产与测试四份输入核对sources_unchanged=true；本轮仅诊断，没有修复或后修复回归。后续如修工具发布，应单独覆盖兼容读者快照、真实阻塞、首次创建/竞争及即时code/path，不能靠重试/sleep/先删目标或放宽终态；不由此恢复Fix1或承诺换API就解决1175。

## 2026-10-18 正式编译器切换

用户确认四份规划修订、同步F审核目录并授权切换；随后确认六个全新目录：Debug/Release全后端游戏及现有自动测试、两配置CPU-only（含D12/分配故障）、Release独立benchmark、Release关闭测试，以及三后端有限帧和安装包检查。主agent独自执行，仅E代码根；不恢复Fix1、不启动正式性能或新增人工体验，不作Git写入/规格sync/archive。

实际已安装稳定VS2026 18.11.12306.390，工具集目录14.51.36231，cl/c1/c1xx/c2为19.51.36260.0、link14.51.36260.0，英文1033齐全；Windows SDK仍10.0.22621.0，CLion既有CMake4.3.1/Ninja1.13.2未升级。D3D DXC沿SDK22621，Vulkan头与SPIR-V DXC仍1.4.363.0，不升级SDL/第三方。构建和原生验证helper默认发现稳定18.x、固定工具集/SDK并核对实际环境，显式安装/工具集可回退；build.py在修改前拒绝不同编译器缓存。旧VS2022、缓存和安装004保持，不改变系统/CLion共享设置。

首轮 `out/tc51-001` 正确识别新编译器，但候选部署检查写死Microsoft.VC143.CRT而配置失败，未构建；保留现场。限定适配为按CMake实际MSVC_TOOLSET_VERSION选择VC145及同工具集Redist，打包校验同样读取实际族，不借旧CRT/放宽版本校验。后续六目录均在 `out/tc51-002`，全部构建成功；没有修改游戏C++算法、预算或故障语义。工具链入口定向Python22/22通过；日常build.py另以默认发现/固定版本成功配置并构建本批Release目录，Ninja仅执行既有资产同步，无C++重编。

| 新工具链范围 | 实际结果 |
| --- | --- |
| 全后端游戏Debug | 启用95/95通过，world.allocations沿既有默认Debug STL规则禁用1 |
| 全后端游戏Release | 96/96通过 |
| CPU-only Debug | 启用83/84，foundation.files的closed_target阶段ReplaceFileW实际1175失败；world.allocations禁用1 |
| CPU-only Release | 85/85通过，含world.allocations |
| 独立benchmark Release | 6/8；benchmark.executor、benchmark.vulkan均报Publish YAML: Access is denied.，未输出数值码，不冒称1175 |
| 全后端BUILD_TESTING=OFF Release | 构建通过，CTest0项 |

D12逐次对照、核心/消费者与边界、最小ECS/foundation分配故障均随适用CPU/游戏CTest通过，未改变阈值或用专用ABI配置替默认Debug。六份真实compile_commands核对生产cpp单owner，CPU无SDL/平台实现/原生后端，独立工具无game源码，OFF无test源码。独立工具初始未导出编译数据库，后仅开启CMAKE_EXPORT_COMPILE_COMMANDS生成查验输入，没有重跑失败测试。

工具两项失败属于本批新观察：保留原始失败及.tmp，源码可定位到独立benchmark的WriteYaml/MoveFileExW发布路径；当前只有错误文字，不把它与foundation的ReplaceFileW/1175混为已证同因，也不推定MSVC回归。未修改发布机制、恢复Fix1、刷重试或把6/8写成工具协议全通过；这两项的因果诊断/修复尚未完成。

真实部署驱动前两次分别因缺独立工具编译数据库、环境键大小写读取失败，均发生在启动游戏前，保留delivery及delivery-002而不计运行成绩。最终delivery-003通过Release三后端资产预检，以及Debug/Release每后端30帧（六次），全部exit0/ready/准确帧数/显式关闭。Release使用48文件安装包 `out/tc51-002/deploy space 中文`、无关空CWD及仅System32的PATH，八份app-local CRT逐字节匹配所选VC145目录、实际文件版本14.51.36247.0；静态导入无SDL3/DXC编译器DLL，所需MSVC运行库均app-local。Debug仅本机诊断，PATH明确提供匹配VC145 DebugCRT及SDK debug UCRT，不称可分发或干净机器验证。

两现代实际RTX5070Ti、vendor10de/device2c05、4x/Fifo/validation开启；D3D12 driver32.0.16.1664，Vulkan NVIDIA:616.64；GL实际4.6.0 NVIDIA616.64。不属于VSync-off性能场景、真实键盘/人工体验、罕见时序或长时验证。Release exe SHA256 `13c060179f15e03c7376ab606f308a3d4fb8a819ec5aaa74c5f6310b3ca4f7e6`，Debug `ca4f675e8bda19704c59b1416f0ae656da526f1c88e726cb1f5866622ebc666e`；3296项production输入及构建/工具/CRT/包身份在 `out/tc51-002/delivery-003/identity.json`，该记录SHA256 `4ae6041c8870c1565dcb3af54d9b9bdad0139e1576d95613974cf7dc1641827d`，检查期间production_sources_unchanged=true。不是完整可重现仓库或新冻结附件。

E/F本change严格validate及受影响diff空白检查通过；全库文档audit仍分别53/17个既有错误，本change新增错误0，不称全库通过。本轮实际切换的是E候选的命令行构建/验证入口及产物；CLion共享toolchain未修改，IDE实际选择未实证，不能把CLI成功当IDE配置完成。F仅同步本任务docs，不同步T4生产代码或改其他worktree工具链。

旧人工反馈及测试继续绑定安装004/旧MSVC，不自动继承给新二进制；D/Q与D12阈值保持，S06仍未完成。当前原始日志/命令在上述out目录，最终必要结论维护本节，不新增附件或平行验证报告。

## 2026-10-18 人工短体验反馈

本聊天提供S06 Release安装004的D3D12/Vulkan启动命令（seed424242、regression普通入口）后，用户反馈“人工短体验没有问题”。按本次约定的人工短体验范围记录为无异常，不补造逐组/逐后端操作明细、实际用时或新的自动验证成绩；不扩展为原15分钟长时、正式性能、中低端/干净机器或T4整体验收。未填写额外人工checkbox，S06仍未关闭。

候选仍为下节绑定的安装004及Release游戏 `1f6a6f2a75834d9c9be878809a187c4013244fb4ca7b23c401b4e43d5b986050`，本次只登记反馈并同步F审核目录，无代码/资产/包变动。共享发布1175及同类失败仍按原证据保留，人工普通入口无异常不证明记录发布可靠性已修复；Fix1暂停状态不变。

剩余正式性能对照尚未执行：聊天提出的27轮30秒预热+90秒采样、预计65-80分钟仍待确认，发布失败会使采样或导出失效，不能跳过、改判有效或自动重采。用户本次询问下一验证项，不构成恢复Fix1、修改生产发布实现或启动正式采样的授权。

## 2026-10-17 S06组合验证续行

用户本轮授权主agent独自继续剩余构建、真实mtime增量及故障组合验证；正式性能须先在聊天说明方案与耗时，人工短体验准备好后再通知。代码/测试仍仅E worktree，相关docs及时同步F审核目录；不启动子agent、不扩大T5、不作Git写入或规格sync/archive。S06保持未勾选。

首次配置矩阵 `out/m3-t4/s06-config-matrix-001` 的D3D12 Debug配置成功，但两个simulation公开头测试的obj绝对路径达266字符，MSVC报C1083/Invalid argument；不是公开API编译语义失败，也未算通过。保留原构建与日志，改短构建父目录 `out/s6b1`，同一公开头路径243字符；新 `s06-config-matrix-002` 按11模式各Debug/Release重跑，包括四种游戏后端配置、Vulkan/双现代关闭测试、根/独立工具及关闭测试和all-off预期拒绝，执行结果待补，不用预期冒充实测。

仅补测试：D3D12/Vulkan缓存CPU用例增加写入中断后重试、失败纹理替换不沿用旧命中、selection写失败重试及零/上限/size_t端点容量检查。不改生产缓存、公共分配器或诊断API；这些不证明真实GPU描述符堆/池耗尽或系统OOM。后续真实GPU、在途、异步容量及受控分配/提交/设备/超时组合分别取证，不以CPU元数据替代。

增量已实际完成：`s06-incremental-003` 的Debug/Release各8项真实mtime探针通过、settled baseline均0编译；核对私有renderer/backend、公开types、app world-cache、D3D12 pipeline、Vulkan私有头及三组同源shader的准确owner/生成数量，源码内容与原mtime恢复。Debug事后CTest启用95/95、world.allocations禁用1；Release95/96，foundation.files实际Win32 1175失败，不能称全绿。此前临时驱动001参数别名错误未执行探针，002在前置CTest的foundation.files/1175后又遇日志输出GBK编码错误，均保留；003将增量与CTest结果独立记录、已知1175具名保留，不以重试刷通过。

CPU目录刷新包括新增缓存断言：Debug `s06-cpu-001` 为83/84启用、world.allocations禁用1，telemetry.renderer_performance在Export finished的ReplaceFileW发布失败；该探针原stderr未输出数值码，因此不冒称已精确核到1175，临时驱动也有GBK打印错误。Release余下工作单独在 `s06-cpu-002` 完成：84/85，foundation.files有实际1175；未重复Debug或恢复暂停的Fix1。原始失败与上一轮CPU全通过分别保留其身份，不把旧成绩覆盖本轮失败。

执行跨至2026-10-18。配置矩阵002已完成且source_unchanged=true：20个有效配置全部构建成功，all-off两配置明确拒绝；D3D12、Vulkan、双现代各D/R的启用CTest分别91/92、92/93、92/93通过（Debug各禁用1）；GL为93/94启用及94/95，均foundation.files实际1175失败，故矩阵all_passed=false。根工具D/R各17/17、独立工具各8/8；八个关闭测试构建CTest均空集，原生SDK/生成资产隔离及三后端选择/具名Unsupported核对成立。加上另行刷新CPU两配置，本轮覆盖原12模式D/R范围，但不称全套测试全通过。

原生001在Debug D3D12生命周期通过后，Vulkan的Export exporting发布失败导致旧探针提前结束，未计整体通过。仅改测试记录原生码并累计导出失败，让十阶段/重复关闭/次错后Runtime的独立清理断言继续；最终有任一导出失败仍exit1，不重试、不改生产发布实现。shared ExportSealed测试helper也只增强失败诊断数值码。`s06-refresh-001` 的CPU/游戏D/R两个导出消费者各2/2通过，属于这次日志改动的定向复验，不覆盖此前失败或证明1175已修复；矩阵002保留此前测试源码快照身份，无需借日志增强重跑全部构建。

`s06-native-002` 的Debug/Release均已完成真实SDL工厂/输入分配失败、D3D12/Vulkan/GL生命周期独立清理断言、四个正常GPU缓存probe、两现代受控游戏故障、Vulkan资产/WSI、13模式在途/故障、5模式异步、D3D12三模式在途/异步及四种门面故障。两配置Vulkan生命周期各1次finished导出确有system/1175，全部清理断言完成但各exit1，不能称生命周期完整通过；其余上述检查exit0。26条顶层执行记录中24条exit0、2条具名导出失败；子矩阵按各自结果解释，不把预期终止保留算正常释放。原生在途有实际未完成fence/timeline证据，异步256容量/301提交/45具名缺失及大frame ID等沿既有严格校验；受控OOM/提交/设备故障不冒称系统耗尽或自发硬件事故。

| 本轮最终补齐 | 结果及边界 |
| --- | --- |
| 绘制/窗口组合 | `s06-window-001` 两配置各8个backend×drawing/world-window×1x/4x，共16案、400张实际截图通过；441区块/361初始网格、32项正反编辑、实际尺寸重建及最小化约10秒恢复按原探针核对。另两现代各D/R共4个普通生产入口有限帧窗口案通过，关闭exit0，无强杀；Vulkan实际暂停段Acquire/Submit/Present/重建均0。有限帧入口豁免失焦暂停，受控键事件与窗口动作不是人工键鼠、无限帧焦点策略或长时稳定性证明 |
| 当前候选身份 | 本轮仅改四个测试文件，无生产/资产改动；3296项输入与上一轮整合记录逐字节一致。增量重链后Debug游戏SHA256 `f5fe70c93c4380aed8784332060570acbd3e9b3cecec0b4c08036bb2b82c22e3`，Release/新安装游戏 `1f6a6f2a75834d9c9be878809a187c4013244fb4ca7b23c401b4e43d5b986050`；旧构建hash不移作新包身份 |
| 新安装与普通运行 | 新 `out/m3-t4/deploy T4 S06 space 中文-004` 为48文件，无测试/fixture/lib/obj/pdb；旧安装003的全部48项原字节前后复核保持。新包从独立且含空格/中文的CWD、PATH仅系统目录，三后端资产预检/普通4帧全部exit0。身份记录为 `s06-final-001/source-build-package.json`，SHA256 `cce308b51b739ce6ffd7d99d7e62a024bb49df418be0a422f935b295f0e17fd5`；不是Git提交、SDK快照或真正干净机器 |
| 新包短协议 | 三后端static/walk/edit各0秒预热+2秒功能短跑，共9次尝试，8份有效报告及各自生产probe/独立原生消费者通过。D3D12 edit在warmup状态的ReplaceFileW发布失败，原入口随后成功保存Sealed无效报告、exit3；未输出该首错数值码，不能补写为已实证1175，更不能算该场景有效通过。失败原样保留、未重采；实际查看Vulkan static截图有正确非空世界/纹理/红色选框，不作用户视觉验收或性能成绩 |

新包已具备普通玩法人工短体验条件，已在聊天通知；尚无人类反馈，不填A22或整体验收。人工候选用上述Release安装、seed424242/regression普通入口（不带benchmark/smoke-frames），两现代分别显式选择，共用既定每后端300秒上限。实际设备仍为本机RTX5070Ti，现代正常缓存probe实际4x/Immediate/validation-on；共享发布1175及同类未输出码的失败仍会影响记录/正式采样，Fix1继续暂停，不因后续定向通过注销旧失败。未单独覆盖全部原生描述符堆/池失败点、真实系统OOM、持续增长、罕见时序、中低端或干净机器；不自动添加长时替代组。S06仍5/6聚合中的未完成项。

正式性能方案已在聊天提出但尚未获本轮执行确认：同一T4 Release候选，GL/D3D12/Vulkan各static/walk/edit三轮，共27轮，每轮30秒预热+90秒采样，协议时间54分钟、预计总耗时65-80分钟。沿T3最后通过协议的分辨率/4x/VSync-off/seed、严格前台、原分位数及内存逐对预算，不自动重采或删帧；目的为核验当前T4预算，不把不同时段T3历史数值当迁移净收益。此轮尚未启动正式采样或人工体验，人工每后端四组合计300秒上限不变。

最终E/F的T4严格validate均通过，T4 change及本轮涉及的三个架构文档无链接错误，差异空白及四个改动测试文件的尾空白检查通过。审核副本四个指向尚未合并T4源码的链接已改到实际E候选；F全库审计因此21降至17项，E仍53项，剩余旧归档/导航缺项未修，不报告全库通过、不复制冻结材料。相关docs已校验上游未独立修改后同步F，未Git写入、规格sync/archive、重启Fix1或填写人工批准。

## 2026-10-17 T3批准基线整合

本轮用户明确要求将上游通过的T3改动同步至T4 worktree，并核对过时测试。基线为上游 `feature-refactoring` 的 `604c2b58`（父提交 `66cee95f`），不是早期安装004/007或未经绑定的工作目录。T3批准及最后局部优化未补完整回归等保留风险沿[T3完成对账](../../m3-t3-renderer/verification.md#2026-10-17-r4及t3完成确认)，不重判历史失败或继承为T4已验收。

生产整合仅在 `E:/Codex/Worktrees/refactor-application-simulation/Symocraft`，F仅同步可维护docs供审核；不提交、合并分支、规格sync/archive或扩大T5。保留T4实例Application/Runtime、自有输入、SimulationSession、ShutdownReport与Seal/Export；T3共享RunGame入口转接到同一T4生产owner，不恢复namespace静态生命周期。引入Vulkan、生成身份、最新层号摘要/绑定缓存、前台准备与原生消费者；旧探针须适配新owner/输入/观测接口，不能以复制旧app实现满足构建。

旧T4 `identity-001`、game `*-001`构建/包、D3D12/GL组合与退出结果仍是原候选历史依据，不代表本次源码；旧“Vulkan未编入/Unsupported”结论不适用于显式启用后的新候选。物理/Session/ECS/分配器生产实现未受上游T3修改，既有CPU数值依据不撤销；本轮已刷新其构建/数值及集成回归。T3预算与人工批准保留其原版本身份，不转成新T4性能等价或人工通过，不自动重启27轮长采样或追加人工时长。

来源核对：保留旧T4改动后移入上游94个非docs路径中的相应内容，应用重叠处按T4所有权适配；当前renderer子树相对批准提交的规范化文本仅Vulkan/legacy两实现的Window尺寸getter适配不同，其余相同。相对旧T4 identity，3296项生产/资源/选定vendor输入中31项变动，foundation/ECS/simulation/World/platform及assets全部逐字节不变；不扩公共分配器或T5。记录为E目录 `out/m3-t4/t3-integration-001/source-build-package.json`，SHA256 `e3f98854fe8b4087063c2b0807d9e330c29ec2023a892c378465f8309e6f9f99`，绑定两配置构建图、生成清单与48项安装文件；不是完整仓库/SDK快照、Git提交或T3冻结包。最终Debug游戏SHA256 `aba9501879b487e724482c85ea9ddfeae12781a7bfcd9a5a595d066e7d0ced01`，Release/新安装游戏 `83f98d0540c59b2a12668011680070da473ebe8510d21aa2b842067e28cffbcf`。

| 本轮刷新 | 实际结果与限制 |
| --- | --- |
| 游戏三后端构建及自动测试 | 新 `game-debug-002` / `game-release-002`，显式Vulkan ON并保默认D3D12与GL；最终 `t3-integration-test-002.log` 分别95/95启用、96/96通过。Debug原world.allocations禁用1不算通过；公开头、100项正负边界、D12逐次Session/World、最小ECS/分配器、封闭/导出与新增T3 CPU回归均在该现行集合内 |
| CPU与独立/关闭测试 | 刷新既有CPU D/R，84/84启用（禁用1）及85/85；独立工具8/8含Vulkan新消费者。关闭测试CPU及Vulkan开启游戏均生产构建成功、CTest空集。结果在 `t3-closure-{cpu,tool,off}-001`，不是重跑上游全部22配置 |
| 真实Application生命周期 | `t3-lifecycle-{debug,release}-001` 各D3D12/Vulkan/GL通过：十个装配取得后故障、复制配置寿命、全部适用清理、成功/失败缓存重复关闭、Window次错后Runtime仍关闭及原code32；受控故障不是实际硬件事故 |
| 原生缓存及GPU | `t3-gpu-release-001` 两后端层号摘要、D3D12 SRV缓存、Vulkan帧槽绑定/选框与buffer policy通过，实际5070Ti/616.64、validation启用，正常关闭；该组只链接未因后续telemetry阶段修补变化的renderer闭包 |
| 游戏故障及WSI | 最终 `t3-faults-release-003`：D3D12正常/分配/提交/受控设备移除/真实gate在途超时5案；Vulkan游戏5案、隔离资产5案及WSI能力拒绝/重建6案通过。首错、停止续交、实际在途及终止完整owner/loader保留分别核对，未把gate解除改成正常退休；不泛化为真实OOM、自发设备事故或Debug故障全矩阵 |
| 游戏及协议 | 最终 `t3-protocol-release-004`：三后端普通4帧与static/walk/edit各0秒预热＋2秒采样，共9份完成有效报告/截图；生产报告probe及新独立原生消费者各9/9接受，世界seed/441区块/361网格、摘要、实际1080p/4x/呈现/生成身份及旧25列/现代32列保持。查看Vulkan static实际截图非空、有世界纹理与选框，不作用户视觉验收或性能成绩 |
| 安装与环境 | 新 `deploy T3 space 中文-003`，48文件且无测试/开发库；`t3-deploy-release-001` 从无关CWD、PATH仅系统目录，三后端资产预检/普通4帧均exit0；同一安装游戏hash如上，未覆盖旧包。仍是有驱动/开发环境的本机受控部署，不是真正干净机器 |

发现的实际整合缺口为T3 `awaiting-foreground` 被T4 Status白名单拒绝：仅加入已批准的准备阶段，保持Recording要求及exporting/finished只能导出内部发布；新增Recording可用、Sealed拒绝与终态不可伪造回归。最终真实1秒准备deadline确实以 `Foreground readiness timed out before warmup` exit3并安全关闭、Sealed保存无效报告，未把任意exit3算超时。成功准备/焦点稳定算法有真实生产helper单测，但本轮未额外执行成功前台准备的正式采样或Computer Use置前。

保留失败：Debug构建001暴露Vulkan仍读旧Window字段、002暴露绘制fixture缺PlayerView完整声明，适配后003/004成功；短协议001/002为测试驱动给GL不支持的MSAA及给benchmark重复scene参数，003发现上述Status缺口（其exit3不算预期超时通过），004完成精确原因核对。故障001校验器仍要求旧“shutdown failed”文字，实际T4已报告原Vulkan Shutdown/Timeout、exit3且保留loader；改为兼容明确操作/类别并补反例后002/最终003通过，不修改生产故障语义或抹去首轮。

S01交接已收口：H01中立PlayerView/实际像素；H02三后端实例清理、两现代首错/有界等待/终止保留；H03两版本列协议、双现代生成身份及实际原生消费者；H04本次源码/资源/构建/安装身份分离。Q01=2、Q02=1、Q03=2、Q04=3组合未发现新设计冲突，完整入口沿已记录授权，不增设人工接口批准。S06仍未完成：本次不重跑T3全单/双后端配置与真实mtime增量矩阵、完整在途/缓存故障扩容组合、GUI新包运行、成本/正式同条件性能、四组人工短体验、长时/干净机器或中低端门槛。旧T3相应结果及其未覆盖风险仍有原身份，不直接充当本T4新候选成绩；性能/人工缺口不交由用户逐份审日志或默认扩大时长。

最终OpenSpec strict validate通过；受影响T4/app/engineering文档链接检查无错误。全库审计仍有53项既有归档/旧out路径缺项，不报告全库通过，不为消除这些提示复制冻结材料。相关可维护docs已逐次校验上游未并发改动后同步F审核目录；S01完成后任务5/6，S06仍未勾选。

## 历史CPU依据

旧CPU夹具采用真实生产simulation/World/ECS，24组案例串行运行，每案显式Reset旧共享固定步预算，无窗口/renderer。物理15组与玩法9组在Debug/Release各重复三次；旧原地改写、共享预算限制保留，不证明D04/D11/D13、双会话或D12新旧装配对照。历史工具为MSVC14.38.33145.0/SDK10.0.22621.0，原构建目录out/m3-t4/cpu-b02-debug及cpu-b02-release；完整旧方法和命令由全局退役材料恢复导航读取，文档迁移时未重测。

文档迁移整理时只整理既有验证入口，未构建、执行 CPU/GPU 测试或运行游戏，也未扩大 T4 实施授权。关键节点批准仅见 [全局索引](../../../../verification/VERIFICATION.md#关键节点)。

2026-10-09 CPU 局部授权内仅旧行为基线/夹具已交付：15 物理和 9 玩法组，Debug/Release 各重复三次；Debug 启用 58/58、world.allocations 禁用 1，Release 59/59。关闭测试 CPU 生产构建、physics_system.cpp 单编译及真实闭包检查有既有依据，原沙盒 C1902 失败仍保留。结果来源见 [原任务范围](tasks.md#已交付范围)、[T4 验证记录](verification.md)和 [旧冻结包](../../../../verification/archive/milestone-evidence-20261011.7z)，不复制或重新验证旧包内材料。
P01 接口候选和消费者静态核对已交付，交付时Q01-Q04尚未选择；当前选择状态仅见[decisions](decisions.md#未决接口)，不以选型补造实施/验证结论。app 参考未冻结，不是完整可复现仓库/游戏包。该历史基线没有新 physics/SimulationSession、双会话、SDL/GPU、性能或 D12 结果，不能由基线成绩推定目标验收通过。

## 2026-10-13 局部核心与交接核对

本轮用户明确指定在 `E:/Codex/Worktrees/refactor-application-simulation/Symocraft` 先同步本聊天T4文档，再由三个同推理强度子agent继续S01静态核对与S04独立核心/单测。基于feature-refactoring的 `66cee95fe53222bf2b6b5e3bf105af5c0b138bc1` detached HEAD；同步的8份change文档逐份SHA-256一致，原工作区及个人文件保留。以下是该worktree未提交局部实现，不扩成完整生产入口或人工批准。

后续按用户要求将本任务docs更新及时同步至F原工作区供审核；实现与本页CPU验证身份仍属于E worktree，不表示F代码、构建或运行结果已同步。个人文件、冻结证据和其他任务修改不在文档同步范围。

S01：C01-C14消费者已静态定位，H01沿现有相机值与实际drawable像素；H02要求仍合法时收集已就绪GPU样本，Failed/Stopped不额外Poll/等待，Shutdown未知或quarantine不可当安全释放。H03当前原生runner已支持GL v2/25列和D3D12 modern v1/32列、Vulkan未支持；现役renderer spec“仅旧GL”一句仍是待纠正覆盖缺口，本轮不执行spec同步。完整运行生成身份仍待交接。H04原F工作区既有默认D3D12候选的来源/资源清单3270项及包56项大小/哈希核对一致；E检出的21/35个比较文件仅行尾不同，规范化文本相同，不能称E字节等同旧候选，更不能把新增核心纳入旧包身份。

Q01后续生产装配仍须最小落实组件注册/空分配/代数实体有效性安全前提；Q02后续reducer须保Benchmark Escape跨失焦且无后台键策略；Q03后续须缓存首次Shutdown失败事实，不能被底层重复no-op覆盖；Q04生产诊断adapter尚未实现。无需要重选D/Q的静态冲突，但S01未整体完成。

S04局部：真实STATIC `symocraft_physics` 提供中立值、只读同步查询、StepBody/RayCast及三类具名错误；保持输入、成功结果自有，查询异常保嵌套原因，无预算/World/ECS/SDK。16组合成地形/字面旧预期检查覆盖积分、sensor、限速、支撑/容差、墙角/低顶、offset/负坐标/区块边界、扫描/平局、射线和参数/数值/查询失败。不是新World适配与旧系统逐次直接对照，也不证明A14会话Failed或D12完整序列。

本轮工具链为MSVC19.38.33145.0（14.38.33130路径）、SDK10.0.22621.0、Ninja、Python3.12.14。新建CPU-only Debug/Release输出为 `out/m3-t4/core-debug-001` / `core-release-001`；各自真实构建核心/消费者/公开头及旧15组物理基线，最终定向CTest各6/6通过（核心16组含整数端点/积分越界/射线保护域断言、消费者、公开头、旧物理基线、源码与正负边界）。边界脚本94个fixture按预期：10正例接受、84负例拒绝，其中新增physics为5正例/28负例，覆盖嵌套owner、私有头、窄include、直接/传递闭包、PUBLIC传播及alias/genex绕行；这些配置/源码负例不冒称GPU或完整生产组合验证。Release关闭测试目录 `core-notests-001` 单独构建真实核心及foundation成功，CTest枚举0项；三目录的编译数据库/生产manifest核对核心单owner、闭包仅foundation/math，消费者无simulation/World/ECS私有根、关闭测试无fixture。旧foundation/ECS警告仍在，未冒称全库无警告。

OpenSpec严格validate及git diff --check通过；任务整体仍0/6，CLI检查不替代运行正确性或用户批准。

同日按用户要求，主agent独自浓缩测试：核心原16个入口归并为8个主题组，单步积分/碰撞使用带名称的字面预期表，共用全字段及输入不变断言；序列、寿命、参数和嵌套故障检查保留。核心测试504→427行，边界脚本567→493行；边界统一执行/日志/正负断言及physics源夹具生成，94例仍独立运行，不由首个预期失败遮住其他规则。浓缩前后94份夹具的源码/头/probe/CMake输入规范化哈希全部一致，仍10正例/84负例，无删例。相同工具链/目录重新构建核心并执行最终定向CTest，Debug/Release各6/6通过；初次MSVC C2797来自测试表initializer_list成员的默认初始化，已移除该默认初始化后重建通过。未改生产代码、未重跑关闭测试或GPU；子agent开启授权已收回，后续须重新明确授权。

全库文档检查未通过：新worktree未带既有忽略的冻结证据与历史out，报告53个历史缺失引用/注册项，本轮新增链接未报错。未复制旧冻结包、改历史链接或补造现场，不将此检查冒称全库通过。

尚未执行完整CPU/独立工具矩阵、真实SDL/游戏/现代GPU、性能、部署或人工体验；尚无Session、生产适配切换、旧共享预算撤除、Q01-Q04生产落实或D12。旧游戏算法与未接入核心临时并存，最终单算法迁移未完成；S01/S04保持未勾，聚合0/6。

## 2026-10-15 最小 ECS 前提修补

用户先授权T4所需最小ECS修补和必要回归，并要求公共分配器先说明再确认；在确认点后进一步明确授权限定修补Allocate/ReAlloc及故障回归，再完成ECS初始化、注册转交和扩容失败清理。始终独自推进、docs及时同步原F审核目录，不扩大T5或完整Session/应用迁移。本轮代码仍只在E worktree，S01及聚合0/6不整项勾选。

首轮交付非分配部分：注册状态检查与重复/错序/坏定义的分配前异常；typed Add/Get拒绝空返回、byte Get拒绝缺组件/dense越界；同Registry完整ID校验及非法/重复Destroy前置拒绝；iterator/view保64位代数且筛选核对注册；Free后拒绝继续装配/访问，显式Clear可按原顺序重建。当时未改变组件布局、注册顺序、进程类型号、私有裸存储/分配策略或通用Remove；跨Registry、跨Clear旧ID、版本耗尽及一般删除可靠性仍未解决。

实际验证沿用前述MSVC19.38.33145.0/SDK10.0.22621.0及core-debug-001/core-release-001，最终定向CTest各20/20：真实ECS普通/跟踪模式、受影响ECS/simulation/physics公开头、角色数学/模拟契约、旧15物理+9玩法基线、新8组物理核心/消费者、源码与94例正负边界。新增ECS覆盖正常门面、注册/缺组件拒绝、旧代/非法/重复销毁、两份复用ID过滤遍历、可检测装配错误后的整体清理及Free/Clear；不是OOM注入或真实Session工厂测试。core-notests-001在BUILD_TESTING=OFF构建真实ECS/physics成功，CTest枚举0项。新测试C4702警告已修复后重建，旧RawMemory及旧物理警告保留。未运行游戏/GPU、完整CPU矩阵、性能或用户体验。

首轮分配安全部分曾停在确认点：foundation跟踪分配器的空地址偏移、realloc/记账及临时分配缺口使ECS InitRaw、注册转交及双realloc无法仅靠判空证明可清理；当时未修改公共Allocate/ReAlloc或换分配器绕开。该确认点现已获上述限定授权，以下为其后续实际结果，不将首轮20/20追记为故障测试。

限定修补已完成：公共Allocate/ReAlloc检查零请求/padding溢出并返回真实nullptr，正数空源ReAlloc沿Allocate，零请求不释放旧块；tracked Allocate在元数据失败时释放raw块，tracked ReAlloc保旧失败且成功后只复用已有记录，尾哨兵临时分配失败也不破坏旧状态。ECS容器改为可移动、不可复制的私有RAII owner；初始化三个分配和注册名称/两份vector转交失败可展开清理；dense增长用两份临时缓冲完整复制后再提交，失败无新映射/不丢旧payload，sparse成功后允许保留额外空池。仍使用原公共分配器、容量策略和组件布局，无一般Remove重写或共享Registry装配事务承诺。当前机制分别见 [foundation](../../../../architecture/foundation.md#旧分配与诊断边界) 与 [ecs](../../../../architecture/ecs.md#失败与清理边界)。

新增memory/registry_allocation测试直接链接正式foundation/ECS，普通及tracking(buffer=1024)各执行：malloc、尾哨兵临时块、realloc、记录拒绝和真实记录vector抛bad_alloc；零/溢出、失败后旧payload/记账/释放、增缩容/哨兵证据及buffer=0；ECS三个初始化点、实际string/vector分配序列、sparse和dense两处分配故障、无半注册/幽灵组件、旧多字节值与映射、重试或立即丢弃、持续分配失败下重复Free和显式Clear重建。私有探针窄根仅授予这两个测试，关闭测试仍构建同一生产实现；Debug CRT另核对记录失败未泄漏raw块，不把可控故障等同操作系统整体耗尽。

初次Debug故障扫查因MSVC string的noexcept移动仍分配proxy而终止/超时，未计为通过；改为名称unique owner转交后重测通过，测试terminate出口也改为明确失败而非弹窗。最后完整CPU-only构建及CTest：Debug启用68/68通过、world.allocations按既有Debug STL规则禁用1；Release69/69通过（含world.allocations）。覆盖全部CPU公开头、foundation/资产/World/ECS、旧15物理+9玩法、新8组核心/消费者、平台快照、应用冷路径、telemetry和renderer CPU门面，源码/94例正负/renderer边界；最后追加持续失败下清理断言后，两配置新增故障测试各4/4复测通过。工具链/构建目录沿前述，旧localtime/RawMemory等警告未清除。BUILD_TESTING=OFF的core-notests-001真实foundation/ECS/physics构建通过、CTest0项；三目录编译数据库及manifest核对上述生产cpp单owner、无测试根/fixture进入生产。

本轮最小ECS前提修补完成，不代表T5、真实Session工厂/部分推进、D12、完整应用迁移或阶段验收。未运行游戏/SDL窗口/现代GPU、独立工具矩阵、性能、部署和人工体验；公共Free非法输入/对齐/模式同步、一般Remove、跨Registry/Clear身份和版本耗尽仍留原边界。全库文档检查的历史材料缺失仍按上节如实保留，不复制冻结数据补齐。

## 2026-10-15 完整生产迁移

本聊天已明确取得[完整生产实施入口](../../../../verification/VERIFICATION.md#关键节点)，仍由主agent独自实施并及时同步docs至F审核目录。初始E/F生产文本核对：除本聊天的simulation CMake、ECS和foundation修补外，已有生产文件一致；H01-H04继续以实际版本复核，不以旧包身份包含新增实现。上游context已说明Vulkan opt-in候选在独立refactor-renderer worktree、待R4验收；当前E目录未包含该后端，不跨worktree修改/合并，不把CPU或D3D12结果补为双现代验证。

完整迁移实施与验证进行中；本节初期成绩及后续补齐分开保留，当前S02-S05已按实现/定向自动证据勾选，S01/S06仍未关闭。旧局部授权边界按新入口覆盖，不重开既定D/Q，不扩大T5或恢复Fix1。

当前E候选已接通实例Application、显式Runtime/受控Window、owned InputFrame、SimulationSession及唯一活动物理适配、独立Scenario、受控记录/Seal/自有ExportResult。Q01使用专属Registry/World及最小ECS安全前提，工厂失败整体丢弃；Q02 Down纯读held OR pressed、app独立选择普通/场景输入；Q03正常报告与自有ShutdownError汇合后Seal，重复关闭缓存首次事实；Q04正式simulation内的私有只读adapter、测试窄头授权，不新增公共Inspect。组合源码核对无新选型冲突；对应架构正在随实现更新，不代表已整体验收。

同前工具链，新增全游戏Debug目录 `out/m3-t4/game-debug-001`（SDL3 3.4.18，D3D12与GL均编入，独立benchmark工具关闭）完整构建，CTest启用82/82通过、既有Debug world.allocations禁用1；CPU-only Debug的core-debug-001启用70/70、禁用1。新增正式Session每次Advance对照/部分提交、owned input、telemetry封闭/失败重试/终态Publish原生共享错误32、Scenario和关闭报告测试均在其中。旧15物理+9玩法基线现调用正式核心及各fixture独立预算，零方向射线按D13从旧NoHit改为明确拒绝；不把适配后成绩冒称原冻结源码未变。D12全部序列/不同调用装配及Release/工具/OFF矩阵尚在补齐。

显式真实SDL Native探针通过外部SDL video引用不被释放、主线程/单窗口/错误次序拒绝、真实resize/minimize/restore、Wait首事件恰好一次和旧输入跨Destroy自有。进一步真实vector分配故障使后续Poll/Capture/Reset保留失败；初次测试误用exception_ptr地址相等，MSVC可复制异常对象，改按类型/诊断事实比较后通过（t4-runtime-003）。事件由SDL队列受控注入，不冒称物理键盘、全部DPI/任务栏或GPU体验。

正式Application探针在D3D12/OpenGL各通过十个真实装配阶段后的受控故障、部分记录封闭/保存、全清理与重复关闭；另成功运行一帧后注入Window已释放处的system_error(32)，Runtime仍完成，报告离开异常寿命后保留原生码、重复关闭不重做。实际输出分别为 `out/m3-t4/lifecycle-debug-d3d12-002` / `lifecycle-debug-opengl-001`，并非真实设备丢失。D3D12实际GPU NVIDIA GeForce RTX 5070 Ti，LUID=00000000:000146a5、vendor=0x10de/device=0x2c05，driver=32.0.16.1664，4x MSAA/Immediate/validation开启。此前d3d12-001因探针exe未邻接生产assets而失败，调整为相同资产部署后重测，不删除失败输出。尚未完成主程序三场景报告/截图、Release、性能对照、部署及人工短体验；不声称Vulkan或A22通过。

后续结果统一更新本页，默认无附件，无特别要求不附原始数据；已确认的逐次对照、完整序列或性能留存要求按 [治理规则](../../../../DOCUMENTATION_MAP.md#每个-change-一份验证记录) 执行，不补造预算、轮数或人工通过。旧冻结材料不动。

### 同轮补齐与收尾检查

Session逐次对照调用同一正式simulation/physics/World/ECS，真实配置、seed424242、独立专属Registry/World，逻辑player对应而非原ID。每Advance比较自有结果及只读诊断、每次World提交比较status/rejection/相关块和digest；一侧额外重复视图/采集、另一侧省略。覆盖混合零/短/多步/长帧/跳跃、有序pitch/FOV裁剪、选材/编辑冷却、墙地角/低顶、正负区块边界、支撑移除、重叠放置拒绝及仍消费冷却、World未知ID/域外拒绝、Pause/Resume、预检拒绝/长帧修正、出生恢复、部分单体提交/Failed、旧结果跨推进与销毁自有。浮点保持位置/速度/加速度/射线1e-4、角/FOV1e-4度、冷却1e-6、余数1e-9绝对阈值，离散精确/有限值检查；失败输出绑定序列位置/字段/左右数值/阈值或阶段/提交数。没有拿不同实际frame dt的GPU场景互作确定性证明。工厂另扫实际operator new抛错，失败丢弃专属范围，成功Session析构不删caller实体/World。

边界在原94例保留基础上新增Q04六例，现100例=11正/89负：指定session_tests的PRIVATE diagnostics窄根接受，PUBLIC传播/整个src/其他测试/app直接私有根/生成表达式绕行拒绝；正式库仍可独立构建，不依赖测试。初次新增无语言配置正例漏既有archive规则而生成失败，补与原夹具一致规则后100例通过，保留boundaries-100-001失败、002复测。

真实Window工厂进一步扫实际C++分配故障、每次核对SDL原生窗口归零/Runtime仍Active。初次只catch bad_alloc未匹配已有nested诊断，改查嵌套原因后暴露MSVC Debug vector默认构造noexcept的proxy分配terminate（测试明确exit86，非弹窗/超时）。限定调整T4三个输入vector为可抛零count构造，无allocator/ABI布局或T5扩展；Debug/Release的runtime-factory-003均通过，窗口创建/输入增长失败可以清理且latch不伪成功。未推广任意标准库持续OOM保证，保留前两次失败日志。

完整自动矩阵已通过：CPU-only core-debug-001启用72/72、world.allocations禁用1；core-release-001为73/73。游戏game-debug-001启用82/82/禁用1，game-release-001为83/83；公开session头、正式消费者/physics、全部现有CPU基线及100正负边界均含在内。关闭测试core-notests-001和全游戏game-notests-001构建同一生产实现、CTest均0。独立tools/benchmark Release目录tool-release-001构建及7/7通过，根仅工具tool-root-release-001构建及9/9通过，未把fixture进入产品。原localtime/RawMemory/getenv等警告仍在，无全库无警告声明。

真实Application十阶段故障/首错报告/重复关闭在当前Debug/Release各对D3D12/OpenGL通过，只有受控注入，不称真实设备丢失。主程序Debug两后端static/walk/edit各warmup0/sample1秒功能短跑及真实game_report_probe均通过，exit0/Saved/valid_run，25/32列、实际1920×1080/4x、全GPU样本关联、生成/场景摘要与截图成立；edit各提交3项并改变digest，static/walk不重传持久几何。非正式性能采样、不造九轮或FPS等价；静态D3D12/GL截图人工像素查看非空、地形/纹理/selection与镜头构图一致，不等同用户四组玩法体验或完整视觉容差。

S02-S05按实现及局部证据勾选，S01跨Vk候选交接与S06整体验证保留。当前E候选与F主目录都未合入独立refactor-renderer的Vk生产候选；跨worktree来源/整合需用户明确，不擅自Git合并或改另一个任务。Vulkan自动组合、两现代后端四组用户限时体验、正式性能协议对照/必要成本结论及T4整体验收尚不据本轮短跑关闭；长时增长、罕见时序、中低端性能、1175/Fix1和T5既有边界保留。源码/包及后续部署身份补在本节，不交给用户逐份审工程日志。

Release真实安装到 `out/m3-t4/deploy space 中文-002`，42个安装文件，exe与实际Release链接产物SHA256相同，无测试/fixture/lib/obj入包。从独立空CWD、仅System32运行PATH（无VS/SDK/Python/源树查找路径）执行两后端资产预检、普通有限帧和static/walk/edit六个完成场景，均exit0、完整报告probe通过并保存截图；仍是本机已有驱动/调试层，不称干净机器/可分发Debug或正式性能轮次。首轮安装查验因Python以GBK解码CMake中文路径输出失败，保留中文-001现场、不算成功；002以显式UTF-8接收后重测。最终离散首分歧诊断细化后，四个CPU/游戏配置的Session定向测试各1/1复测通过，生产代码未再改变。

真实main出口另外在 `out/m3-t4/main-outcomes-001` 通过：专属复制包非空坏定义令初始化失败exit3，部分记录仍Saved/invalid；只对自己启动PID核对的HWND发送WM_CLOSE，未完成记录Saved/invalid、exit4；持有自己status.yaml的无delete-sharing读者使真实exporting Publish失败，exit3且原生32/stage/path保留，未自动重试或伪finished。当前包显式Vulkan预检为Unsupported/exit3/no fallback，此为缺能力证据而非Vk验证。没有全局键盘注入或冒称人工Escape。

H04当前身份复核沿用既有production_inputs枚举方法，不调用T3专用打包/冻结入口：base仍 `66cee95fe53222bf2b6b5e3bf105af5c0b138bc1`，3284项production源码/资源/选定vendor输入清单SHA256 `2c82a916438642b56da4d928719cb766654f50583b857ae275eceb838190ae2a`；Debug exe `eab3b75d95923491c8a8c333b20d225a9822406e1074bf4d6f083484b152052f`，Release/安装exe `3730a7b3c0027a486695afe22475b62255632dd2b7d8d015f0ea7c83acc1eaa9`。同轮可重核的源码/构建/42文件安装身份在E的 `out/m3-t4/identity-001/source-build-package.json`，该记录SHA256 `e1a69dc3c7bac895bb8117e39d625edbdd62e77a0aaf2e6bc336d032bbe157ed`；不是Git提交、完整可重现仓库或新冻结证据。七种根构建的真实compile_commands/production manifests确认生产cpp单owner、CPU无SDL/platform/原生后端、工具无game、OFF无测试；CPU仍允许纯Renderer门面/工厂，未把它误判为原生SDK。源码身份不混用旧包/并行Vk候选。

本轮OpenSpec严格validate及git diff --check通过，未执行current specs sync/archive或Git写操作。全库文档audit仍为53个既有历史冻结/out缺失，当前T4/架构链接无新增错误，不复制冻结材料补假通过。F审核目录已及时同步T4可维护文档，独立Vk/T3/T5、个人及Obsidian修改保留；E app的Vk链接指向F权威正文，F原独立段保留其本地相对链接，不要求两工作目录所有文档字节相等。
