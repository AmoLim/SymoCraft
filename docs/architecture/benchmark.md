---
tags:
  - area/architecture
---

# Benchmark 与采样契约

`tools/benchmark` 是独立 Windows x64 原生工具，负责固定计划、进程管理、结果校验和导出；不链接游戏生产模块、SDL 或 GL，不直接访问世界、窗口、渲染器或 GPU 查询。游戏经命令行接收工作负载，经文件协议交付观测。工具仅复用 yaml-cpp，生产者使用 telemetry 的 CPU 数据与文档接口。

操作见 [工具 README](../../tools/benchmark/README.md)，数据有效性与比较方法见 [验证方法](../verification/methods.md)，固定输入见 [共享场景](../verification/scenarios.md)。基准、失败样本和批准只由 [VERIFICATION](../verification/VERIFICATION.md) 索引，不在此维护成绩或批准表。

## 模块边界与协议

公开工具边界是 [runner.h](../../tools/benchmark/include/benchmark/runner.h) 中的配置、进度、结果、`Run`、`ValidateCapture` 与 `Export`。`Run` 和 `Export` 同步执行；观察回调在调用线程执行，调用方负责其寿命与异常边界。GUI 传配置副本和 stop token，不能在运行中修改本轮参数。

主目录既有原生执行器明确选择GL/D3D12；refactor-renderer工作目录候选新增GUI/CLI `--renderer opengl|d3d12|vulkan`，仍默认GL，无静默回退。以下新增机制指[E盘候选runner](E:/Codex/Worktrees/refactor-renderer/Symocraft/tools/benchmark/src/runner.cpp)，文档同步不表示主目录源码已合并或R4已批准。摘要 schema / protocol / workload 均为 `2`，会话 `runner_schema: 1`；旧 GL 保持既有 25 列与 `gl_renderer/gl_version` 身份，现代数据只接受 32 列及 `renderer_telemetry_version: 1`。未知版本、请求与实际后端不符或现代数据混入 gl 字段均明确拒绝。

候选的现代验证默认仍开启；GUI复选框/CLI `--validation 0|1`允许显式选择，实际renderer记录须与请求一致，缺验证层不自动关闭。GL拒绝该覆盖项；省略时保留旧设置形状。普通Vulkan运行不需DXC/SDK；显式开启验证仍需合法验证层，不能将关闭验证的部署检查称为A16诊断通过。

候选构建在四份SPIR-V旁生成`build.json`，保存Debug/Release、Vulkan目标/Shader Model、实际编译参数、DXC可执行文件及dxcompiler模块哈希、三份同源HLSL/include和四份二进制哈希，不嵌本机路径。编译规则将编译器/模块列为shader依赖，身份产物依赖实际二进制，安装一并部署。GameRenderer只在Vulkan采样Session记录生成身份，运行不加载编译器；工具在包身份中收录清单、校验每个SPIR-V哈希，并在新轮次及resume核对报告身份与安装包相符。独立ValidateCapture仅验证协议/身份格式，不能在无包路径时证明来源；CPU夹具的伪编译器身份不冒称实际生成。

现代消费者逐行校验精确 uint64 frame ID 的递增（不转 double）、CPU prepare/submit 归属、GPU 值/缺失原因互斥及分离上传计数，并复核正式帧摘要。GPU 缺失保持空值与原因，不补零；result 保留实际后端/设备/驱动/API/配置、范围和 CPU/GPU/上传摘要。现代 GPU 数据的身份与计时边界须与 [Renderer 契约](renderer.md) 一并理解；历史 GL 冻结包不能重标为现代后端结果。

| 文件 | 权威生产者与用途 |
| --- | --- |
| `session.yaml` | 执行器的计划、身份、各轮最新 attempt、连续性与整组结果 |
| `<scene>-<repeat>-attempt-<n>/result.yaml` | 执行器的该次退出、取消/超时、校验结果与错误 |
| `stdout.log` / `stderr.log` | 子进程原始诊断，不替代退出码及结构化校验 |
| `capture/status.yaml` | 游戏阶段发布；只用于近似进度与终态确认 |
| `capture/summary.yaml` | 游戏观测摘要、完成/有效性、无效原因及计时范围 |
| `capture/frames.csv` | 全部预热与正式帧，包括慢帧、失焦和缺失 GPU 值 |
| `capture/focus.csv` / `memory.csv` | 帧边界焦点变化与约每秒的可选内存观测 |
| `capture/final-frame.png` | 测量后的额外渲染/读回，不计入采样帧 |

阶段顺序为 `initializing -> warmup -> sampling -> exporting -> finished`。状态文件在同目录临时写入后原子替换，读取句柄允许替换；不存在或暂不可读的阶段文件不代表成功。执行器每 250 ms 轮询，用阶段时间加本地经过时间估算进度，不能用进度条反推帧统计。

## 会话、失败与重试

快速计划为三场景各 10 秒预热、10 秒采样；正式计划为三场景各三轮，每轮 60 秒预热、180 秒采样，共 2160 秒协议时间。启动、校验、截图与落盘另计，不自动缩短计划。两种计划不能互称正式基线。

新会话和新 attempt 都拒绝覆盖已有目录。`session.lock` 不继承给子进程，排他句柄限制会话的并发执行/导出，进程退出后锁释放。执行器保存游戏与所选资源的SHA256：GL是公共YAML/PNG加四项GLSL，D3D12是公共YAML/PNG加四项DXIL；候选Vulkan是公共YAML/PNG加四项SPIR-V及生成清单，共七项资源。轮次前后复核身份，同组复核实际GL设备/版本或现代renderer身份；重试也重新校验已通过轮次。这个资源白名单不等于整个资源树都被纳入身份；旧GL会话设置形状不因新增默认选择而改变，现代会话显式记录renderer。

每轮总时限为预热加采样加 120 秒。失败、取消或超时停止后续轮次，保留已有 attempt。重试只接受同配置和同包，先重新校验已通过轮次，从首个未通过轮次新建 attempt 并完整预热，不拼接中断帧。`continuous_full_protocol` 仅表示无重试的正式九轮完成，不表示环境安静、Release 构建、性能达标或用户批准。

游戏退出码 `0/2/3/4/64` 分别为正常有效、资源预检失败、运行或导出异常、无效采样、参数错误；执行器 `0/4/64` 分别为成功、已记录采样失败或取消、参数或外层操作错误。磁盘故障等外层异常不保证最终报告可写出；已有文件不删除，运行中的自有进程仍按 Job 寿命收尾。

## 所有权与线程约束

GUI 的窗口、GDI 与控件只在 UI 线程操作；最多一个 `std::jthread`，重用前 join。进度快照持 mutex 复制后再更新控件，不在锁内执行文件或图形工作。最终 outcome/error/exported 经 `finished` release/acquire 交接并 join 后读取。App 地址覆盖 HWND 绑定与全部回调；忙时关窗仅请求停止，工作完成才销毁窗口，析构先停止并 join 再释放字体。保留原 App 不变量 **A1-A7**，不把 stop request 当作子进程已经退出。

Win32 Procedure 在 Message 分发边界捕获 `std::exception` 并显示错误，WM_CREATE 失败返回 -1，其他受捕获错误返回 0；这不是任意异常、错误显示再次分配失败或 noexcept 回调的兜底保证。历史固定布局尺寸与当时的 GUI 验证缺口只留归档，不作为当前界面实测。

Process 独占 Job、进程及局部句柄，不可复制/移动，由一个 Run 工作线程操作。保留原 Process 不变量 **P1-P5**：挂起创建、先绑定 `KILL_ON_JOB_CLOSE` Job 再恢复；仅继承显式 stdin/stdout/stderr；Stop 只向自有 PID 窗口发 WM_CLOSE；退出后才使用最终退出码；异常与析构释放自有句柄。Job 绑定失败会终止挂起子进程并报错，不降级为无约束启动。取消先等 3 秒，必要时终止自有 Job，再最多等 5 秒，不按名称扫杀其他游戏。日志 `CREATE_NEW`，路径以 UTF-16/CRT 引号规则传入，不经 shell。

Session 由游戏 main 的 unique_ptr 独占，Application 借用；所有访问在游戏主线程，非线程安全容器。GL/native 资源清理在纯 CPU 导出之前完成。保留原 Session 不变量 **S1-S6**：新目录、帧追加与正确身份匹配、固定焦点策略且不删失焦帧、正常完成且有正式样本才有效、主线程访问、CSV/摘要写完才发布 finished。无效原因去重并单调保留，completed 不等于 valid_run，最多保存一百万帧。

当前 Session 仍可变，未实现封闭、成功幂等或显式重试状态机。`Session::Export` 逐个截断写 CSV/摘要，失败不回滚，可能留下部分文件或此前摘要，也不保证发布 finished；单个 status 原子替换不是整组导出事务。T4 的 Seal/重试/幂等只是目标设计，不能作为当前失败保证。

现代采样以稳定 frame ID 映射 CPU 行，不用返回顺序猜测 GPU 身份。重复 ID、未知 ID、重复现代 GPU 结果及“时间与缺失原因同时存在”等输入显式拒绝；未返回 GPU 时间保留缺失原因，不补 0。帧与映射插入失败会撤回该次容器追加，但已发生的失焦无效原因仍保留。启动上传单独归属，正式帧区分成功缓存资源上传与 Render-local 上传，避免重复计数。

## 焦点与导出边界

候选游戏可选--foreground-ready-timeout-seconds 1..120仅用于strict benchmark，默认0且allow-unfocused不接受。初始世界/mesh后，以Poll/Capture/有界Wait处理窗口事实，连续1秒获焦/非最小化/实际尺寸匹配后清输入与物理余数、重设帧时钟，再从零完整预热；超时/Esc/关闭具名失败并保留清理/导出。准备期间不模拟/Render，耗时单列startup，不是删除已测失焦帧或启动预算；计时后失焦仍单调无效、不暂停/重置/择帧。Python前台入口显式设置60秒准备，桌面激活仍由用户授权Computer Use完成，短验证与完整对照分别见[T3记录](../openspec/changes/m3-t3-renderer/verification.md#2026-10-17-前台有界准备与短启动验证)。下文“不暂停时钟”均指预热时钟已开始后，默认及旧allow-unfocused路径不变。

前台Python入口仅隐藏控制台（CREATE_NO_WINDOW），不传STARTF_USESHOWWINDOW/SW_HIDE去覆盖GUI首次显示；这不保证Windows允许激活或桌面已解锁，实际逐帧焦点仍是有效性依据。只读WTS的UNLOCK状态也不证明游戏已经前台，必须分开核验；准确状态见[T3获焦记录](../openspec/changes/m3-t3-renderer/verification.md#2026-10-17-已解锁但前台激活失败)，失败不自动重试。

独立Python对照入口verify-renderer-comparison.py仅在refactor-renderer工作目录，复用既有预算分析与原生消费者；--focus-mode foreground选用游戏strict启动激活并只读核验自有PID窗口前台、可见、非最小化及尺寸，桌面点击另由用户授权的Computer Use执行，脚本不持续抢焦或改变系统设置。游戏预热/采样时钟不因外部等待暂停；strict逐帧失焦无效原因单调保留，即使后来获焦也不能删除预热坏帧。新入口前台轮要求全部原始帧focused=1，失败停止后续、不自动重试；默认allow-unfocused和既有原生执行器不变。独立GL→D3D12→Vulkan对照按同场景/轮次分别配对，不混池或以旧失焦GL充当前台基线；本批30＋90/三轮的授权、锁屏未完成及后续状态见[T3验证](../openspec/changes/m3-t3-renderer/verification.md#2026-10-17-前台三后端采样准备与锁屏中止)，不是GUI产品默认或性能通过。

游戏默认 `strict`，原生执行器显式默认 `allow-unfocused`；普通非 benchmark 参数不接受焦点策略。两策略都保留原帧，strict 失焦记录无效原因，allow 不把仅失焦判为失败。最小化、零尺寸和帧缓冲改变仍令采样无效，不暂停后续拼接；普通游戏失焦暂停和输入恢复语义不因此改变。采样恢复焦点不重置物理时序，负载按真实经过时间推进，不承诺玩家轨迹位级确定。

导出创建独立副本，固定场景/attempt 白名单枚举，不信任文件内任意路径，拒绝 reparse 路径。仅在导出 YAML/log 副本替换已知本地路径，原文件保持不变；副本哈希与原文件哈希不可混用。ZIP 调用系统 tar 且有限等待，失败保留导出目录，无上传、自动装工具或系统设置修改。路径替换不是通用匿名化；手填信息与第三方错误仍需检查。

实现依据：[runner](../../tools/benchmark/src/runner.cpp)、[原生平台](../../tools/benchmark/src/platform.cpp)、[GUI](../../tools/benchmark/src/gui.cpp)、[Session](../../game/modules/telemetry/src/performance.cpp)、[游戏主循环](../../game/app/src/application.cpp) 与 [GameRenderer](../../game/app/src/game_renderer.cpp)。本页经源码核对，不代表本次已运行 GUI、游戏、GPU 或正式采样。
