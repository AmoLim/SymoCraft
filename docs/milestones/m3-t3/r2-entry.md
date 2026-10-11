---
type: 实施交接
status: R2顺序1至4已实施，生产候选已交付，人工及R3整组验收未完成
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
  - topic/rendering
---

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# M3-T3 R2 实施入口

2026-10-09 用户审阅 R1 后明确批准冻结并进入 R2。依据：[R1 冻结基线](../../verification/archive/t3-r1/freeze-20261009-001/README.md)、[T3 spec](../../openspec/changes/m3-t3-renderer/proposal.md)、[CPU 契约](../../openspec/changes/m3-t3-renderer/design.md#architecture) 和 [T2 平台交接](../m3-t2/s4-platform-handoff.md)。R2 的目标是 **真实 PImpl 门面和 D3D12 完整玩法接管实现**；R3 再做 D3D12 整组验收与默认切换，R4 再做 Vulkan 与双现代整体验收。

## 当前实际状态

冻结交接本身只记录授权。随后按用户要求，**顺序1至4已实施**，见 [顺序1报告](r2-step1-implementation.md)、[顺序2报告](r2-step2-implementation.md)、[统一游戏调用](r2-step3-implementation.md) 和 [生产候选交付](r2-step4-implementation.md)。启用native构建时新门面承接完整游戏；未编入或未实施后端仍具名Unsupported。测试recorder不是生产后端，短时世界运行也不替代完整R3验收。

当前默认仍使用隔离旧GL对照；显式 `--renderer d3d12` 进入Native Window和真实门面，共用输入/物理/交互循环。现代按发布版本持久缓存、唯一Render/Present、中立采样及截图已迁移。顺序4交付Debug/Release `002`真实安装候选，完整世界、隔离故障及窗口复核有独立证据；[T3-M08至M15](manual-verification.md#附录-b-r2-生产候选命令) 均待人工，不使A01–A19或R3通过。

OpenGL 默认及旧对照继续保留，不在 R2 入口提前切默认或删除 GL。顺序3已在创建窗口前选择实际后端；D3D12 借用 T2 的 Native Window 私有桥接，未交付或未编入后端具名拒绝，不静默回退。

## 实施顺序

1. **公开边界与真实门面：已完成本步实施与验证。** 在 `game/modules/renderer` 落实 `types.h`、`error.h`、`renderer.h`、PImpl、私有后端契约/工厂和句柄语义；旧 GL 头/实现隔离并保持可编译。实际 API/core target、CPU-only 消费者、SDK 隔离、真实链接及增量证据见 [顺序1报告](r2-step1-implementation.md)。没有创建空 D3D12/Vulkan target，未交付后端明确 Unsupported；本步不是现代 runtime 或 R2 接管验收。
2. **D3D12 真实后端：已实施并完成本机有限验证。** 设备/呈现、资源/生产 shader、深度/剔除/获批准的 alpha 深度修正/选择框、尺寸重建、query/readback 与故障清理已有私有实现；RTX Debug/Release 各四个真实进程通过，原生边界各 11/11，见 [顺序2报告与限制](r2-step2-implementation.md)。无实验依赖；未完成且活设备的工作终端保留所有权，不以超时正常回收。实际 DeviceLost/在途 timeout 仍未动态验证，CPU 全量仍有 Win32 1175，不宣称全部回归绿。
3. **统一游戏调用方：已实施并完成完整世界有限短测。** 后端选择先于建窗；按 World + Chunk 身份与成功接受的发布 `revision` 持久缓存，只更新变更网格。callback 数据在借用有效期内自有化，不以 `mesh_input_revision` 或未遍历误当发布/卸载，不再次平移 world-space 顶点。完整 `FrameInput`、唯一 Present、中立采样/截图与退出一起迁移，只有一份玩法循环。Debug/Release各三组D3D12及Release三组GL对照通过，静止/移动几何上传0，见 [顺序3报告及限制](r2-step3-implementation.md)；不代表生产包、人工玩法或R3已验收。
4. **同源验证与真实候选交付：已完成候选交付及有限验证。** Debug/Release各3282项生产输入一致；真实cmake install、工具/exe/DXIL/CRT/notices及精确清单核查完成。本机RTX完整441区块三组短负载、普通400帧、隔离资源故障、连续最小化12秒/恢复/缩放/WM_CLOSE均有实际结果。见 [顺序4报告及缺口](r2-step4-implementation.md)、[库内归档](../../verification/archive/t3-r2/step4-20261009-001/README.md)。唯一人工主表已换入T3-M08至M15与逐项Python命令，不自动勾选；当前过渡组合、实际设备/提交/在途故障、工具协议及视觉/性能标准仍不能当作已完成。

## 转接与验证约束

真实 Camera 继续使用角度到弧度、右手系、near 0.1 / far 2000；不能把候选默认 far 1000 当成游戏参数。保留原 512×512 RGBA 图集、64×64 tile、64 层与底行优先层号，top-left 存储及单次 UV 归一化按 CPU 转接依据落实，再验证生产 GPU 画面。

生产采样/颜色、mip/边界和混层三角形不因 R1 夹具通过而自动通过或被禁止；顺序2已通过实际 point/repeat、颜色与混层 GPU 像素核查，范围不是完整世界。用户另已批准现代 alpha discard 不写深度，旧 GL 不改；其它公开语义变化仍须确认。实际设备丢失、提交失败、真正在途等待失败必须单独留证，合成超时或源码审计不能替代。

旧 R1 成绩绑定其输入 hash；当前 `foundation/src/files.cpp` 已变化，须重新构建同源依赖。T2 的 Win32 1175/Fix1 暂停、正式同源 Q06 与首次可操作时间预算保留原状态，不在本交接中改判。各轮原始运行写入新的 `out/m3-t3` 目录，待交付再按 `evidence/r2/review-日期-序号/` 复制查验材料；不覆盖 R1 包、归档或冻结快照，不归档 exe/依赖库来假装可运行包。

## 进入 R3 的条件

真实D3D12门面已承接固定完整世界、纹理/相机/选框与现有循环，生产候选及来源/资源/采样/截图已有可复核证据。实际DeviceLost/提交失败/真正在途等待、现代独占/全关闭及未执行的关闭测试/工具补充配置、GUI现代协议和正式视觉/性能标准仍须收尾；人工14项/15分钟尚未执行。缺口成立后再独立申请R3的A01–A19整组验收，用户批准接管后切默认。**顺序4交付不是R3进入或默认切换批准。**
