---
type: 节点冻结记录
status: R1已冻结，R2已批准进入
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestone
  - topic/rendering
---

# M3-T3 R1 冻结记录

2026-10-09，用户在本聊天明确决定：“我已经审查了R1了，可以冻结R1，批准进入R2”。人工入口的 T3-M01–T3-M07 均已由用户勾选。**R1 关闭，Renderer v1 公开契约及 target 设计基线冻结，R2 生产实施获准开始。** 这不是生产接管验收、默认后端切换或整个 T3 通过。

权威身份见 [冻结清单](freeze-manifest.json)。当前导航：[T3 spec](../../../../../spec/M3-T3-渲染器重构.md)、[人工核查与历史结论](../../../manul-verification.md)、[R2 实施入口](../../../r2-entry.md)。T2 已批准且遗留风险不改判，见 [T2 批准身份](../../../../m3-t2/evidence/t2-approval-20261009.json)。

## 冻结范围

| 基线 | 承诺与实施边界 |
| --- | --- |
| 公开门面 | 生产命名空间 `SymoCraft::Rendering`；公开头为 `renderer.h`、`types.h`、`error.h`，只含 CPU 契约及 opaque handle，不传播图形 SDK/原生类型。PImpl 独占一个后端，Renderer 不复制/移动，借用真实 `SymoCraft::Window`，先于 Window 清理 |
| 输入与资源语义 | 保留 spec 的 CPU 自有化、成功后发布更新、空网格清旧内容、过期/跨实例句柄拒绝、逻辑销毁与 GPU 延迟回收分离、线程/非重入与有界失败清理；World 的发布版本仅在接受更新后记录 |
| 帧与诊断 | 游戏组装完整 `FrameInput`；新门面 `Render` 唯一承担提交与 Present；中立设备身份、统计、异步 frame ID 样本和 top-left RGBA8 截图按 spec 实施，不冒充旧 GL 字段 |
| target 设计 | `symocraft_renderer_api` 只提供公开 CPU 使用要求；门面实现通过私有契约/工厂依赖实际后端，不形成反向环。生产不依赖实验或 `test/support`，未实现后端不建空 target |
| 私有实现 | 内部后端、资源、命令、同步和绑定模型不冻结；按实际复用需求演进，不提前冻结完整通用 RHI，不承诺跨 MSVC/STL ABI |

批准前 spec 是行为基线，实验声明是签名和 CPU 转接参考。实验命名空间、inline 校验器、CPU registry 与临时适配不自动成为生产公开 API；R2 应按上述三头拆分，并验证真实生产 target，而不是把声明消费者成功当作已实现门面。改变公开输入/所有权/失败语义须按 spec 的受控变更规则留档，不借实施静默收窄验收。

## 固定来源

本目录保存 **12 份批准前原字节快照**，每份的来源、归档路径、字节数及 SHA-256 均在清单中。包含 spec、CPU 清单、实验声明/转接与构建边界、两份验证身份、R1 查验归档身份、T2 批准身份及用户已勾选的人工表。快照及清单设为只读；后续修改当前文档不覆盖这些原件。

- [批准前 T3 spec](sources/docs/spec/M3-T3-渲染器重构.md)
- [批准前 CPU 契约清单](sources/docs/spec/M3-T1-T3-CPU契约清单.md)
- [用户已勾选的人工表快照](sources/docs/milestones/m3-t3/manul-verification.md)
- [声明参考](sources/test/experimental/d3d12-r1/contract/candidate_renderer.h)
- [既有查验归档](../review-20261009-001/README.md)

快照中“草稿/待审核/未冻结”等文字是批准前原貌，不是当前状态；其原相对链接也只是历史原字节，不作为当前导航。旧 JSON 中 `freeze_authorized=false`、`r2_started=false` 等字段保留当时事实，由本次明确用户决定和新冻结清单记录后续状态，不回写实验成绩或旧包。

## 保留边界

既有成绩仍仅覆盖其原来源：CPU Debug/Release 各 4/4，D3D12 正常/合成超时各 2/2，E01–E03 本机 RTX 5070 Ti 实验。**本次没有新 CPU/GPU 运行、没有新验证包，也没有生产验收成绩。**

`foundation/src/files.cpp` 已与旧运行输入不同，旧 SHA `01885fcc...`、当前冻结时 SHA `17c28320...` 详见清单；R2 须以最新同源依赖重新构建和复核，不能以本次批准证明改动后的源码通过旧成绩。

生产 NEAREST/REPEAT 与 Linear 资产、夹具 POINT/CLAMP 与 sRGB RTV 的差异，真实相机 GPU 转接、完整世界、混层三角形行为及故障路径继续在 R2/R3 落实。不得因为夹具同层三角形通过而偷偷增加混层输入禁令。合成超时不等于实际设备丢失或在途 GPU 停滞安全证明。D3D12 A01–A19 及玩法验收属于 R3，Vulkan 与双现代整体验收属于 R4，均未通过。

OpenGL 仍是当前生产默认与过渡对照；R3 用户批准 D3D12 接管后才切默认，最终退役另行授权。核显专项不恢复，中低端整机性能仍是下一玩法阶段前的独立门槛。
