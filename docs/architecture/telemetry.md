---
tags:
  - area/architecture
---

# Telemetry

`symocraft_telemetry`收集自有CPU记录、按身份回填GPU、统计导出并提供Data::Value YAML适配。[封闭/导出实现](../../game/modules/telemetry/src/performance.cpp)已随获批准的T4实现于2026-10-19同步F主工作区；当前生命周期契约见[telemetry-lifecycle](../openspec/specs/telemetry-lifecycle/spec.md)，文件发布风险仍按[foundation-files](../openspec/specs/foundation-files/spec.md)保留，不由源码同步关闭1175或恢复Fix1。

## 所有权与采样边界

Application拥有可选Session，早于被测装配、覆盖显式关闭和main Seal/Export；Scenario选择与该指针独立。Session拥有路径、frames/GPU索引/memory/invalid reasons/metadata/startup/FinalFacts/导出receipt，不借CLI、Window、World或GPU query。配置复制，Directory的const path引用仍仅在Session寿命内有效。

Session不可复制/移动，析构不Export/删除现场。metadata/startup/completed/entry私有，只读getter与SetMetadata/SetStartup/Complete受Recording检查；所有采样、GPU、Invalidate/Status写入口在Sealed后拒绝。晚到GPU仍串行Poll回填，不代表锁、异步或并发安全。

构造只创建全新输出目录，父目录必须存在，不覆盖已有结果；UTF-8 字节路径在此转 filesystem path。创建后 reserve 或 initializing Status 失败可能留下空目录/部分文件，不清场回滚。frames/gpu-resolved 预留 100000、帧上限 1000000；memory 预留 1300 但没有相同硬上限，现代 ID map 首次 SetRendererInfo 时预留。预留不是全程无分配保证，也没有全面重新验证 CLI 选项。

Frame 是自有值：elapsed 为秒，其余时间为毫秒；计数/上传为整数，GPU duration 为 optional。Memory 的 process working-set/private 为 bytes，device-dedicated/available 为 KiB 且是设备级估计，不是进程 VRAM。缺失写空值而非 0；有效 0 毫秒 GPU 样本仍保留。温度不由游戏自动采集，不能从空字段伪造观测值。

依据：[公开数据与 Session](../../game/modules/telemetry/include/symocraft/telemetry/performance.h)、[当前实现](../../game/modules/telemetry/src/performance.cpp)、[目标](../../game/modules/telemetry/CMakeLists.txt)、[main 组合](../../game/app/src/main.cpp)。

## 旧行号与现代身份

旧 GL `NextFrame` 是即将 Add 的行号；Add 成功后 SetGpu 才能回填已存在行。SetGpu 拒绝越界和负/非有限 duration，允许替换已有该行 GPU 值；不能把旧笔记的“未校验 ms”继续写作当前实现。

现代 Frame 的 `frame_id` 是 Renderer 稳定身份，独立于 CSV row。Add 拒绝重复 ID、非法预填 GPU/prepare/submit duration 与 duration 同时含 missing reason；SetGpuSample 按 ID 查行，可乱序/延后接收，但同一 ID 只能解析一次，必须给 duration 或非空 missing reason 二选一。未知/重复 ID 与不完整结果拒绝，不用行号猜映射；legacy SetGpu 仍是旧兼容入口，调用方不能混用它绕过现代一次解析约定。

Add 先在 strict 失焦时记录 invalid reason，再 push Frame，随后加 resolved 标记和 ID map；后两步分配失败会 pop 新 Frame/标记，恢复行数，但已经记录的 invalid reason 不回滚。一般 Frame 字段未在 Add 全面验证；有些非法值到 Export 的 Statistics 才暴露。SetGpuSample 的 missing string 拷贝可能失败，不能推定所有异常都是无分配操作。

SetRendererInfo 从真实 RendererInfo 复制 backend/device/driver/API、实际 framebuffer/samples/present、validation/capabilities 和 timing scope，不把现代后端标为 GL。局部构造值后仍连续写公开 metadata 并 reserve map；中途失败不具整个 metadata 的事务保证。AddStartupUpload 先验证 duration、溢出和相加结果再计算合计；metadata/startup 写入分配失败也不能推广为所有字段原子回滚。

现代 prepare/submit 采用 Renderer 返回的具名计时；prepare 包括门面校验/自有帧准备与后端准备（含 slot 等待），不能以旧 `pack+render-upload` 推导它们。scene-cache Synchronize 的 upload_cpu interval 包括扫描/自有转换/Create/Update/draw-list 组装，startup upload 独立归属，资源上传与 Render-local 上传各计一次。CPU 时间与延迟 GPU 样本可能对应不同到达帧，绑定必须按返回身份。具体边界由 app/renderer 生产者保证，不由 Session 测量硬件。

当前 GL 的 event 是 frame_start 至 Poll 返回（含首个 measured 帧的 sampling 发布）；simulation 含输入、编辑、角色/物理与交互，mesh 只包围 RebuildDirtyMeshes，pack 包围 Synchronize。instrumentation 包围 GPU Poll 与到期内存采样，不含随后 Add(Frame)。render_cpu 从 viewport/selection 准备至 EndFrame，upload_cpu 是其中显式上传的子区间；present 仅包围 Present 调用。`pack + render_cpu - upload_cpu` 只是一种 CPU 提交推导值，不是纯驱动/GPU 时间，也不是现代具名 submit。first_frame_swap_return 是入口至首次呈现调用返回的近似，不是肉眼首次可操作时间。

## 导出顺序与失败后状态

公开Status仅在Recording允许initializing/awaiting-foreground/warmup/sampling；私有WriteStatus负责exporting/finished。前台准备状态不能绕过Recording或提前宣布导出完成。closed status.tmp经真实Files::Publish发布，classic locale/precision10/checked close保留；system_error保原error_code加phase上下文。磁盘phase与内存RecordState/ExportState分开，不因summary存在推定Saved。

先结束所有关闭步骤，汇合自有FinalFacts再Seal。Seal校验completed一致，预构造最终事实/invalid reasons后一次提交；同事实重复无操作，冲突拒绝，内存失败不半封闭。Export只接受Sealed，顺序仍exporting→frames/focus→memory→summary→finished；中途保部分/临时文件，无目录事务或自动回滚。

finished 发布失败时 summary 可能已经存在且 completed=true；不能只看到 summary/CSV 就称成功导出。Files::Publish 的晚期 1175/1176/1177 失败没有通用源/目标回滚保证，状态须按 [暂停 Fix1](../openspec/changes/m3-t2-publication-fix1/proposal.md) 边界保留；本轮未诊断或修复根因。

valid_run要求runtime_succeeded、shutdown_succeeded、completed、无invalid reasons、measured非空；保存成功但无效是Saved/false，不等同I/O失败。FinalFacts保首错/次错，summary新增lifecycle不改旧列与协议。所有关闭事实确定后才Seal，不用析构补封闭。

Export返回自有ExportResult：Saved或Failed、valid_run、path/stage/diagnostic和原生error_code。Failed保持Sealed且不自动重试，下次显式Export才重写同目录；Saved缓存receipt，重复调用不截断/写文件，也不因文件被外部改变重做。stdio等标准I/O故障不一定带原生Win32码，完整OOM诊断不保证；不扩为通用错误框架。

## 统计、协议与 YAML

Milliseconds 用 steady_clock 两个 time_point 返回 double 毫秒，不修改时钟或记录；调用者给出的 end 早于 start 时可为负，不自动裁剪或生成有效采样。

统计仅选 measured 帧；warmup、原始慢帧、缺失 GPU 和 focus 记录仍完整输出，慢帧不剔除。Statistics 接收 vector 值副本并排序，不改原帧序/身份；非空样本逐项拒绝负/非有限值，nearest-rank 为 ceil(p*N)-1，p50/p95/p99，空集合只写 count=0。逐值校验不等于浮点累计和永不溢出；mean/throughput 的累计未提供额外溢出保证。

普通 GL 保持 v2 metadata 和 25 列 frames.csv。出现 renderer info 或现代字段时追加 stable frame ID、prepare/submit、missing reason、资源/帧上传和 calls；CSV 字符串带引号并转义内部引号。未返回现代 GPU 结果标 not-returned，旧无采集标 not-collected，已解释 missing 保留原原因；统计不以缺失填 0。focus 时长是按 poll 时焦点分类累加帧间隔的近似，不是硬件焦点时间线；frame_ms 也是连续呈现返回间隔，不是物理显示延迟。

原生Benchmark runner已支持旧GL v2/25列与D3D12/Vulkan modern v1/32列，实际消费者及发布边界见[benchmark](benchmark.md)；候选运行和批准分别见T3/T4验证，不从新增字段推定。schema/protocol不因T4生命周期扩成新性能协议或自动预算达标。

YAML parser 仅是 telemetry 私有依赖，公共边界仍为 foundation Data::Value。Dump 构造独立 YAML node/string，不修改输入；Mapping/Sequence 保持值结构及 flow style，能被解释为 bool/number 的 string 显式标为字符串。Load 同步复制 text，再返回自有值；显式字符串不重解释，其他 scalar 按 bool → int64 → uint64 → double → string 尝试，map key 转 string，null 为 monostate。LoadYamlFile 直接使用传入 path，不经 Assets resolver；文件/语法/转换异常传播，不返回半个成功文档。它不是业务 schema 校验器，也没有对任意文档分配量提供界限。

依据：[YAML 适配](../../game/modules/telemetry/src/document_io.cpp)、[YAML 公开入口](../../game/modules/telemetry/include/symocraft/telemetry/document_io.h)。

## 保留的不变量与验证边界

| 原 Session 编号 | 当前条件 |
| --- | --- |
| I1 | 构造成功需要新输出目录和已存在父目录，不覆盖已有结果；失败现场不自动删除。 |
| I2 | legacy SetGpu 只回填已存在 CPU 行；现代结果额外按稳定 ID 精确关联并一次解析。 |
| I3 | valid_run = 正常退出、completed、无 invalid reason、measured 帧非空，不由文件存在推定。 |
| I4 | GPU 缺失不填 0，warmup 排除统计，慢帧不剔除。 |

现有performance/renderer_performance协议测试保留，lifecycle补封闭后全部写入口拒绝、同事实/冲突Seal、失败显式重试与Saved无重写，真实finished Publish被无delete-sharing读者阻断的Win32共享错误32保留。正式库内窄私有probe只在冷路径发布前安排外部竞争，不代替Files::Publish。实际结果见[T4 verification](../openspec/changes/archive/2026-10-19-m3-t4-application-simulation/verification.md#2026-10-15-完整生产迁移)；不证明1175根因消除、任意OOM或并发安全。
