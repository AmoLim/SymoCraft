---
tags:
  - area/spec
---

# Platform Scenario Delta

## MODIFIED Requirements

### Requirement: Benchmark window isolation

本 requirement 中 Benchmark SHALL 仅指保留的 static/walk/edit 工作负载，不再要求独立 benchmark 产品或旧输出协议。场景窗口 SHALL 保持普通无边框/非独占/非置顶/NonRudeHWND，默认实际 1920×1080，显式配置不同尺寸必须真实核验、不静默降负载。app SHALL 依据平台原始事实隔离玩家键鼠并选择场景，不能以旧 Performance::Session、调试快照或工具连接决定。

场景 SHALL 使用 allow-unfocused 行为，无需严格前台采样准备；只允许本窗口 Escape/关闭影响取消，已排队未消费 Escape 跨失焦保留，不补采后台全局键盘。失焦本身不暂停/重启脚本时钟，最小化/零 drawable/实际尺寸偏离请求 SHALL 明确失败并有界退出，不继续提交、不以失焦许可豁免，不套普通玩法恢复/无限等待。Poll 与 Capture 成本如经 Tracy 观测 SHALL 分开保留原归属，Capture 不伪装为旧 event_ms。普通玩法焦点/暂停与有限帧诊断契约保持。

#### Scenario: M3-T2-A06 workload and cancellation
- **WHEN** 场景收到玩家控制、失焦、最小化或尺寸不符
- **THEN** 玩家输入不改脚本，单纯失焦按允许后台场景继续，最小化/尺寸偏离具名运行失败且不生成有效成绩；不需要旧状态文件证明退出

#### Scenario: M3-T4-A20 fact completeness and cancellation
- **WHEN** Escape 已排队后失焦/恢复、同轮失焦再获焦或无完整观测 Session 运行场景
- **THEN** app 仍按窗口局部事实保留取消及场景动作，不依靠全局键或只删除 benchmark_ 筛选；原 Poll/Capture 计时归属和 T2 有限帧诊断例外保留，不授权零尺寸绘制

### Requirement: Platform SDK and deployment boundary

SDL SHALL 固定本地 release-3.4.18、静态构建并只由 platform 私有消费，普通 main 与私有 SDL_MAIN_HANDLED/SetMainReady 不传播到 app。公共 Window SHALL 无 SDL/GLAD/Win32/D3D/Vulkan 类型或裸 `void*` 逃生口；renderer 仅消费授权三份私有桥，不取得整个 platform/src。CPU-only SHALL 不配置/链接 SDL 或 GPU SDK；旧独立 benchmark 已退役，不再作为构建/部署产品；普通 GL/Native 路径不强制外部 Vulkan SDK 或 loader。静态 SDL 不代表静态 CRT/Vulkan loader，不新增 SDL3.dll 部署。

#### Scenario: M3-T2-A01 A09 build and deployment

- **WHEN** 构建公开消费者、CPU-only 或部署当前游戏
- **THEN** 头和 target 不传播 SDL/SDK；所需运行条件、CRT/资源和干净机器证明仍分开，不由安装成功推定无开发环境运行通过，也不要求安装旧 benchmark 或外部 profiler

## ADDED Requirements

### Requirement: Legacy platform measurement applicability

本能力保留的 Q06/A11、FB01/MB01、strict 前台准备和原始数据判据 SHALL 作为旧 T2 候选及历史协议的评价依据，不再要求退役后的当前产品保留旧 Session/CSV/status/runner 或 strict 入口。未完成采样、发布风险和原预算结论 SHALL 保持原状态；后续其他机器及正式性能验证仍按 scope 和已确认预算另行确定工具口径，不由退役获得通过、免验或新的执行授权。

#### Scenario: DP-A08 current window contract versus legacy measurement
- **WHEN** 验证当前场景窗口/输入/部署，或读取旧 T2 测量与预算材料
- **THEN** 当前场景适用本能力的新隔离/取消规则，不以缺少旧性能终态或 strict 参数判交付缺失；历史轮次仍按其原协议解释，不删除失焦/慢帧/失败，不用工具接入替代 Q06/预算或 Fix1 的未完成结论
