---
type: 实施报告
status: R2顺序3统一调用已实施，完整世界短测通过，生产候选待顺序4
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestone
  - topic/rendering
---

# R2 顺序3：统一游戏调用

## 范围与状态

按用户授权完成 [R2 入口](r2-entry.md) 的顺序3。真实 D3D12 门面现在承接现有游戏的完整世界、相机、纹理、选择框、编辑和采样；输入、物理及交互仍共用一份应用循环。**默认仍为 OpenGL，R2 生产包及 R3 人工验收尚未交付。** 未切默认、删除 GL、恢复核显专项或改动冻结公开三头。

查验材料按节点保存于 [顺序3归档](evidence/r2/step3-20261009-001/README.md)，详细输入、原始副本 SHA 和实际结果见 [验证清单](evidence/r2/step3-20261009-001/validation-manifest.json)。归档不含 exe、库、PDB 或 obj，不是运行包；原始 `out/` 不移动或覆盖。

## 实施内容

| 范围 | 当前实现 |
| --- | --- |
| 启动与预检 | `--renderer opengl/d3d12/vulkan` 在建窗前选择；默认 GL。D3D12 使用 Native Window，无 GL context；未实现/未编入后端具名 Unsupported，无 fallback。现代 `--msaa`、`--validation` 不混入 GL 参数；资产检查只检查选中路径 |
| 私有游戏转接 | `GameRenderer` 隔离现代门面与旧 GL 差异，不复制输入/物理/交互。现代 `Renderer::Render` 唯一 Present；应用不再对 Native Window 额外交换缓冲 |
| 持久网格 | `WorldRenderCache` 以 WorldId + Chunk 身份记账，只在成功接受发布 `revision` 后更新记录。callback 内复制借用顶点；不拿 `mesh_input_revision` 当发布版本，不从未遍历记录推断卸载。显式 ReleaseWorld/ReleaseAll |
| 坐标与资产 | world-space 顶点不二次平移，model 恒等；实际 Camera FOV 度转弧度、RH、near 0.1 / far 2000。真实 512×512 RGBA 图集为64个64×64层、底行优先层号、层内 top-left、Linear；mesh V 仅变换一次 |
| 中立采样 | 延迟 GPU 样本按稳定 frame ID 回填；缺失保留原因、不填0。现代设备/API/driver/实际 MSAA/呈现独立记录，不冒充 `gl_*`；旧 GL CSV/schema 与计时口径保留 |
| 上传归属 | startup 记录 atlas 与初始网格；帧内分别记录 cache Create/Update 与 Render-local selection staging，成功 bytes/calls 只计一次。cache CPU 时间包括扫描、转换、上传及 draw-list 组装，不冒称纯 DMA 耗时 |
| 截图 | PNG 写入提取为 assets 私有实现；现代保存 top-left RGBA，旧 GL 保留 GL_BACK/RGB/翻转。额外诊断帧在采样区间外，其 GPU ID 不混入正式样本 |
| 生命周期 | 正常显式释放世界缓存；异常也不保留 World 借用。图形对象、query/cache 和门面先于 Window 销毁，逐段清理保留首故障；底层 quarantine 策略不由 app 强行回收替代 |

成员与方法见 [GameRenderer](../../project/architecture/app/GameRenderer-类设计.md)、[WorldRenderCache](../../project/architecture/app/WorldRenderCache-类设计.md) 和 [App 编排](../../project/architecture/app/App-运行编排-功能.md)。CPU helper 是真实生产 target `symocraft_app_render_data`，不依赖测试 recorder、R1 实验或 SDK。

## 实际验证

Windows x64，MSVC 19.38.33145 / tools 14.38.33130、SDK 10.0.22621.0、DXC 1.6.2112.16、CMake 4.3.5、Ninja 1.13.2。四套构建均新建目录并完整构建；CPU-only 的 D3D12 缓存请求可为 ON，但有效 native 分支关闭、不发现图形 SDK/DXC。现行 native 游戏是保留 GL 的过渡组合，不是无 GL 依赖的单现代包。

| 项目 | Debug | Release |
| --- | --- | --- |
| CPU-only 初次全量 CTest | 56/56 启用项通过；world.allocations 原有禁用 | 57/57 |
| native 初次全量 CTest | **64/65，foundation.files Win32 1175 失败**；world.allocations 原有禁用 | 66/66 |
| 最后改动后的重点回归 | CPU 6/6；native 8/8 | CPU 6/6；native 8/8 |
| 最终公开头/边界/启动/旧协议/资产回归 | CPU 37/37；native 42/42 | CPU 37/37；native 42/42 |
| 增量边界 | CPU 3个 / native 5个 probes | CPU 3个 / native 5个 probes |
| 最终 `004` 实际 D3D12 游戏 | static / walk / edit 各 exit0，3/3 | static / walk / edit 各 exit0，3/3 |
| 最终 `004` 旧 GL 对照 | 未另跑 Debug GL 工作负载 | static / walk / edit 3/3 |
| 无采样器普通玩法短测 | 请求400帧后 exit0 | 请求400帧后 exit0 |

全量结果是当时完整套件，之后修正采样口径/跳帧及无采样器 drain，再执行重点与边界回归和最终同源 GPU `004`。不将分组测试相加冒称最终全量重跑，也不由 Release 的通过关闭 Win32 1175、暂停的 Fix1、正式同源 Q06 或首次可操作时间预算。

另新建GL-only Release，关闭D3D12，仅构建实际 `SymoCraft` target：六项无关CWD预检通过。默认资产检查exit0；未编入D3D12的运行/资产入口及Vulkan均建窗前 `SelectBackend` exit3；显式/隐式GL的现代MSAA参数exit64。见 [预检记录](evidence/r2/step3-20261009-001/raw/out/m3-t3/r2-step3-gl-selection-001/verification.json)。没有运行该构建的全量CTest或GPU，不扩大为GL-only整组验收。

### 完整世界与上传

最终 [Debug](evidence/r2/step3-20261009-001/raw/out/m3-t3/r2-step3-game-debug-004/verification.json) / [Release](evidence/r2/step3-20261009-001/raw/out/m3-t3/r2-step3-game-release-004/verification.json) 均使用 radius10、441区块、seed424242、完整默认世界与既有 regression fixtures，未缩减世界。首轮发布361个非外围网格；启动累计58,818,904 bytes、362次调用，含1次 atlas 和361次网格上传。地形摘要 `fnv1a64:cd80ebb0446c15c6`、初始场景摘要 `fnv1a64:bddd435ea737fa62` 在两配置及 GL 一致。

实际设备为 NVIDIA GeForce RTX 5070 Ti，LUID `00000000:0001435b`、PCI `10de:2c05`、driver `32.0.16.1664`；FL12_0 / SM6_0。真实1920×1080、4x MSAA、Immediate、validation开启；UNORM存储，无输出sRGB编码。

| D3D12 工作负载 | Debug 正式帧/GPU样本 | Release 正式帧/GPU样本 | 资源上传 |
| --- | --- | --- | --- |
| static，2秒 | 317 / 317 | 450 / 450 | cache几何 bytes/calls 均0；selection staging 另计 |
| walk，3秒，真实物理移动 | 381 / 381 | 669 / 669 | cache几何 bytes/calls 均0，摘要不变；CSV实际位置变化 |
| edit，9秒，既有正负坐标/四块交点计划 | 920 / 920 | 1757 / 1757 | 两配置均35次计划编辑、105次网格更新、25,137,000 bytes；逐帧调用数等于实际发布重建数 |

六次 native 的 GPU 缺失均0；这不是承诺每次运行都有全部样本。GL static 为276帧/275有效GPU/1缺失，walk302/302、edit995/995；旧缺失保持空 CSV 单元格。edit最终摘要均为 `fnv1a64:dc99578b13c5bcf2`，既有 forward16/reverse16计划已实际执行，但这是计划世界写入，不冒称人工鼠标输入回放。

九张最终1920×1080 PNG已逐张视觉复核，方块/纹理/草地/天空可见，静态与编辑画面有选框，无空白或倒置；三个 D3D12 工作负载的 Debug/Release 解码RGB hash各一致。静态 GL/D3D12 比较有10,082/2,073,600像素不同，RGB平均字节差0.068451、最大103；edit为10,075像素、平均0.068542。见 [像素观察](evidence/r2/step3-20261009-001/raw/out/m3-t3/r2-step3-game-release-004/image-observations.json)。只记录真实差异，没有预定容差或逐资产GPU断言，不据此宣称全部视觉等价或性能无回归。

无 Session 的普通玩法非阻塞取出并丢弃样本，避免长期无人消费导致256项有界环持续溢出。两次400帧请求短测均正常退出；没有独立帧数统计或捕获 OutputDebugString，不把 stderr 空白说成 Debug Layer 零消息。

### 边界与原失败

私有 backend/pipeline 变动不重编 world、模拟或 app；公共 types 精确重编2个 CPU app helper，native另重编 application/game_renderer，禁止用 main 误编替代计数。私有 world cache 只重编所属 helper、cache测试及 native game_renderer；共享HLSL只生成4份DXIL、C/CXX为0。探测恢复原字节/hash与mtime。

保留全部早期失败：CPU与GL-only沙箱C1902；启动参数单测旧 unsigned literal 的 /WX告警；Debug game001 的空 stdout PowerShell读取错误；game002 探针误要求无前缀16位摘要；native incremental001 因探针新变动 baseline未稳定而拒绝；native boundary003 资产预检及打包沙箱访问拒绝。各自另轮修正/正常环境复测，不覆盖原记录。

只读复核还发现：Skipped Present 提前把GPU列标已决，会与后端迟到结果冲突。现只使运行无效，不预填该列，backend discarded/有效/未返回结果各正常归属；新增CPU回归验证一次回填与重复拒绝。该回归不是动态最小化/遮挡验证。

## 开发复跑

以下为当前同源开发构建入口，不是无SDK生产包。新建输出目录，勿覆盖 `004`：

```powershell
Set-Location 'F:\GameDevelop\OpenGLProject\Symocraft'
& './test/integration/renderer/verify-d3d12-game.ps1' `
  -BuildDirectory 'out/m3-t3/r2-step3-d3d12-release-001' `
  -OutputDirectory 'out/m3-t3/r2-step3-game-release-review-001' `
  -IncludeOpenGLComparison
```

顺序4再交付真实候选、完整来源/运行依赖和场景入口、隔离故障与人工查验命令。**本轮未完成14项人工玩法、15分钟连续操作、生产包安装运行、实际 DeviceLost/提交失败/在途timeout、完整游戏窗口恢复或R3批准。** 不新增/勾选 [人工主表](manul-verification.md) 的 T3-M08起项目。R1与旧节点raw保持原字节，当前获授权生产改动不倒写旧来源成绩。
