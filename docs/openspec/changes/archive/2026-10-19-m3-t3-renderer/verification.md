---
tags:
  - area/spec
---

# T3 验证记录

本页集中原全局索引的 T3 验证正文，既有结果、失败、用户反馈及结论边界保持；本次只调整记录归属，未重跑验证。关键节点批准仅见 [全局索引](../../../../verification/VERIFICATION.md#关键节点)。
既有证据链接指向当前压缩包，不复制、解包或修改旧冻结材料；包内成员身份按原清单解释，本次不证明压缩包恢复可用。后续按 [归档规则](../../../../DOCUMENTATION_MAP.md#每个-change-一份验证记录) 更新本页，默认无新附件、无特别要求不附原始数据。

2026-10-13审核同步：本页后续工作目录补证、R4前置及E01/E02/E03记录从 `E:/Codex/Worktrees/refactor-renderer/Symocraft/docs` 同步至主目录供用户审核，相关T3 proposal/decisions/delta/tasks已更新。对应新增源码与运行证据仍在该托管worktree，链接明确指向实际E盘位置；文档同步本身不复制代码/运行产物或产生新的测试成绩，不改变既有证据字节或用户批准，未Git提交/推送及规格sync/archive。

## 2026-10-19 主规格同步与归档

用户本轮明确要求同步主规格并归档，另要求移除主目录残留milestones目录。本次按24/24完成状态及2026-10-17 R4/T3明确批准，将renderer的一项修改、三项新增requirement合入现役规格，保留全部既有场景；同时按已实现代码修正Vulkan不可用描述及Benchmark只读GL的旧说明。标题及Purpose语义不重写，仅重定位必要链接。

T3目录归档为docs/openspec/changes/archive/2026-10-19-m3-t3-renderer，refactor-renderer与上游主目录工作区各保留原有审核补充和版本身份，verification随change移动；修正当前导航及归档内相对链接，不重写历史命令、结果或旧冻结证据。F盘docs/milestones仅余m3-t3/evidence/r1空目录链，已按用户要求删除整个空树，无文件或Git跟踪输入被删除。

本轮仅执行结构、同步完整性和文档引用检查，不重跑构建、CPU/GPU、玩法或性能；旧失败及本机/短窗口/未补完整回归等风险沿完成对账保留，GL不删、默认D3D12不变。主目录既有platform长文本警告及旧归档交叉链接按原状态保留，不混作本轮renderer验证失败或改写T4/T5成果；未暂存、提交、推送或合并代码。

## 2026-10-17 R4及T3完成确认

本聊天用户明确批准R4/T3完成，唯一关键节点登记见[全局索引](../../../../verification/VERIFICATION.md#关键节点)。结合既有逐后端技术证据、用户人工反馈及本轮预算结果，本机RTX5070Ti范围的R4与T3整体验收收口，任务24/24。下文21/24、未批准及未验收等旧记录保留其当时状态，不再作为当前节点结论；本次登记未重新构建、测试或采样。

| 收口范围 | 对账依据与边界 |
| --- | --- |
| R2技术聚合 | 生产故障/关闭整合、构建及增量、现代协议/生成身份/受控部署已补；见[最终材料](#2026-10-15-vulkan能力与wsi生成身份及r4材料补齐)。正式视觉沿用户确认的同版本GL人工对照、透明深度差异单列，不追加像素数值阈值 |
| R4适用A01–A19 | 第1至4批及最终补证覆盖构建、绘制、窗口、在途寿命、故障、异步和产品协议；D3D12专项见[在途及容量](#2026-10-15-d3d12在途寿命与异步容量补证)，GUI见[实测](#2026-10-15-gui实测与a18对账)，Vulkan具名退出/重启见[补证](#2026-10-15-vulkan具名退出与重启补证) |
| 人工核查 | D3D12既有用户反馈与R3批准独立保留；Vulkan同版本GL视觉及至少15分钟不中途重启玩法由用户确认，见[人工对账](#2026-10-14-vulkan人工确认与r4剩余对账)。不补填未提供的逐项人工checkbox |
| 当前性能预算 | [006前台27轮](#2026-10-17-三后端前台27轮预算对照完成)全部有效；D3D12/Vulkan各9/9配对同时满足P95/P99绝对及相对门槛、工作集及私有提交各自相对门槛，无删帧、混池或放宽预算。由此关闭当前节点预算阻塞，不将旧失败改判通过 |

本轮预算绑定refactor-renderer普通Release游戏SHA256 `4de4cfb9e534a414eaccfb15f8b6e165fae8ca442834b6742a6665a55fcab4a0`及对应运行输入；早期安装007人工、安装004补证等保持各自身份，不冒称对最新版字节重做全部人工/故障/构建矩阵。最后几项局部优化及前台准备仅完成各自已授权的基本测试、正常GPU验证与采样，未补跑完整边界/回归；该覆盖风险随本次节点完成明确保留。

保留事项：旧D3D12 edit第3轮超限、Vulkan旧static尖峰及失焦失败原始结果不改写，本批未复现不等于旧根因已查明或所有性能缺陷消失，相关追因继续暂缓、不再阻断本次已批准节点；Win32 1175/Fix1暂停、正式同源Q06及未确认的启动/首次可操作预算保持原状态。受控原生故障与有限注入不扩成自发设备事故/真实OOM保证；本机受控安装不等于真正干净机器。90秒窗口不证明罕见卡顿、长期内存增长或M5长时验收，下一玩法前中低端整机门槛及M6适用覆盖仍独立保留。

默认D3D12、Vulkan显式选择、GL保留；本次只登记批准和同步文档，不合并worktree代码，不执行Git写入、OpenSpec规格sync/archive、GL退役或额外后续工作。主目录并发T4内容保持，不由T3完成反推T4已验收。

## 2026-10-17 三后端前台27轮预算对照完成

**本批数据27/27轮有效，D3D12与Vulkan各9/9组配对满足原预算；不是R4/T3节点批准。** 按用户已批准的准备阶段及分步验证后采样范围，独立在refactor-renderer完成[006采集](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-006/samples/result.json)及[执行/完整性结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-006/result.json)。北京时间2026-10-17 14:23:11–15:27:29，协议测量54分钟，额外时间为准备、启动、导出及消费者核对；GL九轮→D3D12九轮→Vulkan九轮，各static/walk/edit三轮，完整30秒预热＋90秒采样，不混入2＋5秒短启动或旧失焦数据、不自动重采。

同一普通Release游戏SHA256=4de4cfb9e534a414eaccfb15f8b6e165fae8ca442834b6742a6665a55fcab4a0及17项运行输入，固定1920×1080/4x/VSync-off、seed424242、441区块/361 drawable、regression/workload/digest身份匹配。RTX5070Ti、NVIDIA616.64（D3D12 32.0.16.1664），现代实际Immediate/validation-on；各轮现代诊断无错误，Vulkan全部0警告/0错误、native_retired=true。Python不注入输入，Computer Use仅在本轮准备阶段置前；每轮连续1秒获焦后从零计时，期间不模拟/Render，准备耗时单列startup。所有27个进程exit0、完成导出及两个原生消费者检查，无超时/强杀；全部1241587帧（正式932556帧）focused=1，完整预热同样无失焦。

[独立离线复算](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-006/postcheck.json)从原始CSV重新按nearest-rank计算P95/P99，并独立重取30≤elapsed<120约每秒观测的工作集/私有提交最大值、校验覆盖/间隔和逐对预算，全部与原结果一致；不相加、平均、混池、挑最好或删慢帧。下表各格为P95/P99（ms），**每一组**同时满足现代P95≤10、P99≤12且两者各自≤对应GL的110%。

| 场景/轮次 | GL P95/P99 | D3D12 P95/P99 | Vulkan P95/P99 |
| --- | --- | --- | --- |
| static 1 | 7.4982 / 8.2495 | 2.6660 / 3.1217 | 2.6595 / 3.0991 |
| static 2 | 6.9856 / 7.6262 | 2.6964 / 3.2259 | 2.6168 / 2.9801 |
| static 3 | 6.9750 / 7.5904 | 2.5891 / 2.9422 | 2.8200 / 3.3276 |
| walk 1 | 7.3460 / 8.0829 | 1.7817 / 2.0934 | 2.5671 / 2.9322 |
| walk 2 | 7.3924 / 8.0886 | 1.9199 / 2.2231 | 2.8719 / 3.4986 |
| walk 3 | 7.1773 / 7.7752 | 1.9031 / 2.2422 | 2.6963 / 3.1253 |
| edit 1 | 7.4766 / 8.1668 | 2.6419 / 3.4742 | 2.9099 / 4.2496 |
| edit 2 | 7.4043 / 8.1076 | 2.6527 / 3.6640 | 2.8381 / 3.9825 |
| edit 3 | 7.6038 / 8.3207 | 2.6701 / 3.6890 | 2.8438 / 4.0150 |

内存九组逐对应轮次同样全部通过。下表为各轮正式窗口最大值的范围（MiB），最后两列是逐对比值的最不利值，不用范围端点交叉配对；门槛均为对应GL的110%。不含显存/启动/准备/导出或瞬时全程峰值，不叠加T2绝对上限。

| 后端 | 工作集范围 | 私有提交范围 | 最高工作集/对应GL | 最高私有提交/对应GL |
| --- | --- | --- | --- | --- |
| GL | 489.781–532.453 | 1074.723–1118.305 | 基线 | 基线 |
| D3D12 | 451.984–461.809 | 725.012–732.180 | 94.28% | 67.82% |
| Vulkan | 461.145–464.113 | 754.250–759.594 | 94.32% | 70.37% |

全部正式帧无>20ms或>50ms帧，最大全帧耗时GL13.5344ms、D3D12 8.0847ms、Vulkan9.5106ms，原始极值仍保留；max不是新增硬预算。1576项旧证据/准备输入、17项当前运行输入及采样期间全部源码hash一致，修改前1551c5fa包17项另保留，001–004真实失焦失败与005采集前执行器错误不覆盖。会话结束，无遗留采样游戏进程。

风险与边界：本批是本机、固定顺序、前台协议、每轮90秒采样的限定预算通过，不证明罕见尖峰/长时内存增长、中低端整机门槛、启动时间预算或完整回归；本次仅基本测试与短启动/采样，未追加真实超时/取消/最小化/焦点边界套件。准备时间和非隔离桌面后台活动仍可能影响启动/热状态，不以完整预热推定消除所有系统噪声。旧全失焦/不同版本结果不能直接用于计算这次优化净收益，旧尖峰本轮未复现也不是根因已修复；历史暂缓D3D12缺陷及R4整体关闭需后续证据对账/用户处置，不由新数字自动注销。21/24及R4/T3未批准状态保持，GL保留、默认不变；文档已增量同步主目录，无Git/sync/archive或额外采样。

## 2026-10-17 前台有界准备与短启动验证

用户明确批准准备阶段及分步验证后采样。仅在refactor-renderer的app加入可选`--foreground-ready-timeout-seconds 1..120`（默认0不等待，要求strict benchmark），世界/网格完成后发布awaiting-foreground，连续1秒实际获焦、非最小化/尺寸匹配后才完整开始预热；等待有界、不推进workload/Render，Esc/关闭/超时具名失败。准备耗时单列startup，成功后清输入/物理余数并重设初始帧时钟。计时后strict失焦无效、原始帧保存和所有预算保持，独立Python前台入口设置60秒准备，不增加强制抢焦或系统设置修改。

[基本检查](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/foreground-ready-001/basic.json)通过：7项Python模拟、Release游戏/报告消费者及两个CPU测试目标构建、core.startup_options与app.foreground_readiness 2/2通过（后者含稳定/重新累计/有限deadline三项基本策略）。[短启动](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/foreground-ready-001/short-startup/result.json)GL/D3D12/Vulkan各一次static、2秒预热＋5秒采样均exit0/valid_run=1，原生消费者/报告通过，1062/3156/3025总帧全部获焦；现代4x/Immediate/validation-on，Vulkan诊断0/0并正常退休。Computer Use分别置前，准备耗时17663.2/8132.17/13706.3ms，未进入elapsed测量窗口；这些数据只验证启动，不作性能预算、完整回归/边界或节点验收。

修改前17项1551c5fa普通游戏包字节已保留，1495项历史证据hash保持；本次生成的新游戏身份独立于旧失败。后续按本轮批准执行同版本GL→D3D12→Vulkan各static/walk/edit三轮30＋90秒、全部27轮；未执行项和数值结果待实际完成，不借短启动成绩预判。旧失败、21/24、默认D3D12与GL保留不变，文档增量同步主目录，不复制源码/产物或进行sync/archive/Git写入。

完整对照现已进入006批次，游戏SHA256为4de4cfb9e534a414eaccfb15f8b6e165fae8ca442834b6742a6665a55fcab4a0，复用同一新版包及60秒/连续1秒准备。005在采集器/游戏启动前因执行器把包hash映射误传为identity列表而失败，原failed保留、未执行任何GPU轮；006仅纠正该适配，不是重采GPU失败。后续以[006原结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-006/samples/result.json)实际有效轮/逐对预算为准，采集中不预判通过，1576项旧证据与准备验证输入受保护。

## 2026-10-17 Computer Use获焦成功但预热失焦仍无效

用户再次明确授权Computer Use操作桌面、将焦点移至采样窗口并开始采样。本轮先经sky.launch_app启动现有普通游戏，未设置benchmark参数、不计性能成绩；sky.activate_window成功，随后只读核对自有HWND134656/PID452确为前台、可见、非最小化、960×600。经Computer Use按Esc退出，窗口消失。该预检只证明当时普通窗口可被激活，不证明后续独立benchmark进程会在第一帧前自动获焦，也不证明前三批激活失败的底层原因已查清。

独立004按既定27轮30＋90秒启动，7项模拟检查及Release构建通过；本轮未修改游戏、工具源码或启动计时。Computer Use成功将首个opengl-static-1置前，外部确认实际1920×1080且发生于runtime ready之后。[原failed](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-004/samples/result.json)保留北京时间14:11:51–14:13:53的执行：19381总帧中2051个预热帧失焦，最后失焦elapsed=12.9765278秒，首个获焦帧elapsed=12.983113秒；90秒采样段14632帧全部获焦。严格模式已因预热失焦单调判无效，游戏完成清理/导出后exit4，无超时/强杀。**不能只取后90秒就判通过或作为前台GL基线；有效0/27、余26轮未启动，两现代未采样。**

[离线核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-004/postcheck.json)仅证明焦点归属及失败保留：含前三批的1474项历史文件、所有原生产/工具源码及17项运行输入hash保持，采样会话结束、无遗留游戏进程。当前已证明本次桌面工具能够获焦，剩余明确流程缺口是游戏先于外部获焦操作开始预热；15秒观察器等待不是游戏启动屏障。已在聊天提出仅前台模式的有界获焦准备阶段、计时后严格语义不变，并请求另行批准基本/短启动验证后再采样；本次桌面授权不自动解释为该代码/计时改动批准，不再直接重复同一采集流程。原预算失败、21/24、默认与GL保留不变，本节同步主目录。

## 2026-10-17 已解锁但前台激活失败

用户要求“查看是否已经进入桌面，如已进入则继续采样”。只读WTS查询确认当前进程与本机控制台同为session 1、SessionState=Active(0)、SessionFlags=UNLOCK(1)，据此新建003而非覆盖前两批。实际游戏截图也确认已进入桌面，但聊天等窗口遮挡游戏。Computer Use置前失败，刷新后点击游戏裸露区域仍报failed to activate captured window；未点击聊天、网络设置或认证界面，未绕过前台限制。无游戏时尝试读取其他应用截图曾被权限审查拒绝，未获取该应用截图或继续该操作，随后仅改用不读取应用内容的会话状态查询。

七项采集器基本检查及Release构建通过，生产/采集器源码未再修改。[003原failed](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-003/samples/result.json)保留北京时间14:03:13–14:05:15的唯一opengl-static-1：前台确认超时，18201总帧及13582采样帧全部失焦；正常完成导出/资源清理后exit4，无超时/强杀。有效0/27，余26轮未运行，两现代未启动，不能用该轮作前台基线或预算判定，不抹去失焦预热帧或自动重试。

[离线完整性核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-003/postcheck.json)仅证明失败现场和输入保持：含001/002的1453项历史文件、全部生产/工具源码及17项游戏运行输入hash未变；会话结束，无遗留采样游戏进程。已在聊天请用户选择手动移开遮挡后明确重开，或先排查自动获焦；在新回复前不再启动。当前阻塞为自动获焦失败，不再归因锁屏，也不推定GPU性能故障。原预算、历史失败、21/24及GL保留不变，本节及时同步主目录。

## 2026-10-17 解锁后重开仍锁屏并按用户要求暂停

用户明确回复“已解锁，保留失败并重新开始”后，新建独立002批次，不覆盖001。仅调整前台采集器的进程启动选项：保留CREATE_NO_WINDOW以隐藏控制台，不再给GUI传STARTF_USESHOWWINDOW/SW_HIDE；不改游戏显示/焦点实现、既有预算或allow-unfocused执行路径。该项是避免启动器隐藏GUI的防御修正，不将其推定为001锁屏的原因。新增可见GUI启动选项基本检查后，七项模拟检查及Release游戏/报告消费者构建通过。

**002仍未获得有效前台轮，用户随后明确选择“先暂停采样”。** [原执行结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-002/result.json)与[采集器failed](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-002/samples/result.json)保持原样。北京时间12:54:49–12:56:50仅执行opengl-static-1：前台确认超时、Computer Use激活失败，重新观察仍为Windows锁屏，立即停止UI输入，未操作锁屏或认证。该轮按既定时长完成清理/导出后exit4，无超时/强杀；共19565帧、采样段14623帧全部失焦，不能作GL基线或判断预算。有效0/27，余26轮均未启动，两现代本批未运行，不拼接001或改为后台采样。

[离线核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-002/postcheck.json)通过仅指失败保留与完整性：含001的1430项历史文件、原生产源码及17项运行包输入hash保持，普通游戏仍1551c5fa；采集器/测试改前字节单独保留。运行会话已结束，未发现遗留SymoCraft采样进程。用户暂停后不再重开或自动安排后续；恢复需用户另行授权并确认实际桌面可见。

本节与机制/任务状态增量同步主目录；旧正式预算失败、D3D12暂缓缺陷、GL保留、默认及21/24不变。本次锁屏失败不是三后端性能结论或R4/T3验收通过。

## 2026-10-17 前台三后端采样准备与锁屏中止

用户批准增加可选前台采样入口，并明确本次GL、D3D12、Vulkan各18分钟：各static/walk/edit三轮，每轮30秒预热＋90秒采样，共27个独立进程、约54分钟协议时间，先GL九轮、再D3D12九轮、最后Vulkan九轮。使用同一最新版普通游戏、1920×1080/4x/VSync-off/seed424242/441区块，现代validation-on；同批GL按场景/轮次分别配对两现代。原P95/P99绝对及GL110%相对门槛、两项进程内存采样窗最大值110%不变，不自动重采、删慢帧或因新窗口/焦点覆盖旧失败。本次采样授权不等于节点批准，也不追溯把旧失焦数据改为前台基线。

仅新增test/integration/renderer/verify-renderer-comparison.py及其模拟测试，不修改生产、既有预算脚本/默认或原生Benchmark工具。新入口默认仍10＋50/三轮/GL与D3D12/allow-unfocused，--focus-mode foreground才使用游戏已有strict参数，使SDL显示时请求激活，并用自有PID窗口只读核对前台/可见/非最小化/1920×1080。源脚本不注入点击或反复抢焦，用户后续明确授权的桌面操作仅经Computer Use处理该次游戏窗口。游戏保持原时钟和严格逐帧焦点语义，不暂停主线程等外部点击、不移动预热起点；外部确认不能补救此前失焦，只有预热与采样的全部原始帧均focused=1才接受前台轮。退出码、失败attempt、原始CSV/PNG与日志持续保留，所有分析复用原nearest-rank及独立内存最大值，不新增GPU私有观察器。

**脚本六项纯模拟检查通过，Release游戏/原生报告消费者构建通过；本次前台采样未完成。** [原执行记录](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-001/result.json)与[采集器原failed](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-001/samples/result.json)保留北京时间2026-10-17 12:40:58–12:43:00的唯一opengl-static-1 attempt。初次前台确认超时，Computer Use一次激活失败；重新观察截图明确为Windows锁屏后立即停止UI输入，未点击/绕过锁屏或认证。游戏继续到既定时长后正常资源清理和导出，exit4、无超时或强杀，completed=true但valid_run=false且window-not-focused；共20671帧、采样段15579帧，全部失焦，**不得用作本批GL基线或作预算判定**。

该轮失败后采集器自动停止：有效轮0/27，未启动的26轮（余下8个GL＋9个D3D12＋9个Vulkan）不标已测；D3D12/Vulkan本轮均未运行。已要求用户手动解锁并明确确认是否保留首轮失败、重新开始；在回复前不自动重试、续交或操作锁屏。当前没有遗留采样进程，[离线完整性核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/focused-three-backends-30-90-001/postcheck.json)仅证明失败现场保留，不是采样通过；原游戏1551c5fa及17项运行输入、原有源码与1409项历史证据hash未变。

相关机制与本节及时同步主目录供审核，主目录不复制新脚本/代码或运行产物；旧预算失败、D3D12暂缓缺陷、GL保留与21/24不变。当前阻塞为锁屏/无法获得前台焦点，不推定三后端运行或性能故障；解除后仍须新的准确attempt身份和全部27轮实际有效数据，不拼接本轮失焦帧。

## 2026-10-17 Vulkan网格与选框正常GPU验证

用户在两项方案1基本交付后批准GPU验证；独立在refactor-renderer托管worktree仅扩展既有Vulkan frame-bindings probe的可选--buffer-policy正常模式，不改生产源码、shader、公开v1、默认、能力/WSI/等待或预算。沿用同一Release生产库、4x/Immediate/validation-on、640×480固定双纹理层夹具及普通2500ms等待；无门控、故障注入、边界/回归套件或性能采样。observe_performance仅为读取既有实际原生计数/内存表而启用，也会带入已有host时钟和每Present两次非阻塞fence快照，不据此计算性能收益。

**本轮正常GPU检查通过，原执行器收尾失败另保留。** [原执行记录](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-buffer-gpu-001/result.json)中的构建及实际GPU进程均exit0、无超时；[独立离线复核](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-buffer-gpu-001/postcheck.json)为passed，不覆盖原failed。实际GPU为RTX5070Ti/vendor10de/device2c05/UUID d43c9988ca0711129c89be697511f1d0、NVIDIA616.64/API1.4.351；执行时间以UTC记录换算北京时间2026-10-17 12:25，原生日志时钟不替代执行器身份。51帧全部Presented且每阶段覆盖三个真实slot，共17阶段/17张截图；GPU诊断0警告/0错误，正常Shutdown、Native/loader释放、无quarantine。

| 核查 | 实际结果 |
| --- | --- |
| 网格创建/更新 | 两个初始mesh及左侧UV更新/恢复共4个原生版本，全部实际绑定type4/flags0x7/DEVICE_LOCAL heap0；请求累计672字节、实际分配累计704字节，兼容位均31。原始/更新/恢复画面核对通过，4次版本分配/绑定/释放平衡、live=0；只证明本夹具，不冒称441区块或WDDM物理驻留/PCIe因果。 |
| 选框复用 | 前3帧各slot首次创建/绑定一次，共3份、实际/请求均864字节；随后48帧（含隐藏3帧、恢复和左右移动）新增/释放均0，隐藏不画/不写，恢复/移动内容正确；关闭后3份全部释放。实际MapWrite次数和上传字节逐帧对应常量与288字节选框写入，不能把phase访问当分配。 |
| 绘制/绑定 | 纹理切换、重复、draw重排、恢复、选框显隐/左右移动、draw数2→1→2及object矩阵变化通过；新增mesh UV左右翻转与恢复通过。三slot同阶段RGBA一致，7种恢复/重复PNG与initial字节hash一致；固定色块/UV像素oracle及三张代表图目视核对，不重开正式人工视觉容差。 |
| 原生计数/关闭 | 全程Buffer创建/分配/绑定/销毁/释放各67次，其中51次为逐帧截图readback，不能错误要求全体Buffer零创建。mesh/selection常量各3份、6个pool/9个set、24次描述符API调用均符合预期；154次MapWrite/Unmap、54次submit（51帧＋3次纹理上传）和51次Present成功，各角色终态live=0。 |

原执行器在GPU成功后错误要求所有native_calls的calls等于successes；既有观察模型仅将VK_SUCCESS累计为successes，102次非阻塞fence poll中34次VK_SUCCESS并不要求其余也已完成。原failed/result、traceback及所有输入/原始CSV/PNG保持；没有重采或修改生产算法来消除失败。离线复核只对其他具名原生调用检查全部成功，fence poll分别保留102/34及68次未分类差额；**未保存逐次VkResult，不能将68次全部标为NOT_READY或GPU错误，也不由该计数证明连续完成时间。** 本轮资源/画面及最终有界完成依据与这个诊断信息缺口分开。

实际probe、生产源、SPIR-V及运行身份见上述原result；普通游戏仍为1551c5fa398c51fd15265995042a35a077f1cfe09e02705638196f12f7ed3bb8，未作为完整游戏运行。389项其他源码/工具/资产、17项当前游戏运行输入及1377项历史证据hash一致，旧probe源码/二进制在本轮before-probe保留。源码和产物未复制主目录，仅本页、受影响架构/任务说明增量同步供审核。

同步截图每帧都等待提交完成，因此**不覆盖真正未完成的slot复用、失败回滚、尺寸重建/有限关闭故障或长时回收**；HOST_VISIBLE回退只沿用既有CPU基本策略检查，未在另一设备实测。未运行完整世界/玩法、Debug/边界/回归、普通未绑定观察路径或性能对照；旧Present/正式预算失败、D3D12暂缓缺陷与21/24保持，不据本次关闭R4验收/批准或T3节点。

## 2026-10-17 Vulkan网格内存与选框局部优化

用户批准两项方案1同时实现、仅做基本测试；独立在refactor-renderer托管worktree限定修改Vulkan私有Buffer策略、后端及新基本测试接入，不改变公开v1、shader、能力基线、提交/WSI/有限等待或默认后端。D3D12不改/不运行，GL保留；本轮不进行GPU、边界/故障、回归或性能采样。

网格按每个Buffer实际requirements_bits优先选择HOST_VISIBLE|DEVICE_LOCAL，不固定memoryType索引；没有兼容本地可映射类型时保持首个兼容HOST_VISIBLE类型，成功绑定后按实际类型各记录一次policy/type/flags/heap/fallback。分配失败不重试其他类型、不降级后端，仍沿用原类别；所有其他Buffer角色保持原HOST_VISIBLE选择。Map/flush/Unmap及Create/Update的新版本完整写入成功后替换不变，在途旧版本仍由frame引用保留。CPU构造类型表选出type4只证明策略，不冒充本机真实新分配/物理驻留或GPU收益。

三个frame slot各懒建一份288字节选框顶点Buffer。Finish/Collect及ResetCommandPool成功后才重写该slot；正常RetireFrame只清mesh/texture/readback，保留选框与现有常量bundle。首次创建先写成功再发布缓存；隐藏时保留Buffer、不写/不画，显示或移动时按原24顶点与1.004扩张完整写入，上传字节继续计数。DropFrame在重建完成证明后及正常销毁中释放，未证完成仍完整Native终止保留；没有新增持久映射、共享跨slot写入或单位立方体变换。SelectionAllocate阶段暖槽后可能只做命中判断，不能把phase访问计为原生分配。

**限定基本检查通过，非GPU/性能验收。** [本轮结果与准确输入身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-buffer-basic-001/result.json)记录北京时间2026-10-17 00:09的Release基本目标构建、唯一CTest renderer.vulkan_buffer_basic（1/1，内部5个正常CPU用例）及游戏构建全部exit0、无超时。用例执行生产私有policy helper，核对可映射本地优先、兼容HOST_VISIBLE路径、非mesh策略、三槽12次写仅3次CPU创建、隐藏/恢复与首次写入后发布；CPU fake Buffer不是实际vkCreateBuffer/Map/Write或GPU同步证据。测试只在Vulkan target存在时引私有SDK头、不链接loader；未运行CPU-only配置，不能宣称本轮复验了SDK隔离。

Release仅重编Vulkan后端并重链游戏，当前SymoCraft.exe SHA256为1551c5fa398c51fd15265995042a35a077f1cfe09e02705638196f12f7ed3bb8，**未运行**。编译仍有既存MakeFrameBindings参数selection遮蔽同名pipeline的C4458警告，不称零编译告警。改动前cd8ed208游戏及完整17项运行输入在本轮before-game逐字节保留；351项未授权变更的既有源码/资产/工具及1351项历史证据hash一致。

后续尚需独立授权的正常GPU画面/实际memoryType与分配计数、在途寿命/故障与尺寸重建回归，以及无探针同条件性能对照。未证明GPU draw或P99下降，不将旧约0.40–0.44ms原生选框操作成本当成确定整帧收益，不注销旧Present/预算失败；D3D12暂缓缺陷、21/24和R4/T3未批准状态保持。本页与受影响架构/任务说明增量同步主目录，源码和产物不复制。

## 2026-10-16 Vulkan实际内存与等待原生诊断

用户批准补齐实际内存类型、等待分项和原生分配计数并执行短诊断；独立在refactor-renderer托管worktree仅改Vulkan私有观察及对应probe/解析器，不改内存选择、shader/公开v1、同步/呈现策略或预算，不优化、不新采GL。普通未绑定路径不读新增时钟、不增加fence查询；专用probe启用native_calls_v1，按成功VkResult或void调用正常返回累计实际API次数/host墙钟，并记录Buffer角色、所选memoryType/heap/flags/requirements位及分配/绑定/释放。void正常返回不是GPU完成。每次Present前后额外各做一次非阻塞提交fence状态查询；软件pending计数不是硬件队列占用或显示完成。

**限定诊断完成，非正式预算通过。** [最终002结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-present-memory-diagnostic-002/result.json)合并原001有效第1轮与用户明确授权重开的第2轮及原计划第3轮；每轮10秒预热＋50秒static采样，1920×1080/4x/Immediate/validation1/seed424242/441区块/361初始mesh及2500ms等待不变，仍为同一5070Ti UUID/616.64/API1.4.351。成功采样共33887帧，三个游戏、原生消费者及报告检查均exit0，GPU/CPU诊断缺失0、validation 0/0、正常退休，三个slot/图像及图形和呈现同queue/family0、三图swapchain已实测。原[001第2轮](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-present-memory-diagnostic-001/result.json)被用户误最小化，87个预热帧后exit4，framebuffer-changed-or-minimized与duration-not-completed保留，非超时/强杀；没有进入采样段，也不覆盖为成功。第1轮约北京时间21:15–21:16；明确重开后的第2/3轮为21:17:53–21:20:05，未重跑第1轮。

| 有效轮 | 帧数 | 帧P95/P99 ms | 原生submit fence等待均值 ms/帧 | 原生Acquire均值 ms/帧 | 原生QueuePresent均值/P99 ms | 焦点 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 11530 | 5.0497/5.5926 | 1.01779 | 0.01116 | 0.23261/0.47810 | 全失焦 |
| 2（用户授权重开） | 11202 | 5.4968/6.2359 | 0.66903 | 0.01312 | 0.29988/0.68520 | 273帧获焦，其余失焦 |
| 3 | 11155 | 5.4352/6.0122 | 0.70010 | 0.01310 | 0.27838/0.53440 | 全失焦 |

均值按全部采样帧计算，包括未调用时的0，并非每次调用均值；原生区间嵌套于对应phase，不重复相加。实际WaitForFences提交等待次数为10462/8443/8664；重获图像的presentation fence等待每帧一次，均值仅0.02634/0.03068/0.02809ms。QueuePresent最大分别1.4020/6.9605/1.3125ms，未复现普通150秒轮次中密集约10ms的长Present；第2轮有20个提交fence由调用前NOT_READY变为调用后SUCCESS，20帧均失焦，不能简单归因获焦，亦无连续完成时间可区分WSI/驱动或GPU背压。其余33867帧前后均NOT_READY，只证明当前提交仍异步在途，不是故障。探针/窗口/焦点/时段不同且新增查询改变节奏，不据较低P99注销原失败或宣布Present已修复。

**顶点分配疑点已取得实际证据，PCIe因果仍未证实。** 三轮361个持久mesh均实际绑定type2、flags0x6（HOST_VISIBLE|HOST_COHERENT）、非DEVICE_LOCAL的heap1；合计请求57770328字节（约55.09MiB）、实际分配57771632字节，三个采样段DEVICE_LOCAL mesh数量/字节均0。各Buffer的requirements_bits AND/OR均31，设备本地type1及可Map的device-local type4均在兼容集合；现有首个HOST_VISIBLE策略实际选择了type2，并非设备不支持本地类型。它确认分配类型，不等于WDDM物理驻留/PCIe带宽或已证实4ms唯一根因。GPU draw三轮均值4.2587/4.3491/4.3882ms，现有结果更支持优先验证顶点驻留策略与GPU/slot等待，而不是再次重做描述符；本轮没有进行该优化或受控A/B。

原生计数确认完整世界暖槽后mesh/两类常量Buffer、pool/sets新增均0，vkUpdateDescriptorSets和UBO/image/sampler写均0；每slot各保留mesh容量512及selection容量1，合计6个pool/1539个set。每帧仍有一份288字节选框顶点Buffer的Create/Allocate/Bind/Destroy/Free，三轮合计33887次各项操作；这五个原生区间均值合计0.402/0.440/0.443ms/帧。矩阵内容照常写入，不能把缓存命中当零上传。command_record均值仍1.973/2.221/2.226ms，facade约0.026–0.028ms，描述符逻辑phase约0.002ms但实际API调用0；共享扫描/全量描述符重建不再是当前主要候选。

不利帧保留：第3轮最大17.9831ms（原frame8452、elapsed37.6934s），同帧resource_retire6.5697ms，其中实际vkFreeMemory6.4385ms，command_record4.0789ms；快照间fence状态查询host区间累计3.9848ms且采样instrumentation3.9933ms。不能把所有长帧都解释为Present或GPU draw。下一步优先候选为顶点device-local驻留验证、选框分配/释放复用和命令录制/验证成本分解；旧Present排队原因仍开放，不删等待、不关验证或换模式来达标。

当前Release诊断构建及13项解析器自身测试通过；[独立离线复算](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-present-memory-diagnostic-002/audit-result.json)核对原始分位数、全部原生次数/墙钟差分、分配释放及最小化失败保留。231项来源及1305项旧证据hash未变，160项其他生产/资产输入未改，D3D12不改/不运行、GL保留。旧普通游戏d7d32f完整17项运行输入在001/before-game逐字节保留；当前重链游戏cd8ed208e6d6c27498709237791cbcdca796d09c3a11c713a718f5b773cbf35f未作为无观察器游戏运行，实际probe为0ed95a534ebaa4ffa2d6c7d8522d808cafcf8ece86967a404343e39e701f1f2a。预留100000条/138400000字节记录及额外时钟、查询会扰动观测，两项进程内存不用于GL相对预算。未执行正式长采样/新GL、Debug/完整玩法或生产故障/边界回归；预算、默认、21/24和旧正式失败保持，文档及时同步主目录。

## 2026-10-16 Vulkan优化后热点只读重评

用户要求依据现有采样和代码重新评估，并批准仅将结论写入本页、同步主目录；本轮独立只读，不改代码/规格/计划，不新采样或启动GPU工具。依据为[当前六轮普通游戏](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/gl-vulkan-medium-30-150-001/result.json)、其原始frames.csv及[已通过的离线复算](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/gl-vulkan-medium-30-150-001/audit-result.json)，历史参考仅为[摘要已优化、描述符未优化的三轮static探针](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/layer-summary-diagnostic-001/vulkan-after/result.json)。原帧时间/内存及焦点判定不改；230项源码/工具、1140项旧证据和本轮六个capture的全部产物hash再次匹配。

不能把“相对GL的P95/P99未达标”直接等同于“描述符优化无收益”。历史三轮static的cpu_prepare均值4.4353–4.7829ms、整帧均值5.2026–5.6206ms，当前static分别3.1472/4.3394ms；但历史是10＋50秒且安装私有计时/原生observer，当前是30＋150秒普通游戏，非交错、不同时间、无温度/频率/队列轨迹，不能报净收益百分比或由此证明因果。当前与同批GL相比，三场景整帧均值约4.31–4.35ms而GL约6.32–6.44ms，说明平均吞吐与尾延迟是不同问题，预算仍以已选P95/P99判断，不改用均值宣称通过。GPU003已经证明正常小夹具三个slot暖槽后零UBO/pool/set创建、重复零描述符写；完整世界旧probe没有新原生计数，不能把其phase visits当成当前重建次数。

重新按每场景sample内frame_ms从大到小取ceil(N×1%)行，始终使用同一原frame ID，不将阶段分位数相加：

| 场景 | 尾部帧数 | 整帧均值 ms | 同帧prepare均值 ms | 同帧Present均值 ms | Present占该组整帧时间 | 同帧GPU draw均值 ms |
| --- | --- | --- | --- | --- | --- | --- |
| static | 346 | 13.2957 | 3.6787 | 9.2158 | 69.31% | 3.8275 |
| walk | 349 | 13.6210 | 3.6819 | 9.5310 | 69.97% | 3.8417 |
| edit | 345 | 13.0644 | 3.6444 | 8.9231 | 68.30% | 3.8818 |

上述1040个尾部帧的present_ms全部>5ms；edit尾部仅11帧发生编辑，不能把普遍尾部归因编辑上传。三场景全部GPU draw均值约4.217–4.224ms、P99约4.655–4.676ms，而尾部GPU均值反而较低；没有同帧GPU draw升至12–15ms的证据。按30秒分段，static的Present P99始终约7.25–8.33ms，其他场景各段也持续出现，并非只在末段或启动发生。当前cpu_prepare含slot/query/Acquire与present fence等待，不能将其3.10–3.15ms均值全叫CPU计算；present_ms在[QueuePresent调用](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp:1259)外紧邻取时，不包含前面的Acquire或显式present fence等待。[Vulkan原生规范](https://docs.vulkan.org/refpages/latest/refpages/source/vkQueuePresentKHR.html)允许该调用阻塞，Immediate不等于host零等待；现有数据未区分验证层/驱动调用、WSI背压、GPU队列饱和或桌面合成/失焦策略。

**下一步诊断优先级，而非已批准的优化计划：**

| 优先级 | 当前热点/候选 | 依据与证据上限 |
| --- | --- | --- |
| 1 | Present/帧排队路径，首要尾延迟落点 | 尾部约68%–70%在同帧Present；先区分QueuePresent、Acquire、slot wait/查询、present fence wait及实际队列占用，不能直接删fence、换呈现模式或关validation来“达标”。CPU变快后暴露GPU/WSI背压与现象相符，但未证明。 |
| 2 | GPU侧持久顶点Buffer驻留，优先核实的吞吐候选 | 当前GPU draw约4.2ms，GL约0.11–0.12ms，但不同API/查询、已批准透明深度差异不支持仅按倍数归因。源码Mesh使用MakeBuffer，后者只要求HOST_VISIBLE并取首个可用类型，没有优先DEVICE_LOCAL。 |
| 3 | 逐draw命令录制/验证与剩余prepare分解 | 每帧361个mesh逐个BindVertexBuffer/BindDescriptorSets/Draw，另有选框；旧探针command_record均值1.817–1.952ms，提示CPU常驻成本，但不是当前时间，当前无细分不能沿用其数值承诺收益。验证开销与驱动/调度尚未拆开。 |
| 4 | 选框临时Buffer及Map/Unmap | 当前仍每帧创建/分配/释放24顶点Buffer；mesh/选框常量分别写入，选框顶点也写入。旧选框分配均值0.115–0.129ms仅供定位参考，不再把旧混合retire的1.08–1.16ms全部算给选框，常量池退休已改变。 |
| 5 | 编辑资源分配/复制与CPU网格重建 | 当前600个编辑帧中mesh_ms均值1.2811ms、pack_ms0.5610ms（upload_cpu_ms嵌套其中，不能相加），编辑总帧均值5.9111ms；33893非编辑帧均值4.3211ms。编辑成本确实存在，但解释不了static/walk的同类尾部。 |

内存驻留候选的准确依据：[既有2026-10-13能力记录](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-preflight-001/hardware/vulkaninfo-full.stdout.txt)同一5070Ti UUID/616.64的memoryTypes[2]/[3]属于非DEVICE_LOCAL heap1，type4才是DEVICE_LOCAL|HOST_VISIBLE|HOST_COHERENT的heap0。[当前MemoryType/MakeBuffer](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp:249)按索引升序选匹配HOST_VISIBLE的类型；若具体Buffer的memoryTypeBits允许type2，就会先选系统heap而非type4。当前采样没有记录每个Buffer的实际memoryTypeIndex/flags/heap或访问带宽，不能将条件推断写成“已实测所有顶点走PCIe”。[Khronos内存指南](https://docs.vulkan.org/guide/latest/memory_allocation.html)为独显常见staging→device-local路径提供机制依据，不证明本项目收益；后续须先核实实际分配及GPU时间戳口径，再决定是否改上传/驻留策略。同帧GPU查询区间不是完整GPU/显示时延，尾部draw没有同步增大，也不能排除更早在途工作、区间外GPU工作或队列背压。

共享摘要和描述符全量重建降为“已有局部正确性证据、当前不优先重做”；缓存metadata仍有按draw比较，但没有继续扫描206万顶点的路径，不把历史约1.2ms描述符成本继续冒充当前值。推荐下一轮先适配私有诊断的实际分配/写入计数和记录上述等待分项、Buffer实际内存类型，再在独立授权下做最小验证；缺这些数据时，不承诺再做选框/常量微优化就能消除P99。当前只完成分析和本页同步，未补采、埋点、关验证、改变预算/默认或GL保留，21/24和原正式失败保持。

## 2026-10-16 GL与Vulkan三分钟中等长度对照

用户在Vulkan方案2正常GPU验证后批准“同样采用18分钟的对比采样”，沿用最近GL/D3D12中等长度窗口：同版本普通游戏先GL、再最新版Vulkan，static/walk/edit各一组，每轮独立进程、30秒预热＋150秒采样，共六轮，启动/导出另计。不自动重试、删除慢帧、改预算或启动三重复正式复测；无额外私有计时sink/原生observer，不把GPU003同步截图夹具当成本轮性能输入。独立在refactor-renderer托管worktree执行，D3D12不改/不运行、GL保留，主目录仅增量同步文档。

**六轮采集有效；三组帧时间均未满足本轮已选数值条件，两项内存均满足。** [本轮运行及配对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/gl-vulkan-medium-30-150-001/result.json)记录北京时间2026-10-16 19:40:08–19:58:27；六个普通游戏进程、六次原生消费者和六次报告probe均exit0、无超时/强制终止，未重采。9项采集器单元检查及当前Release目标构建通过，无生产代码重编；使用GPU003对应游戏EXE d7d32f1105bc1f58540a8174d4ef87e4b835f107b93ff169382b508e6f410500及同一运行资产。1920×1080/实际4x/VSync-off/seed424242、441区块、361项初始mesh与世界初终摘要按场景匹配；Vulkan实际RTX5070Ti/vendor10de/device2c05/UUID d43c9988ca0711129c89be697511f1d0、API1.4.351、NVIDIA616.64/Immediate/validation1。三轮Vulkan均0告警/0错误且native_retired=true；GL stderr为空。

nearest-rank仅取30≤elapsed_s<180的全部帧，合计174450帧；每轮150项约每秒内存观测。六轮采样焦点全部为0，三组均全失焦匹配，失焦不作无效，也不借此豁免帧时间超限。GL各场景1个GPU样本缺失保留为空，Vulkan缺失0；对应帧全部参与帧时间统计，不补0或删除。

| 场景 | GL/Vulkan帧数 | P95 GL→Vulkan ms | P99 GL→Vulkan ms | 最大帧 GL→Vulkan ms | Vulkan相对GL的P95/P99 |
| --- | --- | --- | --- | --- | --- |
| static | 23292/34567 | 7.4158→8.4113 | 8.0852→11.8321 | 11.8653→14.5950 | +13.42%/+46.34% |
| walk | 23724/34813 | 7.1763→8.5105 | 7.7355→12.0753 | 12.2072→15.1063 | +18.59%/+56.10% |
| edit | 23561/34493 | 7.2314→8.4879 | 7.9005→11.6759 | 10.1229→15.0041 | +17.38%/+47.79% |

三组P95/P99均超过GL的110%相对上限；walk的P99同时超过12ms绝对门槛，其余绝对条件满足。六轮均无超过20ms帧，但不能由单轮短窗口宣称罕见卡顿已消失。内存分别按本轮采样观测段内最大值判断，不包含启动/截图/显存、不叠加T2上限：

| 场景 | 工作集 GL→Vulkan MiB | 相对变化 | 私有提交 GL→Vulkan MiB | 相对变化 |
| --- | --- | --- | --- | --- |
| static | 488.18→510.84 | +4.64% | 1074.15→742.95 | -30.83% |
| walk | 486.08→510.27 | +4.98% | 1075.07→742.80 | -30.91% |
| edit | 489.74→515.74 | +5.31% | 1078.29→748.13 | -30.62% |

两后端edit各600次采样编辑、719次全程计划编辑；Vulkan采样1800次mesh资源上传/430409280字节，static/walk零几何重传，常量/选框顶点上传仍正常计入render_upload_bytes，不称零上传。旧计时probe尚未适配原生分配计数，本轮直接使用普通游戏，未启用它，也不从逻辑phase推断重建次数或缓存收益。

[离线独立复算](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/gl-vulkan-medium-30-150-001/audit-result.json)核对全部原始P95/P99、内存最大值、焦点、GPU缺失、编辑/资源上传、各进程退出及输入hash均一致；230项源码/工具输入、162项生产/资产输入及1140项此前证据未变。普通telemetry另观察到Vulkan present_ms的P99约7.62–7.93ms，三轮最慢帧的同帧present约10.25–10.48ms；这是当前host呈现区间观测，不是已定位驱动/WSI根因，各阶段分位数不可相加。未开展新增细分诊断、优化或补采。

本轮为三场景各单重复的中等长度观察，不替代既定三重复正式验收，不与旧10＋50诊断、旧GL或其他二进制拼接为正式通过；没有同条件优化前的Vulkan中等长度轮次，不能把本轮与旧短诊断差值当作该优化的净收益或净退化。原Vulkan/D3D12正式失败保留，R2技术聚合及R4验收/批准仍未关闭，21/24；不扩大至新人工/回归、Git/sync/archive或GL退役。文档同步主目录，未自动重试、放宽或继续优化。

## 2026-10-16 Vulkan帧槽绑定正常GPU验证

用户追加批准真实GPU验证，独立在refactor-renderer托管worktree执行Release正常路径，不恢复子agent、边界/故障/回归或性能采样；D3D12不改/不运行，GL保留。新增显式GPU probe链接生产Renderer，并在Vulkan私有单次诊断绑定内记录三个真实slot的成功原生调用；不改变公开v1、shader、能力/等待或绑定缓存算法。缓存helper SHA256仍为f8488956e0e793441daa27ac3adde49de1eb2759a5b462cb1cb13188f4ccb3a1，普通未绑定游戏不分配这些观测记录或读取新增时钟。

**最终正常GPU路径通过，非性能结论。** [GPU003运行身份与结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-frame-bindings-gpu-003/result.json)记录北京时间2026-10-16 19:27:32–19:27:39：当前Release显式目标构建通过，实际RTX5070Ti/vendor10de/device2c05/UUID d43c9988ca0711129c89be697511f1d0、Vulkan1.4.351、NVIDIA616.64、640×480/4x/Immediate/validation1。15阶段共45个Presented且读回成功的原frame ID，每阶段遍历实际slot0/1/2；保存15张PNG、90条mesh/selection原生计数行。当前游戏EXE为d7d32f1105bc1f58540a8174d4ef87e4b835f107b93ff169382b508e6f410500，实际运行probe为6238ba47001abdaf00ac34b76dc7b38999f76ee1e1b984e9cca4f34e5c023574；不是普通完整游戏运行或旧二进制的继承成绩。

初始三个slot各创建mesh/选框常量Buffer及pool各一套，合计6个Buffer、6个pool、9个set；后续所有正常阶段这三项新增均为0。重复帧零描述符写；单纹理替换每slot只写一次image，draw重排及恢复每slot各写两次image，UBO/sampler绑定不重写。总计24次vkUpdateDescriptorSets调用、9项UBO/21项image/6项sampler描述符写；mesh矩阵仍每帧写入，选框可见时仍写其独立常量，共87次/8256字节实际uniform写入，没有把缓存命中当作省略矩阵更新。

固定双纹理层及不对称UV/颜色5点逐通道容差1通过；重复、重排及恢复全RGBA与对应参考完全相同，各slot图像完全一致。选框隐藏/恢复/移到右侧、mesh数2→1→2时的选框绑定及左mesh物体矩阵平移/恢复均通过图像区域差异与恢复相等检查；已直接查看[实际截图](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-frame-bindings-gpu-003/captures)。此像素oracle只适用于本局部固定夹具，不替代已确认的正式人工视觉口径。正常Shutdown退出0，warnings/errors均0、native_retired=true，descriptor pool计数和全部Buffer/Image分配释放平衡、live_bytes=0、loader已释放、无quarantine。

首次[GPU001](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-frame-bindings-gpu-001/result.json)因执行器误传shader父目录，在绘制前报invalid SPIR-V length；[GPU002](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-frame-bindings-gpu-002/result.json)修正路径后前13阶段/39帧通过，在物体平移阶段误用原始屏幕取样点而失败。固定相机下该平移约31像素，最终仅将左quad的核查点随图像左移32像素并加入失败截图保留，不修改预期颜色、像素容差或后端；GPU003完整通过。旧失败、原probe源码/二进制及先前包均保留，不把两次失败记录改判成功。

160项其他生产/资产输入及1112项此前证据hash匹配；本轮重链前普通游戏835929完整17项运行输入已保存在GPU001/before-game并复核全部hash，既有bed6运行包继续原样保留。未运行受控真正在途门控、容量增长/原生失败回滚、Resize/故障/关闭回归、Debug矩阵、完整世界/人工玩法或性能采样；截图读回会等待，不证明新缓存的真正在途复用安全。选框顶点仍每帧分配，高水位常驻与扩容峰值风险保留；旧正式预算失败与21/24不变。后续短诊断须先适配旧逻辑phase为实际原生分配/写入计数，不能用UniformAllocate/DescriptorAllocate的visits推断重建。文档增量同步主目录，不新增节点批准或规格sync/archive。

## 2026-10-16 Vulkan帧槽常量与描述符局部复用

用户在仅方案评估后明确批准按方案2实施，验证范围沿用“实现及基本测试，GPU验证/采样分步进行”。本轮独立在refactor-renderer托管worktree修改Vulkan私有后端/metadata和新增基本测试，D3D12不改、GL保留；主目录只同步文档，无公开v1、shader、能力基线、预算或呈现/等待协议变更，T3仍21/24。

每个frame-slot在旧提交完成后先ResetCommandPool，再退休旧mesh/texture refs、选框顶点Buffer及readback；mesh常量Buffer/descriptor pool/sets/CPU打包存储按容量保留。mesh与选框常量分别拥有独立bundle，选框只有常量绑定且不占mesh末尾索引；选框顶点Buffer本轮仍按原路径分配。每个bundle固定UBO/stride/offset/range和sampler寿命，metadata只缓存初始化标志与完整纹理owner/slot/generation；矩阵每帧仍写入，纹理身份变化只更新image绑定，稳定资源不重建池/集合或重复写描述符。扩容为独立新bundle/空metadata，mesh和选框候选全部分配成功后才无新分配交换；新Buffer意味着该bundle所有绑定重新初始化，不依赖裸Vk句柄防复用。旧draw_count输入上限保留，容量增长受原上限及可寻址字节约束。保持非coherent flush、range64及原stride对齐；实际每帧写入字节仍是原draw_count×stride，不把描述符命中伪装为省略矩阵上传。

正常重建在Drain后先reset旧命令、再释放全部bundle，正常关闭释放池后再释放其Buffer；提交/等待未证明完成时仍随完整Native终止保留，不把缓存层绕过原有限等待。逻辑phase仍会被访问，descriptor/UniformAllocate的visits不等于真实原生调用数；后续GPU或短诊断须重建适配probe并记录实际分配/写入，不用旧二进制或旧曲线证明新路径。

**实现及限定基本检查通过，未执行GPU或性能验证。** [本轮运行与输入身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-frame-bindings-basic-001/result.json)记录北京时间2026-10-16 19:14:55–19:15:07：CPU-only Release新增目标构建通过，仅renderer.vulkan_frame_bindings_basic一项CTest通过，内部4个正常metadata用例为初始化/重复/纹理替换与重排、三个slot独立、选框独立常量绑定、普通容量候选及交换后重新初始化。它们调用CPU writer检查绑定mask，不是真实Vulkan调用、native回滚或GPU寿命证据。当前Release游戏编译/链接通过，EXE SHA256为835929fbd4efb522055507539c68787ac1e45eb28a9d78763f3907f4c4fd0b08，未启动游戏；实际重编仅Vulkan私有后端及链接，D3D12/GL源码未变。有选框时每slot最多保留mesh/选框两套bundle，并非只保留一个池；以少量额外常驻资源换取独立绑定失效，未承诺净内存下降。4项本轮源码/测试输入有身份，160项其他生产/资产输入及1042份旧证据hash前后匹配；shader/公开契约/共享门面未改。

构建前已将此前GL/D3D12中等长度对照使用的普通游戏EXE（bed6e00f884d29c11d7ff92126ac377fa739668be5da5f1af1e8e3ff0f6ba2fe）及完整17项运行输入逐字节保存在[原运行输入](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-frame-bindings-basic-001/before-game)，构建后再次核对全部包hash匹配。旧安装/正式/短诊断/中等长度采样及混合焦点轮不改，新链接EXE不能冒充其原运行二进制；当前普通构建只证明编译完成，不继承旧D3D12/Vulkan运行成绩。

未执行真实GPU、在途门控、失败回滚/边界、Resize/关闭/故障回归、Debug矩阵、人工玩法或性能采样；CPU writer不是vkUpdateDescriptorSets实测，编译不证明同步安全。高水位常驻内存、扩容旧新并存、原生失效/回收和收益均待后续验证；不将旧正式预算失败或节点改判通过，不开始选框顶点/mesh上传/持久Map等其他优化，文档及时同步主目录。

## 2026-10-16 GL与D3D12三分钟中等长度对照

用户明确选择同版本GL基线与最新版D3D12，static/walk/edit各一组，先GL三轮后D3D12三轮；每轮30秒预热＋150秒采样，启动/导出另计，共六个独立普通游戏进程。不启动三重复正式验收、不自动复测或删慢帧，不放宽原P95/P99绝对及GL相对门槛/两项内存110%上限。独立在refactor-renderer托管worktree执行，主目录仅同步文档，Vulkan不改/不运行、GL保留，进度21/24不变。

使用同一当前Release普通SymoCraft.exe及资产，显式--renderer opengl/d3d12，不安装私有计时sink或原生诊断observer，避免前轮细分探针记录器/额外时钟影响；普通telemetry自身开销仍在。1920×1080/实际4x/VSync-off、seed424242、完整441区块和allow-unfocused保持，D3D12 validation-on，GL沿用自身实际调试回调。统计使用nearest-rank，各场景独立比较，不混池；进程工作集/私有提交分别取[30,180)秒约每秒观测最大值，不含启动或显存，不新增其他硬预算。

**六轮数据有效；static/walk满足本轮单重复观察条件，原edit存在焦点配对缺口。** [六轮运行与配对结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/gl-d3d12-medium-30-150-001/result.json)记录UTC 2026-10-16 10:28:51–10:47:13，六个游戏进程、六次原生Benchmark消费者及六次报告检查均exit0、无超时/强制终止或自动重试；共285171个采样帧，每轮150项内存观测。三对完整世界、seed、初/终摘要、初始361个mesh和实际4x一致；edit各600次采样编辑/719次全程计划编辑，D3D12采样1800次mesh上传/430409280字节，D3D12的static/walk无几何重传（不将此结论移用于legacy GL流式上传路径）。六轮stderr为空，D3D12实际validation1/Immediate/5070Ti/LUID 00000000:000143b9/驱动32.0.16.1664，GL实际4.6.0 NVIDIA616.64；未绑定额外observer，不补造原生warning计数。GL walk/edit各1个GPU耗时缺失且按空值保留，D3D12 GPU缺失0；对应frame仍完整参与帧时间统计，不填0或删帧。

| 场景 | GL/D3D12采样帧数 | P95 GL→D3D12 ms | P99 GL→D3D12 ms | 最大帧 GL→D3D12 ms | 焦点配对 |
| --- | --- | --- | --- | --- | --- |
| static | 22539/62698 | 7.7196→3.1117 | 8.3821→3.5830 | 15.8673→18.5071 | 均全失焦 |
| walk | 22701/83067 | 7.6170→2.4043 | 8.2129→2.8978 | 12.1364→10.9633 | 均全失焦 |
| edit原轮 | 22447/71719 | 7.8382→2.7040 | 8.7889→4.2676 | 30.4252→16.4997 | GL全失焦，D3D12混合 |

两项内存分别是采样段约每秒观测最大值，单位MiB；不是设备显存或瞬时峰值。下列edit原轮数值仅参考，焦点不一致不能视为完整同条件通过。

| 场景 | 工作集 GL→D3D12 MiB | D3D12/GL | 私有提交 GL→D3D12 MiB | D3D12/GL |
| --- | --- | --- | --- | --- |
| static | 491.84→456.02 | 92.72% | 1077.74→725.62 | 67.33% |
| walk | 488.83→463.89 | 94.90% | 1075.53→726.31 | 67.53% |
| edit原轮 | 490.87→462.51 | 94.22% | 1079.27→731.53 | 67.78% |

三组原轮数值均满足P95≤10ms/P99≤12ms、各自≤GL的110%及两项内存≤GL的110%；**static/walk焦点匹配，edit不匹配**，不能笼统写“三组同条件通过”。D3D12 edit的focus.csv仅记录从失焦到获焦的一次切换：elapsed36.6831298秒，即采样开始后约6.683秒；69211/71719帧获焦，按样本帧累计失焦6.6813472秒，GL对应150秒全失焦。allow-unfocused使采集本身仍有效，但不能消除配对差异；不截取前几秒重算或把混合焦点当全失焦，也不猜测触发原因。

GL edit有4帧>20ms、最大30.4252ms，D3D12三轮无>20ms但static最大帧18.5071ms高于GL15.8673ms，该帧Present host区间13.6779ms、同帧GPU draw0.100448ms；无细分探针不能继续归因单条原生API或旧正式尖峰。已查看GL/D3D12 edit最终截图，画面/物体/材质可见，不把截图目测升级为新正式视觉验收。

实际普通游戏EXE SHA256为bed6e00f884d29c11d7ff92126ac377fa739668be5da5f1af1e8e3ff0f6ba2fe，同一EXE/完整资产包先后显式选择两后端，无细分计时probe；227项实际输入、161项生产/资产记录及构建/消费者身份采前后相同，928份旧正式/诊断/失败证据hash不变。仅为采集统计函数增加显式窗口参数（默认10＋50不变），补两项工具自身检查；9项工具测试及Release游戏/报告消费者构建通过，未改生产代码/普通协议、未执行生产边界/回归。源码快照显式排除Python自动字节码缓存，不忽略实际源码或资产。

**适用边界。** 这是各场景单重复、先GL后D3D12的非交错对照，不是三重复正式验收；温度/频率/后台争用未逐帧测量，普通telemetry、两种原生验证机制及驱动成本仍在，150秒不能证明长期无增长/罕见停顿已消除。数值改善不改变旧正式失败或证明单独SRV缓存的净收益；不放宽预算、不勾选R2/R4聚合或批准项，进度21/24、默认D3D12、Vulkan不改/不运行、GL保留。当前文档增量同步主目录。

**用户明确追加的单轮edit重采已完成，实际全失焦匹配。** 用户看到原edit焦点缺口后要求“重启一轮d3d12的edit采样”；仅追加一个30＋150秒普通D3D12 edit进程，继续对照刚才GL edit基线，不重采GL/static/walk、不覆盖混合焦点原轮。新[重采结果及原轮保留身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/gl-d3d12-medium-30-150-edit-retest-001/result.json)记录UTC 10:49:58–10:53:02：游戏/原生消费者/报告检查均exit0，无超时/强制终止，64120个采样帧全部失焦，focus.csv只有初始失焦项、无后续获焦事件。已在聊天提示保持非游戏窗口前台，未最小化游戏、注入UI操作或改变渲染配置，不猜测上一轮获焦原因。GPU64120有效/0缺失，stderr空、正常退出，600次采样编辑/719次全程编辑、1800次mesh上传/430409280字节与原D3D12轮一致，世界/资产/实际设备及配置匹配。

| edit追加轮 | GL基线 | D3D12重采 | D3D12/GL |
| --- | --- | --- | --- |
| P95 ms | 7.8382 | 3.1936 | 40.74% |
| P99 ms | 8.7889 | 4.5768 | 52.07% |
| 工作集观测最大值 MiB | 490.87 | 457.70 | 93.24% |
| 私有提交观测最大值 MiB | 1079.27 | 729.71 | 67.61% |
| 最大帧 ms | 30.4252 | 13.8081 | 不设额外最大帧硬预算 |

重采无>20ms或>50ms帧，227项来源和当前包/构建/消费者身份未变，1025份旧证据（包括原六轮及混合焦点edit）前后hash匹配。原混合焦点轮的P99 4.2676ms仍保留，不因它比重采更低而择优替换；新重采P99 4.5768ms也不能与旧混合轮作严格同焦点前后归因。以原static/walk及此次明确授权edit重采形成的三组**单重复观察**均数据有效、工作负载/焦点匹配，满足原六项帧时间/内存数值条件。仅关闭本次edit观察的焦点配对缺口，不改动原六轮result/pairs、不删除慢帧、不再自动重采、不替代既定三重复正式验收或注销旧正式失败。验证与唯一任务说明已同步主目录，Vulkan不改、GL保留，21/24不变。

## 2026-10-16 D3D12 SRV缓存前后短诊断

用户批准D3D12短诊断核对SRV堆复用/差量写入效果，不放宽预算或启动正式长采样；独立在refactor-renderer托管worktree执行，Vulkan不改、GL保留。对照为共享摘要已优化、SRV尚未优化的layer-summary-diagnostic-001/d3d12-after三轮，避免将摘要收益混入SRV收益。沿用edit三轮独立进程、各10秒预热＋50秒采样、1920×1080/4x/Immediate/validation1、seed424242及allow-unfocused；不自动复测、不删慢帧。

本轮仅扩展D3D12私有计时probe及采集解析器：每原frame ID输出frame_slot_delta_v1、三slot合计的实际成功建堆/写SRV累计数、当前slot及容量，按连续原始行差分后再选诊断采样窗口，启动暖槽不算采样段操作。旧无计数版本按unmeasured_v1/null解读，不补0；旧descriptor阶段字段保留为阶段边界，visits不代表原生调用次数。10项D3D12解析器自身检查及Release诊断probe/报告消费者构建已通过，未执行生产边界/回归套件。本轮不修改生产渲染代码或Vulkan探针。

**三轮诊断数据有效，SRV常驻阶段成本显著下降；不是正式预算通过。** UTC 2026-10-16 10:15:39–10:19:00完成构建及三轮采集，共64502个采样帧。三个游戏进程、三次原生Benchmark消费者及三次报告检查均exit0、无超时/强制终止或重试；GPU样本完整且缺失0，warning/error/corruption均0、正常原生退休/释放平衡、live_resource_bytes为0，生产等待仍2500ms。实际RTX5070Ti、LUID 00000000:000143b9、驱动32.0.16.1664与旧基线一致，全部失焦且allow-unfocused匹配；完整441区块/361初始mesh、同seed/初终世界摘要、每轮200次采样编辑/600次mesh更新及143581032字节资源上传未变。三轮最终PNG分别与旧对应轮字节一致，另查看本轮首轮画面，未见空白/错材质。

[逐轮离线比较](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-srv-cache-diagnostic-001/comparison.json)及[补核结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-srv-cache-diagnostic-001/audit-result.json)保留全部轮次。下表均为ms，前→后按独立历史轮次并列，不是交错A/B；描述符均值是同帧互斥retire/allocate/update阶段之和，不把分位数相加。

| edit轮次 | 本轮帧数 | 描述符阶段均值 前→后 | 整帧P95 前→后 | 整帧P99 前→后 | 最大帧 前→后 |
| --- | --- | --- | --- | --- | --- |
| 1 | 21017 | 0.62140→0.00192 | 4.6685→3.2402 | 6.8926→4.6237 | 19.2385→12.1642 |
| 2 | 21945 | 0.59675→0.00184 | 3.9624→3.0782 | 5.5037→4.3329 | 12.8233→12.8793 |
| 3 | 21540 | 0.60863→0.00174 | 4.2271→3.1879 | 5.9655→4.3338 | 13.8271→14.0959 |

描述符阶段均值下降99.69%–99.71%，约节省0.595–0.619ms/帧；其中retire旧0.156–0.162ms→约0.00018ms、allocate旧0.311–0.325ms→约0.00020–0.00021ms、update旧0.130–0.134ms→0.00136–0.00153ms。**阶段仍被访问并非仍在调用API。** 每轮原生累计仅暖槽建堆3次/写SRV1083次，三个slot容量各512；三轮全部采样帧实际新建堆0、写SRV0。当前edit负载的draw纹理身份始终为同一atlas，因此这是持续缓存命中的收益；纹理切换/重排正确性另见上轮正常GPU验证，不能由本轮零写入外推任意频繁切换负载的同等收益。旧探针无原生计数，保持unmeasured_v1/null，不把按旧源码推断的调用数伪造为旧实测。

**不利结果及剩余热点保留。** 第二、三轮最大帧分别略高于旧对照，三轮均无>20ms样本但不能声称尾部停顿已消除。第三轮最慢frame12437、elapsed29.2301068、14.0959ms，selection_allocate占11.1461ms，描述符三段合计0.0014ms、slot_wait0.0003ms、同ID GPU draw0.09376ms；首轮最慢12.1642ms中resource_retire占8.8730ms。它们支持本轮异常落在未优化的临时分配/资源退休host区间，不证明驱动内部单条API或旧正式尖峰的唯一根因。selection分配/写入均值仍0.277–0.291ms/帧，command_record约0.996–1.034ms；每编辑帧UpdateMesh整体旧1.724/1.554/1.704ms→1.594/1.502/1.505ms，但第二轮Map复制旧0.455→0.479ms反而上升，不能将未改路径的所有波动归因于SRV缓存。

**收尾失败独立保留、未重采。** [原始执行记录](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-srv-cache-diagnostic-001/result.json)状态仍failed：采集完成后，运行脚本将Python自动更新的test_d3d12_frame_timings/verify-d3d12-frame-timings两份cpython-314字节码缓存计入源码快照，触发Sources changed。未覆盖该失败或修改旧样本；新增独立audit仅离线确认准确两条缓存差异，其他258项输入记录、161项生产/资产记录及构建身份不变，866份旧证据hash匹配，并从原始CSV重新计算三轮统计与收集器结果完全一致。采集器自身collection_valid=true与运行脚本收尾failed分别记账，不以忽略未知差异方式放行。

实际运行的D3D12 probe EXE SHA256为23d0452e35e608117fa9c8c5bde5dc8347f3401f642a2716cd51675cbb01e2fa，普通SymoCraft.exe仍bed6e00f884d29c11d7ff92126ac377fa739668be5da5f1af1e8e3ff0f6ba2fe，本轮运行同一RunGame的隔离probe而非普通EXE。shader/纹理/配置/DLL及两原生消费者与旧基线完全一致；生产差异限此前SRV缓存及私有观测，当前仅修改三份D3D12诊断工具文件。相对旧基线的既有源路径变动和当前完整输入身份见执行记录，不冒称同二进制配对。

**证据上限不变。** 新记录器预留由63200000增至65600000字节，额外累计计数读取/记录及持久SRV堆/metadata都会影响观测；本轮进程内存不能据此判净内存收益或GL相对预算。额外计时、验证层、驱动、后台调度混合，线程CPU仍粗粒度，未采温度/频率；历史非交错A/B及50秒窗口不覆盖罕见停顿或持续增长。未新采GL、static/walk、正式长采样、完整预算配对、Debug/人工玩法或生产边界/回归，不放宽数值门槛、不改判旧正式失败、不自动启动selection/Vulkan等下一优化。Vulkan源码与探针未改/未运行，GL保留、默认D3D12、进度21/24不变，文档增量同步主目录。

## 2026-10-16 D3D12 SRV缓存正常GPU验证

用户另行批准真实GPU正常路径验证，核对纹理切换及draw重排后的画面；仍不含边界、回归或性能采样。独立在refactor-renderer托管worktree执行，主目录仅同步文档，Vulkan不改、GL保留。本轮新增专用原生probe及私有观测字段，不改变公开API、缓存算法或游戏默认后端；计数来自成功的实际CreateDescriptorHeap/CreateShaderResourceView调用，不把CPU metadata测试或阶段访问次数当原生操作证据。

最终[运行记录002](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-srv-cache-gpu-002/result.json)通过：Release构建及原生进程均exit 0、未超时，UTC 2026-10-16 10:02:20至10:02:29（含构建）。实际设备NVIDIA GeForce RTX 5070 Ti，LUID 00000000:000143b9，驱动32.0.16.1664，D3D12 FL12_0/SM6_0；640×480、4x MSAA、Immediate、validation开启。固定两个draw、三个纹理数组和两个纹理层，每阶段访问全部三个真实帧槽，共8阶段/24帧。

| 正常阶段 | 帧数 | 新建原生SRV堆 | 实际SRV写入 |
| --- | --- | --- | --- |
| initial | 3 | 3 | 6 |
| initial-repeat | 3 | 0 | 0 |
| texture-switch | 3 | 0 | 3 |
| switch-repeat | 3 | 0 | 0 |
| draw-reordered | 3 | 0 | 6 |
| reorder-repeat | 3 | 0 | 0 |
| restored | 3 | 0 | 6 |
| restored-repeat | 3 | 0 | 0 |

每slot最终容量2、仅建堆1次、累计写入7次；暖槽后无重建。单个纹理替换只重写对应位置，draw顺序互换重写两个位置；重复阶段零写入。每帧检查五个纹理颜色点（每通道误差不超过1），重复帧/跨slot、重排与恢复分别按完整RGBA相等检查。保留[八张阶段截图及逐帧观测](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-srv-cache-gpu-002/native)；另实际查看initial、texture-switch、draw-reordered、restored四张，左右位置、非对称色块/UV、层选择及恢复一致，未见错误遮挡、镜像或错层。D3D12 Debug Layer warning/error/corruption均0；正常释放资源并Shutdown通过，原生owner清空、live_resource_bytes为0且mesh/texture创建释放数匹配。

首次[运行001](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-srv-cache-gpu-001/result.json)exit 1并完整保留：初始像素检查已通过，但新加观测lambda的draw索引遮蔽frame-slot索引，导致计数写入错误slot。仅修正观测索引后运行002，并把观测行flush提前以保存失败上下文；未改SRV缓存算法，也未删除失败尝试或当作通过。002 probe EXE SHA-256为c6b26188e144a106fbc84425ea038000f1aa9c876da46bb5eff1361a44020383；依赖构建的SymoCraft.exe为bed6e00f884d29c11d7ff92126ac377fa739668be5da5f1af1e8e3ff0f6ba2fe，仅构建、未运行完整游戏。153项源码/构建输入、127项受保护其他生产源码及850项旧证据（含001失败）执行前后身份一致；srv_binding_cache.h仍与前轮基本测试相同。

本结论限于上述真实GPU正常路径：每帧截图/readback会同步，不能证明受控真正在途复用/退休；未覆盖扩容/上限、身份复用、失败提交/分配、Resize、Debug配置、完整游戏或15分钟玩法，亦未补跑边界/回归/性能或内存采样。未将原正式预算失败改判通过，不推断吞吐改善；旧D3D12性能缺陷继续暂缓，Vulkan正式性能结论不变。T3进度仍21/24，不勾选聚合验收/人工批准项。

## 2026-10-16 D3D12帧槽SRV缓存局部实现

用户明确批准D3D12方案2：帧槽复用SRV堆并按纹理身份差量写入，仅实现和基本测试，不进行边界/回归/性能采样；Vulkan暂不改、GL保留。本轮独立在refactor-renderer托管worktree实施，主目录只同步受影响文档，不复制源码或运行产物，不新增公开行为/能力或整组规格sync。

每个FrameContext保留SRV堆及固定容量的optional纹理身份数组；仍先等待该slot的旧fence并退休旧command引用，再准备当前帧。容量足够时保留原堆，仅同一draw位置的完整BackendResourceId（owner/slot/generation）变化或未初始化时调用CreateShaderResourceView；不按裸指针/slot猜身份，也不把普通mesh更新当纹理改变。容量不足时按倍增、受原Tier1上限约束准备全空metadata和新堆，全部分配成功后才无新分配交换；新堆重新写必要SRV，不缩减原输入上限或静默降级。写入前清该位置缓存有效标志、writer返回后再登记身份。metadata不持有texture资源，在途寿命仍由既有frame refs负责；Resize尺寸资源释放/正常关闭清堆和metadata，终止保留仍覆盖完整FrameContext。selection、root signature/shader、资源上传、Present与等待策略不改。

**实现和基本测试通过，未作GPU或性能验证。** [本轮构建及基本结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-srv-cache-basic-001/result.json)记录CPU-only新目标构建成功、仅renderer.d3d12_srv_cache_basic一项CTest通过，内部3个正常metadata用例：重复/改变/重排绑定的writer调用、三个帧槽独立缓存、普通容量准备及交换。它们复用生产SrvBindingCache但writer为CPU回调，不是真实CreateDescriptorHeap/CreateShaderResourceView调用，也未验证GPU fence安全。当前Release游戏编译/链接通过，EXE SHA256为 `5f39948643332afef64ba1f3c6d9eefa953387880e890edacbd161fdc2fbba5d`，未启动游戏。4份本轮源码/测试输入有hash，128份此前其他生产输入（含Vulkan）及833份旧正式/诊断/安装/基本/GPU证据hash前后匹配，旧样本/结论未覆盖。

**未测风险保留。** 未执行容量上限/溢出、真实堆扩容/分配失败/部分写入异常、身份复用、GPU在途更新/销毁、Resize/最小化/故障关闭、Debug矩阵、旧回归或任何性能采样；CPU正常容量用例不关闭这些原生边界。保留容量的常驻内存代价未测，不能承诺消除此前0.6ms/帧或原预算已达标。纹理在当前v1内不可更新，若以后引入同身份原生view替换，须扩展缓存键/失效责任。现有私有descriptor阶段保留为逻辑阶段边界，visits不是原生创建/销毁/写入次数；后续GPU/短诊断须按本轮源码重新构建对应probe，当前旧probe/旧曲线不代表新缓存执行。R2技术聚合、R4验收/批准及21/24保持，默认D3D12、Vulkan不改、GL保留；本文、机制和唯一任务增量同步主目录，无Git/sync/archive或子agent。

## 2026-10-16 缓存摘要前后短诊断

用户追加授权适配旧扫描计时字段，并在refactor-renderer托管worktree独立执行D3D12/Vulkan短诊断以核对缓存摘要前后效果；不放宽预算、不启动正式长采样，不扩为描述符/临时资源优化，GL保留。沿用旧D3D12 edit与Vulkan static各三轮10秒预热＋50秒采样，1920×1080/4x/Immediate、validation1、seed424242及allow-unfocused；使用新目录，不重写旧结果或自动重试。运行中状态/实际命令见[本轮入口](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/layer-summary-diagnostic-001/result.json)。

私有计时CSV新增明确的cached_max_v1标识和layer_summary_check嵌套墙钟/调用数，旧layer_scan/顶点数保留为已退役零字段，检查器要求新摘要调用数匹配实际mesh draw且禁止混入旧扫描。旧D3D12无标识行按per_vertex_v1继续读取；旧Vulkan无独立层校验计时，标记unmeasured_v1并返回null，不伪造0或扫描净收益。新增计时仅绑定私有sink时读时钟，不改公开v1/普通报告、等待或接受语义。两解析器13项工具自身检查和Release的摘要基本/私有诊断两项CTest已通过；这些不是生产非法输入/回滚/句柄等边界或全量回归成绩。

**六轮数据有效，缓存摘要显著降低门面准备成本；不是正式预算通过。** UTC 09:25:44–09:32:15完成构建/工具检查和本轮六进程，D3D12三轮样本47944帧、Vulkan三轮27744帧；六个游戏进程、六次原生Benchmark消费者及六次报告检查全部exit0，无重试/超时/强制终止，GPU样本均有效且缺失0，实际诊断warning/error均0（D3D12 corruption亦0）、正常退休、2500ms生产等待未变。前后均完整441区块/361初始mesh、同seed及初始/最终世界摘要、同资产/DLL/原生消费者和驱动/设备；D3D12每轮采样段200次编辑/600次mesh更新、Vulkan零编辑/零几何上传，未缩减负载。前后六轮各自始终失焦，allow-unfocused可比但不能排除其他后台争用。

[逐轮离线比较](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/layer-summary-diagnostic-001/comparison.json)复用此前D3D12 edit诊断和Vulkan static诊断，不重写其原始结果。下表时间均为ms，“前→后”为独立历史三轮按序并列，不是同刻或同进程配对；门面均值是facade_prepare，不含后端准备，分位数不能与各阶段分位数相加。

| 后端/轮次 | 门面准备均值 前→后 | 整帧P95 前→后 | 整帧P99 前→后 | 本轮最大帧 |
| --- | --- | --- | --- | --- |
| D3D12 edit 1 | 1.21790→0.02556 | 5.7595→4.6685 | 7.3808→6.8926 | 19.2385 |
| D3D12 edit 2 | 1.20835→0.02498 | 5.7403→3.9624 | 7.1897→5.5037 | 12.8233 |
| D3D12 edit 3 | 1.20538→0.02474 | 5.7144→4.2271 | 7.0536→5.9655 | 13.8271 |
| Vulkan static 1 | 1.62803→0.02886 | 8.4687→7.4503 | 9.1344→9.0712 | 16.5666 |
| Vulkan static 2 | 1.61109→0.02706 | 8.4923→6.8089 | 9.1323→7.8753 | 15.2919 |
| Vulkan static 3 | 1.60890→0.02658 | 8.3895→6.5507 | 9.0598→7.3302 | 12.5097 |

门面均值三轮分别下降D3D12 97.90%–97.95%、Vulkan 98.23%–98.35%。D3D12旧扫描子项均值1.17272–1.18447ms，当前摘要比较为0.00696–0.00714ms；Vulkan旧独立项未测，当前为0.00676–0.00689ms，不能给旧Vulkan补0来计算子项净收益。当前每样本帧均361次摘要比较、扫描顶点/旧扫描调用/时间均0，两个模型子项均属facade父项，不重复相加。Create/Update仍完整验证顶点并计算摘要，不是取消输入校验。

**剩余成本及不利结果保留。** D3D12描述符销毁/创建/写入均值仍0.597–0.621ms/帧，selection分配/写入0.283–0.295ms；Vulkan描述符分配/更新1.191–1.277ms，混合资源退休1.078–1.162ms，均未优化。D3D12编辑帧的三次mesh更新总均值原1.455/1.487/1.495ms、本轮1.724/1.554/1.704ms，其中校验累计原0.057–0.059ms、本轮0.069–0.072ms；摘要计算仍有更新成本，原生分配/复制也有波动，不能把更新总增加全部归因于摘要。D3D12三轮最大帧均高于旧短诊断9.9625/10.2880/10.0231ms；Vulkan第1轮P99仅小降0.0632ms，其同帧P99尾部均值主要落在command_record 3.434ms、resource_retire 2.241ms、descriptor分配/更新合计2.144ms，而facade仅0.0505ms。它们是含验证/驱动/调度的host区间，不证明单条API或GPU执行根因。六轮无>20ms样本，但不声称罕见尖峰已消除。

新增D3D12/Vulkan探针EXE SHA256分别为 `cf519979d7dc30d794b155cb9cbf05ec2613cafcf586adfa6e63c15b19f2ab1b`、`7ce79ab5302382d1b1c01988511c796d181ef6fdd2c571e547c720a2845c7477`，普通游戏构建为 `f8b14d5b48d73059d08a864673a9d8d4e1c9ccaae6306b505058ee7b10b53dab`，本轮实际运行的是同一RunGame的隔离probe而非该普通EXE。246项运行输入及实际构建身份采前后匹配，718份旧正式/诊断/安装/基本与GPU证据hash前后不变；前后变动路径记录在入口manifest，生产差异限共享摘要及诊断、未改生产后端算法（Vulkan旧诊断之后另有不参与其执行的D3D12计时扩展）。

**证据边界不变。** 新每draw摘要计时给Vulkan增加旧探针没有的时钟扰动，D3D12记录器固定容量从62400000增至63200000字节；不以本轮进程内存判摘要净内存收益。线程CPU粗粒度、温度/频率未采集、历史非交错A/B和短窗口限制均保留。无新GL、正式同版本配对/长时/人工玩法、生产故障/边界或全量回归，不因P99改善或本轮均低于12ms就判相对预算通过。原D3D12与Vulkan正式超限、R2技术聚合/R4验收/批准及21/24保持，不追删旧慢帧、不改变默认或GL保留；结果/机制/任务已增量同步主目录，无Git/sync/archive或子agent。

## 2026-10-16 共享摘要真实GPU验证

用户在此前局部实现后追加批准真实GPU验证，本轮只补正常路径，不将此前“未运行GPU”的点时记录改写为已运行。新增独立公开v1探针链接实际生产renderer/platform/assets，不改生产代码，不执行含非法输入、空网格、故障或窗口恢复的旧smoke整组；CMake仅注册显式构建目标，不加入无人值守CTest。工作仍在refactor-renderer托管worktree，文档增量同步主目录。

**D3D12、Vulkan正常GPU路径均通过。** [本轮运行及身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/layer-summary-gpu-001/result.json)记录UTC 09:15:58–09:16:07的构建与两个独立原生进程，均exit0、无超时/强制终止。使用Release、640×480、实际4x/Immediate/validation-on和RTX5070Ti；D3D12驱动32.0.16.1664，LUID 00000000:000143b9，Vulkan驱动NVIDIA616.64、API1.4.351、UUID d43c9988ca0711129c89be697511f1d0。新探针EXE SHA256为 `c07fbab49cac1ec5e245ac2593f16f81057a38d06462f486114c91bdbca10d90`；游戏EXE仍为前节 `3c9b73b3...`，本轮未启动游戏，不能把探针结果写成完整世界/玩法通过。

每后端用同一持久mesh绘制两个独立纹理层，四层atlas上的正常层号1→2→1更新改变右侧灰/青画面，恢复后与初始RGBA完全一致；连续重复绘制的RGBA分别一致，重新绑定sRGB纹理的预期解码像素通过。左侧四个非对称UV色块方向及右侧实际层色按内部探针容差每通道1检查；这不是新增正式人工视觉阈值。每帧1draw/12顶点、共6张640×480真实读回PNG，合计12张；实际查看两后端创建/更新/sRGB六张图，未见空白/倒置/错层。D3D12启用InfoQueue警告/错误/corruption即失败检查并通过Render与Shutdown；Vulkan关闭报告warning/error=0/0、native_retired=true。截图等待覆盖普通更新序列，不构造或证明强门控真正在途。

153份构建源码输入与10份shader输入留有身份，运行中源码及构建身份未变；此前7份摘要实现/基本测试源码hash仍匹配，697份旧正式/诊断/安装及上一轮基本测试文件hash前后匹配。未运行Debug/其他MSAA、失败回滚、空网格/层数边界、句柄复用、全量或其他回归、GL、完整游戏/15分钟人工及性能采样；未适配旧扫描诊断采集器。仅关闭本次正常GPU验证缺口，不据像素结果推定所有边界/寿命或性能达标，原预算失败和21/24保持，默认D3D12、GL保留，无Git/sync/archive或子agent。

## 2026-10-16 共享纹理层摘要局部优化

用户仅批准“共享顶点扫描改为缓存摘要”的实现和基本测试，明确不包含边界/回归测试；本轮在refactor-renderer托管worktree实施并增量同步主目录。ValidateMesh在原完整校验遍历中取得optional最大纹理层，Create/Update在后端接受成功后才将CPU快照和摘要无新分配地发布；Render按当前纹理层数做常数时间比较，不再逐帧重扫或保留旧LayerScan计时包裹。空网格以nullopt表达，不以0伪造顶点存在；销毁/清理重置摘要。公开v1、错误分类、mesh所有权及GPU退休策略不改，描述符/临时资源优化未启动，GL保留。

**实现及基本测试完成，未作性能验收。** [本轮构建/基本测试结果](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/layer-summary-basic-001/result.json)记录CPU-only Release新目标构建成功、仅运行renderer.layer_summary_basic一项CTest通过（内部3个正常用例：摘要创建/正常更新、D3D12身份CPU recorder、Vulkan身份CPU recorder），以及当前Release游戏仅编译/链接成功。游戏EXE SHA256为 `3c9b73b3c93ced1a2e2db66c180c894b1fb96648cc99bcc82b20897023880890`，未启动游戏。7份本轮源码/测试输入有hash，689份旧正式/诊断/安装证据前后hash一致，未覆盖旧候选或旧结果。

按用户限定，未新增/执行失败回滚、空网格/层数边界、句柄复用等边界用例；未运行已有门面全套、源码/target负向检查、其他回归、Debug矩阵、真实GPU、玩法或任何新性能采样。事务/空网格/句柄语义按实现保留，但不从静态代码或基本测试冒称这些边界已验证；后续完整验收仍需相应授权及证据。旧D3D12逐顶点扫描采集器要求LayerScan>0，当前扫描和该计时包裹已删除；后续若获准重采须先适配已移除阶段并重建对应probe，不能拿旧EXE或旧字段评估优化效果。本轮不扩改诊断工具、不恢复采样。原预算失败、R2聚合/R4验收/批准及21/24保持；描述符/临时资源优化未执行，默认D3D12、GL保留，无Git/sync/archive或子agent。现役CPU Input场景仅将“逐顶点”实现措辞改为“全部顶点所用层号合法性”，不放宽要求、不将未测目标整组同步；机制/任务和本节增量同步主目录。

## 2026-10-16 D3D12细分计时与短诊断

用户明确批准在refactor-renderer托管worktree独立增加D3D12细分计时并短诊断采样，覆盖此前仅分析/暂缓排查的执行范围；只允许诊断，不授权优化、放宽预算或正式复测。复用创建线程私有sink与同一生产RunGame，逐frame ID记录共享纹理层顶点扫描、SRV堆销毁/创建及SRV写入、selection分配/Map写入、编辑UpdateMesh的校验/自有复制/边界扫描/原生分配/Map复制；编辑工作在Render之前发生，累计并一次归到下一个原帧ID，启动Create独立记录。嵌套扫描属于facade_prepare子集，编辑分项属于mesh_update_total子集，均不能与外层重复相加。普通CLI/正式CSV、资源寿命及等待策略不改，GL保留。

**结果：四项重点成本已实测分开，旧27.8316ms非编辑准备尖峰未复现，不宣布旧超限已修复。** 北京时间2026-10-16 11:24:43–11:27:51，edit三轮独立进程各10秒预热＋50秒采样，1920×1080/4x/Immediate、validation1、seed424242及allow-unfocused；无重试/删慢帧。保留[原始诊断与身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-edit-timing-001/result.json)，新probe EXE SHA256为 `f15734d1fe52c7fb702ba24eb755eb20b6fcb4a504020d3d9831983c24ba291a`，不是旧正式Release002或安装004原EXE。327份当前源码/工具输入、诊断包与消费者采样前后不变；640份旧冻结文件hash前后匹配，包含旧D3D12正式包/18轮、Vulkan正式包/18轮及先前Vulkan诊断。未重写其结果或原始数据。

三轮共32507个正式窗口帧完整按renderer_frame_id关联，无CPU读取缺失/记录溢出。各轮200个编辑帧、正式段600次mesh更新（每次编辑重建3块），与CSV upload_calls逐帧精确匹配；正式段资源上传均143581032字节，初/终世界摘要三轮一致。实际RTX5070Ti、LUID=00000000:000143b9、驱动32.0.16.1664，441区块/361初始mesh，全部失焦；每轮GPU样本完整。三轮游戏及六次原生消费者/报告检查均exit0，无超时/强制终止，warning/error/corruption全0，native_cleared且mesh/texture计数平衡、live_resource_bytes=0，无quarantine；probe持有的观察COM引用另行释放。不将native_cleared单独当作全部外部引用已消失。2500ms生产等待上限保持，未使用故障probe默认500ms。

| 诊断轮次 | 正式段帧数 | frame P95 ms | frame P99 ms | 最大帧 ms | >20ms帧 |
| --- | --- | --- | --- | --- | --- |
| 1 | 10864 | 5.7595 | 7.3808 | 9.9625 | 0 |
| 2 | 10737 | 5.7403 | 7.1897 | 10.2880 | 0 |
| 3 | 10906 | 5.7144 | 7.0536 | 10.0231 | 0 |

下表均为三轮各自均值的范围；“每编辑帧”不与“每帧”混用，父子项不重复求和。

| 重点成本 | 均值 ms | 实测解释 |
| --- | --- | --- |
| 共享纹理层顶点扫描/每帧 | 1.173–1.184 | 占本轮facade_prepare的97.21%–97.29%，为约206万顶点逐draw累积的独立内层墙钟；是已量化常驻成本，不是旧尖峰根因证明 |
| SRV堆销毁＋创建＋SRV写入/每帧 | 0.630–0.652 | 销毁0.171–0.175、创建0.329–0.349、写入0.128–0.130；矩阵仍用root constants，不存在Vulkan式每帧常量Buffer |
| 选择框分配＋Map/复制/Unmap/每帧 | 0.286–0.297 | 原生分配0.224–0.231、Map写入0.062–0.067；保留原288字节选择框及原创建/退休策略 |
| UpdateMesh整体/每编辑帧 | 1.455–1.495 | 每编辑帧3次更新，非编辑帧为0；属于原pack/上传大区间的一部分，不包含World重建与调用前World callback复制 |
| 编辑原生分配/每编辑帧 | 0.782–0.800 | CreateCommittedResource调用区间，保留UPLOAD heap与新版本接受，不复用在途资源 |
| 编辑Map/复制/Unmap/每编辑帧 | 0.439–0.493 | 与原生分配合计占UpdateMesh整体84.17%–85.24%，是本次编辑路径主要成本 |
| 编辑CPU校验/自有复制/坐标边界扫描/每编辑帧 | 0.057–0.059 / 0.017–0.019 / 0.109–0.112 | 仍保留完整输入校验、自有快照及double/float边界，不以去校验或去复制换取诊断结果 |

此外，命令录制均值1.134–1.145ms，allocator/list的两次原顺序Reset/Close/Begin均值0.201–0.207ms；这部分不能漏算。FrameContext.Wait与PresentationLatency.Wait各自P99不超过0.0013/0.0019ms，本次没有观察到它们主导慢帧。显式InfoQueue检查均值约0.0078ms，但Debug Layer内嵌处理分散在各原生调用里，不能据此称整个验证层仅耗此值；GPU draw不含clear/resolve/Present，仍不能排除所有GPU/驱动因素。

第二轮最慢帧10035、elapsed=46.7501268，frame10.2880ms，含一次编辑/3块重建：原mesh_ms=1.7029、pack_ms=2.6020，其中UpdateMesh整体2.4642（分配0.9935、Map复制1.0898）；cpu_prepare4.9887，其中共享扫描1.3156、命令录制1.8455；submit0.5376、Present0.4020，GPU draw0.08752ms。它是多段成本叠加，不是原非编辑27.8316ms尖峰的复现。pack_ms已含上传与这些UpdateMesh子项，不再叠加upload_cpu_ms；扫描也不再加到facade_prepare外。

**证据上限与风险。** 私有sink用19个实际D3D12阶段（共享枚举另保留Vulkan阶段），嵌套工作仅墙钟计时，无逐draw CPU系统调用；前帧间工作只消费一次，启动Create归第一帧并在正式窗外。每进程预分配100000条、容量62400000字节，逐帧只写内存、退出后写CSV；额外时钟、扫描边界、内存记录及既有native计数/GetResourceAllocationInfo会扰动数据，未测无埋点净开销，诊断内存不能拿去判GL相对预算。GetThreadTimes仍为粗粒度计费，0不表示未运行/纯等待，不可靠地由墙钟减CPU得到阻塞。新构建/不同会话、只有edit三轮且无同步GL配对：本次P95/P99低于绝对门槛，也低于旧GL edit对应历史相对上限，仅作旁证，不升级正式通过或替换旧失败。旧27.8316ms及其8.6981ms P99超限的唯一根因仍未证实，未自动追加采样/关闭验证/放宽/优化。

[收尾回归](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/d3d12-edit-timing-checks-001/result.json)完成Release与CPU-only Release各10/10定向CTest，包含前置编辑工作只消费一次/扫描独立/未绑定无计时、两个采集器各4项Python拒绝/归属测试及门面、telemetry、边界；预算采集器7项CPU测试通过，三后端各三帧未绑定普通启动/关闭exit0，无追加性能采样。327份来源、诊断包和640份旧冻结文件仍不变；非全量矩阵/Debug/长时/人工玩法复验。当前明确授权诊断已完成，优化/正式预算复测仍未授权；R2聚合、R4验收/批准与21/24不变，默认D3D12、GL保留，文档增量同步主目录，无子agent/Git/sync/archive。

## 2026-10-16 Vulkan细分计时与短诊断

用户已批准独立添加细分计时并短诊断采样，定位static前两轮尖峰；本轮在refactor-renderer托管worktree实施，相关文档增量同步主目录。采用仅由隔离原生probe安装的创建线程私有计时sink，按原renderer_frame_id关联门面准备、原生资源退休/分配/写入、描述符分配/更新、录制、slot退休等待、Acquire、呈现fence、提交与Present；不新增公开v1、普通游戏CLI或正式CSV字段。墙钟与本线程CPU时间分列，CPU观测失败不得填零；未绑定路径不读取额外时钟或分配诊断记录。

**结果：已完成计时与三轮短诊断，未复现旧140–155ms成簇尖峰，原根因仍未闭合。** 北京时间2026-10-16 10:34:00–10:37:07，static三轮独立进程各10秒预热＋50秒采样，保持1920×1080/4x/Immediate、validation1、seed424242及allow-unfocused。保留[诊断结果/身份与原始记录](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-static-timing-001/result.json)，EXE SHA256为 `6db1df72abd919e4957cd43be4de8bd7179bed68f374302e7459926338b086bb`；它是新诊断构建，不冒称安装004原二进制。共有shader/配置/纹理资产hash与004一致，新构建另保留GL资产。324份当前代码/工具输入采样前后不变，356份旧证据/安装文件hash核对不变，包含83份安装文件、108份正式capture产物、正式manifest/其他日志及旧D3D12预算result；未覆盖旧正式失败。

三轮共21449个正式窗口帧均按原renderer_frame_id完整关联，CPU读取无缺失、无记录溢出，截图各1行在窗口外单列。实际RTX5070Ti/NVIDIA616.64、441区块、2063250顶点/362draw，静止无重建/编辑/资源上传，render_upload_bytes逐帧23456；全部失焦，与旧正式口径匹配。交换链创建计数始终1，未观测本轮重建；三轮生产游戏及六次原生消费者/报告检查均exit0，无超时/强制终止，validation warning/error均0/0且native_retired=true，生产2500ms等待上限未被旧故障probe的300ms默认替换。

| 诊断轮次 | 正式段帧数 | frame P95 ms | frame P99 ms | 最大帧 ms | >20ms帧 |
| --- | --- | --- | --- | --- | --- |
| 1 | 7130 | 8.4687 | 9.1344 | 12.6075 | 0 |
| 2 | 7142 | 8.4923 | 9.1323 | 17.4928 | 0 |
| 3 | 7177 | 8.3895 | 9.0598 | 11.9921 | 0 |

**已分开的常驻成本与较小异常。** 下表为三轮各阶段墙钟均值范围，互斥区间按同帧累加；不是相加各列P99，也不把重叠GPU draw加到CPU阶段上。

| 阶段 | 每帧均值范围 ms | 边界 |
| --- | --- | --- |
| 门面准备 | 1.609–1.628 | 相机/transform、句柄/纹理层扫描、自有draw准备整体，不是仅逐顶点循环 |
| frame资源退休 | 1.197–1.216 | DropFrame含引用释放、buffer销毁/释放和descriptor pool销毁，不能全称纯buffer释放 |
| selection/uniform buffer分配合计 | 0.188–0.190 | MakeBuffer整体含CPU对象、Create/GetRequirements/Allocate/Bind；两次写入另列 |
| descriptor pool/set分配 | 0.354–0.356 | 创建pool及分配362sets，不含更新 |
| descriptor更新 | 0.852–0.862 | 362次UpdateDescriptorSets及写入准备 |
| 命令录制 | 1.817–1.829 | draw/bind、barrier、结束命令，含验证层/驱动处理，不是纯应用算术 |
| slot退休/查询、Acquire及呈现fence合计 | 0.019–0.020 | slot区间含Finish/Collect，不冒称纯WaitForFences阻塞时长；Submit/Present另列 |

第二轮最慢帧4209、elapsed=29.4493771、frame=17.4928ms：selection_allocate=10.5035ms，门面1.5087、资源退休1.3774、descriptor分配0.3881/更新0.9318、命令录制1.7211；slot=0.0003、Acquire=0.0173、present fence=0.0066、Present=0.3222ms，同ID GPU draw=4.001888ms。它支持“本次这个较小异常位于MakeBuffer调用区间，不是本帧显式fence/Acquire等待”，不能进一步归给单条vkAllocateMemory，也不能解释旧140–155ms簇。第二轮另一个16.5593ms帧1493包含command_record=7.9362和descriptor_update=3.3507ms，说明慢帧不只一种阶段。

**风险与证据上限。** 预分配100000条有界记录，逐帧只写内存、结束后写独立CSV；19个具名阶段中重复阶段累加，不改普通CSV/v1。记录器和额外时钟/native计数有扰动，诊断内存含记录器，不与GL重算预算，未测“无埋点对照”净开销。GetThreadTimes虽以100ns单位返回，本机本次非零增量只有15.625/31.25ms；0表示该区间未跨计费跳变，不是未执行/纯等待，各阶段CPU值甚至可大于墙钟，不能可靠用相减或阶段CPU和区分10ms分配的忙碌/内部阻塞/失调度。旧尖峰未复现且无原生调用栈/调度跟踪，不宣布唯一底层根因或修复；验证层与驱动成本仍混合。下一步若继续定位，应在受控短窗口补高分辨率线程执行/调度及MakeBuffer内部调用栈，并拆DropFrame中buffer与descriptor pool销毁；验证开关A/B只能作独立诊断，不改变正式validation1口径。本轮不自动追加采样、关验证、优化或放宽预算。

[定向回归001](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-static-timing-checks-001/result.json)记录Release和CPU-only Release各9/9 CTest通过，含计时disabled/binding/区间分解/缺失/异常契约、4项Python按ID/尾帧/拒绝测试、公开门面、telemetry与边界；随后普通冒烟误带benchmark专属参数被合法拒绝exit64，未进入游戏，失败日志保留。[修正命令002](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-static-timing-checks-002/result.json)另目录完成Vulkan/D3D12/GL各三帧未绑定启动/退出exit0及原预算采集器7项CPU测试，诊断源码/包和旧冻结证据仍不变；不是追加性能采样、全矩阵/Debug/玩法重验或D3D12性能排查。原Vulkan正式预算失败和21/24保持，D3D12缺陷继续暂缓、默认D3D12、GL保留，无子agent/Git/sync/archive；文档及时增量同步主目录。

## 2026-10-15 Vulkan超限只读定位

用户要求独立分析已有数据和代码，重点static前两轮，不重新采样、不放宽预算。本轮未修改生产/测试代码、构建或启动游戏，仅用Python标准库离线读取18轮CSV及既有GUI报告；[逐帧分解](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-performance-analysis-004/result.json)与[时段/旁证/身份核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-performance-analysis-004/context-and-identity.json)保存必要结果。319份来源、83份安装文件、108份正式原始产物及正式manifest/旧D3D12预算result均未变。辅助脚本初次对旧GL缺少现代列处理不全的离线失败保留在001–003；最终缺失列保持null，不填0，不把分析脚本成功称新游戏成绩。

**已定位到阶段，不是唯一底层根因。** static1/2为正式段后半部成簇的CPU侧长耗时，主要位于合并cpu_prepare；同帧GPU draw未出现相应放大。GPU计时不含全部GPU/呈现工作，cpu_prepare又含等待、驱动和线程调度，不能由此宣布GPU整体无关或某个fence/分配函数已被证明是根因。

| Vulkan static轮次 | 整轮P99 ms | cpu_prepare P99 ms | GPU draw P99 ms | >20ms帧及elapsed范围 |
| --- | --- | --- | --- | --- |
| 1 | 20.6202 | 17.3139 | 4.200736 | 69帧，37.839–42.484秒 |
| 2 | 37.7910 | 32.4135 | 4.198688 | 188帧，44.311–55.548秒 |
| 3 | 9.2428 | 8.1925 | 4.217472 | 0帧 |

以上是各列独立分位数，不能相加。按各轮P99及以上的实际同帧集合累计，cpu_prepare占这些帧总耗时约83.3%/86.0%；第一轮63个尾帧中60个由render_cpu主导，第二轮56个中53个如此，其余有Present/观测段主导。static2最大帧renderer_frame_id=6370、elapsed=53.9505903：frame154.7113ms，其中prepare153.2154、submit0.7921、Present0.5535，其余0.1503；同ID GPU draw3.842624ms。第一轮最大帧140.4061ms含prepare108.3059和Present30.9271ms。也不能把全部尾帧归为prepare：第一轮P99秩帧5319的20.6202ms中，观测instrumentation为9.8596ms；第二轮另有35.4943ms观测尖峰叠加prepare116.4722ms。

三轮预热frame P99约9.2–9.35ms且无>20ms帧；前两轮尖峰不是单纯启动预热计时混入，截图/导出在正式窗口外。正式static的相机/位置、2063250顶点及362draw恒定，编辑/重建/几何资源上传均0，只有每帧23456字节常量/选框上传，焦点始终0，无validation错误或缺失GPU样本；因此不支持“编辑/世界重传/几何负载突然增加/焦点切换”解释。仍缺逐帧WSI返回、重建和细分等待记录，不能排除运行中的呈现/重建成本。

**持续基础成本与候选热点。** 无20ms尖峰的static3仍P99=9.2428ms，超过对应GL的7.67316ms相对上限；不能删除前两轮尖峰就宣告达标。静态核查发现：
- [共享门面](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/renderer_impl.cpp)第140行每帧每draw调用[ValidateMeshTextureLayers](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/validation.cpp)第57行逐顶点扫描，约206万顶点，耗时在第198行并入cpu_prepare。这是明确重复工作；与既有D3D12热点同源，但本轮只分析Vulkan，不重开已暂缓D3D12工作。
- [Vulkan单帧路径](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp)第988/1012–1042行每次复用frame-slot先清退旧常量/选框/descriptor pool，再新建两份Buffer/独立Memory并Map/写入，新建pool、分配362个set、362次UpdateDescriptorSets（361组各3项＋选框1项），随后录制362draw。三frame-slot已有在途寿命保护，但未复用这些常量资源/描述符；它们不是几何重传，却是每帧真实分配/验证成本。
- 同一prepare区间还含Finish提交fence、AcquireNextImage、第1072行呈现fence等待及可能Rebuild；仅凭总列不能区分“扫描/分配/描述符活跃CPU”与“GPU/WSI等待、驱动阻塞或线程失调度”。第491行启用同步validation，不能因Release就把验证成本视为关闭。
- 观测段在[app](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/app/src/application.cpp)第399–410行合并PollGpu和约每秒进程内存查询，尚无子项耗时；后端Poll使用GetFenceStatus和无WAIT flag查询，源码没有在Poll中显式WaitForFences，不能把35ms直接叫“每秒内存采样卡顿”或“GPU Poll主动等待”。

已有同候选GUI quick可作有限旁证：两组均全失焦、1080p/4x/Immediate、10＋10秒，static validation=false的prepare P50/P99=4.3124/5.1243ms、frame P99=5.5909ms；另一个validation=true会话为7.3752/9.5352ms、frame P99=10.7450ms。支持“验证层与频繁原生调用的成本应优先拆分”，但会话时间/环境未受控、窗口比正式短，不能计算纯验证层净成本、证明它造成晚段尖峰，或用关闭验证的quick替代正式validation1预算。

当前最小后续定位工作是将门面扫描、后端frame-slot退休/等待、临时分配与描述符、命令录制、Acquire/呈现等待及观测子项分别计时，并区分线程运行时间与等待/失调度；现有数据没有这些字段或系统调度跟踪，证据上限止于上述阶段/热点，不能补造唯一根因。本轮未加埋点、未运行验证开关A/B、追踪或优化，执行这些工作须另获授权。原预算失败保持、21/24不变，D3D12缺陷继续暂缓、默认D3D12、GL保留；本节及任务说明及时同步主目录，无子agent/Git/sync/archive。

## 2026-10-15 Vulkan正式预算对照

**结论：18轮数据有效，Vulkan帧时间预算未通过；内存预算九组通过。** 用户在采样前明确同意[独立Vulkan预算及执行口径](decisions.md#2026-10-15-vulkan已选预算与正式配对口径)，不是按实测倒定阈值。北京时间2026-10-15 17:20:05–17:38:34，先GL九轮再Vulkan九轮，无重试；每轮独立进程、10秒预热＋50秒正式采样。保留[完整采集结果与原始报告](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-formal-budget-10-50-001/result.json)、[采样前身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-budget-preflight-002/result.json)及[离线数据审计](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/vulkan-formal-budget-10-50-001/audit/result.json)；审计status=passed仅指数据/失败判定复核通过，不是性能通过。

采用新安装004 Release双现代候选与同批独立GL包，EXE SHA256分别为 `d19a77ba61586908ff3e6a34140892af67488091469187ff9afbafaa4f8ce2ae`、`49d67a025941ebb12a4b395c5b905708c87ac3ba865dd33c82196f9bb6d2c553`。本轮只扩展既有 `verify-d3d12-budgets.py` 的显式现代后端/独立GL入口及超时日志，保持D3D12默认、原nearest-rank、10≤elapsed_s<60正式窗口、独立内存最大值及焦点可比性判断，不改生产代码或重建游戏。采集器7项CPU测试通过；初次新增mock缺输出目录导致CPU夹具失败已修，离线预检001误选旧walk却要求static被报告探针正确拒绝，002在新目录选对static后通过，均非正式游戏失败，旧报告未改。

实际为RTX5070Ti，GL设备NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2，Vulkan UUID d43c9988ca0711129c89be697511f1d0、API1.4.351、NVIDIA616.64；1920×1080/4x、VSync-off，Vulkan实际Immediate且validation开启。18轮均为seed424242、441区块/361初始mesh、相同场景初始摘要；edit每轮正式段200次、总计划239次编辑，static/walk无编辑，未缩减工作负载。18轮游戏、18次原生Benchmark消费者和18次game-report检查均exit0，无超时或强制终止；Vulkan九轮validation warning/error均0/0、native_retired=true，GL stderr均空。108份原始产物hash、319份本轮前来源、83份安装文件和旧D3D12预算result前后不变；生产/普通候选身份不因采集器两份脚本改动而重写。

下表为每组GL→Vulkan，帧时间单位ms，内存为Vulkan/GL正式窗口各自观测最大值百分比。帧时间须P95≤10、P99≤12且各自≤GL的110%，两项内存分别≤110%。

| 场景/轮次 | P95 GL→Vulkan | P99 GL→Vulkan | 工作集比 | 私有提交比 | 本组预算 |
| --- | --- | --- | --- | --- | --- |
| static 1 | 6.7743→9.8087 | 7.2639→20.6202 | 103.69% | 69.45% | 帧未通过，内存通过 |
| static 2 | 6.7600→15.8688 | 7.2355→37.7910 | 103.52% | 69.49% | 帧未通过，内存通过 |
| static 3 | 6.4685→8.6486 | 6.9756→9.2428 | 103.66% | 69.53% | 帧未通过，内存通过 |
| walk 1 | 6.7067→8.3170 | 7.2230→8.9043 | 104.22% | 69.61% | 帧未通过，内存通过 |
| walk 2 | 6.4952→8.4118 | 6.9507→9.0940 | 104.30% | 69.59% | 帧未通过，内存通过 |
| walk 3 | 6.4673→8.3968 | 6.9376→9.0431 | 104.29% | 69.71% | 帧未通过，内存通过 |
| edit 1 | 6.9325→8.8311 | 7.7348→9.7481 | 104.16% | 69.67% | 帧未通过，内存通过 |
| edit 2 | 6.6838→9.3568 | 7.4632→10.8878 | 104.10% | 69.70% | 帧未通过，内存通过 |
| edit 3 | 6.8399→9.0579 | 7.5520→10.2989 | 103.84% | 69.55% | 帧未通过，内存通过 |

九组P95/P99相对门槛全部超限；static1另P99绝对超限，static2另P95/P99绝对超限，其余七组同时满足绝对门槛仍不豁免相对门槛。最大单帧154.7113ms（Vulkan static2）完整保留。工作集相对GL增加3.52%–4.30%，私有提交为GL的69.45%–69.71%，两项九组分别通过；这不是显存、瞬时/全生命周期峰值或无泄漏证明。

18轮共134931个正式帧均失焦，九组皆fully-unfocused且焦点条件匹配；按已确认allow-unfocused不删帧、不改为无效，也不以焦点差异解释本次超限，不声称前台性能。采样前后记录的供电/平衡电源方案/CPU/内存/显示驱动一致；未连续采温度、频率或其他进程负载，不能仅凭分位数确定热点、驱动或外部干扰根因。短窗口不覆盖原60＋180秒协议、罕见卡顿和长期增长，本机结果不代中低端整机/M4/M5/M6。

Vulkan正式预算已独立选择并执行，状态从“未选/未测”更新为“数据有效、帧时间未通过、内存通过”，不能称性能无回归。依确认不自动复测、优化、放宽或排除static前两轮；本次未取得对新Vulkan超限的延期/豁免或R4/T3批准。D3D12 edit第3轮P99缺陷继续暂缓且原结果未改，默认D3D12、GL保留；R2技术聚合、R4验收及R4批准仍未勾选，21/24。结果/目标口径及时增量同步主目录，保留两目录既有导航及人工007/当前004的分版本证据边界，无子agent、Git写入或规格sync/archive。

## 2026-10-15 Vulkan具名退出与重启补证

用户本轮授权在refactor-renderer托管worktree补齐普通Vulkan游戏的Esc、窗口关闭与随后重启，未授权性能复测或节点批准。使用与前次GUI相同的新安装004 Release候选，从无关CWD启动，显式参数为 `--renderer vulkan --seed 424242 --msaa 4 --validation 1`，不带benchmark或smoke限帧；[三次运行与身份复核](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-vulkan-exit-001/result.json)及[操作归属摘要](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-vulkan-exit-001/ui-observations.json)绑定本轮证据。EXE SHA256为 `d19a77ba61586908ff3e6a34140892af67488091469187ff9afbafaa4f8ce2ae`，未改代码或重建，不把004与此前人工007包混为同一字节身份。

| 北京时间（2026-10-15） | 操作及随后结果 | 实际呈现帧 |
| --- | --- | --- |
| 16:41:41–16:42:37 | 普通游戏世界显示后由Computer Use发送Escape，正常退出 | 2571 |
| 16:42:53–16:46:11 | Esc后重启，同一世界正常显示；用户确认点击退出，正常关闭 | 23896 |
| 16:47:08–16:47:38 | 窗口关闭后再次重启，同一世界正常显示；Computer Use发送Escape完成清理 | 1370 |

三次均为独立进程，实际Vulkan/RTX5070Ti、UUID d43c9988ca0711129c89be697511f1d0、NVIDIA616.64、4x/Fifo、validation开启、441区块；普通窗口日志为1280×720，不冒称1920×1080性能协议。每次内层进程退出码0、loop finished和shutdown complete完整，stderr仅一条正常Vulkan关闭摘要：validation_warnings=0、validation_errors=0、native_retired=true；无超时或强制终止，前一进程结束后才启动下一进程。每次退出后重新列举窗口均无该候选残留，三次启动均观察到实际世界画面。已有319份绑定来源和83份安装文件逐次前后复算不变，含独立GL候选；旧证据未写入，主目录不复制代码或产物。

窗口关闭的方法归属保留限制：工具两次点击标题栏关闭按钮均返回但未触发关闭，Alt+Space也未观察到系统菜单，不将这些动作计为通过，也不据此确定鼠标锁定或产品故障根因。随后明确请用户切出释放鼠标并点击标题栏X、不要Esc或Alt+F4，用户回复原文“已点退出”；据该请求上下文登记为用户确认的窗口关闭，并以真实进程结束/正常退休日志核对。人工点击本身及精确点击时刻未由工具直接捕获，不把上述失败点击时间当关闭延迟或GPU等待耗时。

本轮补齐Vulkan A15的具名Esc/窗口关闭/重启证据，不补填历史人工checkbox、不要求重做已认可的15分钟或视觉对照，也不从三个短进程声称长期内存增长已验证。R2技术聚合、R4验收及R4批准仍未勾选，进度21/24；D3D12 edit第3轮P99相对回归11.55%超过10%预算继续暂缓，Vulkan预算未选/未正式验收，节点性能与风险处置仍须明确。默认D3D12、GL保留，无子agent、Git写入或规格sync/archive；本节与任务增量及时同步主目录，保留两目录既有导航差异。

## 2026-10-15 GUI实测与A18对账

用户明确授权GUI访问，并在首次激活受阻后手动置前。随后通过Computer Use真实操作安装004的SymoCraftBenchmark.exe，未改代码、重建或调整预算；[候选身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-gui-001/identity.json)、[GUI状态摘要](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-gui-001/gui-observations.json)与[离线复核](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-gui-001/result.json)共同绑定本次结果。初始OpenGL项的验证开关禁用，切D3D12/Vulkan后可用，最终切回GL再次禁用；没有在双现代GL-off包里冒称运行了GL。

D3D12 GUI quick的static/walk/edit三轮全部通过，验证默认开启；Vulkan显式取消验证勾选后的三轮全部通过，报告实际validation=false。重新勾选验证后的Vulkan预备取消会话也完整通过，因未及时触达取消入口，不把它登记为已取消。调整控制窗口避免游戏遮挡后，第二个Vulkan会话在walk轮真实取消：static已通过，walk标cancelled/内层exit4、timed_out=false、forced_termination=false，edit尚未启动；GUI显示已取消且启用重试。重试跳过static，walk创建attempt2，随后edit完成；三轮最终通过，retried=true、continuous_full_protocol=false，不把重试升级成连续正式协议。重试前锁定的17个已通过/取消attempt文件复算均未改，原取消result仍passed=false。

本次共四个真实GUI会话、12个通过attempt与1个取消attempt；12份有效capture由原生Benchmark消费者离线接受，取消capture被拒绝而非补齐为有效。D3D12及验证开启的Vulkan报告另经现有原生game-report校验；实际RTX5070Ti、D3D12 LUID 00000000:000146a5/驱动32.0.16.1664、Vulkan UUID d43c9988ca0711129c89be697511f1d0/NVIDIA616.64，1920×1080/4x/Immediate、441区块及中立协议/原frame ID保持。界面三行实际后端/设备与报告一致，运行时配置/开始/重试/导出禁用而取消可用，完成后恢复；默认开启与显式关闭验证均通过实际报告确认。机器代号gui-functional-check-5070ti，电源/显卡/频率声明保持未记录，失焦原样保留；这些quick仅核GUI/协议，不比较或重新验收性能。

D3D12、Vulkan显式validation0及取消后重试会话均从GUI成功导出，共三个ZIP；保留shader生成身份及原始结果，重试导出同时含walk attempt1取消和attempt2通过，无exe/DLL/shader混入。打开结果按钮触发具名run-20261015-062626-651Z的Explorer窗口；该外部应用内容访问授权超时，未查看其内容或输入，不把窗口标题当完整Explorer交互验证，也未绕过授权。常规文件/目录选择器、其他DPI/系统主题未在本轮穷举，不声称全GUI无缺陷。

早期下拉命中弹层的安全拒绝、控件设置超时和异步可访问树滞后按实际截图/新状态恢复，再由落盘会话复核，不将自动化读到旧状态判成产品失败；全部会话和预备取消的完整运行均保留。本轮Benchmark通过关闭按钮正常关闭，无本候选游戏窗口残留；原人工安装007现代44项/GL38项共82文件、新安装004及绑定的生产/测试来源均未变。不宣称本轮进行了普通交互Vulkan的Esc/关闭按钮两种具名退出，也不重开已确认15分钟玩法。

结合[前次A18中立报告、生成身份与本机部署证据](#2026-10-15-vulkan能力与wsi生成身份及r4材料补齐)，本轮所列GUI缺口补齐，A18技术聚合关闭，当前21/24；不是新人工checkbox或R4/T3节点批准。剩余三项为R2技术聚合、R4验收和R4批准：D3D12 edit第3轮P99缺陷仍暂缓，Vulkan预算未选/未正式验收，具名Vulkan退出操作及最终节点风险处理仍保留。默认D3D12、GL保留，无子agent/Git/sync/archive；文档及时增量同步主目录。

## 2026-10-15 GUI访问获准后的窗口阻塞

以下保留首次受阻时状态；用户手动置前后已恢复并完成上述GUI核查，不再把激活阻塞列为当前未完成项。

用户明确授权访问并进行GUI实测。本轮已通过Computer Use启动新安装004的SymoCraftBenchmark.exe，并取得候选窗口及可访问控件树（默认OpenGL、验证开关禁用、正式协议默认、尚未开始采样）；未点击开始、取消、重试或导出，不生成本轮通过结果。截图存在其他窗口遮挡，工具置前返回failed to activate captured window；按规则刷新窗口选择后仅重试一次，仍同错，停止输入并请用户手动将候选置前再继续。这是GUI自动化当前阻塞，不判定产品故障，也不绕过访问/激活限制。任务仍20/24，A18与R4保持未勾选；性能缺陷继续暂缓、GL保留、无代码/构建/正式采样或Git/sync/archive，本节同步主目录。

## 2026-10-15 Vulkan能力与WSI、生成身份及R4材料补齐

用户本轮明确授权继续这两组任务；独立在refactor-renderer托管worktree实施，范围为Vulkan私有能力/WSI诊断、DXIL/SPIR-V构建生成身份及游戏/Benchmark中立报告与测试。公开v1、能力下限、默认D3D12和GL保留不变；未启用子agent，不执行正式预算或Git/sync/archive。所列自动技术补证已完成，GUI未执行，R4/T3未批准。

**能力与交换链。** 最终[收尾007](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-finalization-007-close/result.json)中的[Release WSI](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-finalization-007-close/release-wsi/result.json)与[Debug WSI](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-finalization-007-close/debug-wsi/result.json)各六案通过：缺swapchain maintenance扩展、缺feature、Acquire OutOfDate、Acquire Suboptimal、Present Suboptimal、Present OutOfDate。前两案在真实硬件枚举/扩展及feature查询路径上私有cap-off，具名Unsupported/SelectDevice且CreateDevice次数0，不冒称本机天然缺能力。后四案使用同一公开Renderer/生产库、实际RTX5070Ti、UUID d43c9988ca0711129c89be697511f1d0、NVIDIA616.64、640×480/1x/Immediate、validation开启，各重复三轮，交换链代次1→4；恢复图像与基准相同、样本原ID完整、正常/重复Shutdown与loader退休成立，warning/error0/0。共12案，其中八个正向设备案产生32张PNG。

Acquire OutOfDate在真实acquire前注入，该帧原生Acquire/Submit/Present均0次，不留下已signal但无人消费的semaphore，提交fence仍signaled；未调用API的返回值明确为null。Suboptimal及Present OutOfDate仅在真实API成功返回后覆盖有效结果，真实与注入值分列、有效状态沿原生产重建分支执行；不称自发WSI异常或真实不支持硬件。该方法遵守[WSI成功取得图像与同步副作用的区别](https://docs.vulkan.org/spec/latest/chapters/VK_KHR_surface/wsi.html)，不是在取得图像后伪称未取得。早期002结果保留；最后审查仅修改probe/检查器/CPU反例三份测试文件，使未调用API不沿用上一次观测，007重新构建全部22个适用配置并重跑六案D/R。新增CPU检查器七个方法含反例，拒绝伪造原生返回、错误平衡/重建/像素恢复及非零诊断；GPU案不注册无人值守CTest。

**生成身份与安装。** DXIL现在与SPIR-V使用相同schema1生成清单机制，D3D12为DXIL/d3d12/SM6_0，Vulkan仍SPIR-V/vulkan1.2/SM6_0；四个二进制、同源HLSL/include、实际DXC exe和相邻dxcompiler.dll SHA256、配置及flags均记录。DXIL构建补实际编译器输入依赖，安装build.json；现代游戏报告写入所选身份，新Benchmark运行/重试绑定安装清单与实际二进制，缺失或不符具名拒绝。旧GL25列/身份不变，历史D3D12 telemetry-v1缺生成元数据仍可单独读取，不冒称旧报告已有完整生成身份；新运行不能以历史兼容绕过包身份要求。CMake脚本保留原文件名供既有Vulkan调用复用，不引运行编译器。

[矩阵与新安装004](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-finalization-004-matrix-install/result.json)完成现有22个有效配置的最终增量构建、两项全关闭预期拒绝及新安装；包括D/R两种单现代、双现代、GL-only、CPU、独立/根工具及关闭测试组合，不新建第二矩阵。逐现代配置独立复算实际编译器/module/源/二进制与配置，隔离检查适配现代四份shader＋一份生成清单。A04既有D/R各八项真实增量保持原身份，本轮新增编译器依赖及最终实际Ninja/link/安装核对补版本一致性，不声称重做了全部touch实验。新候选为004下candidate with spaces，GL另包；代码/产物仍只在E盘，不复制回主源码。

**受影响回归。** [回归003](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-finalization-003-regression/result.json)两现代×D/R×1x/4x×绘制/完整世界窗口16案、400张PNG通过；包括441区块/361mesh、固定相机与32次编辑反向恢复、至少10秒真实最小化及恢复，非人工游玩。Vulkan D/R各13个门面/游戏故障案＋5个隔离资产案、各五个异步案通过，实际在途timeout与有限分配/提交/DeviceLost/query注入继续分列；D3D12所需原生4案/游戏入口5案各D/R及各三案在途/异步由006完成。007再补D3D12 D/R各缺DXIL、坏DXIL实际驱动拒绝、坏PNG、坏YAML、无效后端五案，内层预期exit2/3/64不误称游戏成功，原包不改。最终D/R各52项定向CTest通过；独立Release工具8项、根工具Debug15项通过，含两现代生成身份错配/缺失反例及原旧GL/转发兼容。普通诊断0/0不扩为故障注入过程也无预期原生错误。

**运行协议与部署。** [运行006](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-finalization-006-runtime/result.json)已完成D/R×三后端×static/walk/edit的一秒报告及原生消费者，共36个检查；安装D3D12产品quick三轮与完成会话resume通过。007从该已完成会话的字节副本成功导出ZIP，保留DXIL生成身份且无exe/DLL/shader；原会话未改。三个新隔离包的缺清单、改DXIL和清单hash不符均在启动会话前exit64拒绝。裁剪进程SDK/开发PATH及显式层搜索后，缺Vulkan loader的D3D12/GL仍运行、选Vulkan有限失败，恢复系统loader后Vulkan显式validation0可运行；不静默关闭验证，不把本机受控环境说成卸载SDK或真正干净机。旧正式D3D12/GL报告只读被新消费者接受，无重新预算采样。

005验证命令错误地同时给benchmark和scene，被参数解析exit64拒绝；006 quick导出命令漏传--quick，被Export session plan mismatch拒绝，故006整体仍failed，成功子集不得改写为整批通过。007在新目录完成剩余检查及副本导出，保留005/006失败与导出部分目录，不复测性能或修改产品以容忍错误协议。新包、最终来源/WSI产物及PE依赖由[身份绑定008](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-finalization-008-binding/result.json)复核，游戏/执行器无vulkan-1或DXC运行导入，旧人工安装007现代44项/GL38项共82个文件字节仍一致。

**GUI与剩余对账。** Computer Use启动Benchmark的应用授权超时，随后只读窗口列表确认未启动；没有改用其他方式绕过授权，也没有GUI点击/截图/人工结果。已有GUI后端下拉、验证开关、quick/取消/重试/导出共享核心且已构建，但CLI/CPU测试不关闭GUI缺口。可在新候选SymoCraftBenchmark.exe核查后端选择及验证开关启用条件、quick与实际后端/生成身份、取消/重试/导出；这一项只需界面核查，不重新要求15分钟玩法。

| 范围 | 本轮对账结论 | 仍保留的边界 |
| --- | --- | --- |
| R3 A01–A05 | 当前公开/边界CTest、最终22配置、既有增量及当前图/安装、实际设备/能力拒绝/惰性loader证据闭合，技术聚合勾选 | cap-off与真实能力分别记账，不推到其他GPU或重做全部历史touch |
| R3 A06–A12 | D3D12已闭；本轮两现代绘制/世界/窗口回归及Vulkan受控WSI已补 | 不称所有驱动自发WSI事件自然发生 |
| R3 A13–A17 | D3D12资产/分配/真实与注入故障、正常/部分构造/重复关闭、InfoQueue及截图/异步已逐项具名覆盖，技术聚合勾选 | 既有D3D12 Esc/关闭人工按原批准身份；终止保留不称正常回收，非M5/任意故障/实际OOM保证 |
| A18 | 两现代生成身份、现代/旧GL消费者与本机受控安装已补 | GUI实际操作未执行，聚合仍未勾选；真正干净机依M6，不加作本机M3第二硬件前置 |
| A19/人工 | 保留既有用户确认的GL对照及Vulkan至少15分钟连续操作/正常退出 | 不伪造新候选人工成绩或逐项checkbox；Vulkan Esc与关闭按钮各自退出/重启未被普通日志具名区分，不重开已确认的15分钟 |
| 性能/节点 | D3D12 edit第3轮P99超限继续暂缓，Vulkan预算未选择/正式验收 | 本轮quick/一秒报告只核协议，不称性能无回归；R2技术缺口、R4验收/批准仍不勾选 |

据以上逐项证据，当前20/24；剩余四项为R2技术聚合、R3 A18、R4验收与R4批准。GUI和具名Vulkan退出操作尚待补证；预算/性能范围需在最终节点明确处理，不能由本轮技术完成自动豁免。本文与任务/机制及时增量同步主目录，不覆盖并发T4、旧人工反馈或冻结证据。

## 2026-10-15 D3D12在途寿命与异步容量补证

用户授权继续推进剩余任务，本轮独立补D3D12 A10/A17及受影响回归；代码限定托管worktree生产D3D12私有诊断与根test，不改公开v1或游戏CLI，不推进Vulkan新分支、shader身份/GUI或性能。普通游戏不安装诊断控制；实际环形队列溢出另补stderr精确frame ID，保留原有有界丢弃语义。所有gate只作用自有队列/fence，不操作整机TDR或其他进程。

最终[Release008](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-d3d12-lifetime-008-release/result.json)与[Debug009](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-d3d12-lifetime-009-debug/result.json)各3案通过，使用同一公开Renderer和真实生产D3D12后端、实际RTX5070Ti（LUID 00000000:000146a5、vendor10de/device2c05）、驱动32.0.16.1664、640×360/1x/Immediate、validation开启。门控普通Render实际返回Presented，无专用跳过Present分支；不是Vulkan实验成绩或普通人工游戏。每进程30秒上限，自有gate watchdog为1500ms；成功案均由主测试显式释放，watchdog未触发。

- 在途寿命：真实native completion分别为3<5、17<19、31<33且gate=0时，执行更新/空网格/逻辑删除，旧mesh/texture弱引用仍存活；每阶段同时增加48个mesh再删除，合计144次容量增长。注入E_OUTOFMEMORY的更新/创建保留旧接受版本，不称实际耗尽内存；释放gate、真实完成及frame slot复用后旧mesh退休，删除阶段旧texture退休。更新/清空/删除截图改变且恢复截图与基准逐字节一致，过期句柄拒绝。
- 正常与重复Shutdown后，无quarantine，成功mesh buffer创建/释放各152、接受texture创建/释放各2、live计数0；本案peak为3,407,872字节。该计数仅覆盖成功mesh buffer/接受texture的GetResourceAllocationInfo分配量，排除临时上传、帧附件、query/readback及进程/显存驻留/预算，不能据此宣称整机无泄漏。native_cleared只指后端成员清空；诊断控制仍持有device/queue/fence，随测试Scene销毁，先于Window释放。
- 异步pending：提交ID9007199254740993仍真实在途时，10次Poll无结果、无新增原生Wait或query Map；完成后恰好一次返回原uint64 ID，再Poll无重复。
- 异步容量：301个间隔3的大整数原始ID、不提前Poll，最后返回256条有效样本，stderr具名丢弃45条；两集合互斥、无重复且完整覆盖301个提交ID，不依返回顺序归属、不补0。逐帧等待仅在测试harness，不能据此声称生产Poll阻塞或正式性能通过。

六案均实际执行InfoQueue检查，warning/error/corruption为0/0/0，正常退休。寿命/pending/overflow单案检查数分别204/22/612。新增证据检查器7个unittest方法含反例，拒绝假在途、提前退休、watchdog代为完成、等待式Poll、失真大整数ID、遗漏丢弃或非零诊断，GPU案不挂入无人值守CTest。

[回归004](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-d3d12-lifetime-004-regression/result.json)完成受影响双现代D/R构建、各52项定向CTest及D3D12原生4案/游戏入口5案各D/R，共18个旧故障/正常案例；受控原生移除、实际在途timeout和注入Signal/OOM仍分别解释，不把终止保留称正常退休。随后[绘制Release005](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-d3d12-lifetime-005-draw-release/result.json)与[Debug006](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-d3d12-lifetime-006-draw-debug/result.json)各4案通过，D3D12生产绘制/完整世界×1x/4x，共8案200张PNG：真实atlas/方向/层/alpha-depth/选择框、441区块/361mesh、七相机位姿、32次编辑及反向恢复、只上传改变网格、四次实际resize、至少10秒真实最小化零draw/upload及画面/输入恢复、FIFO/Immediate与32x拒绝。输入恢复采用本进程真实焦点后的原生合成事件，不冒充新增人工游玩。

收尾代码审查补测试gate的watchdog线程创建异常时先signal再抛出，避免测试自身留下不可释放的队列等待；没有发生该运行故障。初始002/003结果保留，不作为该最终源版本证明。[最终构建007](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-d3d12-lifetime-007-final/result.json)只重编该probe，重跑008/009和D/R各52项定向CTest（最终104次均通过）；005/006之后唯一相关源变化为此新probe，实际draw-window EXE、生产库和普通游戏字节不变。[身份与回归010](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-d3d12-lifetime-010-close/result.json)复核最终来源/产物及该差异，双现代再构建无新增编译/链接，另Vulkan D/R×1x/4x四案smoke和GL-only D/R三帧启动通过；原安装007现代44项/GL38项文件字节全部未改。

本次逐项关闭的是R3下D3D12 A06–A12技术聚合，而非只凭新局部夹具：A06/A07由上述D/R八案绘制/完整世界承担；A08由真实编辑/空网格/恢复及只变更上传承担；A09结合当前公开门面/CPU自有化、非法长度/索引/句柄等定向CTest及原生图集层拒绝/混层语义和既有输入复制证据；A10由新真实在途/容量/失败回滚/最终退休证据补齐；A11/A12由真实窗口/最小化/模式证据承担。既有D3D12人工确认保持原候选身份，不宣称用户核查了本轮重建游戏；不由D3D12关闭Vulkan Acquire OutOfDate/Suboptimal范围。

当前进度18/24，剩余6项为R2聚合技术缺口、R3 A01–A05/A13–A17/A18聚合、R4验收和批准。A17本机D3D12 pending/溢出缺口已补，但含资源/故障/构造关闭/工具等的A13–A17整组仍待最终覆盖对账，不因样本通过批量勾选；最终适用构建组合、Vulkan能力拒绝/WSI分支、D3D12运行shader生成身份、产品GUI/具名退出操作仍按各自范围保留。D3D12 edit第3轮P99性能缺陷继续暂缓，Vulkan预算未验收；本轮时长/分配计数不是性能预算对照。默认D3D12、GL保留，无新人工checkbox或R4/T3批准，无子agent/Git/sync/archive；文档增量同步主目录，保留并发T4和旧证据。

## 2026-10-14 Vulkan人工确认与R4剩余对账

用户明确确认d464899b这轮包含至少15分钟不中途重启的实际操作、未发现异常；结合此前人工视觉对照无异常及正常退出反馈，现接受安装007候选的本次正式人工视觉与连续15分钟玩法核查结果。视觉按[已确认口径](decisions.md#2026-10-14-vulkan正式人工视觉口径)收口：同批GL人工对照、透明深度差异单列、不设像素数值阈值。不是继承D3D12成绩，不再把本轮视觉/连续15分钟作为未完成项，也不要求再玩一轮。

版本和实际运行关联沿[补测日志核对](#2026-10-14-vulkan补测日志核对)：Vulkan Release安装007，EXE SHA256 db3d11f7a69e6c3ff312b0ca07b4185df9f04c1e9e09ab6bb8b503a6c4a16161，实际RTX5070Ti/UUID d43c9988ca0711129c89be697511f1d0、NVIDIA616.64、4x/Fifo/validation1，441区块/seed424242/regression/four-chunk；日志922891帧、validation warning/error0/0、native_retired=true及内外exit0。18:11:38至19:43:35是文件记录的运行跨度，至少15分钟实际连续操作由用户确认；不宣称整段92分钟均在操作、M5/ST-60通过或无资源增长。未提供逐项输入回放、两种退出方式各自的记录及不可达边界清单，不代填14个人工checkbox；M2-14隔离故障仍按第3/4批自动证据单列。

以下是收到本次人工确认后的最新A01–A19对账；旧表保留点时状态。这里只读取既有记录及已核对日志，没有重新运行构建/GPU/采样或修改生产代码。

| 范围 | 当前可用依据 | 准确剩余范围 |
| --- | --- | --- |
| A01–A04 公开/构建/增量 | 第1批单/双现代D/R、CPU/工具/关闭测试及增量；后续各批受影响构建与定向回归 | 不再记为没有矩阵。第1批全22个有效配置不是最终第4批源码的全量重跑，最终收口需按实际改动核对适用组合证据，不自动要求重复全部配置 |
| A05 设备/不可用路径 | 本机真实身份、未编入拒绝、32x不支持、缺验证层，以及未选Vulkan时缺loader仍可用D3D12/GL | 缺swapchainMaintenance1扩展/feature的设备选择拒绝尚无专项运行证据；不能用支持该能力的本机正向运行补格，也不因此要求另一GPU |
| A06–A09 画面/世界/编辑/输入 | 第2批独立像素/441区块/相机/边界编辑/空网格/非法输入，加本次用户GL人工对照无异常 | 本次人工视觉已收口，不追加数值像素容差或固定七视点矩阵；不据画面证明GPU寿命或所有原生输入排列 |
| A10 在途/容量 | 第3批Vulkan实际gate未完成、旧mesh/texture保留、新版本更新及完成后回收、144次容量增长和分配回滚 | Vulkan所列本机范围有实证。D3D12既有正常在途mesh更新/删除/容量与最终退休缺口仍保留，不能用Vulkan通过关闭双现代聚合条件 |
| A11–A12 窗口/模式 | 第2批真实连续尺寸变化、最小化至少10秒/零提交与恢复；1x/4x、FIFO/Immediate与32x拒绝 | 实际Present OutOfDate已观测；Acquire OutOfDate和Suboptimal仍无专项运行覆盖，既有未测分支需补证或明确风险处理，不把它们说成已失败 |
| A13–A14 资源/分配/故障 | 第3批真实生产库/游戏入口、隔离坏资产、有限分配/提交/DeviceLost注入、真实在途timeout；第4批query故障 | 已批准受控路线的所列案有证据。保留注入与真实返回边界，不要求制造整机TDR/真实OOM或自发设备事故，不将quarantine说成正常回收 |
| A15–A16 关闭/诊断 | 第3批正常/部分构造/重复关闭/失败聚合；本次真实游戏正常退出、Vulkan诊断0/0和native退休 | 普通日志不区分Esc还是窗口关闭，用户反馈未分别具名两种退出/重启操作；长期重复启动/资源增长未由92分钟运行跨度证明 |
| A17 异步/截图 | 第2批内容/方向，第4批真实未就绪10次非等待Poll、256返回/45具名丢弃及缺失/故障分类 | Vulkan所列范围已有补证；D3D12旧原生队列溢出等保留范围不能由另一API补格，未知/缺失GPU值不补0 |
| A18 产品/生成身份/部署 | 第4批真实产品Vulkan quick/导出、旧GL/现代协议、SPIR-V生成清单、本机受控裁剪依赖及安装运行 | 原生Benchmark新GUI控件尚未人工核查；Vulkan运行生成身份已闭，D3D12运行报告仍未自带同等完整生成身份，仅包外关联，不混称双现代均闭合 |
| A19 本机人工 | 本次用户独立确认Vulkan人工视觉/至少15分钟连续实际操作无异常，日志正常退出；D3D12原批准/反馈另存 | 本轮视觉和连续15分钟条件已完成；逐项未提供的操作细节不伪造，不推广到其他设备、性能、ST-60或R4整体验收 |

剩余事项分三类：一是Vulkan能力拒绝/未覆盖WSI分支、产品GUI与退出方式的限定补证；二是最终源版本的适用构建证据和双现代整合时D3D12仍保留的A10/A17/A18范围；三是节点决定及独立性能范围。上述未测不等于发现新生产故障，也不是本轮自动开工清单；若要以延期/风险接受方式收口，须用户明确决定，不由人工无异常推定。D3D12 edit第3轮P99缺陷继续暂缓、不优化/复测/放宽；Vulkan预算尚未选择/正式验收，不将quick/人工反馈称性能无回归，本轮不采集。

按[当前M3范围](../../../../scope/project-scope.md#m3-本机-rtx-5070-ti-验收与下一玩法阶段性能门槛)，真正无SDK干净机/第二机器、其他GPU、M5长期及下一玩法前中低端整机性能不冒称已验证，也不新增为本机M3的第二硬件前置；M6和下一玩法原门槛独立保留。GL退役条件未触发，不删GL。R4-验收/R4-批准及旧聚合技术项仍未勾选，进度17/24；这是人工结果登记与证据对账，不是R4/T3节点批准、默认切换、Git或sync/archive授权。本文与任务增量同步主目录，保留并发T4及原始证据字节。

## 2026-10-14 Vulkan补测日志核对

用户先报告人工视觉和15分钟玩法无异常，随后说明中途重启并决定补测。首轮四组日志已核对：两次Vulkan文件创建至exit.txt写入区间约36.445秒、137.623秒，两次GL约17.264秒、574.751秒，均exit0；不拼接会话或借GL时长关闭Vulkan连续15分钟。原始日志未改，首轮核对摘要留worktree out/m3-t3/r4-manual-audit-001/result.json。

补测新增两次Vulkan：240ce0fe案约8.112秒正常退出；最新[d464899b stdout](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-manual-vulkan-d464899b-7f7c-4dee-a505-2a6196d33dc2/stdout.log)从2026-10-14 18:11:38.182至19:43:35.350（北京时间、文件创建/退出记录区间）约5517.167秒，即1小时31分57秒，记录922891个渲染帧，loop finished及shutdown complete exit_code=0，[外层退出记录](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-manual-vulkan-d464899b-7f7c-4dee-a505-2a6196d33dc2/exit.txt)亦0；只读进程查询时无SymoCraft.exe仍运行。实际Vulkan/RTX5070Ti、UUID d43c9988ca0711129c89be697511f1d0、NVIDIA616.64、4x/Fifo/validation1，seed424242/regression/four-chunk、441区块、初始摘要一致、test_edits_applied=false；日志含选材/最终位置变化，不是benchmark或自动test-edits。

最新[stderr](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-manual-vulkan-d464899b-7f7c-4dee-a505-2a6196d33dc2/stderr.log)仅正常关闭计数：validation_warnings=0、validation_errors=0、native_retired=true。当前安装007候选EXE SHA256仍为db3d11f7a69e6c3ff312b0ca07b4185df9f04c1e9e09ab6bb8b503a6c4a16161，未重建/改包。日志运行跨度不等于全程人工操作或前台时长，无法单独证明14项逐项行为、Esc与关闭按钮分别退出或ST-60；核对当时连续实际操作反馈尚待用户明确，随后用户已确认至少15分钟不中途重启、无异常，见[最新人工结果与剩余对账](#2026-10-14-vulkan人工确认与r4剩余对账)。不补造输入回放、性能/内存曲线或逐项勾选。性能缺陷继续暂缓，GL保留、默认D3D12、R4仍未批准；本节增量同步主目录，不新增测试/采样、Git或sync/archive。

## 2026-10-14 Vulkan正式视觉与玩法人工核查准备

以下保留收到结果前的准备时状态；本次已确认结果见[最新人工对账](#2026-10-14-vulkan人工确认与r4剩余对账)。

用户要求提供PowerShell启动代码和人工清单；使用第4批安装007的Release候选，而非主目录IDE产物或随后改变的工作目录源码重建。Vulkan为`candidate with spaces/SymoCraft.exe`（SHA256 db3d11f7a69e6c3ff312b0ca07b4185df9f04c1e9e09ab6bb8b503a6c4a16161），同批GL-only对照为`gl candidate/SymoCraft.exe`（SHA256 e1e66d95ce2babe4624883ee2a853001fe73c496085503ff0450c68a7d6fc267）。本轮只读复核两包44/38项文件均与007清单相符，公共YAML/PNG一致；没有重新构建、启动GPU、性能采样或改写原包。

启动采用普通交互入口、seed424242/regression相同检查点，Vulkan显式4x/validation1；GL沿自身4x配置、不传现代参数。不得混入benchmark、smoke-frames或test-edits冒充人工；按共享M2-01–M2-14观察，Vulkan进入可操作世界后至少15分钟连续操作，并分别核对Esc/窗口关闭及重启。边界不可达或不安全的项目标未执行；M2-14既有隔离故障自动证据只作引用，不指示用户修改原包或冒称人工执行。普通交互的窗口/VSync策略不冒称quick的1920×1080/Immediate配置。

用户随后明确确认Vulkan沿用同版本GL人工画面对照、已批准透明深度差异单列、不设像素数值阈值的正式口径，见[视觉口径决定](decisions.md#2026-10-14-vulkan正式人工视觉口径)。这是核查方法确认，不是本次画面/15分钟玩法通过反馈；尚未收到实际人工结果，不记视觉/A19通过。核查清单与启动代码在本次聊天交付，日志使用新out目录，不新增平行报告/附件。性能缺陷继续暂缓、GL保留、默认不变、无子agent/Git/sync/archive或R4整体验收批准；本节同步主目录供审核。

## 2026-10-14 R4第4批异步样本、协议与部署验证

用户批准在refactor-renderer托管worktree独立执行A17异步/缺失/容量及A18产品协议与部署；本批所列本机实测范围完成，不是R4/T3验收批准。源码/产物留E盘，相关文档增量同步主目录供审核；不覆盖并发T4，不启用子agent、不Git/sync/archive。默认D3D12、GL保留，D3D12 edit第3轮P99缺陷继续暂缓，未做正式预算比较或选定Vulkan预算。

[A17 Release004](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-004-release/result.json)与[Debug005](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-005-debug/result.json)各五案通过，同一公开门面/真实生产Vulkan库，实际RTX5070Ti/UUID d43c9988ca0711129c89be697511f1d0、NVIDIA616.64、640×480/1x/Immediate、validation开启。真实门控提交未完成时连续10次Poll均无结果、无新增Wait/Query，fence为VK_NOT_READY、timeline为0；释放后恰好返回原frame ID2，重复Poll无重复样本。正常门控的私有Present挂起边界沿第3批，不称正常WSI。另301个提交ID中返回256条、stderr明确丢弃45条，集合互斥且完整覆盖原ID，不能依返回顺序归属。真实最小化原生零extent返回Discarded/无耗时，恢复截图像素与基准一致；Unsupported由私有cap-off明确注入，本机实际支持时间戳，不冒称不支持硬件。查询实际成功后注入DeviceLost/-4，首错仍ReadTimestampQuery，Failed入口不续交、重复Shutdown不成功、完整owner/loader终止保留；这是原生错误路径，不冒称自发设备移除或实际返回DeviceFailure样本。后者的标志组合仍由CPU契约/telemetry回归承担。

十个普通观测案warning/error均0/0；正常案原生资源释放归零，终止案保留不作正常回收。[补充API观测006](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-006-trace/result.json)五案通过，pending区间实际只有10次vkGetFenceStatus，无Wait/Submit/Acquire/Present/GetQueryPoolResults；每案2条确切VK_INSTANCE_LAYERS环境warning原样保留、error0、无其他诊断，不称0 warning。截图专项及普通生产内容证明仍引用第2批，不用这一合成门控替代全游戏视觉验收。

产品执行器现接受Vulkan，仍默认GL，无回退；旧GL25列、现代32列/v1与精确uint64 ID、缺失空值、上传分列保持。新增显式validation覆盖，默认开启不变，实际请求/结果不符拒绝；GUI下拉/复选框编译并共享同一核心，GUI人工操作未测。构建/安装新增SPIR-V生成清单，含实际DXC exe及dxcompiler模块、同源HLSL/include/二进制SHA256、配置/目标/参数；Vulkan Session记录清单，新轮次和resume绑定实际安装身份，错误版本/后端/生成身份具名拒绝。D/R清单已对源文件、实际编译器和四份SPIR-V独立复算，Release安装副本一致；不以CPU夹具的占位编译器哈希冒称真实生成。

[安装007](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-007-install/result.json)重建四个D/R现代/GL-only游戏配置，将Release双现代游戏与独立原生执行器安装到含空格的新候选目录，GL-only另包保留，不将双现代GL-off候选说成三后端包。[产品quick008](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-008-quick/result.json)从无关CWD运行安装后的原生执行器：static/walk/edit各10秒预热＋10秒采样，共60秒协议、实际进程65.141秒，三轮被产品核心及独立消费者接受，ZIP导出成功。实际1920×1080/4x/Immediate、validation显式false，记录生成身份、实际设备/驱动/API与新列，最终PNG标明测量后额外render/readback；不删除失焦/慢帧，不将quick或1秒诊断称正式基线/性能无回归。

[运行010](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-010-runtime/result.json)已执行的26个进程检查均符合预期：Release Vulkan/D3D12/GL各static/walk/edit一秒报告及原生消费者18项；空显式层目录下要求validation开启具名Unsupported/exit3；本进程SDL_VULKAN_LIBRARY指向不存在路径时，Vulkan启动exit3，而D3D12/GL有限帧、Vulkan无GPU资产预检仍exit0，非法参数exit64；两份新隔离复制包分别缺生成清单/改SPIR-V被产品执行器exit64拒绝，原包未改。裁剪PATH、移除本进程SDK环境并覆盖显式层搜索，不修改注册表/系统DLL；实际自有Vulkan进程观察到System32/vulkan-1.dll和app-local CRT，无SDK/DXC模块。安装PE也无Vulkan/DXC导入，包无运行编译器/loader副本；许可证目录两份既有source.h按通知材料保留。这是本机受控依赖裁剪，不是物理卸载SDK或真正干净机验证。

[收尾011](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-011-close/result.json)另补Debug Vulkan三场景一秒采样，全部原生消费者接受、validation warning/error0/0并正常退休；核对实际导出位于会话同级目录，三份summary保留生成身份且ZIP无exe/DLL/shader，源报告未改。双现代D/R各50项定向CTest通过（含新增异步证据校验器六项反例）；独立执行器D/R各8项CTest通过（含新增Vulkan、旧GL/D3D12及转发兼容），都不等于GPU玩法/正式性能通过。[最终身份012](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-012-binding/result.json)复核178项来源、实际探针/游戏/执行器与安装包未变，011的四个最终构建无新增编译/链接/生成shader。009检查器误把许可证source.h当开发依赖，010收尾误在会话内部找ZIP，011末尾身份读取漏bin路径，三个原始失败/部分成功记录完整保留；012另行完成绑定，不改写旧状态或把检查器错误说成生产失败。

文档专项：两根目录的m3-t3-renderer严格validate通过、apply仍17/24；本批正文及机制/任务按新增段落同步，保留换行及既有跨根源码链接差异。全库文档审计未全绿：工作目录53项、主目录14项缺链/历史迁移注册问题，涉及旧归档、工作目录未复制的旧out及主目录其他进行中模块；未改写这些历史/旁支材料，本批新引用另行核对。013收尾误要求整文件字节一致，014又错误比较含既有跨根链接差异的整节，失败原样保留；[最终文档核对015](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-async-protocol-015-docs/result.json)限定本批正文/新增段落/引用/源身份，不把全库未通过改称通过。

A17本机异步/缺失/容量及A18产品读取/生成身份/受控部署已有上述补证，仍不勾选含其他条件的聚合任务，17/24不变。真正无开发环境机器、GUI人工、正式Vulkan视觉参考/容差与14项玩法＋15分钟连续操作、未自然覆盖的Acquire OutOfDate/Suboptimal、其他设备/任意故障组合及Vulkan预算均未因此通过。性能缺陷继续暂缓，GL不删、默认不改，不自动执行下一批或申请整体验收。

## 2026-10-14 R4第3批在途资源寿命与故障验证

用户批准在refactor-renderer托管worktree独立执行本批，代码范围为生产Vulkan私有诊断绑定和根test验证入口，沿用既有A10/A13/A14/A15目标；不增加公开v1或游戏CLI，不启用子agent、整机TDR、性能采样或Git/sync/archive。默认D3D12、GL保留、性能缺陷暂缓。源码/产物只留E盘，本文及受影响机制/任务增量同步主目录供审核，保留并发T4内容。

**本批所列实测范围完成：最终[Release 018](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-lifetime-fault-018-release/result.json)与[Debug 019](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-lifetime-fault-019-debug/result.json)各13个生产寿命/故障案＋5个隔离坏资产案，合计36案通过。** 13案为8个公开门面/生产后端案及5个同一Application::RunGame入口案，不是普通交互/人工玩法；后者成功exit0、故障内部exit3，外层probe确认分类后exit0。成功设备身份为实际RTX5070Ti、UUID d43c9988ca0711129c89be697511f1d0、NVIDIA616.64，门面案640×480/1x/Immediate、游戏案1280×720/1x/FIFO，不借D3D12或E02成绩。部分初始化失败不冒称已完成全设备Info输出。[最终022核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-lifetime-fault-022-close/result.json)对两组各194项来源/库/资产/实际exe/cache/Ninja身份复算一致（各含167项来源），受影响回归exe未变，后续构建无编译/链接。

A10的真实在途证明来自同一公开门面及生产资源/绘制提交，不是另建实验registry。私有诊断创建线程绑定只消费一个真实后端；仅需要门控的隔离设备额外查询/启用timelineSemaphore，普通生产能力下限/设备feature链不变。门控期间旧frame的提交fence在更新/清空/删除前后均VK_NOT_READY、timeline计数均0；旧mesh/texture弱引用存活，更新使用不同原生版本。三步各48个资源增长/销毁（共144个Create）不破坏旧版本，host signal后实际fence成功，随后三帧slot复用才释放旧mesh；删除texture也只在完成后回收，过期句柄拒绝。每个D/R寿命案最终165个已分配buffer、4个image全部释放，native资源分配峰值6,588,912字节、归零；该计数含本案附件/帧局部资源，不是进程内存/显存预算。更新/清空/恢复的实际PNG保留，恢复像素等于原图；画面只作内容补证，不承担在途证明。

**门控不是正常呈现路径证据。** 未满足的host timeline依赖不能直接交给WSI Present；只在私有诊断分支，同一生产绘制已真实提交后明确返回Skipped/Occluded，保留逐帧资源及取得的图像，正常门控时host释放/提交完成后才由私有挂起点完成唯一Present；故障终止保留后不补Present，所取得图像仍随完整owner保留，不伪称Presented、不在public调用中重入，也不让后台线程调用Renderer。所有公有API仍在创建线程。纹理超时不涉及Present挂起。300ms原生等待、900ms门控自动释放保护及每进程30秒上限分别留证，最终门控保护均未触发；这些是故障有界性，不是性能预算。普通未绑定路径由下述独立smoke复核。

A13：可恢复Update的host bad_alloc、原生分配返回值OOM两类有限注入，均保留旧接受资源和逐字节原画面；失败Create不泄漏半成品，后续Create/Update成功重试。部分创建时注入pipeline OOM，原生码-2经既有Rebuild归为InitializationFailure，完整清理，不改成可继续Ready；这是结果注入/展开验证，不是真实驱动PSO失败或内存耗尽。隔离复制游戏/资产，在各自新目录缺SPIR-V、坏SPIR-V头、坏PNG、坏YAML及非法renderer的实际入口分别exit2/3/3/3/64，资源/后端具名、有限清理，原exe/资产SHA256未变；CPU坏头拒绝不冒充驱动执行坏shader。

A14/A15：真实纹理copy与帧提交门控后WaitForFences返回VK_TIMEOUT/2，fence仍未完成、timeline仍0；门面保留WaitSubmit首错，四类Failed入口拒绝续交，重复显式Shutdown仍失败。有限submit结果-13是在原生QueueSubmit之前注入；DeviceLost/-4在实际Present返回VK_SUCCESS后注入，真实返回与注入分别记录，不称原生设备移除或自发事故。故障后完整native owner、在途版本和实际loader终止保留，不续交/假恢复；解除门控并确认自有工作完成后也不改判正常回收。游戏入口正常、host分配、提交、DeviceLost和真实纹理超时五案各自处理退出；故障路径执行聚合清理并可重复Free/重建平台窗口，清理失败单列，不覆盖已输出首错。三个终止保留游戏案在Window::Destroy/Free之后，用不增加模块引用的查询核实同一实际loader仍映射；原生owner没有被提前销毁。

[Release补充API观测020](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-lifetime-fault-020-trace/result.json)五案独立核对实际调用：寿命案165次CreateBuffer/DestroyBuffer、169次AllocateMemory/FreeMemory配对；分配失败案12次CreateBuffer/DestroyBuffer（含未接受候选）和14次实际AllocateMemory/FreeMemory配对；pipeline部分创建案2个真实shader module均销毁。以上正常/部分构造案instance/device/surface各创建销毁一次。真实游戏纹理超时案观察到WaitForFences实际VK_TIMEOUT、后续host signal及自有提交完成，但无DestroyDevice/Surface/Instance、DestroyBuffer/FreeMemory调用，符合完整终止保留而非正常关闭。另一个frame-timeout原生观测案也证明owner未销毁，实际Present总数仅原始截图的一次，门控释放后不追加Present；提交/呈现计数断言覆盖解除gate之后。附加API dump的五案各有2条确切VK_INSTANCE_LAYERS环境warning，原样保留并分类，错误0、无其他validation信息，不标0 warning；018/019全部26个正常观测配置的报告则warning/error均0/0（终止保留案由仍存活回调计数，非销毁日志）。

[021受影响回归](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-lifetime-fault-021-regression/result.json)：未绑定诊断/未开启额外timeline feature的生产Vulkan D/R各1x、4x smoke四案通过，各23个实际frame ID/GPU样本、实际纹理/空更新/尺寸/VSync/截图及正常退休0/0；沿用原smoke的截图等待，不扩为A17全异步通过。既有D3D12正常/分配/真实在途超时游戏入口D/R共6案、GL-only D/R完整441区块三帧共2案通过。021最终又执行D/R各18个定向CTest（门面、三公开头、app缓存/转换、资产与边界、新旧证据检查器）共36次全通过、0失败；新校验器8项含反例检查。未重跑22配置全矩阵或全量CTest，不把既有D3D12 A10缺口由Vulkan结果关闭。

失败现场不改写：002的host event门控被validation按待执行event不能host设置的规则拒绝；004将未满足host timeline依赖带入WSI Present无效并触发诊断/30秒外层终止；均是测试门控设计错误，不列生产寿命成绩。006部分构造案误期望AllocationFailure，被既有Rebuild的InitializationFailure拒绝，改正夹具预期后复核。收尾源码审查发现013/014的私有诊断frame-timeout解除gate可能补Present，虽不在普通未绑定路径，仍违背故障后不续交；现禁止终止owner调用该挂起点、gate清理只释放并确认自有提交完成，018/019及020/021/022已全部重跑核对。007/009/010/013/014/015/016的修正前成功不代替最终来源身份。旧文件、失败/冻结材料和历史Win32 1175状态保留，未向旧证据目录追加或改结果。

**A10本机1x受控在途、A13限定失败、A14真实等待与有限注入、A15对应清理已有上述新证据；不等于A06–A12/A13–A17整组或R4验收完成，仍17/24。** 自发DeviceLost/真实系统OOM、真实驱动pipeline失败、长期资源增长/任意容量与故障排列未测，不据有限试验作保证；正式Vulkan视觉/人工、未自然观测WSI分支、A17异步缺失/压力、A18产品工具/部署及A19仍按缺口保留。性能缺陷不追查/优化/复测，未采Vulkan正式预算，GL不删、不切默认、不自动执行下一批。

## 2026-10-14 R4第2批生产绘制与窗口验证

用户批准独立执行第2批，范围为真实生产绘制、完整世界/相机/编辑、窗口重建与受影响回归；不恢复子agent、故障注入或性能采集，不删GL、不改默认、不Git/sync/archive。新增root test探针链接现有app_runtime、GameRenderer、公开门面和生产后端，不重编生产源作为替身或增加公开接口。以下均为RTX5070Ti、同源HEAD加未提交改动的本机结果，不继承E01–E03实验成绩。

**第2批已完成所列实测范围：最终D/R各8个原生绘制/窗口案、4个生产游戏入口窗口案、2个补充Vulkan观测案及6个有限帧启动回归均通过。** [Release 035](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-draw-window-035-release/result.json)与[Debug 036](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-draw-window-036-debug/result.json)各覆盖Vulkan/D3D12 × drawing/world-window × 1x/4x，共400张实际GPU读回PNG。四个受影响构建已重建；[最终040核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-draw-window-040-final/result.json)确认后续无编译/链接、二进制未变，035/036/037各164项来源身份未变。038/040另绑定源码、库、实际exe、cache及Ninja身份，产物和原始日志仅留worktree out，不增文档附件。

- A06/A09/A12专项：透明alpha0及1/255的两种顺序均不写背景深度，opaque顺序无关；反面全剔除、空更新/重传、非对称重复UV/方向、Linear/SRGB和纹理层、selection可见/被遮挡、混层三角形接受均通过实际像素断言。32x采样在本机具名Unsupported、无降级；FIFO/Immediate实际Info吻合。非法层在原生调用前拒绝，同frame ID纠正后可提交。像素断言只用于合成材质专项，不是用户批准的全游戏Vulkan视觉容差。
- A07/A08专项：每案真实441区块/361已接受mesh、初次上传57,770,328字节；七个固定视点、生产Camera移动/转向/缩放，以及32次前后向编辑均通过。每次重建/接受/上传数与改变的发布revision相等；静止/相机变化无几何上传。反向编辑后world摘要及七视点RGBA逐字节恢复。未把直接Camera setter或合成W键称为真实玩家走路/物理玩法。
- A11窗口：960×540、800×600、1280×720、1920×1080连续实际客户区/SDL像素/截图一致；真实Win32最小化10.0075–10.0556秒，每案94次有限间隔检查，均Skipped/ZeroExtent、零draw/upload、无截图，恢复同视点像素一致。真实前台HWND＋SDL获焦后，自有窗口合成W按下/释放经过Window快照，相机画面变化且无粘键；不替代人工鼠标、Esc或焦点玩法。
- [Release Vulkan 4x补充原生观测037](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-draw-window-037-trace/result.json)：实际Present返回OutOfDate一次并恢复；未自然观测到Acquire OutOfDate或Suboptimal，不标该分支通过。world案61 acquire/61 present、62 submit（含纹理上传）、6代swapchain创建/销毁，instance/device/surface各创建销毁一次；最小化区间acquire/submit/present/createSwapchain全0。非法帧区间无任何原生调用；待稳定Resize只查询surface、不提交/重建/销毁旧链，最后接受Info保持原值。未使用设备丢失或返回值注入。
- A16诊断：035/036共8个不附加观测层的Vulkan案均warning/error 0/0且native_retired=true；D3D12实际启用debug/InfoQueue检查，正常关闭未发现warning/error/corruption，不冒充Vulkan计数。037 drawing的两个instance共4条、world的单instance共2条确切VK_INSTANCE_LAYERS环境warning均原样保留，仅分类为观测提示；错误0，无其他诊断，不称0 warning。

**本批修复两处真实生产窗口竞态，不是性能优化。** 010普通Vulkan游戏恢复时SDL缓存像素与surface当前尺寸短暂不同步，原实现InvalidState/exit3；011中间修复又误入Unsupported尺寸检查，均保留失败。最终私有Rebuild在任何退休前发出精确surface_extent_pending诊断，保留已接受Info、旧native代及引用；GameRenderer只识别该诊断，单次WaitEvents(.01)后应用层跳过本帧、后续poll再试，不把请求尺寸伪装为实际Info。Render二次surface查询竞态同样Occluded/Discarded，无旧尺寸提交、无catch-all错误吞并。015 D3D12普通游戏亦复现原生HWND在SDL poll后变化导致InvalidState；现在仅物理尺寸漂移在提交前Skipped/Occluded并记录Discarded，缺swapchain或内部extent不一致仍报错。公开v1、真实故障终止与有限退休语义不变。

[034生产入口Release Vulkan](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-draw-window-034-game-release-vulkan/result.json)及同前缀Release D3D12、Debug Vulkan/D3D12四案最终均exit0、WM_CLOSE正常关闭、未强制终止。真实客户区1280×720变为1262×673（请求的是外框1280×720，不能冒称客户区1280×720），最小化10.031–10.047秒；进程CPU累计增量0–0.03125秒只是非忙循环观察，不是性能预算。两个Vulkan案暂停区间四类原生工作全0，恢复实际present，3代交换链完整退休、owner未热重建；各2条确切观测环境warning/0 error。这里显式使用既有10000有限帧诊断入口，允许失焦、不创建PerformanceSession，最小化暂停分支与生产相同；初始未在前台，故不算普通交互焦点验证。026/030原普通交互入口因系统未给予焦点而在首present前按设计暂停，超时/自有进程终止均保留，不算渲染故障或人工通过。

[038受影响回归](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-draw-window-038-close/result.json)：双现代Debug 67项CTest通过＋1项既有world.allocations禁用，Release 68项通过，0失败；包含三公开头、独立消费者/门面、app缓存、资产与边界。最新新检查器8项、既有辅助37项单测另在040通过。Debug/Release × Vulkan/D3D12/GL六个实际SymoCraft入口均完整441区块、三帧正常退出；Vulkan0/0诊断，GL-only构建/运行保留。未重新宣称第1批22配置全量已按本次代码重跑。

早期测试失败均保留：002误把MSAA细线当全覆盖色，改为离散覆盖混色；003普通Win32滚轮不保证经过SDL原始鼠标路径，改为上述受限键盘验证；004最初将D3D12尺寸突变视为夹具前置问题，015生产入口复现后才确认实际竞态并修复。009误把外框当客户区超时；021 D3D12 Debug4x已完成画面/窗口项后，SDL旧焦点值与真实前台不一致，最终夹具等待两者一致。005/006/019/020等旧版本成功不代替035/036最终源码证明。039收尾检查误要求字面no-work，实际成功构建只同步资产、无编译/链接；原running标记/字节保留，039未完成余项，不列通过，040以新目录完成正确检查。旧失败/冻结证据及Win32 1175等历史状态不改写。

**A06–A08绘制/世界专项及A11真实暂停/恢复已补本批证据，但不等于A06–A12整组或R4验收完成，仍17/24。** 正式Vulkan对GL画面对照/人工口径、Acquire OutOfDate/Suboptimal、真实交互/鼠标/Esc、故障/在途寿命、异步缺失/压力、产品Benchmark与部署及A19人工等仍分别保留；本批WM_CLOSE和像素恢复不能关闭这些缺口。D3D12 edit第3轮P99缺陷继续暂缓，Vulkan正式预算未选择/验收；未采样、优化、放宽阈值或新增通过结论。本文及相关机制/任务及时增量同步主目录供审核，不复制源码/产物、不覆盖并发T4工作，不自动执行下一批。

## 2026-10-14 R4第1批构建与增量验证

用户批准独立执行第1批；本轮仅修改构建/测试入口及发现的工具链能力检查问题，未改公开v1、游戏默认或运行时，不启用子agent、GPU/故障、人工验收、性能采样或Git/sync/archive。代码仍在refactor-renderer托管worktree，同源HEAD加未提交改动；主目录只同步本节、相关任务状态和工程机制，保留其并发T4内容。D3D12 edit第3轮P99缺陷继续暂缓，GL保留。

**第1批完成：24个配置结果成立，其中22个有效构建、2个预期全关闭拒绝。** Debug游戏证据来自002，独立/关闭测试及拒绝来自003，Release游戏来自004；002整体曾中止，不冒称其整轮完成。14个开启测试配置合计691次CTest通过、0失败，另5次world.allocations禁用（4个Debug游戏＋Debug CPU-only）不列通过；以下按配置列出，不是独立用例数量。

| 配置 | Debug | Release |
| --- | --- | --- |
| D3D12-only / Vulkan-only / 双现代，GL关闭 | 各66通过＋1禁用 | 各67通过 |
| GL-only，两现代关闭 | 69通过＋1禁用 | 70通过 |
| CPU-only | 60通过＋1禁用 | 61通过 |
| 根benchmark-only / 独立Benchmark入口 | 9 / 7通过 | 9 / 7通过 |
| 关闭测试：双现代、Vulkan-only、根benchmark-only、独立Benchmark | 各构建通过、CTest清单为空 | 同左 |
| 开启游戏、三后端全关闭 | 具名配置拒绝 | 同左 |

本节SDK隔离仅指图形后端专用头/库/着色器编译器；MSVC和基础Windows SDK仍是Windows程序的必需构建工具链。

最终按修正后的构建规则重新配置/核对22个有效目录，均无额外生产重编，SDK/target/link/资产隔离通过；受影响游戏/CPU目录另执行66次定向边界/门面/资产CTest，全部通过。四组Release只做静态安装资产检查，已启用后端恰有四个shader，关闭后无对应资产；未做本批之外的安装游戏/缺loader运行。所有已编入后端的无GPU资产预检exit0，未编入均exit3/Unsupported；省略renderer仍只选D3D12，未编入D3D12时不改选GL/Vulkan。独立工具在无关含空格CWD的非法参数入口exit64，证明可启动CLI，不是性能采集或Vulkan产品协议支持。关闭测试图/编译命令无测试源码，独立配置无平台/SDL/原生SDK依赖。运行移除进程VULKAN_SDK及其PATH提示，启用Vulkan时显式提供私有头和SPIR-V编译器；此为构建隔离证据，不等于物理卸载SDK/干净机器部署。

双现代[Debug](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-incremental-debug-001/result.json)与[Release](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-incremental-release-001/result.json)各8项实际增量探针通过，起始均0编译：私有Vulkan头只重编工厂1＋Vulkan实现1，私有D3D12 pipeline头只重编其2源，均不波及app/world/simulation；公开types准确重编真实app_render_data两源及app_runtime两源。私有缓存只重编两个生产调用源及其单测；共享常量各重建4DXIL＋4SPIR-V，两个shader源各只重建自身2＋2，shader探针无C++重编。输入SHA256及mtime全部恢复，公开消费者按预期重链接。脚本适配真实生产owner、对shader缺失/重复及CPU门面与原生依赖作正反检查，最终37项Python单测全部通过；配置通过标记移至隔离/身份检查之后，共享HLSL纳入来源身份。

本轮修复Vulkan-only测试仍选D3D12及隔离包预检遗漏SPIR-V。另发现真实工具链检查缺口：Windows SDK DXC1.6.2112.16的帮助宣告-spirv，但实际shader生成报SPIR-V CodeGen not available；原帮助检查未在配置期拒绝。已改为有界离线真实试编译并检查SPIR-V模块头，SDK DXC1.9正向组合通过，旧Windows DXC的Debug/Release现均具名配置拒绝；缺Vulkan头发现亦具名拒绝。保留negative001配置误接受及后续实际shader构建失败、negative002修正后的负例，不将错误假设改写成原先已通过。首轮001另保留本轮误给world-summary测试添加其禁止的renderer参数造成的65通过/1失败/1禁用；该改动已撤回。002的CPU构建/测试通过后，校验器误拒绝合法纯CPU renderer，已修正并用003补齐；这些失败不算游戏故障，也不删除现场。历史Win32 1175等旧失败状态不改，本批有效配置未复现它们。

当前证据以[最终22构建/输入身份核对](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-build-final-001/result.json)及同目录source-inputs为准；[修正能力检查后的闭环](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-build-close-001/result.json)保留24项归属、定向CTest与安装资产身份，原始产物仅留out，不新增文档附件。最后仅调整矩阵脚本的通过标记时机/共享HLSL身份范围，并再次核对全部隔离、无重编和37项单测，未改写先前结果文件。

**A02–A04本批构建/组合/增量缺口已补齐；R4验收与A01–A05聚合任务仍不勾选，进度17/24。** A05真实缺loader/能力拒绝、生产绘制/窗口、A10/A13/A14故障寿命、A17异步缺失状态、A18原生Benchmark Vulkan兼容及A19人工仍按下方对账保留。没有新增Vulkan视觉或性能通过结论，不自动启动后续批次。

## 2026-10-14 R4验收证据对账

本节为第1批执行前的只读对账快照；已补的构建范围由[第1批结果](#2026-10-14-r4第1批构建与增量验证)、绘制/窗口由[第2批结果](#2026-10-14-r4第2批生产绘制与窗口验证)、受控在途/故障由[第3批结果](#2026-10-14-r4第3批在途资源寿命与故障验证)覆盖，不据原快照重记为未测，也不把有限范围扩为整组通过；其余缺口按后续结果分别保留。

本轮按用户要求只读核对A01–A19，不运行新原生/CPU测试、构建、游戏或采样，不修改生产代码；测试执行安排仅在聊天提出、待确认，不以对账创建第二活动计划。复读目标spec及既有结果，复算[运行输入身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/close-input-identities.json)200项均存在且SHA256/大小未变；[最终10条命令](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/close-cases.json)与较早安装002的9条命令均保留原exit0记录，不能将命令数当A项通过数。现有记录中D/R各8项精准CTest、4个D/R×1x/4x生产smoke的0/0诊断及两个缺/坏SPIR-V头的预期失败均核对一致；这些只是读取旧结果，不是本轮重跑通过。

下表以[生产实现证据](#2026-10-14-r4生产vulkan实现)为基础。C=当前D/R公开消费者/门面/三头及边界8项；N=当前四组生产v1 smoke；G=安装003最终static1秒/edit2秒及报告；O=较早安装002的9秒edit、三帧Debug Vulkan与默认D3D12/资产预检。E01–E03实验只作补充依据，不能替代生产对象/路径。旧D3D12批准和人工反馈不变，不移作Vulkan通过。

| 项目 | 现有可用证据 | 本轮判读与剩余缺口 |
| --- | --- | --- |
| A01 公开边界 | C：三公开头、独立消费者、私有/SDK越界拒绝及实际compile/link检查 | 当前组合范围已有充分证据；保持复用，不因A03未完成把它重记为未测，也不据此宣布全部配置通过 |
| A02 生产与独立配置 | C：生产owner/无test依赖图；工具已有实际构建 | 部分；加Vulkan后的CPU-only、benchmark-only、关闭测试独立配置/运行尚无本轮证据 |
| A03 D/R组合 | D3D12+Vulkan+GL两配置实际构建，N/C真实执行 | 部分；现代两个单后端/双后端、GL受影响隔离、全关闭拒绝、关闭后不找SDK/资产缺本轮组合结果 |
| A04 依赖/增量 | C：实际target、compile命令、Ninja/link/安装图 | 部分；Vulkan私有头、公共头、同源HLSL及常量include的实际增量未测，现有增量脚本尚按D3D12/旧app owner计数，需先适配 |
| A05 设备/不支持 | N/G：5070Ti、UUID/API/驱动、专用Window/实际呈现 | 部分；缺loader而未选Vulkan仍能用D3D12/GL，以及未编入/缺维护feature/不支持能力的具名拒绝未闭合 |
| A06 材质/剔除/深度/选框 | N：两层Linear/SRGB、UV、selection截图；G真实atlas/方块画面 | 部分；透明discard不写深度的遮挡与顺序、反面剔除/重复UV等生产专项像素、正式Vulkan画面对照未完成 |
| A07 固定世界/相机 | G：441区块/361初次mesh、seed/摘要、1080p/4x；Debug三帧亦正常 | 部分；生产walk/转向/缩放和恢复后的完整游戏相机未验证 |
| A08 静止/编辑/空mesh | G：静止资源上传0、7次编辑/21次mesh接受；N空更新/恢复；O35次前后向编辑覆盖既有完整计划 | 部分；最新版安装003仅7次编辑，边界前后像素/裂缝与只更新受影响块仍需针对性核对，不能把较早包完整循环说成当前完整覆盖 |
| A09 资源输入/寿命 | C：CPU非法参数/跨实例/过期等；N：修改原容器、旧句柄拒绝 | 部分；CPU门面检查可复用，生产Vulkan的完整负例、混层接受及无原生提交的配对证据仍需补足 |
| A10 真正在途/容量 | N：正常更新/删除与最终退出；E02有实验门控证明 | 生产关键缺口；当前smoke未证明更新时GPU真未完成，缺真实队列gate、原生版本/回收计数、增长及接受失败后的旧版本证明 |
| A11 窗口重建 | N：800×600、显式Resize零/恢复；E03实验有真实10秒最小化 | 生产关键缺口；需真实最小化至少10秒、连续resize、零尺寸不提交/不忙循环、OutOfDate/Suboptimal及输入/相机恢复，不能继承实验成绩 |
| A12 VSync/MSAA | N：D/R真实1x/4x、Immediate/FIFO切换和Info | 正向基本覆盖；不支持采样/呈现请求不得静默降级的负例及压力组合未覆盖 |
| A13 资产/分配 | 独立缺SPIR-V、坏magic头部分构造清理均0/0，原资产不改 | 部分；坏文件头只经过CPU拒绝，不是驱动pipeline失败；坏PNG/YAML、原生/host分配失败及更新旧内容保留未补齐 |
| A14 设备/提交/超时 | 当前原生错误分类/有限Wait/终止保留有源码，CPU门面Failed检查有C | Vulkan生产故障尚未实测；需受控真正在途timeout与DeviceLost/submit结果等注入分列，核对不续交、首错及完整owner/loader保留，不能挪用D3D12故障成绩 |
| A15 关闭/部分创建 | N正常/重复Shutdown；缺/坏SPIR-V部分创建清理；G正常退出 | 部分；Esc/关闭按钮、重复启动与原生对象增长、故障关闭后的Window/loader顺序尚缺生产证据 |
| A16 诊断 | N/G的D/R Vulkan validation及同步校验0/0；首轮3条depth冲突已修正且旧失败保留 | 已覆盖现有成功路径；后续故障/压力仍逐案留诊断，D3D12/保留GL受影响实际路径也需复查，不能一次空日志永久关闭 |
| A17 截图/GPU样本 | N每案23个frame ID↔23个有效样本、4张PNG；G最终static145/edit199个有效GPU样本 | 部分；原生未就绪时非阻塞、256队列满/丢弃、unsupported/device-failure等缺失状态及计时scope仍须验证，不能只在截图等待后Poll充当全异步证明 |
| A18 工具/协议/安装 | 安装003本机无关含空格CWD运行，shader/CRT/notices及中立报告已验证 | 明确产品兼容缺口：原生Benchmark仍拒绝Vulkan，既有report probe不是产品；需适配后核对SPIR-V生成身份、协议/旧GL兼容、实际依赖与无开发SDK条件，不能称已过干净机器部署 |
| A19 人工玩法 | 无绑定当前Vulkan候选的人工通过记录 | 未执行；本机独立14项玩法与至少15分钟连续操作/正常退出，旧D3D12认可不能继承 |

特别注意：A10/A13/A14不是单纯“把已有命令再跑一次”，当前Vulkan没有生产专用受控故障/gate及退休计数probe；A04脚本和A18产品消费者也需要先适配。未来测试入口只允许私有、创建线程作用域、独立进程链接真实门面/后端/必要游戏入口，不改公开v1、不建软件替代后端、不制造整机TDR；真实原生完成/超时、受控返回值注入和自发硬件故障分开。具体实施与执行范围以用户对聊天安排的确认及后续设计核实为准，本轮未进行这些代码改动。

本轮不复测/优化D3D12 edit第3轮P99，不放宽预算，不自动启动任何长采样。Vulkan正式性能预算尚未选择/验收，本次对账与短诊断不产生性能通过；正式Vulkan视觉参考/人工口径也须明确，建议同版GL人工对照、已批准透明深度差异单列，不把D3D12人工反馈或此前未获选的像素数值方案强加给Vulkan。用户确认前不将建议变成已批准门槛。

**结论：R4验收仍未完成，进度保持17/24，未新增人工勾选或节点批准。** 同一verification增量同步主目录供审核，不复制原始产物/冻结材料或新增报告；D3D12默认、GL保留、性能缺陷暂缓和无子agent状态不变。

## 2026-10-14 R4生产Vulkan实现

用户授权继续R4实现，并明确批准[KHR/EXT呈现fence必需能力](decisions.md#2026-10-14-vulkan有限呈现退休能力)；本轮独立实施，未启用子agent。代码在 `E:/Codex/Worktrees/refactor-renderer/Symocraft`，同源HEAD仍为 `66cee95fe53222bf2b6b5e3bf105af5c0b138bc1` 加未提交改动；原D3D12/实验/用户改动保留，未Git提交/推送、spec sync/archive或启动T4/Fix1。D3D12性能缺陷暂缓，默认仍D3D12，GL保留。

真实私有[Vulkan后端](E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/backend/vulkan/vulkan_backend.cpp)接入同一公开v1/工厂、GameRenderer、持久网格缓存、真实atlas/相机/selection及同一游戏循环，公开头未改，生产不include/link实验代码。已落实原生资源版本与接受、三帧上下文及逐图像呈现退休、MSAA/depth/resolve、尺寸/VSync、有限等待/终止保留及实际loader引用、截图/非阻塞带frame ID的GPU样本。同源HLSL的DXIL路径不改，独立SPIR-V编译器与四个shader输出/安装由[构建规则](E:/Codex/Worktrees/refactor-renderer/Symocraft/cmake/RendererVulkan.cmake)负责；默认OFF的 `SYMOCRAFT_RENDER_VULKAN` 只在显式编入/选择时启用，不是假target或清屏接管。当前机制见[架构](../../../../architecture/renderer.md#vulkan工作目录候选机制)。

使用本机MSVC14.38/19.38.33145、CMake4.3.5、Ninja1.13.2、WindowsSDK10.0.22621.0及VulkanSDK1.4.363.0/SPIR-V-capable DXC；D/R独立构建目录为 `out/build/windows-vulkan-debug`、`out/build/windows-vulkan-release`，本轮组合D3D12+Vulkan+GL，未执行全部单/双后端矩阵。实际原生运行均为RTX5070Ti、vendor10de/device2c05、UUID `d43c9988ca0711129c89be697511f1d0`、Vulkan1.4.351、NVIDIA616.64，不借D3D12/旧实验成绩。

[最终用例与命令](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/close-cases.json)及[200项源码/构建/安装输入身份](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/close-input-identities.json)绑定实际产物。D/R各1x和4x的[生产v1 smoke](E:/Codex/Worktrees/refactor-renderer/Symocraft/test/integration/renderer/vulkan_backend_smoke.cpp)四案均exit0、validation warning/error均0；每案绘制Linear/SRGB双层非对称UV、selection，保留4张PNG，修改原CPU容器后已接受画面不变，更新/清空/再上传、删除后拒绝旧句柄、800×600重建、显式零extent暂停/恢复、FIFO/Immediate及重复Shutdown通过。各案23个实际提交frame ID与23个有效GPU样本一一对应；截图为真实GPU读回，RGBA方向/patch值通过，实际看图。最终画面见[纹理](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/release-4x-003/linear.png)、[selection](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/release-4x-003/selection.png)。这不是强门控真正在途A10证明，也没有把旧E02/E03成绩移植为生产通过。

最终D/R各8项定向CTest均通过：renderer三公开头独立消费者、公开API消费者、门面契约及source/negative/renderer三项边界；日志为[Release](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/release-ctest-close.log)、[Debug](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/debug-ctest-close.log)。没有重跑D/R全套、CPU-only、benchmark-only、关闭测试或完整增量/组合矩阵。

完整安装为[Release003](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/release-install-003/SymoCraft.exe)，含四个SPIR-V、原DXIL/GLSL、公共资产及CRT/notices；从无关含空格CWD显式选Vulkan，4x/1080p/Immediate/validation-on、seed424242、完整441区块/361初次mesh的static1秒和edit2秒实现诊断均exit0，正常关闭/finished及中立报告校验通过。最终edit实际7次有效编辑、21次mesh接受、199个有效GPU样本，World摘要实际改变；static几何上传0，不混淆每帧常量上传。见[static报告校验](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/final-installed-static-probe.log)、[edit报告校验](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/final-installed-edit-probe.log)、[最终游戏截图](E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-implementation-001/final-installed-edit-001/final-frame.png)。本轮另有早期9秒edit及Debug三帧Vulkan游戏检查，均正常退出/零诊断；安装002三后端资产预检通过，省略renderer的两帧仍实际D3D12，未改变默认。短窗口只验证实现/导出，不判Vulkan性能预算，亦不继承D3D12预算或关闭其P99缺陷。

保留失败：首次Release1x虽然进程exit0/像素通过，但同步校验报告3条depth attachment WRITE_AFTER_WRITE，明确不算通过；修正external subpass依赖覆盖late fragment tests及depth写访问后，后续D/R各1x/4x零诊断。初次构建缺Window前置声明已修复。一次过宽CTest筛选误纳36个未构建exe，记Not Run而非测试通过；补建本次三公开头后精准8项重验，不宣称其余33头/旧GL suites已测。安装001因当前已启用的Benchmark exe未构建而失败，保留部分目录，不运行/冒称完整；补建独立工具后安装002/003成功。独立缺SPIR-V及坏magic头两夹具预期exit1、部分构造清理0/0通过，原shader/包不改；不是实际驱动坏shader、分配或设备故障证明。

**R4-实现任务完成，OpenSpec进度17/24；R4-验收与批准保持未勾选。** 未覆盖真实设备/提交/在途timeout及分配故障、强门控生产在途回收/容量压力、生产实际最小化至少10秒/并发resize/OutOfDate-Suboptimal异常分支、未选loader缺失、完整配置/增量矩阵、无SDK干净机器部署及15分钟人工玩法/正式视觉/性能预算。原生Benchmark产品目前仍只接受GL/D3D12，不把本轮Vulkan游戏中立报告probe当其已支持。下一项是按A01–A19对账现有生产证据、提出剩余验收执行安排供确认，不默认启动新的阶段测试矩阵/长采样；不批准T3或GL退役。工作目录结果/任务/机制及时同步F盘主目录审核，代码和日常运行产物仍在E盘，不新增docs附件或平行报告。

本轮工作目录与主目录的change严格validate均通过；完整文档巡检未通过，仍报告本轮未改动的历史archive/out断链及主目录并行physics材料链接，未复制/伪造历史文件或修改其他任务。已修正本轮同步时一处应指向E盘的smoke源码链接；两处49个新增本地/跨盘链接与锚点定向通过，200项运行输入SHA256复核未变，新增源码空白及实际diff检查通过。文档同步不表示历史证据已恢复。

## 未通过与未覆盖

2026-10-17当前状态以[完成对账及保留风险](#2026-10-17-r4及t3完成确认)为准，R4/T3已获用户批准；本节以下保留早期R2/R3时点的未覆盖事实，不将其中“R4验收未完成”等点时表述作为当前节点状态。

当前默认已按后续用户明确R3批准切为D3D12，GL保留显式过渡；Vulkan工作目录候选已实现、R4验收未完成，见上节。批准与定向切换结果见下节。以下R2/R3对账保留批准前的点时身份及未验证事实，批准不使A01–A19部分项自动变为全绿。Release004人工玩法已有用户通过反馈，受控原生设备移除/真正在途超时及提交注入已有独立证据，不继续笼统记为未测。
原生 Benchmark 已在本轮补充显式 D3D12/现代 v1 消费者，具体版本/自动运行范围见下节；GUI 人工与正式基准不能从 CLI 短测推定。Win32 1175、暂停 Fix1、正式同源 Q06 和首次可操作时间预算继续保留原状态。
本机 RTX 5070 Ti 功能范围不代替下一玩法前的 Y9000P/GTX1650 整机性能门槛，旧 GL 冻结数字不是新后端通过依据。

## 默认切换验证

2026-10-12，用户在已知R3证据对账仍有部分项后明确批准R3并授权切默认；唯一节点记录见 [全局索引](../../../../verification/VERIFICATION.md#关键节点)。本轮实施只切游戏默认及首次构建默认，保留显式GL、原validation/能力/失败分类；没有删除GL、开始Vulkan、恢复长采样/Fix1或执行Git/sync/archive。下方历史对账中的A10正常退休、故障关闭整合、新版增量、运行生成身份/适用部署、视觉容差及正式预算仍保持未验证/部分，不反推实测通过。

新包为 [d3d12-default-release-001](../../../../../out/m3-t3/package/d3d12-default-release-001/package-manifest.json)，默认 `d3d12`、保留 `opengl`；manifest SHA256 `57b9fbeae40898f84865c4de3e1b1f25d59eb3ce3e1d37c8bf0f92961553599f`，游戏EXE SHA256 `83e084a791754252a43eaf0f6617e7d84058677dcc7ab9e881002e8dab8a3cf8`。新包另建，不覆盖旧Release004、现代独占002或历史候选；旧包保持其原默认选择。

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
| [完整配置矩阵 004](../../../../verification/archive/m3-t3-r2-config-004.7z)、[故障入口加入后的图形矩阵 006](../../../../verification/archive/m3-t3-r2-config-006.7z) | 全新 MSVC14.38/Ninja D/R：现代独占、GL+D3D12、CPU-only、根/独立工具、关闭测试/工具及游戏全后端关闭拒绝。004 完成18组合；006完成6个受影响图形组合，source_unchanged 为真。004 CPU Debug、006现代Debug/过渡Debug/现代Release的 foundation.files 均实际失败并具名记录1175；普通Debug world.allocations 禁用，不能报全通过或关闭遗留风险。关闭测试实际空测试集，现代包无GLAD/旧GL target/GLSL；未交付Vulkan未测 |
| [首轮夹具失败 001](../../../../verification/archive/m3-t3-r2-config-001.7z)、[旧资源假设失败 002](../../../../verification/archive/m3-t3-r2-config-002.7z)、[隔离路径失败 003](../../../../verification/archive/m3-t3-r2-config-003.7z)、[被中断的 005](../../../../verification/archive/m3-t3-r2-config-005.7z) | 保留现代夹具缺字段、世界摘要/资产测试旧GL假设及隔离子进程路径错误的实际失败；修正测试部署与诊断后在新目录复测，不改Files实现/输出位置来关闭1175。005只有部分实际结果，未完成不得当整组通过 |
| [最终Debug受控故障](../../../../verification/archive/m3-t3-r2-faults-debug-001.7z)、[最终Release受控故障](../../../../verification/archive/m3-t3-r2-faults-release-001.7z) | 各4个独立进程，真实RendererState/D3D12库、RTX5070Ti/32.0.16.1664、validation开启、1x samples。分配 HRESULT 注入保旧画面并能显式空更新；实际Execute后的Signal HRESULT注入停止续交。自有设备API移除产生原生DeviceLost，非自发硬件事故。自有队列gate后的真实texture copy/fence在途，500ms原生Timeout，失败关闭quarantine/重复关闭仍失败，gate解除并确认完成后有限退出；不是正常回收、整游戏/public wrapper 或所有驱动故障验收 |
| [旧数据原生消费者](../../../../verification/archive/m3-t3-r2-consumers-001.7z)、[新候选正常运行与快速协议](../../../../verification/archive/m3-t3-r2-consumers-002.7z) | 原生Release消费者读取冻结Debug004的GL/D3D12各三场景全部接受，未重写原数据。新现代独占Release完成普通无collector的400帧；同一过渡Release新EXE的GL/D3D12各三场景10+10秒、1080p/4x/VSync-off/441块及原生终态校验通过，旧25列和现代32列/v1身份分开，原包前后身份不变。快速结果与截图是诊断材料，不是正式九轮、视觉容差/性能预算或15分钟玩法通过；裁剪环境仍是有SDK的同一主机 |
| [现代独占Release002包身份](../../../../../out/m3-t3/package/d3d12-modern-release-002/package-manifest.json)、[过渡Release004包身份](../../../../../out/m3-t3/package/d3d12-transition-release-004/package-manifest.json) | 新独立候选，未覆盖原d3d12-game-D/R-002。现代包须显式选择d3d12，缺省仍请求GL且具名失败；过渡包默认GL。完整运行包留out，archive中的运行/来源/依赖日志不冒称完整可运行包、无SDK新机器认证或R3批准 |

2026-10-12 用户已确认 D3D12 帧时间方案4与两项进程内存相对预算，口径唯一见 [T3 决策](decisions.md#d3d12-已选预算口径)；启动/首次可操作等其他指标仅观测、不新增本轮硬预算，不改变 T2 的历史未决状态。本次仅同步活动规格/计划，未启动正式采集、未标预算实测通过；正式视觉参考的完整场景身份/数值容差、正式同版本同条件 GL 配对采集及预算判定仍未关闭。实际提交故障/真实内存耗尽、自发设备丢失、完整A01–A18与适用人工要求继续按原任务核对；受控与注入不混成硬件事故。shader字节与构建工具/profile在候选来源中绑定，但运行时完整生成身份/适用部署要求不从消费者兼容自动推定。当前任务仍未整项完成。

## R2 短采样收口与保留风险

2026-10-12 用户明确“批准收口，然后进行R3证据对账”：批准的是上述限定范围的 R2 工作收口并保留风险；同时授权只读 R3 证据核对。按现行治理在本节登记，不新增日常子步骤关键节点表、伪造完整技术验收或推定 R3 批准；未授权新测试/长采样、默认切换、Fix1 恢复、Git 或 sync/archive。

2026-10-12 用户报告按本轮提供的过渡 Release004 / 显式 D3D12 命令完成的人工核查全部通过；作为用户反馈记录，不补造逐项日志、具体游玩起止时刻或测量曲线，不将肉眼对照写成数值视觉容差批准。用户随后取消正式九轮，限定 5 分钟改做 GL/D3D12 短对照并收口本轮 R2 工作；该执行范围替代及验收边界见 [追加决定](decisions.md#本轮-r2-短采样收口与风险接受边界)。

证据唯一入口：[短采样比较与身份](../../../../verification/archive/m3-t3-r2-short-001.7z)、[GL 原生会话](../../../../verification/archive/m3-t3-r2-short-001.7z)、[D3D12 原生会话](../../../../verification/archive/m3-t3-r2-short-001.7z)。先 GL 后 D3D12，同一 Release004 EXE/资源及原生执行器；两后端 static/walk/edit 各一轮、10 秒预热+10 秒采样，六轮全部通过原生终态/数据校验及既有报告探针，无重试。实际 RTX5070Ti、GL616.64/D3D12驱动32.0.16.1664、1080p/4x/VSync-off、D3D12 validation 开启；采样后包内声明文件的大小/哈希匹配原 manifest。原始 CSV/summary/PNG/日志保留在同目录，未提交或上传。

| 场景 | GL P95 / P99（ms） | D3D12 P95 / P99（ms） | D3D12/GL 工作集 / 私有提交观测最大值 |
| --- | --- | --- | --- |
| static | 6.861 / 7.581 | 5.570 / 6.141 | 87.95% / 67.85% |
| walk | 6.643 / 7.398 | 5.313 / 5.751 | 88.55% / 67.94% |
| edit | 7.499 / 8.660 | 5.946 / 7.761 | 88.38% / 68.23% |

上述只按短窗口筛查所选数值条件，全部未触线，**不是正式预算验收通过或性能无回归证明**。内存取 `10 <= elapsed_s < 20` 的每项观测最大值，每轮各10次，非正式180秒窗口/生命周期峰值/显存。P95/P99 从 sample CSV 最近秩复算，不删慢帧；D3D12 最大帧 static/walk/edit 为17.817/13.587/19.377 ms，不能由分位数合格声称无卡顿。static 的 GL 采样全程获焦、D3D12全程失焦，严格同条件比较不成立；其余两场景双方全程失焦，allow-unfocused 有效不代表消除后台争用。长采样/重复性/罕见尾帧与持续资源增长未证明，温度/频率未采集。

保留风险：正式视觉完整参考身份/数值容差未闭合；Win32 1175/Fix1 仍开放且暂停；自发设备丢失、真实内存耗尽/原生提交失败、完整 A01–A18 与无 SDK 干净机器部署未由短测覆盖，受控原生/注入证据保持各自边界。Vulkan与下一玩法前中低端整机性能仍独立。按用户限定本轮 R2 工作已收口，不自动恢复长采样；聚合技术验收保持未完成，不登记 R3/默认切换/T3 整体验收批准。

## R3 证据对账

2026-10-12，按用户授权只读核对已有材料，未构建、重测或运行游戏。判定对照 [A01–A19 原目标](specs/renderer/spec.md#requirement-modern-backend-takeover-acceptance)；“已覆盖”只指所列本机 D3D12 条件，“部分”不得变为通过。旧证据仍绑定旧版本，不因本表变为 Release004 新实测，用户人工反馈与自动记录分开。

版本核对：当前 [Release004 manifest](../../../../../out/m3-t3/package/d3d12-transition-release-004/package-manifest.json)声明的56个包文件及 [构建来源](../../../../../out/m3-t3/package/d3d12-transition-release-004/provenance/candidate-build-identities.json)的3270个源码/资源输入在本轮逐项匹配；短采样44个登记的会话/结果/原始产物、D/R故障8个YAML报告均匹配既存哈希。游戏EXE为 `41d9f5ee928b458fa2b715755465c4e16f30ea7f1075a5858ee378b450cf075f`。Renderer公开三头与历史顺序1–3相同；私有D3D12已增加诊断访问/工厂，旧夹具不能无条件迁成新执行成绩。对照顺序4归档源，新增主要为可选诊断owner、分配/Signal注入、quarantine标记与诊断工厂，普通游戏不创建诊断控制；新006构建/CPU结果和Release004正常短运行作为补充，而不是抹除版本差异。

复用入口：[旧CPU/增量顺序1](../../../../verification/archive/t3-r2.7z)、[旧GPU/增量顺序2](../../../../verification/archive/t3-r2.7z)、[旧游戏/增量顺序3](../../../../verification/archive/t3-r2.7z)、[顺序4候选/原生/窗口/隔离资源](../../../../verification/archive/t3-r2.7z)。新配置004/006、故障、消费者及短采样的身份/实际结果仍由上两节链接管理，不另造证据副本或测试矩阵。

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

## 2026-10-13 工作目录补充验证

用户授权先补生产故障/关闭整合、增量/部署及正式视觉/预算。新目录为Codex已登记的托管worktree `E:\Codex\Worktrees\refactor-renderer\Symocraft`，从当前分支的 `66cee95fe53222bf2b6b5e3bf105af5c0b138bc1` 创建，detached HEAD；本轮未commit/stage/push/sync/archive、未进入Vulkan或恢复Fix1。原F盘源码与旧候选/冻结材料未改写；原记录的out路径仍绑定原工作目录，未搬成新版本证据。使用MSVC14.38/19.38.33145、CMake4.3.5/Ninja1.13.2、SDK10.0.22621.0，本机RTX5070Ti/driver32.0.16.1664。

### 生产入口故障与关闭

同一生产 `RunGame` 启动/异常分类/关闭/退出处理、Application、GameRenderer与公开门面归属 `symocraft_app_runtime` 唯一STATIC实现，游戏main与隔离probe链接同一实现，不复制生产CPP或新增游戏CLI。私有同线程single-use诊断绑定仅probe主动安装；普通游戏保持普通工厂。此处是实际生产入口/库整合验证，但probe EXE不是普通SymoCraft.exe的外部故障注入。

[Release结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/game-entry-faults-release-001/run.json>)及[Debug结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/game-entry-faults-debug-001/run.json>)各5个独立进程全部符合预期：正常实际入口exit0；分配HRESULT `E_OUTOFMEMORY`、实际Execute后Signal HRESULT `E_FAIL`、自有设备 `RemoveDevice`、真实texture copy/fence在途gate超时分别实际入口exit3。分类具名为AllocationFailure/SubmissionFailure/DeviceLost/Timeout；设备移除本轮观察到Present1失败 `0x887A0005`，timeout为500ms/`0x102`。前两类是有限注入，后两类是受控原生状态；不声称自发设备事故、实际OOM或原生Signal API自然失败。

真实超时触发完整应用关闭聚合、保留quarantine而非伪造正常回收；重复Application::Free完成且Window可重新初始化/创建/关闭，验证窗口/platform owner释放。故障清理仍可能具名拒绝资源Destroy，日志不抹除；初次关闭失败不由重复Free幂等性变为成功。probe释放自有gate并在3秒有限等待确认队列工作完成后退出。报告的 `completed_fence_at_timeout` 实际读取点是RunGame已尝试故障关闭之后、gate解除之前，证明此时仍在途，不冒称第一次timeout的瞬间值。不是A10正常mesh退休、所有构造失败或长期反复启动资源增长证明。

### 定向构建、增量与部署

全新Debug/Release实际构建生产游戏和上述入口；最终Release定向CTest七项7/7（启动选项、公开消费者、门面契约、包资产、三项架构边界），Debug启动选项1/1，预算Python单测4/4。初次过宽architecture正则选入未构建公共头测试而产生NotRun，随后改为精确七项；不将定向通过报告为全套D/R或CPU-only矩阵通过。未采用的数值视觉工具只做过构建/自测，未进行正式画面矩阵，最终从本轮源修改中移除。

[实际增量结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/incremental-followup-001/result.json>)：只触及时间戳并恢复、头文件SHA不变。私有diagnostic_access.h只重编d3d12_backend.cpp；公开renderer.h重编renderer.cpp、game_renderer.cpp与public_api_consumer.cpp，游戏/消费者目标构建成功。未声称所有头/模块增量矩阵在本版重跑。

[安装运行结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/deployment-followup-001/result.json>)：先因启用但未构建Benchmark导致install001不完整，保留该目录；构建Benchmark后另建完整 `installed followup release 002`。含两个EXE、10项游戏资产、匹配MSVC的8个app-local CRT及notices/licenses。安装路径含空格，从无关CWD运行；PATH仅System32、移除SDK/VS/INCLUDE/LIB相关变量，GL/D3D12各资产预检与120帧smoke共4案exit0，包前后hash一致。实际PE导入未见DXC/D3DCompiler/SDL3 DLL；不是新无SDK机器、全部传递DLL实际来源或中文路径部署证明。此为普通install，不冒称另建严格候选manifest/provenance。

### 人工视觉与正式预算

用户明确“人工画面对照已经足够了，我已进行对照，未发现明显错误”，本轮按该反馈收口视觉核查；同版本GL参考与透明深度差异单列仍保留。不采用未批准的2/255、1像素或7视点方案，不补造数值容差/截图/逐视点结果；此次回复未提供新包身份，原Release004人工核查仍绑定原版本，不冒称新E盘游戏EXE已人工核查。人工视觉反馈与下述自动预算的版本/判定分开。

按新确认的10秒预热+50秒采样三重复，先GL后D3D12、static/walk/edit各三轮，[正式结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/formal-budget-10-50-001/result.json>)完成18轮。UTC `02:59:50`–`03:18:19`，同一安装Release002游戏EXE SHA256 `06002fee1afcbf015ac795a1c771fe13466e0b293cb242997d86d317ab229b8a`及资源/CRT逐轮身份不变；collector/native消费者/report probe身份记在结果中。全部原始CSV/YAML/PNG/日志留同一out目录，不另造文档附件/长期archive。

18轮原生数据与游戏报告校验均通过，实际1920×1080/4x/VSync-off/seed424242/441块，D3D12 validation开启/Immediate。各轮50次采样段内存观测，焦点均fully-unfocused，各配对焦点一致；适用allow-unfocused，不声称前台性能。P95/P99最近秩复算全部采样帧，内存两项各取 `10 <= elapsed_s < 60` 观测最大值，未删慢帧、混池、平均或重测。

| 场景/轮次 | GL P95/P99 ms | D3D12 P95/P99 ms | D3D12/GL工作集/私有提交 | 全部预算 |
| --- | --- | --- | --- | --- |
| static/1 | 6.4581/6.8589 | 5.6303/6.0647 | 89.61%/67.19% | 通过 |
| static/2 | 6.6567/7.0771 | 5.7778/6.2480 | 89.81%/67.39% | 通过 |
| static/3 | 6.6683/7.0416 | 5.9944/6.8535 | 89.66%/67.19% | 通过 |
| walk/1 | 6.4501/6.9178 | 5.1003/5.4908 | 90.42%/67.52% | 通过 |
| walk/2 | 6.6035/6.9939 | 5.3808/7.0100 | 90.38%/67.39% | 通过 |
| walk/3 | 6.3486/6.7357 | 5.1734/5.9125 | 90.32%/67.29% | 通过 |
| edit/1 | 6.9017/7.6686 | 6.4589/8.4009 | 90.28%/67.60% | 通过 |
| edit/2 | 6.8190/7.8687 | 6.1447/7.7553 | 89.35%/67.27% | 通过 |
| edit/3 | 6.8599/7.7975 | 6.7976/8.6981 | 90.15%/67.73% | **P99相对超限** |

**数据有效，预算未通过。** P95绝对/相对、P99绝对及两项内存相对门槛九组均通过；edit/3的P99为GL的111.55%，超过110%，允许上限8.57725ms、实际8.6981ms（超0.12085ms）。整体8/9配对通过不能平均成9/9；edit/1的109.55%也如实保留。最大帧包括D3D12 edit/3的27.8316ms，分位数不证明无卡顿。采集没有失败、未自动复测，超限原因未定位，不凭单轮结果归因渲染API或后台噪声。

### edit 第3轮只读热点核查

2026-10-13按用户要求由同推理强度的三个子agent交叉核查现有数据和代码，未修改原始数据/代码、未重新采样或放宽预算。D3D12该轮9512个正式样本全部失焦、0次焦点转换，对应GL8043个样本也全失焦；nearest-rank复算一致，不由失焦直接归因或作废。按编辑序号对账，D3D12三轮200次采样编辑的重建数/顶点数/上传字节/上传次数及相机条件相同，无工作量增加或编辑积压证据。

该轮106帧超过其8.57725ms相对上限，其中76个编辑帧、30个非编辑帧；27≤elapsed_s<29的297帧中有27个超限，含21个非编辑帧。编辑帧平均较第2轮增加约0.565ms，主要表现为cpu_prepare约+0.277ms、缓存同步/上传约+0.193ms；这是同组均值差，不是分位数相加。最慢帧5283的27.8316ms中cpu_prepare为26.5504ms，且无编辑/重建。支持“编辑尾部成本增加及局部准备区间尖峰”的耗时构成，不证明唯一底层根因；不剔除任何帧或区间来改变预算结论。

**待定位性能热点，尚未证实为超限根因**：[RendererState::Render](<E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/renderer_impl.cpp>)每帧每draw调用[ValidateMeshTextureLayers](<E:/Codex/Worktrees/refactor-renderer/Symocraft/game/modules/renderer/src/validation.cpp>)，逐顶点重扫，当前约206万顶点，计入门面准备时间并合并cpu_prepare。该重复扫描是明确代码成本，但没有独立耗时。准备区间还包含FrameContext fence等待、描述符/SRV创建、资源清退、命令录制及Debug Layer检查；编辑更新另有CPU校验/owned-copy及新UPLOAD资源创建/Map/memcpy。现有统计无法分离这些成本或系统调度影响，GPU draw计时也不覆盖全部GPU/呈现路径，不能确认尖峰来自扫描、fence、驱动、调度或失焦。

用户本轮仅要求记录热点并评估后续工作量，不授权新增计时埋点、优化、性能运行或正式复测；后续执行范围须另确认。热点不转为独立活动计划，不更改原预算/验收状态。

2026-10-13用户随后明确“先跳过这一项，标明存在一定程度的性能缺陷”，本项状态改为**已观察到性能回归、暂缓处理**。适用范围仅上述本机/版本/协议的edit第3轮P99相对预算失败；实际回归11.55%，不据此推定所有场景/设备都有同样缺陷。保留逐帧验证热点候选及未确认根因，不继续本项排查、优化或复测，也不将跳过写成预算达标/技术完成。此次暂缓只针对性能项，不消除A10、部署、1175/Fix1等其余保留风险；用户要求告知下一迁移事项，尚未授权本轮启动Vulkan实验/实现。

### 本轮结论与风险

生产调用方受控故障/关闭整合已补实证，所列增量及本机隔离安装运行已覆盖，视觉按用户指定人工口径收口；**正式性能预算未通过，性能缺陷按用户决定暂缓处理，不继续占用本轮迁移排查范围**。原R2限定工作收口与R3批准保留，不由本次超限自动撤销，也不能用它们豁免本次性能失败。聚合A01–A18任务保持未勾选。

保留10+50短窗口不充分覆盖罕见尾帧/持续内存增长、全程后台运行与先GL后D3D12的顺序/热状态风险，温度/频率未采集。A10正常在途mesh更新/删除/容量及最终退休仍部分；干净无SDK机器/运行shader完整生成身份、全套本版配置回归未由本轮覆盖；Win32 1175/Fix1仍开放且暂停。启动/显存无新增硬预算，中低端整机与Vulkan独立；不追加未经批准的优化、重测、T4迁移或T3整体通过。

最终本change严格OpenSpec validate通过，本轮14个新增文档链接/锚点检查无错误、diff空白检查通过；不是全仓历史文档审计或运行验收。任务仍15/24，聚合缺口未完成不以文档校验代替预算通过。

## R4 本机能力与工具链前置实验

2026-10-13用户明确授权在同一refactor-renderer托管worktree开展T3-R4 Vulkan前置实验，先核实本机能力/工具链，并批准三个相同推理强度子agent。代码授权限本轮必要原生实验，CLI规划根docs不用于扩大代码权限；本轮未交付生产Vulkan backend、未改公开Renderer v1/游戏选择/默认D3D12、未删除GL、未处理暂缓性能缺陷、未恢复Fix1、未Git写或sync/archive。能力核实不冒称完整R4/R1-E01–E03或玩法验收。

### 实际设备与生产桥接

系统loader/SDK为Vulkan1.4.363，实际RTX5070Ti device API1.4.351，vendor/device `10de/2c05`、NVIDIA616.64，deviceUUID `d43c9988ca0711129c89be697511f1d0`；有效LUID字节 `1645010000000000`对应已有D3D12 `00000000:00014516`。AMD集显也枚举但未作为替代。本机最低1.2请求成功，不根据型号或SDK库存推定实际设备通过。

现成vulkaninfoSDK的full枚举60秒超时，保留277023字节部分stdout及stderr，不自动重跑或称完整工具成功；summary有输出但初次collector未在后续超时前保存退出码，记为未知。stderr含OBS implicit layer API版本及ICD direct-display入口警告，超时原因未定位、不修改全局layer配置。[枚举身份与失败](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-preflight-001/hardware/vulkaninfo-run.json>)记录真实状态。

继而在[现有生产平台探针配置](<E:/Codex/Worktrees/refactor-renderer/Symocraft/test/experimental/platform-sdl3/CMakeLists.txt>)的显式Vulkan实验条件下增加[能力probe](<E:/Codex/Worktrees/refactor-renderer/Symocraft/test/experimental/platform-sdl3/vulkan_capabilities.cpp>)，链接同源码/配置的实际platform/foundation/SDL/GLAD STATIC库，不重编window.cpp/SDL，不建空生产backend、不将Vulkan SDK传播到普通产品。通过专用Vulkan Window和同一生产VulkanBridge/SDL loader创建caller-owned instance/surface，而非另造Win32 loader路径。

初次Release编译因泛型枚举lambda不能从裸nullptr推断类型而失败；日志保留于 `out/m3-t3/r4-preflight-001/platform-release`，修正为带类型的空指针后另建Release002，不覆盖首轮。[最终Release](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-preflight-001/platform-release-002/result.json>)与[Debug](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-preflight-001/platform-debug-001/result.json>)各实际configure/build、生产bridge有限轮询、能力probe及PE依赖检查全部exit0；MSVC19.38.33145/CMake4.3.5/Ninja1.13.2，header显式取SDK1.4.363。每个原生进程30秒外层有限超时，无性能采样。

| 本轮项目 | 实际结果与限制 |
| --- | --- |
| instance/surface/device | 经生产桥创建API1.2 instance/surface，明确请求KHRONOS validation及debug messenger；创建上述RTX的logical device和真实queue，无软件fallback |
| queue/呈现支持 | family0同时graphics/present，16个可用queue；timestampValidBits64、period1ns、timestampComputeAndGraphics为真，仅查询能力、不提交GPU查询 |
| surface/swapchain | 查询格式/模式/caps，实际创建640×480、BGRA8_UNORM、SRGB_NONLINEAR、FIFO、COLOR_ATTACHMENT+TRANSFER_SRC交换链，得到3张图像；其他呈现模式只枚举，未acquire/submit/Present/显示图像 |
| 格式/数组/4x | 精确format×usage image-properties查询：RGBA8 UNORM/SRGB的SAMPLED+TRANSFER_DST可用，最大2048层/32768²，覆盖64×64×64目标；BGRA8颜色+TRANSFER_SRC及D32深度sample-mask15含1/2/4/8。未实际分配纹理/MSAA图像、sample/blend/pipeline或readback，不由全局sample-mask代替精确查询 |
| 清理与诊断 | 两构建warning0/error0，显式销毁swapchain/device/surface/messenger/instance后Window/SDL责任释放；没有提交工作故无device-idle等待。缺destroy入口时失败保留owner至进程退出，不假称正常清理；未证明在途退休或OS loader实际卸载 |
| 动态依赖 | 两probe PE均无vulkan-1.dll/SDL3.dll直接导入，使用SDL惰性loader；仅证明本probe，不推定缺loader机器/尚未交付生产Vulkan后端的组合边界 |

共同graphics+present queue、D32及640×480窗口是本机能力probe的具体选择，不增加公开v1的其他硬件强制基线，不冒称1920×1080/4x实际绘制。三个子agent复核设备身份、查询/创建区别、诊断、源/产物hash与未运行标志一致；没有用另一API的旧成绩补格。

### 离线shader与工具链

[工具链结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-preflight-001/toolchain/result.json>)及[头/库编译结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-preflight-001/toolchain/header-link-result.json>)记录绝对工具路径、版本/hash、HLSL输入/命令/产物/退出码。SDK DXC1.9.0.5512、SPIRV-Tools2026.4、spirv-cross等实际存在；对现有blocks/selection同源HLSL的vs_6_0/ps_6_0四阶段生成SPIR-V1.5，并以vulkan1.2验证、反射各4/4通过。SDK DXC另离线生成四阶段DXIL无编译警告，但未进行新DXIL运行/验证器来源证明，不替换现有生产DXIL工具或包字节。

**实际工具路径限制**：现有生产WindowsSDK DXC1.6.2112.16尝试SPIR-V编译exit1，原文 `SPIR-V CodeGen not available`；后续Vulkan必须显式使用具有SPIR-V能力的构建期编译器，本机SDK DXC已满足这一离线前置。SDK Bin缺dxil.dll未阻止上述离线编译，不反推验证器来源或将它称为SPIR-V阻塞。MSVC使用Vulkan Win32 headers/import lib的最小工具链编译链接1/1，该EXE未运行；与实际能力probe的无import-library方案分开。

显式实验映射set0的b0/t0/s0到binding0/1/2，dx-layout，未添加Y翻转。未映射时blocks片元texture和sampler均反射为binding0，不能直接沿用；正式pipeline layout仍待E01。反射常量64字节/offset0/matrix-stride16、SPIR-V RowMajor装饰，blocks位置ivec3 location0、texcoord vec3 location1，未使用direction被优化掉；不能仅凭RowMajor直接转置CPU矩阵、把D3D root constants当Vulkan push constants或宣称28字节输入ABI已GPU验证。blocks片元有OpKill而无EarlyFragmentTests，selection片元有EarlyFragmentTests，只是离线指令证据，不代替UV/颜色/透明深度像素核查。

### 结论与下一步

本轮能力及工具链前置已取得实际依据，无发现必须安装新SDK/修改公开v1的阻塞；现有生产DXC不能产SPIR-V的限制已有可用SDK编译器路径，但该路径尚未接入生产构建。vulkaninfo全枚举超时/旧layer警告保留，不归因具体驱动，也不将后续probe成功改写为原工具成功。

**R4-实验仍未整项完成**：下一项为R1-E01，落实同源shader的真实pipeline/顶点常量binding/双纹理层绘制及截图像素，再独立E02在途更新/清空/删除与CPU寿命、E03实际像素重建/零尺寸恢复；不以本轮device/swapchain创建替代它们。Vulkan性能预算/完整人工及A01–A19仍另行确认与验证，D3D12性能缺陷继续暂缓，GL不删除。日常输出留上述out位置，未复制到docs附件或新archive；源与产物身份由各结果维护。

最终Release三项架构边界定向CTest 3/3，本change严格validate及本轮累计25个新增文档链接/锚点检查通过、diff空白检查通过；未重跑全部D/R/CPU-only矩阵。整体任务仍15/24，能力核实不勾选未完成的R4聚合任务。

## R1-E01 Vulkan真实双纹理层绘制

2026-10-13用户明确授权在同一refactor-renderer托管worktree执行E01并保留截图，继续三个相同推理强度子agent分别实现原生实验、独立检查脚本及只读交叉核对。代码根授权限本轮必要实验；使用前述生产platform/foundation/SDL/GLAD STATIC库与VulkanBridge，未交付生产Vulkan后端、改变公开v1/默认D3D12或删除GL，未恢复暂缓性能项、未Git写或sync/archive。

新增[真实纹理probe](<E:/Codex/Worktrees/refactor-renderer/Symocraft/test/experimental/platform-sdl3/vulkan_textured_probe.cpp>)及[标准库核查runner](<E:/Codex/Worktrees/refactor-renderer/Symocraft/test/experimental/platform-sdl3/verify_vulkan_textured.py>)，只在实验CMake显式Vulkan+textured开关下构建。真实RTX5070Ti、NVIDIA616.64、API1.2 instance/实际device1.4.351、graphics/present family0，经SDL创建640×480窗口与BGRA8_UNORM/FIFO交换链；实验明确1x、无深度、全部不透明。12个真实顶点/4个三角形的两个网格区域分别采样实际2×2×2纹理数组的layer0/1，不以清屏或彩色三角形代替。

使用未经改写的生产blocks.hlsl/draw_constants.hlsli，经显式SDK DXC生成并验证vulkan1.2 SPIR-V；set0中b0/t0/s0映射binding0/1/2。GPU候选顶点stride28、int3@0、float3 UV/layer@12、direction@24；Debug的-Od反射保留location2 float DIRECTION，提供第三属性，Release的-O3只保留location0/1，两者严格分别核对而非统一放宽。Debug保留的同源跨stage资源声明按精确类型/绑定核对，descriptor stageFlags匹配；不改变28字节CPU表达。64字节column-major原始矩阵含非单位缩放和平移，未经额外转置上传；只有negative-height viewport转换Y，COUNTER_CLOCKWISE+back cull。非单位矩阵后的像素位置和背景guard与预先确定真值一致，但不声称所有矩阵/剔除组合均已覆盖。

| 最终配置 | 实际命令/诊断 | 截图 |
| --- | --- | --- |
| [Debug002结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/debug-002/result.json>) | 15条版本/编译/验证/反射/configure/build/import/native命令均exit0；validation warning0/error0 | [Linear](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/debug-002/capture/linear.png>)、[SRGB](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/debug-002/capture/srgb.png>) |
| [Release003结果](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/release-003/result.json>) | 同上，最终同一冻结源码；validation warning0/error0 | [Linear](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/release-003/capture/linear.png>)、[SRGB](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/release-003/capture/srgb.png>) |

每图独立预设8个内部3×3纹理采样patch及9个背景3×3guard，检查层号、UV上下/左右、矩阵后位置、RGBA和alpha。两配置Linear最大RGB差0，SRGB硬件解码到UNORM附件的最大RGB差1（layer1左上蓝实际160、公式量化161），在实验RGB每通道最多2/255、alpha精确255的口径内；这是E01夹具的采样容差，不替换已按人工口径收口的完整游戏视觉验收。SRGB结果不额外编码，不按“肉眼更暗”判失败。原生交换链GPU读回转为top-left RGBA，PNG编码后完整解码复核与原始字节相等；父/子agent实际看图并交叉复算，非合成图或只读native自报数值作真值。

两次各实际acquire→draw/upload/readback submit→Present，等待真实EXT_swapchain_maintenance1 presentation fence及draw/readback fence后才允许清理；每次等待最多5秒，原生进程外层60秒，无无限WaitIdle或用空submit fence冒称呈现完成。选择该本机已支持扩展仅服务此实验的有限呈现退休，不把它增为公开v1硬件基线；依据[Vulkan呈现同步说明](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)。最终两图均有提交/退休/present-fence完成及正常逆序清理证据，源码、工具、生产archive、实际运行SPIR-V、EXE、PNG/raw哈希及命令由各结果记录。CMake生成shader与runner独立验证后实际传入shader分别记录，不混用身份。

**失败与修正不覆盖**：[Release001](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/release-001/result.json>)真实原生exit1、validation error8，VUID pCode-08742；初始-fspv-reflect添加了未启用设备支持的GOOGLE SPIR-V扩展。移除不必要的该编译参数，保留spirv-cross独立反射/validation，另运行Release002取得通过，不改首轮raw/log/result。随后[Debug001](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e01-001/debug-001/result.json>)在离线反射gate因保留DIRECTION及同源资源而失败，未执行GPU；精确修正配置对应接口及pipeline后另运行Debug002/最终Release003。原生增加资源创建后及提交前validation guard，已知诊断不继续submit；未禁用validation、优化伪装Debug、剔除失败或放宽像素预算。首次GPU运行前的源码审阅还修正了front-face选择，没有将未运行的错误绕序代码伪记成GPU失败。

**结论：本机R1-E01已完成；R4-实验聚合项仍未完成，任务仍15/24。** 未运行E02调用者输入寿命/在途更新清空删除/过期句柄及容量、E03尺寸重建/零尺寸恢复，也未覆盖MSAA/透明深度/混层三角形、完整游戏/A06/A09或GPU计时，不借用D3D12旧成绩补格。下一事项为E02；D3D12已观察P99回归继续暂缓，GL保留。截图按用户明确要求留out同轮目录，没有复制docs附件或新增报告/归档。

最终runner CPU正/负反射、PNG/CRC、颜色/层号/几何自测通过，Release三项定向架构CTest 3/3，本change严格validate通过，11个本轮新增本地链接/锚点及diff空白检查通过。未构建/运行全套公开头消费者或D/R/CPU-only矩阵，未启动游戏人工/性能采样；以上定向检查不冒称它们通过。

## R1-E02 Vulkan输入寿命与在途回收

2026-10-13用户撤销子agent授权，明确要求在同一refactor-renderer托管worktree独立进行E02，本轮未启动/调度子agent。授权限原生实验与必要定向复验，未接入生产Vulkan/公开Renderer v1、改变默认D3D12或删除GL，暂缓P99性能缺陷不处理，未Git写或sync/archive。

新增[原生寿命probe](<E:/Codex/Worktrees/refactor-renderer/Symocraft/test/experimental/platform-sdl3/vulkan_lifetime_probe.cpp>)，只在实验CMake显式Vulkan+textured+lifetime开关下构建。把E01既有设备/布局/绘制/截图及呈现退休代码提取到[实验共用私有头](<E:/Codex/Worktrees/refactor-renderer/Symocraft/test/experimental/platform-sdl3/vulkan_probe_common.h>)，不引入生产target或通用RHI。仍导入对应配置的实际生产platform/foundation/SDL/GLAD STATIC库、使用同一VulkanBridge、SDK DXC与未经改写的blocks HLSL；实际RTX5070Ti/NVIDIA616.64/device API1.4.351、API1.2 instance、family0、640×480/BGRA8_UNORM/FIFO/1x。E02另外明确查询/启用timelineSemaphore及KHRONOS同步验证，不增加公开v1硬件强制基线。

采用实验单槽generation句柄与自有CPU快照/原生资源版本：Create/Update复制输入，非空mesh每个接受版本使用独立vertex buffer；texture拥有自有staging/image/view。创建/更新返回后实际覆写并释放调用者vector，帧只保留已接受版本。每帧真实acquire及draw/upload/readback submit在ALL_COMMANDS等待timeline值1，host暂不signal；修改前后各查询实际fence为VK_NOT_READY(1)、timeline counter为0，然后由host signal放行，真实draw fence为VK_SUCCESS(0)且maintenance presentation fence完成后才解除帧资源引用。依据[Vulkan timeline原生host signal示例](https://docs.vulkan.org/samples/latest/samples/extensions/timeline_semaphore/README.html)。这是**真实已提交、受控等待的原生在途工作**，不是mock/injected fence，也不冒称自然长GPU执行、设备事故或超时故障。

最终[Debug001](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/debug-001/result.json>)与[Release002](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/release-002/result.json>)各15条版本/编译/验证/反射/configure/build/import/native命令均exit0，来源/工具/生产archive/实际运行SPIR-V/EXE/raw/PNG身份记录并复算一致。Debug保留第三DIRECTION属性、Release仅前两属性，stride28/同源binding保持E01契约。两配置validation和同步验证均启用，warning0/error0，正常关闭完成；首轮Release001也成功，之后仅整理缩进与通用检查说明再另建最终Release002，未覆盖任何旧轮字节。

| 真实帧与在途操作 | 完成后的独立像素与寿命结论 |
| --- | --- |
| [创建后帧，期间Update](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/release-002/capture/created-before-update.png>) | 原输入已覆写/释放，旧帧仍使用原两层；新vertex buffer与旧的不同，pending期间旧版本未回收，完成后回收旧版本 |
| [更新后帧，期间清空](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/release-002/capture/updated-before-clear.png>) | 下一帧正确交换两层，更新输入也已覆写/释放；清空只替换已接受版本，不破坏仍在途的旧帧 |
| [空网格帧，期间恢复](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/release-002/capture/cleared-before-restore.png>) | 实际vertex_count0、无vkCmdDraw；整张307200像素均精确为RGBA(8,16,24,255)，无旧几何残留 |
| [恢复后帧，期间删除mesh/texture](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/release-002/capture/restored-before-destroy.png>) | 删除立即使句柄失效，旧画面仍正确；pending期间mesh/texture原生版本均保留，完成/呈现退休后才释放 |
| [重建槽位帧，期间再删除](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/release-002/capture/recreated-before-destroy.png>) | generation由1变2，旧句柄仍拒绝；新texture调换原始层存储且调用者已释放，画面与新内容一致，完成后原生资源再归零 |

每配置4张有几何图分别核对8个3×3采样patch/9个3×3背景guard，RGB/alpha均精确匹配预设独立真值；空网格检查全图。PNG完整解码后与GPU raw字节相等，实际看图与只读复算一致，不把native自报值当像素真值。总计9次过期访问/Update/重复Destroy拒绝，删除后和槽位复用后分开核对，拒绝不增加提交；全部5次真实提交在修改前后都未完成、资源回收计数不变，完成后依次增长。最终5个mesh CPU版本（含1个空版本）全部释放，对应4个非空原生vertex buffer全部释放；2组texture staging/image/view全部释放。mesh版本峰值2、texture峰值1、最终live均0；这是限定五帧资源账目，不是进程/显存预算或增长压力组。

帧描述符/命令/呈现相关引用在完成后先清退，随后解除mesh/texture版本引用；surface/instance/device及loader/Window责任保持原顺序。每次wait最多5秒、原生进程外层60秒，没有无限WaitIdle。缺完成证明时保留整个frame/registry/native/Window/loader owner graph到进程退出，未在本轮制造故障或证明这条终止路径已实测通过，不能代替A14/A15异常关闭组。

共用代码提取后的[E01 Debug回归](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/e01-regression/debug-001/result.json>)与[E01 Release回归](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e02-001/e01-regression/release-001/result.json>)也各全部exit0、双颜色空间像素通过、validation0/0；原E01截图/失败记录未改写。新增CPU自测拒绝错误pending/completion、早释放、同buffer覆写证明及呈现未完成，也拒绝单个残留像素和未换层画面；不以这些CPU负例代替真实GPU运行。

**结论：本机R1-E02已完成，E03未执行，R4-实验聚合任务仍未完成、进度仍15/24。** 实验registry不冒称生产公开v1整合；未覆盖多帧并行压力、容量增长/分配失败、跨实例句柄及完整非法输入、DeviceLost/自然timeout、E03重建/零尺寸、MSAA/透明深度、A08–A10/A15完整组或完整游戏接管。下一事项为E03；D3D12性能缺陷继续暂缓、GL保留。日常raw/截图/命令/结果留同轮out目录，未复制docs附件或另建报告/归档。

最终runner CPU正/负例通过，Release三项架构边界定向CTest 3/3，本change严格validate通过，本轮12个新增本地链接/锚点、diff及新源码空白检查通过。未运行全部公开头消费者、D/R/CPU-only配置矩阵、完整游戏人工或性能采样，未执行项不标通过。

## R1-E03 Vulkan尺寸重建与零尺寸恢复

2026-10-13用户明确授权继续独立E03，并要求文档及时同步至主目录供审核。本轮未启动/调度子agent，仅在同一refactor-renderer托管worktree实施必要原生实验与定向复验；不新增生产Vulkan/公开v1、切默认、删除GL、恢复暂缓P99缺陷或Fix1，不Git写或规格sync/archive。

新增[真实呈现重建probe](<E:/Codex/Worktrees/refactor-renderer/Symocraft/test/experimental/platform-sdl3/vulkan_resize_probe.cpp>)，仅实验显式Vulkan+textured+resize开关启用，仍导入实际生产platform/foundation/SDL/GLAD STATIC库及同源HLSL。实际RTX5070Ti/NVIDIA616.64/device API1.4.351/API1.2 instance、family0、BGRA8_UNORM/SRGB_NONLINEAR/FIFO/1x，validation及同步验证明确启用；SDK DXC、MSVC/CMake/Ninja与运行SPIR-V/EXE/源码/生产archive/raw/PNG身份由结果记录并复算。共用头只提取既有真实swapchain创建流程为重建方法，未另造Win32 loader/window或生产后端。

重建要求先取得draw/readback及EXT_swapchain_maintenance1呈现fence完成证明，清退旧framebuffer、swapchain image view、命令/描述符等帧引用，再以实际surface caps创建新链并传oldSwapchain、枚举新图像及释放旧链；窗口/surface/instance/device始终同一owner。oldSwapchain一经创建调用可能不可回滚，失败不得续用其旧Ready假设；本轮没有注入该失败或OutOfDate/Suboptimal，不冒称异常分支已测。依据[Vulkan oldSwapchain语义](https://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainCreateInfoKHR.html)。截图实验将clipped明确设false，避免被其他窗口遮盖的像素失去读回保证；这不是修改生产呈现策略或回写旧成功/失败证据。

最终[Debug001](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/debug-001/result.json>)与[Release001](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/release-001/result.json>)各15条版本/编译/验证/反射/configure/build/import/native命令均exit0；首轮即成功，无自动重试。实际像素序列如下，SDL生产Window尺寸、作用域DPI-aware Win32 client pixels与Vulkan surface currentExtent逐帧一致，不拿逻辑请求尺寸冒充实际drawable：

| 帧 | 实际像素与交换链代次 | 几何/画面 |
| --- | --- | --- |
| initial | 640×480，generation1 | 双纹理层真实绘制 |
| [larger](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/release-001/capture/larger.png>) | 800×600，generation2 | 尺寸相关呈现资源重新创建 |
| [wide](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/release-001/capture/wide.png>) | 960×540，generation3 | 投影sx按实际height/(3×width)从0.25调整为0.1875，保持夹具几何比例 |
| narrow | 720×600，generation4 | sx约0.27777779；UV/层/颜色和边界guard正确 |
| [restored](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/release-001/capture/restored.png>) | 720×600，generation5 | 真正最小化后恢复，不热重建Window；同尺寸raw与narrow逐字节一致 |
| [final](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/release-001/capture/final.png>) | 640×480，generation6 | raw与initial逐字节一致 |

最小化由私有NativeBridge取得本probe自有HWND后真实SW_MINIMIZE/SW_RESTORE操作，不操作其他窗口。两配置实际minimized/Iconic成立：SDL仍报告720×600，Win32 client与Vulkan surface均实际0×0；记录分别保留，不把SDL数值伪改为0。候选沿用[GameRenderer已有最小化映射](<E:/Codex/Worktrees/refactor-renderer/Symocraft/game/app/src/game_renderer.cpp>)，有效渲染尺寸为0×0。Debug停留10.04709秒、Release10.0396125秒，各94个限频等待/有效输入核对步骤；暂停前后acquire/submit/present/swapchain-create计数均保持[4,4,4,4]，无零尺寸acquire、重建、Present或忙循环。不是人工玩法测试、CPU性能测量或独立A11整组通过。

两配置各6张实际GPU截图，分别以预设非单位矩阵和独立动态宽高公式核对8个3×3纹理patch及9个3×3背景guard，RGBA最大差0。PNG完整解码后与原始top-left RGBA字节一致；另只读复算了上述两对同尺寸全图相等，并实际看图。投影真值从实际宽高独立计算，不用native自报常量当真值；只证明此夹具比例和恢复，不推定完整游戏相机/深度/MSAA画面通过。原生readback同时保存结构化实际尺寸sidecar，失败时按真实尺寸保留诊断PNG但acceptance仍false，不将变尺寸图按固定640×480猜测。

每配置共6次实际acquire/submit/Present、6次swapchain创建/销毁；每帧完成后旧swapchain image view及framebuffer live均0。持久mesh buffer、texture image/view/staging跨所有尺寸保留，几何写入1次、texture GPU上传1次，正常末尾全部清理；没有以每次resize重新传mesh/texture绕过寿命。全部draw及呈现等待最多5秒，窗口状态收敛等待3秒、原生外层60秒；缺完成证明仍终止保留owner图，不是无限WaitIdle或已实测故障恢复。

受共用创建/资源/读回改动影响的[E01 Debug](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/e01-regression/debug-001/result.json>)、[E01 Release](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/e01-regression/release-001/result.json>)、[E02 Debug](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/e02-regression/debug-001/result.json>)、[E02 Release](<E:/Codex/Worktrees/refactor-renderer/Symocraft/out/m3-t3/r4-e03-001/e02-regression/release-001/result.json>)均定向复验通过、validation0/0；原E01/E02截图/失败/结果字节未改。CPU自测拒绝旧尺寸/错误surface像素、旧projection、旧代次/未退休引用、零尺寸期间提交及假最小化/忙循环/过短停留，不代替本轮实际操作和GPU证据。

**结论：本机E03完成，结合本机前置能力/工具链及E01/E02，R4-实验技术任务完成，进度16/24。** 仍未交付生产Vulkan或公开v1整合，未覆盖MSAA/depth重建、OutOfDate/Suboptimal/分配或设备失败、多帧重叠resize压力、完整A11/A15、完整游戏人工/性能或双现代接管验收；不继承D3D12成绩关闭它们。下一项为R4-实现，须另确认生产实施范围，不因本次实验通过自动启动或批准T3。D3D12性能缺陷继续暂缓，GL保留；文档同步主目录供审核，代码及日常截图/命令/结果仍留E盘worktree，不新增附件、平行报告或归档。

最终runner CPU正/负例与Release三项定向架构CTest 3/3通过；worktree及主目录的本change严格validate均通过，本轮两处各13个新增本地/跨盘链接与锚点、diff及新源码空白检查通过，apply实际进度16/24。E03验证正文与任务状态已增量同步主目录，保留主目录已有审核说明，不复制代码或原始运行产物。未重跑全部公开头或D/R/CPU-only矩阵、游戏人工和性能采样；规划校验/链接检查不证明生产接管。
