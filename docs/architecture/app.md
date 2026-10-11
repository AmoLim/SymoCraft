---
tags:
  - area/architecture
---

# App 组合与运行

`game/app` 是私有组合根，不是下层模块的服务定位器。公开行为引用 [module-boundaries](../openspec/specs/module-boundaries/spec.md)、[platform](../openspec/specs/platform/spec.md)、[world](../openspec/specs/world/spec.md)、[simulation](../openspec/specs/simulation/spec.md) 和 [renderer](../openspec/specs/renderer/spec.md)。
当前仍是 namespace `Application::Init/Run/Free` 与静态运行状态，不是实例化 Application/SimulationSession；目标及 Q01-Q04 只在 [T4](../openspec/changes/m3-t4-application-simulation/design.md)。节点与历史结果唯一从 [VERIFICATION](../verification/VERIFICATION.md) 判读。

## 所有权与寿命

main 拥有 `StartupOptions` 和可选 Performance::Session；记录覆盖装配、运行、Free 与显式 Export。参数中的 checkpoint/benchmark/output_directory/focus_policy/renderer 等 `string_view` 借 argv 或静态字面量，参数存储必须覆盖使用；转成自有 SessionConfig 不延长原 view。
Application 静态 unique_ptr 拥有 Window、Registry、Camera、GameRenderer；Camera 借 Registry，GameRenderer 借 Window。玩家 ID、选材/输入状态与放置冷却也在 app 保存，物理余数仍属于 simulation 的共享预算。
pending BlockDefinition 在 Init 取得，Run 校验图集层后移动到局部 PreparedWorld 并 reset。PreparedWorld 独占 VoxelWorld，同时持有 Pose/报告值；Pose 名字借静态字符串。World、夹具、物理、射线、编辑、摘要与网格都用这一实例，至 Run/summary 返回或异常展开自动销毁。

Init 先确定后端/WindowMode，再取得本模块 SDL 责任并立即设置清理标志，依次建立窗口、Registry 注册、Camera/GameRenderer、定义及玩家。重入 Init 拒绝；失败可能已取得部分对象，由 main 的 Free 清理，不提供失败后续跑或任意状态回滚。
Run 要求上述 owner 和未消费的定义全部存在；成功消费定义后不能直接再 Run。PrepareWorld 的候选 RAII 不等于 Session 字段事务：后续失败前写入的启动元数据可保留。PrintWorldSummary 不创建窗口、Renderer 或 CPU 网格，但仍加载玩法定义/世界数据。
全部装配、模拟、缓存与清理在主线程串行、非重入，不提供多窗口、多会话或后台 GPU 交接。未验证同进程反复 Init/Free 的旧 ECS 全局类型状态，不把重复独立进程测试扩为此保证。

## 帧策略与提交顺序

加载/首网格结束后清共享物理余数，才开启帧时钟。帧顺序是 Poll/关闭与实际尺寸事实 → 暂停策略 → 规则时间/冷却 → Capture → 普通输入或 benchmark 场景 → Transform/角色 → 物理 → 安全出生点恢复/相机同步 → 普通射线请求及 World 提交 → 重建 → 缓存/绘制 → 观测。
普通有序指针逐事件应用，键盘映射只传意图；不重复使用指针汇总值。普通编辑在模拟后，benchmark edit 在模拟前；基准预条件错误抛出而非悄悄跳过操作。当前 benchmark 是否启用仍由非空 Session 分支决定，尚未分离 Scenario 与记录。
普通无限帧玩法失焦、最小化或零 drawable 暂停：清旧输入/运动/待跳、水平速度及物理余数，WaitEvents 有限等待；恢复首帧规则 dt=0。同轮失焦再获焦也处理清理事实。普通有限帧诊断只豁免失焦暂停，不豁免零尺寸/最小化绘制。
普通规则 float 时间由实际差值 clamp 到 [0,0.1] 后转换，物理预算另最多 8/120 秒；不是 T4 的全量预检/Rejected/Failed 会话保证。玩家低于 y=-8 时恢复 World 出生点、清力/接地/跳跃/预算。每轮持有组件引用期间不能扩容或结构修改 Registry，实际 ECS 风险见 [ecs](ecs.md)。
benchmark 使用实际经过时间调度原 static/walk/edit，不被玩家键鼠修改；本窗口 Escape/关闭仍能取消。实际尺寸改变/最小化使记录无效，allow-unfocused 不豁免。Poll 计入 event_ms，Capture 在 simulation_ms 中，不挪计时点制造收益。

## 两条渲染路径

`GameRenderer` 私有 PImpl 将同一玩法循环连接到两条真实路径。游戏默认 legacy OpenGL；编入且显式选择 D3D12 才使用 Native Window 和现代 Renderer。没有 Vulkan 工厂、热切换或失败 fallback，门面默认 D3D12 不改游戏默认。
GameRenderer 独占现代 Renderer、port/cache、图集句柄、draws 或旧 timer；port 只借门面且其寿命覆盖 cache 使用/释放，World/Camera/Session 仅同步借用。普通无记录现代帧仍非阻塞清空有界 GPU 结果队列。
现代每帧只调用一次 Renderer::Render，由其唯一 Present；app 不额外交换 Native 缓冲。旧 GL 继续每帧 VisitMeshes→AppendMesh→Render→独立 Present，不继承现代持久缓存和 Failed/Stopped 强保证。
现代 Resize 在帧边界使用 Window 的实际 drawable extent，最小化传零；调用成功后才更新本地 extent。相机使用真实 getter、度转弧度、near=0.1/far=2000，不复制 GL 投影到现代接口；旧投影在 app 按实际 aspect 派生。
现代真实图集必须 512x512、64 个 64x64 tile，底行优先层号、层内 top-left RGBA8、RGB 补 alpha=255、Linear。World 顶点已是 world-space，model 恒等；自有化时只作一次 V=1-V，shader 不再翻转，不裁剪 repeat UV 或混层三角形。
资源预检按选中路径检查：legacy 必需项只要求 regular_file，D3D12 必需项还检查非零大小；不初始化 GPU、不证明解码或 shader/PSO 正确。新 RenderError 保类别/后端/操作与原生诊断，具名向上退出，不转普通 World::Rejected。

## 发布缓存与失效

`WorldRenderCache<MeshPort>` 是 app 私有不可复制/移动模板，不是 renderer 后端或 fence 队列。账本只保存 WorldId+ChunkCoord、句柄、成功接受的发布 revision 与顶点数，不保存 World/Chunk 指针、span 或完整世界快照。
callback 内先复制网格并预检统计溢出；首次 Create 先分配 map 节点，接受失败删占位，避免资源先创建而账本分配失败。Update 成功后才推进 accepted revision/顶点数/统计；失败保旧状态。使用发布 revision 而非 mesh-input revision，仍 pending 的旧发布不重复上传，空发布有效清旧几何。
同步是逐记录接受，不是整批事务。前序成功保留；异常退出 World guard 恢复，未到 TakeStatistics 的成功统计仍保留。draws 重组或 AppendDraws 分配失败可能留下部分 vector，app 退出，不提交半轮 FrameInput。
缺席记录不是卸载；只用显式 ReleaseMesh/World/All。Destroy 成功才 erase，失败保对应及未处理 entry；正常 Run 在 World 尚有效时显式释放该身份。析构不调用借用 port；异常终端依门面完整资源所有权清理，不能用已销毁 port 释放。
Find 返回账本元素只读借用，在对应 entry erase/cache 销毁后失效；TakeStatistics 只归零统计。计数为成功接受的 CPU mesh 字节/调用，empty Update 计一次且零字节，不是显存分配/峰值。资源上传与 Render-local staging 分开记一次。

## 关闭与失败后状态

Run 正常结束先非阻塞交接已就绪 GPU 结果、记录最终摘要；completed 时才在正式采样外作独立额外截图帧，排除其 GPU frame ID。PNG 写入错误传播，不能把旧文件存在当成功。该结束观测不是额外 GPU 等待/Seal 保证。
Free 分段处理 pending定义 → GameRenderer::Shutdown/reset → Camera → Registry::Clear/reset → Window::Destroy/reset → 本模块 Window::Free。图形 owner 在借用窗口仍有效时终结；Camera 先于 Registry。CleanupSequence 捕获全部异常，继续后续清理、保首异常并尽力诊断次错，最后包装重抛；诊断输出失败不阻断其他步骤。
GameRenderer::Shutdown 先置 stopped，再尝试 ReleaseAll、图集 Destroy 和现代 Shutdown，清 draws/cache并 reset门面；旧路径释放 timer和旧Renderer。首次清理失败后重复调用不重做原步骤，不代表成功/可恢复；终止 quarantine 与安全等待由 [renderer](renderer.md) 保证，app 不强行释放未知在途资源。析构仅诊断兜底，不能替代显式成功关闭。
main 保持解析64、缺资产2、运行/关闭/导出错误3、保存成功但观测无效4、正常0的现有分类；先 Free 再 Export，清理/导出次错不把失败变绿。运行 std/非std 异常都进入清理，预检仅捕获 RenderError，Export 仅捕获 std::exception，不能声称所有入口/OOM/非std都有完整诊断出口。记录仍可变、无 Seal/显式失败重试幂等状态，未来目标见 T4。

## 稳定不变量与验证

保留原 Application I1-I6 的局部身份：I1 Camera 借用期 Registry 有效；I2 图形先于 Window/context；I3 modules 不包含 app或查静态owner；I4 benchmark 不提交普通意图；I5 生产 cpp 唯一owner；I6 运行数据均来自同一局部World。与 renderer/T4 的同号身份不得混淆。
缓存和 GameRenderer 原无编号约束由上述所有权、成功接受/显式释放、单次 Present、复制与转换章节维护，不新造历史验收身份。
依据：[Application](../../game/app/src/application.cpp)、[main](../../game/app/src/main.cpp)、[GameRenderer](../../game/app/src/game_renderer.cpp)、[cache](../../game/app/src/world_render_cache.h)、[统计](../../game/app/src/world_render_cache.cpp)、[转换](../../game/app/src/render_conversion.cpp)、[参数](../../game/app/src/startup_options.h)、[cleanup](../../game/app/src/cleanup_sequence.h)。
聚焦测试：[cache](../../test/unit/app/render_cache_tests.cpp)、[conversion](../../test/unit/app/render_conversion_tests.cpp)、[参数](../../test/unit/app/startup_options_tests.cpp)；它们不能证明实际 GPU 同步/故障、长期资源回收或整组玩法。R2 顺序1-4为候选，R3整组/默认切换/Vulkan仍未通过；本次只静态核对，不重跑游戏。
