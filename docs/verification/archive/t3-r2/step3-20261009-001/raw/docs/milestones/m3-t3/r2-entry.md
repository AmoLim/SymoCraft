---
type: 实施交接
status: R2顺序1至3已实施，完整世界短测通过，生产候选待顺序4
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestone
  - topic/rendering
---

# M3-T3 R2 实施入口

2026-10-09 用户审阅 R1 后明确批准冻结并进入 R2。依据：[R1 冻结基线](evidence/r1/freeze-20261009-001/README.md)、[T3 spec](../../spec/M3/M3-T3-渲染器重构.md)、[CPU 契约](../../spec/M3/M3-T1-T3-CPU契约清单.md) 和 [T2 平台交接](../m3-t2/s4-platform-handoff.md)。R2 的目标是 **真实 PImpl 门面和 D3D12 完整玩法接管实现**；R3 再做 D3D12 整组验收与默认切换，R4 再做 Vulkan 与双现代整体验收。

## 当前实际状态

冻结交接本身只记录授权。随后按用户要求，**顺序1至3已实施**，见 [顺序1报告](r2-step1-implementation.md)、[顺序2报告](r2-step2-implementation.md) 和 [统一游戏调用报告](r2-step3-implementation.md)。启用 native 构建时新门面承接完整游戏；未编入或未实施后端仍具名 Unsupported。测试 recorder 不是生产后端，短时世界运行也不替代完整 R3 验收。

当前默认仍使用隔离旧 GL 对照；显式 `--renderer d3d12` 进入 Native Window 和真实门面，共用输入/物理/交互循环。现代按发布版本持久缓存、唯一 Render/Present、中立采样及截图已迁移；旧 GL 的逐帧 Append 与专属采样只留在私有转接分支。完整世界 Debug/Release static/walk/edit 已短测，生产候选仍待顺序4。

OpenGL 默认及旧对照继续保留，不在 R2 入口提前切默认或删除 GL。顺序3已在创建窗口前选择实际后端；D3D12 借用 T2 的 Native Window 私有桥接，未交付或未编入后端具名拒绝，不静默回退。

## 实施顺序

1. **公开边界与真实门面：已完成本步实施与验证。** 在 `game/modules/renderer` 落实 `types.h`、`error.h`、`renderer.h`、PImpl、私有后端契约/工厂和句柄语义；旧 GL 头/实现隔离并保持可编译。实际 API/core target、CPU-only 消费者、SDK 隔离、真实链接及增量证据见 [顺序1报告](r2-step1-implementation.md)。没有创建空 D3D12/Vulkan target，未交付后端明确 Unsupported；本步不是现代 runtime 或 R2 接管验收。
2. **D3D12 真实后端：已实施并完成本机有限验证。** 设备/呈现、资源/生产 shader、深度/剔除/获批准的 alpha 深度修正/选择框、尺寸重建、query/readback 与故障清理已有私有实现；RTX Debug/Release 各四个真实进程通过，原生边界各 11/11，见 [顺序2报告与限制](r2-step2-implementation.md)。无实验依赖；未完成且活设备的工作终端保留所有权，不以超时正常回收。实际 DeviceLost/在途 timeout 仍未动态验证，CPU 全量仍有 Win32 1175，不宣称全部回归绿。
3. **统一游戏调用方：已实施并完成完整世界有限短测。** 后端选择先于建窗；按 World + Chunk 身份与成功接受的发布 `revision` 持久缓存，只更新变更网格。callback 数据在借用有效期内自有化，不以 `mesh_input_revision` 或未遍历误当发布/卸载，不再次平移 world-space 顶点。完整 `FrameInput`、唯一 Present、中立采样/截图与退出一起迁移，只有一份玩法循环。Debug/Release各三组D3D12及Release三组GL对照通过，静止/移动几何上传0，见 [顺序3报告及限制](r2-step3-implementation.md)；不代表生产包、人工玩法或R3已验收。
4. **同源验证与真实候选交付。** 用当前 foundation/platform/assets/world 源码构建 Debug/Release，验证公开边界与生产生命周期，再跑本机 RTX 5070 Ti 的完整画面和玩法准备。交付真实可运行包、固定场景入口、身份/诊断/失败记录后，才在唯一 [人工核查入口](manul-verification.md) 换入下一组稳定编号；不要求用户核查不存在的生产包。

## 转接与验证约束

真实 Camera 继续使用角度到弧度、右手系、near 0.1 / far 2000；不能把候选默认 far 1000 当成游戏参数。保留原 512×512 RGBA 图集、64×64 tile、64 层与底行优先层号，top-left 存储及单次 UV 归一化按 CPU 转接依据落实，再验证生产 GPU 画面。

生产采样/颜色、mip/边界和混层三角形不因 R1 夹具通过而自动通过或被禁止；顺序2已通过实际 point/repeat、颜色与混层 GPU 像素核查，范围不是完整世界。用户另已批准现代 alpha discard 不写深度，旧 GL 不改；其它公开语义变化仍须确认。实际设备丢失、提交失败、真正在途等待失败必须单独留证，合成超时或源码审计不能替代。

旧 R1 成绩绑定其输入 hash；当前 `foundation/src/files.cpp` 已变化，须重新构建同源依赖。T2 的 Win32 1175/Fix1 暂停、正式同源 Q06 与首次可操作时间预算保留原状态，不在本交接中改判。各轮原始运行写入新的 `out/m3-t3` 目录，待交付再按 `evidence/r2/review-日期-序号/` 复制查验材料；不覆盖 R1 包、归档或冻结快照，不归档 exe/依赖库来假装可运行包。

## 进入 R3 的条件

真实 D3D12 门面已经承接固定完整世界、纹理/相机/选框与现有操作；网格变更/空网格、窗口恢复、诊断/正常与故障清理成立，没有双份 Present 或重复全量上传。生产包、构建/来源/资源身份、后端采样与截图有可复核证据。此时再申请 R3 的 A01–A19、适用 14 项玩法及至少 15 分钟连续操作验收，用户批准后切默认。R2 获准进入不代表这些条件已满足。
