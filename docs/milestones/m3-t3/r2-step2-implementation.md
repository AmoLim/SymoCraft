---
type: 实施报告
status: R2顺序2原生后端已交付，RTX有限验证通过，完整玩法待顺序3
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
  - topic/rendering
---

> 2026-10-11 旧导航引用已失效，已改指现行权威或明确归档入口；历史命令、结果和批准含义保持不变。

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# R2 顺序2：生产 D3D12 后端

## 范围与决定

按用户授权完成 [R2 入口](r2-entry.md) 的顺序2：新 `Rendering::Renderer` 已能通过真实私有 D3D12 后端创建资源、绘制与呈现，不依赖 R1 实验实现。冻结公开三头和签名不变。旧 OpenGL 游戏与默认不变，未实施顺序3的完整世界/玩法迁移，未交付现代便携包或申请 R3。

用户在本聊天明确批准“透明像素不写深度”。据此移除现代 blocks shader 的 `earlydepthstencil`，保留 alpha `<0.01` discard；真实前后景双绘制顺序的 RGB 结果一致。旧 GL shader、透明排序和材质编号均未修改。这是获批准的既有深度缺陷修正，不声称逐像素复现旧错误行为。

查验材料已按节点复制到 [顺序2归档](../../verification/archive/t3-r2/step2-20261009-001/README.md)，所有产物链接均在 Obsidian 库内。现行 [身份与 SHA 清单](../../verification/archive/t3-r2/step2-20261009-001/validation-manifest.json) 包含 **822 份原字节副本、21,766,344 字节、525 项定域输入**；原始 `out/` 不移动，不覆盖任何旧运行。

## 实施内容

| 范围 | 实际实现 |
| --- | --- |
| 工厂与构建 | `SYMOCRAFT_RENDER_D3D12` 默认 OFF；启用时真实 STATIC backend 私有链接门面。CPU-only 即使缓存请求 ON 也有效关闭原生分支，不发现图形 SDK/DXC。未编入或未实施的新门面后端具名 Unsupported，不静默回退 |
| 原生平台与身份 | 借用 T2 Native Window 私有 HWND 桥接；硬件高性能 adapter、实际 device LUID 对照、FL12_0/SM6_0 检查、PCI/driver；不用 WARP，不重新实现 Window |
| 网格与纹理 | 网格为持久、不可变版本的原生 UPLOAD heap buffer，Update 替换版本；纹理为 DEFAULT resource + staging/copy/barrier。旧版本由使用它的 frame 持有至 fence 完成；空网格不绘制 |
| Pipeline 与生产资产 | 构建时 DXC 生成 blocks/selection 的 VS/PS 共四份 SM6 DXIL；运行时只读编译资产。28 字节 int3/float3/float 输入；point/repeat、单 mip、浮点层插值后原生 array selection，不新增同层三角形限制 |
| 颜色与裁剪 | UNORM swapchain/RTV，不做输出 sRGB 编码；SRGB texture 只做输入解码。RH_ZO、背面剔除、深度写入与获批准的 alpha discard；选择框保留源码设计颜色与 1.004 膨胀 |
| 数值转换 | 双精度稳定相机基，不计算 `eye+forward`；draw MVP 按 `projection*(view*model)` 先取消匹配的大平移，再保留 near 项。GPU float/算术无法表达时 Execute 前具名拒绝，允许同 ID 修正后重试，不收窄 CPU 有限输入校验 |
| 呈现与重建 | 三 frame contexts；严格请求采样数，能力不足不降级；4x 真正多采样 color/depth 与 resolve，交换链仍单采样。零尺寸暂停、实际 drawable 重建、真实 VSync 身份、有限 latency/fence 等待与非阻塞 Present |
| Query 与截图 | 每 context 双 timestamp 和独立 readback，fence 完成后读取、精确来源 frame ID、Poll 不等 GPU；截图独立有限等待并剥除 row pitch。query 仅包围网格/选框，不包括 clear、MSAA resolve、截图复制或 Present |
| 故障与关闭 | 部分初始化 RAII；提交后不可恢复失败停止新提交。未完成且仍存活的设备没有正常回收证明时，将整套所有权移入预分配终端 quarantine，保留至进程退出并报告失败，不把超时当成 fence 完成 |
| 中立统计 | `cpu_prepare` 累计门面与 backend 准备，保留 submit/wait 和计数；累计溢出先置 Failed 再报告 SubmissionFailure。原生 prepare 包含 frame-slot 等待和录制；latency 等待计入 present_wait；截图等待不在 GPU draw query 中 |

成员、方法和所有权依据见 [D3D12Backend 类设计](../../architecture/renderer.md)、[Renderer/PImpl](../../architecture/renderer.md)。关键源码原字节见归档内 [backend](../../verification/archive/t3-r2/step2-20261009-001/raw/game/modules/renderer/src/backend/d3d12/d3d12_backend.cpp)、[pipeline](../../verification/archive/t3-r2/step2-20261009-001/raw/game/modules/renderer/src/backend/d3d12/pipeline.cpp) 和 [blocks shader](../../verification/archive/t3-r2/step2-20261009-001/raw/assets/shaders/hlsl/blocks.hlsl)。

## 实际验证

MSVC 19.38.33145 / tools 14.38.33130、SDK 10.0.22621.0、DXC 1.6.2112.16、CMake 4.3.5、Ninja 1.13.2；Windows x64。使用获批准的正常 MSVC 环境，没有修改 PDB 选项。不是最小 CMake 版本兼容或性能验收。

| 项目 | Debug | Release |
| --- | --- | --- |
| CPU-only 完整构建 | 通过 | 通过 |
| CPU-only 最终全量 CTest | 52/52 启用项通过；world.allocations 原有禁用 | **52/53，foundation.files Win32 1175 失败** |
| CPU-only 最终 Renderer/边界 | 8/8 | 8/8 |
| game + 真实 D3D12 完整构建 | 通过，旧 GL app 仍可链接 | 通过，旧 GL app 仍可链接 |
| 原生配置最终无窗口回归 | 11/11 | 11/11 |
| 最终真实 GPU `005` | 4/4 独立进程 | 4/4 独立进程 |
| 最终增量边界 | CPU 2 个 / native 4 个 probes 通过 | CPU 2 个 / native 4 个 probes 通过 |

CPU 全量并非全部绿：Debug 前两次也出现 1175，Release 首轮 53/53 后最终复现。所有失败与成功日志均保留；不据重跑关闭 T2 风险、恢复暂停的 Fix1，或改判正式同源 Q06。

原生配置的 STATIC 门面消费者实际链接私有 backend/platform/SDK 库，这是合法 LINK_ONLY 依赖，不等于公开编译头/宏泄漏。CPU-only 图中无原生 targets/链接。pipeline 私有头只重编两个 D3D12 translation units，真实消费者重链接但不重编；共享 HLSL 只重生成四份 DXIL，C/CXX 编译为零。每次探测恢复内容 hash 和原 mtime；baseline 计数只代表 C/CXX，不据此声称 shader 也零编译。

### RTX 与画面

两配置实际设备均为 NVIDIA GeForce RTX 5070 Ti，LUID `00000000:0001435b`、PCI `10de:2c05`、driver `32.0.16.1664`；validation 实际开启。初始实际 drawable 为 640×480，恢复/缩放至 803×557；1x/4x 均不降级，实际 Immediate/Fifo 切换。具体观察绑定各自 [Debug](../../verification/archive/t3-r2/step2-20261009-001/raw/out/m3-t3/r2-step2-gpu-debug-005/verification.json) / [Release](../../verification/archive/t3-r2/step2-20261009-001/raw/out/m3-t3/r2-step2-gpu-release-005/verification.json) 的 exe、DXIL、atlas、cache 与 62 个选定源码 hash，不是完整依赖快照。

每个 normal 进程有 320 项检查、41 个 frame results、39 个精确 ID GPU 样本。实际 checks 覆盖正反面、双顺序深度/alpha、point/repeat、混层浮点插值、Linear/SRGB、更新/空网格/句柄、selection、尺寸恢复、无截图异步帧、有限数值极值及同 ID 重试。斜边中间覆盖像素 1x 为 0、4x 为 320，证明实际 MSAA resolve，不只读取配置。

真实 512×512 图集按底行优先 64 层上传，层内 top-left；layer 0 四点 RGB 与归一化 CPU texel 匹配，报告保存 RGBA 观测，不声称全部 64 层都逐像素 GPU 验收。selection 只断言 RGB 像素变化，crimson 是源码保留值，不冒称精确颜色或 alpha 字节验收。

`005` 的真实最小化持续时间：Debug 1x/4x 为 **10009 / 10005 ms**，Release 为 **10012 / 10008 ms**，持续观察 SDL 与 Win32 状态；期间不追加 Render，恢复后真正绘制。正常重复关闭为 Debug 40/42 ms、Release 31/31 ms；这些不是故障最坏时长。

六张最终代表 PNG 已视觉复核：实际 atlas、物理 MSAA 斜边、远深度轴位置几何、插值 layer、resize 和 selection。按相同 case 与采样数对照 Debug/Release，1x 与 4x 各 33 对 PNG、两种 shader 恢复各 1 对，共 **68 对原字节 SHA 一致**，见 [最终复核记录](../../verification/archive/t3-r2/step2-20261009-001/final-check.json)。这不表示 1x 与 4x 相同，也不增加未执行的像素断言。它们是生产后端上的固定几何夹具，不是完整世界或真实游戏 Camera far 2000 迁移成绩。

### 失败与限定

Debug GPU `001` 的 atlas 测试错误用了顶行优先，两个 normal 失败；修为底行优先并增加 CPU/GPU texel 对照。`002` 错误要求全剔除 DrawInstanced 区间也有正时长，4x 失败；改为保留真实非负测量，并另要求至少一个非空 draw 有正实测，missing 从不填零。原失败不覆盖。

`003`/`004` 各 4/4 仅属于当时范围：`003` 大位置检查为空场景，`004` 才改为真正深度轴几何；两轮都没有十秒最小化 hold，不回填此成绩。只有最终 `005` 同时包含真实几何精度和十秒持续段。归档最初输入聚合错误另存 `validation-initial-001.json`，现行清单修正为 525 项并重核全部 822 对原字节，不把失效聚合当成完整来源身份。

missing/bad shader 各实际执行两次构造拒绝，然后同一 Native Window 用正确 DXIL 恢复绘制，每进程 15 项通过。**没有**强制设备丢失、Signal 失败或真正在途 timeout；quarantine 目前为源码审计，不作为 A14 动态成绩。无截图提交后立即逻辑销毁再绘制已测，但未强制 GPU gate，所以不声称真实重叠已证明。

ready query ring 有界 256；满时可丢最旧样本并诊断原 ID，不承诺每个丢弃 ID 都经 Poll 返回。当前 run 未触发容量溢出。顺序3 telemetry 必须保留缺失，不写成 0 或已测量。UPLOAD mesh 路径尚无完整世界上传/显存/性能验收。

当前启用 native 仍要求 BUILD_GAME=ON，并只支持分离的单配置 Debug/Release 构建；本轮未交付去 GL 的单现代游戏组合、Visual Studio multi-config 或可脱离 SDK 的运行包。相关候选交付属于后续步骤。

R1 冻结 12 份、R1 查验 114 份及其原始 out、顺序1 108 份 raw 均重新核 SHA，未变化；冻结 manifest 仍为 `ce27adb37619672634521bd5faca54ef605a3dcf73019826476746b897c49985`。

## 复跑与下一步

以下是开发验证入口，不是人工完整玩法候选。要求当前构建与源码身份相符；会开 Native 窗口、使用 Debug Layer，并新建输出，不覆盖 `005`：

```powershell
Set-Location 'F:\GameDevelop\OpenGLProject\Symocraft'
$Build = 'out/m3-t3/r2-step2-d3d12-release-001'
& './test/integration/renderer/verify-d3d12-backend.ps1' `
  -Executable "$Build/test/integration/renderer_d3d12_backend_smoke.exe" `
  -BuildDirectory $Build `
  -ShaderRoot "$Build/bin/assets/shaders/d3d12" `
  -OutputDirectory 'out/m3-t3/r2-step2-gpu-release-review-001'
```

顺序3再统一游戏调用、World/Chunk/成功发布 revision 持久缓存、真实 Camera/UV 转接、唯一 Present 与中立采样/退出；顺序4再交付生产候选及故障证据。唯一 [人工入口](manual-verification.md) 仍不新增或勾选 T3-M08 起项目，不提前宣称 R2/R3/T3 通过或切默认。

原生生命周期设计参考 Microsoft 的 [fence 资源管理](https://learn.microsoft.com/en-us/windows/win32/direct3d12/fence-based-resource-management)、[readback 与同步](https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps)、[queries](https://learn.microsoft.com/en-us/windows/win32/direct3d12/queries)；这些是实现依据，不替代本项目实测。
