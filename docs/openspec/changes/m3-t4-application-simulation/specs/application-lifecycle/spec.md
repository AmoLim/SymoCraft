---
tags:
  - area/spec
---

# Application Lifecycle Delta

## Purpose

定义私有单机组合的T4目标，不把当前 Init/Run/Free 静态状态改述为实例已交付。机制见[design](../../design.md)，Q01/Q03待选见[decisions](../../decisions.md#未决接口)。

## ADDED Requirements

### Requirement: Private single run assembly and bounded shutdown

game/app私有Application SHALL 独占本次配置、Runtime/Window、World/Registry/SimulationSession、Renderer/Bindings及可选记录/Scenario；活动namespace静态玩法状态退出。一实例 SHALL 一次Run、主线程串行非重入、不复制移动；成功装配才可Run，部分失败保留可关闭owner及首诊断。Shutdown SHALL 尝试全部适用清理、保持Renderer→Window→Runtime依赖、幂等返回同一关闭事实且不无限重试；析构noexcept有界兜底、不Seal/Export。World/Registry SHALL 覆盖会话寿命，Window覆盖Renderer，Runtime覆盖Window。

#### Scenario: M3-T4-A04 staged assembly failure
- **WHEN** CPU预检、Runtime、Window、Renderer、World/Registry或会话装配失败
- **THEN** 不发布半初始化运行，回收已取得资源；专属Registry装配范围丢弃，不宣称任意共享Registry精确回滚/重试，mock与真实结果分开

#### Scenario: M3-T4-A05 repeat shutdown and secondary errors
- **WHEN** 显式关闭有次要错误或重复Shutdown/独立进程重新启动
- **THEN** 首原因保持、次要事实分别记录，关闭不重试取得资源或回调失效Window，重复独立进程启动无未解释资源增长；未知/未取得不写成安全回收，后端终止保留不是正常清理通过

### Requirement: Local frame policy and commit order

Application SHALL 依次处理平台事实→关闭/暂停/输入来源→普通意图或Scenario模拟前编辑→每次Advance角色一次/物理零到八步→恢复/射线结果→普通World校验编辑→World网格/绑定→Render/Present→中立观测。窗口回调 SHALL 不推进会话/改ECS/提交GPU；关闭始终优先不继续新模拟/渲染。普通暂停 SHALL 丢旧玩法记录并Reset平台，映射会话Pause/Resume，恢复用当前held、无旧短按/指针回放及首运动保护；同轮失焦再获焦也清一次，零实际像素不绘制，有限帧失焦诊断例外保留。基准策略按platform delta独立，不沿Session指针分支。

#### Scenario: M3-T4-A09 A16 A17 ordinary and scenario composition
- **WHEN** 普通编辑、static/walk/edit场景、暂停/恢复或窗口关闭发生
- **THEN** 普通编辑在模拟后、benchmark在模拟前且保原动作/游标/预条件；普通可观测、场景可无记录，真实像素与renderer比例一致；关闭/Wait/同轮失焦不会误推进或重放

### Requirement: Target completion requires applicable evidence

T4 SHALL 保持 M3-T4-A01-A27 的稳定身份：A01-A22及A24-A27适用项须真实依据与用户整体验收；A23按D02后续不适用，不勾通过。纯CPU/静态/局部候选不替代真实SDL、现代后端、部署、性能及用户批准。缺环境/未执行/失败 SHALL 明示，不因迁移或planningComplete通过。

#### Scenario: M3-T4-A01 A03 A21 build and deployment
- **WHEN** 验证公开头/target、CPU-only/独立工具/关闭测试、旧新文件协议与安装包
- **THEN** app私有且无反向/SDK泄漏或生产重复编译，必要数据及资源在无关/空格/中文CWD成立，元数据/seed/摘要/workload/CPU-GPU-present语义不漂移，不覆盖冻结记录

#### Scenario: M3-T4-A22 modern local gameplay
- **WHEN** 申请当前T4整体验收
- **THEN** D3D12/Vulkan各自用本机RTX5070Ti同一真实调用方验证适用14项玩法及至少15分钟并留adapter/driver/源码包身份；缺能力具名受阻、不fallback，GL仅受影响回归，不待其他GPU或称Y9000P/M4已通过
