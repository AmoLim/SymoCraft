# 性能观测与采样数据流

## 目标与非目标

M2-T3 建立能够解释 CPU、网格、上传和 GPU 成本的最小观测链路，不在采样时修改网格算法、视距或世界大小。当前仍是生成版本 1、441 区块，最多 361 区块绘制；全世界有效网格仍每帧装入批次并上传。观测不是优化，也不表示 M4 性能目标通过。

主要入口：[CPU 采集与导出](../../src/core/performance.cpp)、[应用编排](../../src/core/application.cpp)、[GPU 查询环](../../src/renderer/gpu_timer.cpp)、[批次计数](../../include/renderer/batch.hpp)、[内存与截图](../../src/renderer/performance_memory.cpp)、[启动与多轮采样](../../scripts/benchmark.ps1)。运行方法见 [双机采样指南](../testing/performance-baseline.md)。

## 数据流

```text
benchmark.ps1: 新输出目录、机器条件、exe 哈希、可选 nvidia-smi 监测
  -> main: 参数检查 -> Session -> Application::Init / Run
  -> 固定配置世界与夹具 -> 初始化分段计时
  -> 帧循环:
       事件 -> 固定物理/脚本工作负载 -> 射线显示
       -> 脏区块 CPU 网格 -> CPU 批次装入
       -> 回收已就绪 GPU 查询、按 1 秒间隔采内存
       -> 渲染（实际上传计数 + 绘制区间 GPU 查询）
       -> SwapBuffers -> 保存 CPU 帧记录
  -> 到期停止 -> 最后一次非阻塞查询回收
  -> 采样外生成最终摘要与额外一帧截图 -> 清理 GL/世界
  -> 导出 CSV、YAML -> 采样脚本核对日志并生成 results.json
```

采样模式忽略外部游戏移动、鼠标编辑及视角输入，但保留 Esc 和窗口关闭。它不发送系统键鼠事件，也不控制其他应用。普通游戏的输入与失焦暂停行为保留；性能采样中失焦不会悄悄暂停时间，而是保留帧并令整轮无效。分辨率变化或最小化会中止并保留部分结果。

## 帧计时口径

所有 CPU 区间使用 `steady_clock`。CPU 标记测量的是调用线程的墙钟耗时，包括可能的调度和驱动等待，不等于 CPU 核心实际执行时间。模拟仍沿用现有 `delta_time` 夹到 0..0.1 秒的规则，不能把它当成真实性能帧耗时。

| CSV 字段 | 边界与意义 |
| --- | --- |
| `frame_ms` | 相邻两次 `SwapBuffers` 返回之间的真实时间；包含帧末记录开销，不截断长帧，不是显示器实际呈现延迟 |
| `simulation_delta_ms` | 本次送入模拟的截断后时间；与 frame_ms 分开记录 |
| `event_ms` | 本帧开始至窗口事件轮询完成 |
| `simulation_ms` | 游戏/脚本输入、Transform、角色、物理、相机和射线；连续编辑写入也在这里 |
| `mesh_ms` | `UpdateAllChunks`，包括遍历与本帧真实 CPU 网格重建 |
| `pack_ms` | `LoadAllChunks` 把有效区块顶点复制到批次的 CPU 成本 |
| `render_cpu_ms` | 纹理绑定、清屏、shader 状态、上传及绘制提交；包含 GPU 查询提交，不代表 GPU 完成 |
| `upload_cpu_ms` | 实际 `glNamedBufferSubData` 调用的 CPU 耗时之和，是 render_cpu_ms 的子集，不是 PCIe 传输或 GPU 复制时间 |
| `present_ms` | CPU 调用 `SwapBuffers` 到返回的等待，可能含驱动/合成器/呈现等待，不冒充 GPU 绘制时间 |
| `instrumentation_ms` | 查询结果轮询、内存采集与本帧查询槽选择；不包含所有时间戳和记录写入开销 |
| `gpu_draw_ms` | 本帧所有非空批次 DrawArrays 的 GPU 计时间隔之和，不含显式上传、清屏或交换缓冲；缺失为空单元格 |

汇总中的 `cpu_submit_excluding_upload_ms = pack_ms + render_cpu_ms - upload_cpu_ms`，避免把上传重复相加。CPU 与 GPU 可以重叠，不能把两者相加声称是帧耗时。

逐帧还记录重建区块数、实际绘制顶点数、上传字节数、绘制调用数、编辑操作数/调度延迟、玩家位置、yaw、焦点状态。批次只有真的执行上传/绘制才递增计数；第二次调用已干净的 `ReloadData` 不重复计算。上传字节不代表显存占用，预分配 VBO 容量也不等于每帧有效顶点数据。

## 启动口径

`startup_ms` 分别记录参数/存在性预检及采集器设置、窗口/上下文、shader/缓冲/方块 YAML、纹理解码与上传、区块分配、地形、植被、夹具/摘要/元数据、首批 CPU 网格及首次上传 CPU 耗时。首批网格数和首次上传字节放在 metadata。

`ready_from_main_entry` 是世界和首批 CPU 网格就绪，尚未完成首帧绘制；`first_frame_swap_return_from_main_entry` 是从 main 入口到首帧交换返回，作为本项的首次可操作画面时间近似。这不是物理显示器呈现时间或真实输入到画面响应延迟，benchmark 本身也不接受普通操作。操作系统启动进程到 main 的时间不在此区间，外部脚本的进程总墙钟时间另列。

摘要扫描与测试平台安装单列，不能混入“地形算法耗时”。各分段不保证恰好加总到启动总时间：ECS、日志、验证和采集器初始化等额外开销仍存在。原有 `loading_ms` 保留兼容，不用它替代分段结果。

## GPU 查询与所有权

`GpuTimer` 只由持有当前 GL 上下文的主线程访问，拥有 64 个槽、每槽最多 2 个查询，对应当前世界和选择框两个非空批次。在批次上传之后围绕绘制调用使用 `GL_TIME_ELAPSED`。后续帧先检查 `GL_QUERY_RESULT_AVAILABLE`，仅就绪时读取 64 位纳秒结果，以原帧编号写回 CPU 记录。

槽未就绪时不复用；环满时该帧 GPU 数据缺失，而不是覆盖旧结果、返回假零或等待。没有逐帧 `glFinish`、`glFlush` 或忙轮询。末尾只再轮询一次，未完成的尾部结果保持缺失，并报告 `gpu_missing_sample_frames`。不支持有效查询位数时同样标缺失。析构在上下文销毁前释放查询。

查询环解决的是异步结果的生命周期：CPU 第 N 帧结束时，GPU 未必完成第 N 帧；必须用帧 ID 关联延迟返回值，而不是把最新拿到的数填到当前帧。它不是线程池，也没有让 GL 调用跨线程。行为依据 [Khronos ARB_timer_query](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_timer_query.txt)。

## 内存、温度与截图

`memory.csv` 在主循环内约每秒采一次，记录进程 working set 和 private bytes。缺少系统读数时留空。`GL_NVX_gpu_memory_info` 可用时记录设备 dedicated/available KiB，属于驱动估计的显存容量/可用量，不是本进程精确驻留显存；不能把设备用量全部归于游戏。VBO 请求分配字节另外记录，不混同进程总 GPU 内存。扩展含义见 [Khronos NVX_gpu_memory_info](https://registry.khronos.org/OpenGL/extensions/NVX/NVX_gpu_memory_info.txt)。

脚本可启动独立的 `nvidia-smi` 每秒记录所有 NVIDIA GPU 的 UUID、型号、驱动、温度、功耗、频率、显存和利用率；报告必须对照实际 GL renderer，混合显卡不能只看显示控制器列表。采集工具缺失、字段不支持或进程提前结束均保留说明和 stderr，不编造数据。CPU 温度当前缺失，没有安装传感器驱动；精确每进程 VRAM 也未提供。后续需要时再引入明确的系统工具及测量协议。

采样到期后额外渲染一帧，再读 framebuffer 并用仓库现有 `stb_image_write` 保存 PNG。这个同步读回和图片编码在采样区间之外，不污染被报告的帧分位数。截图证明终态画面，不证明整段动画、玩家输入或 15 分钟玩法通过。

## 工作负载版本 1

| 模式 | 冻结输入 |
| --- | --- |
| static | regression / four-chunk，seed 默认 424242；静止，无编辑 |
| walk | regression / spawn；真实角色和物理步行，速度 4.4；x=-20.5..20.5 往返，端点切换 yaw=0/180，pitch=-10；z 初始 -6.5 |
| edit | regression / four-chunk；M2-T2 的 16 步写入，接着按逆序撤销 16 步，循环；每 0.25 秒计划一次 |

移动按真实位置到端点后转向，可能存在一帧的轻微越界和下一帧端点校正；不同机器不会产生位级相同的物理轨迹，原始位置/yaw 用于检查路程和偏差。没有通过相机传送替代正常移动成本。

编辑按单调墙钟调度，长帧错过的操作在后续帧全部补做，记录实际操作数与最大迟到量，不为了平滑结果跳过编辑。它通过真实方块写入、脏标记、CPU 重建及原有全批次上传路径，但不模拟鼠标射线点击或人为走到每个目标；用于构造可重复的编辑负载，不是交互验收。

预热期间也执行对应负载，采样开始不重新生成世界或清除慢帧。按本帧开始的 elapsed 划分 warmup/sample；边界帧完整保留，区间长度可能差一个帧间隔。配置/脚本变更应提升 workload/schema 版本并重新冻结对照，不能与旧协议无条件合并。

## 导出、错误与测试

采集器拥有 CPU 帧/内存数组，预留 100000 帧容量、上限 1000000；超限抛错并尝试导出已有数据，不静默抽样。增长、计时和查询本身有开销，已被真实帧时间包含；此版本未测量“完全关闭采集”的开销差，不能声称零扰动。CSV 写盘、分位数排序在主循环结束后执行。

结果目录必须不存在且父目录已存在，拒绝覆盖旧数据。`frames.csv` 保留预热及所有采样帧；`summary.yaml` 的均值、P50/P95/P99 使用 nearest-rank，即排序后 `ceil(p*N)-1`。空样本只报告 count=0。FPS 为帧数除以实际累计帧时间，不平均逐帧 FPS。GPU 分位数只基于真实可用值，同时报告缺失数。

受控异常保留部分 CSV/YAML并标无效；提前关闭、失焦、最小化或分辨率变化不算完成有效基线。新增退出码 4 表示采样完成导出但质量或时长不满足要求；其余 0/2/3/64 保持原意。强杀、系统崩溃或磁盘写满可能没有完整 CSV，脚本日志仍应保留，不能补写成功。

`performance.export` 测 nearest-rank、预热排除、编辑慢帧保留、GPU 缺失、目录保护及部分导出；`performance.gpu_timer` 用 GL 替身验证就绪判断、环满不覆盖/不阻塞、帧 ID 和释放。`world.generation` 追加正向/逆向循环编辑和时钟边界测试；启动参数测试覆盖冲突组合。真实驱动、PNG 和传感器仍由短测补充，单元测试不冒充真实 GPU 验证。
