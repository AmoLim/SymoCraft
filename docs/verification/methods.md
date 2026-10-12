---
tags:
  - area/verification
---

# 当前验证方法

本页维护当前验证层次与共享 Benchmark/telemetry 方法，不保存批准或基准成绩。方法存在不等于已经执行；具体版本、结果、失败样本及关键节点只通过 [VERIFICATION](VERIFICATION.md) 查询。模块专用契约/测试入口按 [架构导航](../architecture/Overview.md) 进入，不复制各模块测试清单。

## 验证层次

- 静态文档检查：路径、锚点、双链、证据哈希与非文档差异，不证明程序运行正确。
- CPU/native 夹具：参数、统计、文件协议、进程管理和失败清理，不证明真实 GPU、干净机或玩法通过。
- 真实游戏短测：观察资产、窗口、实际设备、绘制、有限帧退出及采样/截图链路，不是正式性能基线。
- 正式基准：按确认的版本、工作负载、环境和计划采集原始数据；数据有效、预算达标、用户验收分别判定。
- 人工玩法与长时稳定性：按 [场景](scenarios.md) 执行，自动测试和少量截图不能替代连续操作。

[runtime 探针](../../scripts/verify-runtime.py) 的退出码 0 判定包含未超时、ready/loop/shutdown 标记、无 fatal 和零 GL 诊断；Frames > 0 还要求帧数精确相等，Frames=0 不传 smoke 限帧。退出码 3 只要求预期退出、ready 前失败及 shutdown；不是任意运行故障均已覆盖。[资产故障探针](../../scripts/verify-runtime-faults.py) 仅在独立副本注入 shader、图片、空方块配置三种故障，核对实际 GL 身份、指定诊断、退出码 3、清理标记及源资产哈希不变。副本只复制 EXE/assets、不复制运行 DLL，因此不能作为干净机部署证明；当前 D3D12 与任意异常安全也不由它证明。

## 选择采集入口

| 入口 | 当前行为与限制 |
| --- | --- |
| 原生 `SymoCraftBenchmark.exe` GUI/CLI | 固定三场景、1080p、seed 424242；显式 GL/D3D12，分别接受 25 列/现代 v1 的 32 列；默认 allow-unfocused，支持 attempt 重试与原生 ZIP |
| `scripts/benchmark.py` | collector 3，可选场景/时长/尺寸；不传 focus-policy，沿用游戏 strict；失焦帧判失败，可选 nvidia-smi |
| 直接游戏 `--benchmark` | 支持游戏已编译的后端及采样参数，但不是原生执行器的协议兼容或正式计划保证 |

Python 入口、原生协议和旧 PowerShell collector 2 的机器记录/结果结构不同，不把 JSON、YAML 或焦点策略混作同一协议。现有 `.ps1` 是 Python 启动兼容层，不再是独立采集实现。操作参数见 [工具 README](../../tools/benchmark/README.md) 与 [脚本 README](../../scripts/README.md)。

原生执行器默认 GL，D3D12 必须显式选择；未知版本、实际后端不符和现代数据伪装成 GL 明确拒绝。现代后端验证仍须按对应任务的版本、身份和预算执行，不能直接沿用旧 GL 成绩或宽松跳过协议检查。

## 数据有效性

原生快速协议每场景 10+10 秒、各一轮；正式协议每场景 60+180 秒、各三轮，共 36 分钟加额外开销。固定 framebuffer 1920x1080、VSync 请求关闭、seed 424242、441 区块。固定 [场景](scenarios.md) 的生成、工作负载与摘要身份需要一起保存，不能只存 seed。

原生校验必须包括：

- 退出码 0、completed 与 valid_run 为真、invalid_reasons 为空，摘要 schema/protocol/workload 均为 2。
- 场景、焦点策略、时长、尺寸、VSync、seed、区块数量及固定 terrain/scene digest 匹配；实际 GL 设备/版本或现代 renderer 身份/实际配置/计时范围完整。
- memory/focus CSV 存在且非空，status 的 protocol_version=2 且为 finished；frames CSV 按所选后端匹配 25/32 列、帧号连续、阶段和时间顺序合法、帧数不超一百万。
- 正式样本非空，最后帧区间覆盖协议时长减 0.5 秒的容差；帧数、编辑计数、焦点帧数与估计秒数和摘要一致。
- 从原始正式帧重算 P50/P95/P99、均值与吞吐量，nearest-rank 为 `ceil(p*N)-1`，不剔除慢帧。
- 现代扩展的 uint64 frame ID 精确递增，CPU prepare/submit 和上传归属一致，GPU 空值与缺失原因互斥，正式帧 GPU 有效/缺失及上传摘要吻合。
- 同组游戏及所选 shader/公共资源 SHA256 不变、实际设备身份不变；重试先复核原通过轮次，不覆盖或拼接失败 attempt。

`ValidateCapture` 不验证每一列的全部语义，也不校验 PNG 图像内容；`final-frame.png` 是诊断证据，视觉正确性仍需查看。通过上述规则不等于 CPU/GPU 温度稳定、完整资源树身份相同或画面与另一 backend 相同。

两种焦点策略都拒绝最小化或尺寸改变。allow-unfocused 的失焦样本保留且仅作分析条件；它不隔离后台竞争、驱动限帧、遮挡呈现或系统睡眠。strict 的旧失败保持原判定，不能因新策略追认为有效。

## 观测范围与比较

帧时是连续呈现返回间隔，FPS 是吞吐量，不是物理显示延迟。预热帧不进入正式统计，但保留在 CSV。GPU 时间缺失不补 0，不与 CPU 区间简单相加。

当前 app 在首个 measured 帧的 frame_start 之后、Poll 之前一次性发布 sampling 状态；文件发布开销可进入该帧的 event_ms 及帧区间，未从原始数据扣除。阶段文件不是独立于测量的无成本操作，也不把该边界帧当作可以删除的异常值。

GL GPU 时间为 draw 周围的 GL_TIME_ELAPSED，排除显式上传、clear、swap；旧 CPU submit 推导值与现代 Renderer 返回的具名 prepare/submit 不是同一测量，prepare 含门面与后端准备。现代时间戳排除 clear、resolve、截图 copy 与 Present；缓存扫描/复制/Create/Update 的 CPU 区间单独计为 upload，不再通过相减构造 submit。

失焦秒数是按每帧事件轮询后焦点状态分类累加 frame interval 的估计，不是精确事件区间。内存是进程 working set/private bytes；NVX 显存为可选设备级驱动估计，不是进程 VRAM。原生工具不采集 CPU/GPU 温度，Python 的可选 nvidia-smi 是外部设备级传感器，缺失或引入的采集开销照实记录。

逐轮比较相同 game/assets、工作负载、焦点策略、构建、画面/采样参数与实际设备环境；不同机器/版本/策略不混成一个分布。保留编辑慢帧、最大帧和缺失 GPU 数量，不只看平均 FPS。优化对照说明被测因素及各版身份，不能用新数值倒定通过线。

设备角色、预算与后续进入门槛以 [项目范围](../scope/project-scope.md#性能目标与测量约束) 为准：旧冻结基准不能替代新候选在指定中低端整机上的性能验证，低端硬阈值须先确认。本页不复制阶段批准状态或冻结成绩。

## 结果保存与测试入口

每次运行使用新目录，不覆盖失败现场。结果统一记录于对应 change 的 verification.md，注明必要版本/配置、方法、实际结论、失败与未覆盖项；默认无附件，无特别要求不附原始数据，不另建逐步报告或归档包。特别要求和旧冻结保护按 [治理规则](../DOCUMENTATION_MAP.md#每个-change-一份验证记录) 执行。
正式协议中已明确要求的完整逐帧数据留存及复算不因默认规则取消，精选摘要不能冒充可复算原始数据；其余原始工作产物不自动成为长期归档附件。旧冻结内容、当前候选和暂停任务现场不在本次规则修改中清理。

在已配置的实际构建目录运行 `ctest --test-dir <目录> --output-on-failure -R "benchmark\.|performance\.export|performance\.script|telemetry\.renderer_performance"`，不要把旧目录的通过记录当成当前源码结果。native fixture、Python 和可选 PowerShell launcher 用例定义见 [Benchmark 测试配置](../../test/benchmark/CMakeLists.txt)；现代 frame ID/缺失原因/上传归属的 CPU 用例见 [telemetry 测试配置](../../test/unit/telemetry/CMakeLists.txt)，不产生真实 GPU 结论。

本页的方法与源码核对不代表已经执行游戏、GUI、GPU、干净机或正式 36 分钟基准。当前未覆盖项及执行结果见对应 change 的 verification.md，关键节点批准和导航见 VERIFICATION。

## 模块与交接验证

配置、编译、CPU契约、真实SDL/原生GPU、人工玩法、性能和用户批准分别留结论。工程矩阵/公开消费者/负向边界/增量及安装依据见 [engineering](../architecture/engineering.md#测试及验证边界)，窄模块回归及实际失败状态见对应模块末节，不以历史测试数量推定本次全部通过。
World 分配故障扫描要核对配置：普通MSVC Debug的禁用项不是通过，Release与专用`_ITERATOR_DEBUG_LEVEL=0`结果各具身份；C++ new计数不代表全部内存。ECS旧存储/allocator、PNG写入和多文件导出的失败边界不可由局部绿测试扩为事务保证。
现代后端必须分别核实真实device/driver、完整资源/呈现及在途同步/故障；recording backend、短测、静态审查和另一API不补真实缺项。共享人工身份M2-01至M2-14及ST-60/ST-RESTART见 [场景](scenarios.md#玩法与恢复观察)。
文档迁移额外检查单一权威、source→section、旧输入的实际消费者、生成索引与保护哈希。隔离安装只证明文档规则/输出字节，临时捕获只证明清单/复制，不证明原生产品编译或无开发环境部署。迁移工具全绿仍需语义复核，不自动删除、批准、sync或archive。
