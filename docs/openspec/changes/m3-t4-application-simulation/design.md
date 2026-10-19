---
tags:
  - area/spec
---

## Context

目标行为只由 [simulation](specs/simulation/spec.md)、[platform](specs/platform/spec.md)、[physics-core](specs/physics-core/spec.md)、[application-lifecycle](specs/application-lifecycle/spec.md)、[telemetry-lifecycle](specs/telemetry-lifecycle/spec.md)维护。
当前Application仍为namespace Init/Run/Free与静态窗口/Registry/相机/玩家ID/冷却；Window PImpl的Init/Free/Create及快照仍是旧接口；Physics共享预算且原地写ECS；Performance metadata/startup/completed可写、无Seal。新对象/独立核心未实现，目标不能进入当前architecture或现役spec。
P01候选/消费者静态核对与旧CPU基线已交付；局部核心/单测授权不是完整T4入口。批准/冻结只从[VERIFICATION](../../../verification/VERIFICATION.md)判读；[scope](../../../scope/project-scope.md)保留前序、硬件和后续门槛。

## Technical Approach

### 组合与寿命

Application作为game/app私有不可复制/移动owner，一实例一次Run，主线程串行非重入；不向底层传播StartupOptions、Application或Performance指针。获得对象后立即由成员接管，不依赖“全初始化成功”才清理。装配路径：CPU配置/资产预检→组合根/可选记录→Runtime→Window→Renderer→World/Registry→SimulationSession→Bindings/可选Scenario。
Runtime只管理自己取得的SDL责任；Window独占原生对象，借Runtime；Renderer借Window。Window模式创建前选定，T2三桥仍是唯一私有原生交接，不增加public SDK逃生口。CPU调用方只加载必要玩法数据并创建World/Registry/会话，无空窗口/假Renderer；无窗口不等于无资产或新服务器入口。

会话借稳定World及专属Registry，拥有预算/Controller/镜头/材料冷却；默认不复制/移动，析构不删除借用对象或实体，不推进世界。Q01装配失败不发布会话，销毁专属装配范围，不能承诺任意共享Registry事务。运动仍以组件为唯一长期权威，适配临时取值/提交，不跨帧留组件地址、不在遍历时结构修改；现有组件存储约束与T5审计分开。

### 输入、视图与时序

app拥有InputFrame，平台交换已发布事件缓冲并保留独立pending缓冲；消费结束清内容留容量，不保存caller地址、不造归还令牌/池。平台PointerEvent逐值映射中立PointerInput复用容器，Advance只同步读const span/意图引用，不以reinterpret_cast或共享引擎输入头连接target。Q02保留三候选，不以类型候选暗选。
帧顺序沿delta：事实/关闭/策略→普通意图或场景模拟前编辑→角色一次→预算0..8→实体求解/提交→恢复/射线→普通编辑→网格/Renderer→观测；输入不在子步重复。Wait累积待发布事实，同轮失焦获焦仍保留清理事实。恢复首个成功推进规则时间为0，有限帧诊断失焦例外不允许零像素绘制。基准Esc跨失焦/最小化无效/allow-unfocused独立按app策略，不靠Session指针或后台全局键取消。

结果为小型自有PlayerPose（身体而非眼点）、LensState、optional SelectionHit（格/法线/距离）、EditRequest及推进统计；下一次推进不改旧结果。镜头初始FOV45度、滚轮限1..45度、near/far0.1/2000，eye offset与身体位姿分开；朝向参与玩法射线，不依赖投影。
现代app适配T3 CameraParameters的position/forward/up/FOV弧度/near/far，不往其塞aspect；后端处理RH_ZO/Vulkan差异。legacy GL由app按实际像素比例派生RH_NO矩阵；零extent不伪造成1。视图派生只读，非法比例仅拒绝视图，不Failed模拟或修改已返结果；不留下冗余Camera实体权威。

### 物理硬边界与提交

目标子树为game/modules/simulation/physics，定义唯一STATIC symocraft_physics，独立include/symocraft/physics根及SymoCraft::Physics；simulation单向消费，World/ECS适配留simulation/src。核心仅标准库/foundation数学，不取得simulation/src/include或其他模块SDK。若会话public确需physics类型，逐项说明PUBLIC传播原因，不开放整个私有目录。
BodyState只含有限位置/速度/加速度/重力/sensor/接地；AabbShape正尺寸/有限偏移、世界轴对齐不带Transform缩放；PhysicsSettings按值保存现有重力/轴限速/容差及单固定步，无全局可写配置/帧累计。TerrainCollisionQuery为const void*上下文+非空bool函数指针（无状态可空上下文），只同步读不保存；false为非碰撞，World域外false、坏定义错误。
单步取实体小值→求解→校验成功→提交该实体；先前实体/子步不回滚，角色已执行也保留。扫描/积分/接地/平局完全沿旧算法，RayHit自有且无draw开关，不改为新物理世界/批事务/CCD。D02正式模块搬迁保持target/include/namespace/行为与单实现，只换所有者；独立Debug/Release、负向边界、隔离预算及旧新行为依据成立后在T5后独立执行。

### 预检与故障

double秒先finite/非负校验→double min(0.1)→float规则时间→单一旧预算Consume。所有中立指针预检在任何角色/镜头/冷却/预算修改前完成；Rejected不Failed、不补回已采输入。推进开始后故障先置Failed停止后续提交，再包装中立AdvanceFailed，携带操作/阶段/可确定进度；物理错误不带ECSID，由simulation补逻辑进度，不携成功编辑载荷。结果分配失败也不是成功；不承诺OOM完整字符串或noexcept计算。顶层未知/bad_alloc兜底仍停止，不继续错误帧。
适配先用IsEntityValid/HasComponent验证已知前提；旧Logger Error只打印、Assert/exit不可被catch修复，实际所需空值/分配路径若阻断则最小修补/报告，不以T5未开始掩盖。World正常Rejected/Unchanged/Changed继续是合法编辑结果，不一律抛计算异常。Paused必须Resume；Failed无自动恢复或续Advance。

### 关闭、封闭与文件

显式关闭顺序：停止场景/模拟/新采样→交接已就绪GPU→结束Bindings借用并Shutdown Renderer→销毁会话/World/Registry及Renderer对象→Destroy Window→结束Runtime→汇合FinalFacts并Seal→显式Export。记录建立早于被测装配且活到导出，部分启动也保留；Seal不早于窗口/平台结果确定。停止新采样不等于记录已封闭，Renderer为安全释放的有界等待不能因“不等指标”取消。
关闭聚合逐步事实及首/次原因，重复Shutdown回同事实不重试资源取得/无限清理。Q03决定返回/异常报告形状，三候选未选；未取得/未知/终止quarantine不得当安全释放。OOM可保有限诊断；析构只有界兜底，不Seal/Export。Renderer迟到GPU缺失具名、不补0/不额外等，原首错误不被清理/保存次错覆盖。
Performance记录自有且四类最终事实分开，Seal不可变；Export独立NotAttempted/Failed/Saved，在同进程同专属目录显式失败重试，Saved缓存receipt不写第二次。不增加完整文件事务或崩溃恢复；finished须所有文件/终态发布都成功，[Files允许晚期失败](../../specs/foundation-files/spec.md)及Fix1暂停保持。Scenario游标/实际时间独立，不因Session存在走路线，不引入探针档位/新包。

### 不变量身份

| 稳定编号 | 必要成立边界 |
| --- | --- |
| T4-I01 | Running时World/Registry覆盖会话、Window覆盖Renderer、Runtime覆盖Window |
| T4-I02 | 单会话一份预算/统一规则时间；零时间不清余数，暂停恢复清；核心不二次Consume |
| T4-I03 | 运动唯一权威，不跨帧缓存组件地址或遍历中结构修改 |
| T4-I04 | 事件/汇总不双用，子步不重复输入；自有交接不清pending，全消费者结束才复用 |
| T4-I05 | physics中立依赖；记录不留运行指针，会话仅明确借World/Registry |
| T4-I06 | 预检拒绝不改状态/预算；推进故障保已提交范围并Failed，不续交 |
| T4-I07 | Sealed不可变；四类事实分开，失败重试不改结论、Saved重复不写文件 |
| T4-I08 | 先停再有界清理，次错不盖首因，不靠Release断言保证安全 |
| T4-I09 | 中立意图/显式时间推进，不依赖窗口/Renderer/aspect/GPU |
| T4-I10 | 显示/诊断自有，无组件/World/Registry/GPU借用，不造第二可写权威 |
| T4-I11 | 相同生产CPU逐次固定阈值对照，视图/只读采集不影响推进 |

### 消费者交接

P01来源的稳定 C01-C14/H01-H04保留为映射，不作第二API清单：C01main→实例/FinalFacts；C02帧输入→自有意图；C03世界摘要/场景→无Runtime CPU与独立Scenario；C04玩家创建→Q01；C05Physics→核心适配；C06现代Camera→T3值；C07legacy→app矩阵；C08Window reducer→Q02；C09记录写入→受控Seal；C10现数学/模拟测试→新会话回归；C11平台/记录/cleanup测试→新事实/Seal；C12后端/报告探针→实际T3版本；C13公开头/target→内部physics边界；C14外部runner→旧GL版本2/25列兼容，现代协议单独交接。
H01以已存在GameRenderer/ToRendererCamera和bridge实际像素为准，不保留旧“尚未顺序3/4”的实施叙述；H02保Renderer有界Shutdown/已就绪样本/终止保留，H03核对GL与现代metadata/新增列/版本和原生工具尚不支持的缺口，H04绑定实际源码/资源/生成身份及文件归属，不将不同并行版本混成一次基准。全部交接仍须S01按实施时版本复核。

## Decisions

[decisions](decisions.md)是D01-D13及Q01-Q04唯一选择正文；Q不代选，命名候选不冻结。D09值返回未来输出缓冲触发条件、D11未来已校验时间值及D02正式模块属于不同演进项，不互相推定。

## Risks And Compatibility

历史CPU基线只证明旧生产行为/夹具，不覆盖新核心、会话、D04/D11/D13强保证或A24-A27。迁移不运行测试，更不补用户批准。D12固定阈值/全部序列见simulation delta；Q04采集可见范围未选，检查必须只读、自有且Failed未知值不补0。
真实SDL/现代GPU/性能/部署仍各自取证；当前功能仅本机RTX5070Ti，T4按2026-10-12用户确认改为自动验证加四组人工体验，D3D12/Vulkan每后端四组合计最多5分钟，包含退出/重启，不逐项重复14项人工表或按D/R再追加。权威操作、实施方验证责任及限时未完成处理见[application delta](specs/application-lifecycle/spec.md#requirement-target-completion-requires-applicable-evidence)；构建/故障/身份/协议检查不交给用户逐份审日志，自动缺口如实报告，不用mock替真实组合，也不默认扩大人工工作。
此次替代的是本change原每后端至少15分钟连续操作门槛，不修改T3或共享场景的历史要求，不将短体验称为原长时通过。持续资源增长、罕见时序与长时稳定性仍未由短体验证明，适用自动结果/未覆盖风险在同一verification.md维护；不自动增加长时自动替代测试。另一设备不阻塞，但中低端候选整机性能仍是下一玩法前门槛。测量沿批准冻结协议/同条件预算，未确定的不造数；结构提取不声称FPS/M4通过或关闭1175/Fix1/Q06，当前修改不扩大实施授权或产生验收成绩。
资源定位若必须显式化，仅可配套已审核小型AssetLocator并保持路径/CWD限制，不扩成资源管理器或physics前置。兼容包装仅短期，正式候选不留两预算/两算法/两相机权威；后续并行、服务/IPC、输入缓冲和热探针按scope专项，不借T4自动实施。
