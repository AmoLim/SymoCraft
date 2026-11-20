---
tags:
  - area/architecture
---

# Assets

## 边界与权威

`symocraft_assets` 提供以可执行文件为根的资源定位、自有字节/CPU 图片解码、PNG 写入与角色 GLB 的静态/刚性分件导入，不解释方块规则、持有 GPU 或改变 CWD。Windows 路径实现、stb 与 cgltf 私有，公开依赖 foundation 与 scene 的中立 CPU 网格；边界见 [module-boundaries](../openspec/specs/module-boundaries/spec.md)。GPU 接受/截图值与生命周期由 [renderer spec](../openspec/specs/renderer/spec.md) 管理。
实现入口：[CMake](../../game/modules/assets/CMakeLists.txt)、[路径](../../game/modules/assets/src/asset_paths.cpp)、[解码](../../game/modules/assets/src/image.cpp)、[写入](../../game/modules/assets/src/image_write.cpp)、[公开头](../../game/modules/assets/include/symocraft/assets)。

## 路径与完整读取

`ExecutableDirectory` 动态扩展 `GetModuleFileNameW` 缓冲，返回 exe 的父目录；`Root` 是其 `assets` 子目录，不承诺存在。`Resolve` 拒绝空值、根路径、NUL、冒号/备用数据流和任何 `..`；weakly-canonicalize 后逐路径组件比较，要求结果是根内严格子路径，根自身也不接受。Root 自身的 link/junction 也先解析为检查根，并非禁止所有链接；根内既有链接解析后越界才拒绝。检查与实际打开之间仍可发生文件系统变更，不是事务或安全隔离，现有路径测试没有 link/junction 专项夹具。

Resolve 返回自有 path，既不保证文件存在，也不检查内容。**ReadBytes/DecodeImage/WritePng 不自动 Resolve**，可以处理调用方选择的其他路径；路径政策不能从加载函数隐含推定。启动自检仅检查六个固定 legacy 必需项为普通文件，不解码或验证现代 shader/PSO；选中现代后端的资源预检留 app，不扩大这份静态表的含义。

运行期 `RequiredFiles` 与构建期 [RequiredAssets.cmake](../../cmake/RequiredAssets.cmake) 是两份实际清单；新增/移除必需项须同时核对 staging、install 与启动检查，不能把运行期缺文件测试当作自动证明清单无漂移。宽字符定位和动态缓冲不等于游戏全链路 Unicode/长路径保证：[legacy renderer](../../game/modules/renderer/src/legacy_renderer.cpp) 的纹理/GLSL 路径仍转窄 `string()`，而当前 app 的 YAML 读取已使用 Assets::ReadBytes(path)。

`RequiredFiles` 返回静态只读数组/字符串借用，进程期有效。`CheckRequiredAssets` 借用诊断 ostream 到返回，逐项捕获解析/检查异常并继续、整体 false；诊断流自身故障仍可逃逸，不是无异常接口。
`ReadBytes` 按文件长度分配自有 vector 并完整读取，空文件可返回空；打开、长度、短读或分配失败抛异常，不返回部分字节，不缓存 caller 的 path/stream。

## 图片所有权与解码

Image 聚合默认 width/height/channels=0、pixels 空；成功解码取得正尺寸、源通道 1..4 与紧密行字节。字段公开，任意手工构造或移动源不自动满足有效图片条件；renderer/app 必须校验自己的支持格式，assets 的 1/2 通道能力不等于游戏纹理接入能力。

DecodeImage 先完整读文件，检查输入长度可用 stb 的 int 表示；每次显式设置**当前线程**翻转选项，再保留源通道解码。局部 unique_ptr 负责释放 stb 内存，复制到 Image 自有 vector 后返回；读取/解码/分配失败不交付半成品。该加载期复制不进入逐帧渲染。
Image 复制深复制字节；标准移动后的源只有效但状态未指定，数值尺寸不会自动被重置为重新解码成功值。pixels 增长、赋值、释放或 owner 销毁可使指针/引用失效；返回不借 stb 缓冲。

方向由调用方显式选择，不靠修改全局 GLM 或加载后端修 World 数据。当前 legacy atlas 使用翻转解码；现代 app 用 top-left 解码，再 [render_conversion](../../game/app/src/render_conversion.cpp) 保留原底行优先层号、按层 top-left 行排列并转换一次网格 V。颜色空间/通道扩展是消费端的明确选择，不是 DecodeImage 的隐式转换。

## PNG 输出与失败状态

`WritePng` 同步借用紧密 top-left pixels，支持 1..4 通道并保持 caller 字节不变。打开文件之前检查正尺寸、通道、int 行长及编码器容量边界、恰好匹配的 byte count；非法参数抛 invalid_argument，不创建输出。需要翻转时先分配局部自有副本并逐行逆序，不使用跨调用翻转开关或修改源。

写入采用 `ofstream(binary|trunc)` 和私有 stb 回调，完成编码后 close 并检查编码/流状态。打开/编码/写入失败具名 runtime_error；**打开后的失败可能留下截断或部分文件，不承诺保留旧输出，也不是 Files::Publish 的原子替换**。分配失败正常传播；API 不创建父目录，不自动清除失败样本，caller 负责输出位置与后续处置。
输入 span 只借到返回；调用期间 caller 不得修改/释放像素。截图 readback/GPU 同步早已属于 renderer，写 PNG 不证明 GPU 读回完成或测量区间有效。

## 稳定不变量与验证边界

| 原身份 | 继续成立的边界 |
| --- | --- |
| Assets I1 | DecodeImage 成功时 pixels.size()==width*height*channels；不保证所有公开聚合始终一致。 |
| Assets I2 | Resolve 正常返回严格根内子路径；不等于存在/成功解码或并发文件系统隔离。 |
| Assets I3 | 解码失败抛异常，不返回半初始化图片，临时 decoder 缓冲由 RAII 释放。 |

没有共享缓存、异步任务/取消或完整并发/重入承诺；线程翻转隔离不证明同路径读写、全部解码故障或 OOM 已专项验证。资源规模/复制瓶颈需测量，不从值语义宣称性能优化。
[asset_paths_tests](../../test/unit/assets/asset_paths_tests.cpp) 核对 CWD 无关/搬迁包和非法路径；[image_tests](../../test/unit/assets/image_tests.cpp) 核对通道/翻转；[image_write_tests](../../test/unit/assets/image_write_tests.cpp) 覆盖 1..4 通道、写入翻转隔离、尺寸与文件失败。存在测试不等于本轮重跑，Unicode 路径探针也不覆盖任意旧游戏 loader。
历史成绩及点时身份从 [VERIFICATION](../verification/VERIFICATION.md) 进入；本轮只有源码/文档核对，未运行图片、安装包、GPU 或分配故障验收。

## 角色导入、刚性分件与 Blender 交付

[LoadCharacter](../../game/modules/assets/src/character.cpp) 在 CPU 上私有使用固定 cgltf v1.15，将自包含 GLB 转成 [CharacterMeshData](../../game/modules/scene/include/symocraft/scene/character_mesh.h) 的自有浮点三角形。输入文件字节和 cgltf 对象仅活到本次调用结束；返回网格不借解析器、文件或 GPU。调用失败具名报告路径与原因，不返回半成品。

支持 POSITION、NORMAL、COLOR_0，颜色三或四分量，alpha 必须为 1；有索引时展开为完整三角形。完整绑定姿态的 `vertices` 保留原静态路径：node 的仿射变换烘焙到位置，逆转置变换并归一化法线，负行列式调整绕序。验证缓冲区/accessor 边界、索引范围、有限值与颜色范围；文件上限 64 MiB，accessor 与最终展开网格上限各 1,048,576 个顶点，节点上限 128。所有节点（含辅助节点和父节点）在计算世界变换前校验有限、仿射、非奇异及无环。外部缓冲、材质/纹理、透明、稀疏、压缩、蒙皮、形态目标、动画及 required extensions 具名拒绝。这是受限角色格式入口，不是通用 glTF 场景加载器。

GLB 根空节点 `symocraft_rigid_v1` 显式启用分件契约；无该标记的静态输入仍返回完整网格和空 `parts`。分件模型必须恰有具名的 `body`、`head`、`left_arm`、`right_arm`、`left_leg`、`right_leg` 六个网格，拒绝缺件、同名和未归属网格。`CharacterPart` 持有自有局部顶点、名称、最近部件父索引与局部绑定变换。非部件祖先的变换折入对应绑定变换，保留部件间层级；折算后的矩阵再次校验。分件当前拒绝世界变换反射，整体静态输入仍保留负行列式绕序支持。

各部件的局部原点是显式 pivot，随模型返回零 `pivot`；其直接空子节点 `<部件名>_axis` 的非零平移向量明确旋转轴，不能带旋转、缩放、网格或子节点。导入检查该节点唯一、归属正确、方向有限且长度非零，再归一化保存 `rotation_axis`。肩和髋位置由 Blender 中设定的部件原点决定，不依据顶点或包围盒猜测。模型数据同时保留静态绑定三角形与部件局部三角形；返回后不借解析器，失败时整个局部候选销毁。姿态生成和 GPU 所有权属于 app/renderer，导入器不推进动作或保存每个 NPC 的姿态。

[human.blend](../../assets/characters/human.blend) 与 [zombie.blend](../../assets/characters/zombie.blend) 保留可编辑顶点色及硬边模型，对应 GLB 是 Blender 实际导出。源模型按米建模，原点位于脚底，Blender Z 向上、-Y 向前；GLB 导出为 +Y 向上、+Z 向前。编辑时保留分面的 NORMAL、活动 Color 属性及不透明颜色，不加材质/贴图、rig 或动画。导出选择 GLB、+Y Up、Normals、Active vertex colors、无 materials/skins/morph/animations。2026-11-16 扩展后的模型保留六个身体网格和明确肩髋原点；脸、眼睛和头发仅合并到头部，随后随头部刚性运动。躯干挂在标记根下，头和四肢挂在躯干下，各有显式轴向空节点。网格仍无蒙皮或动画片段，简单动作由消费端程序旋转实现；原静态候选验证仅覆盖当时范围，扩展行为见 [角色资产规格](../openspec/specs/character-assets/spec.md)。

[scripts/create-character-fixtures.py](../../scripts/create-character-fixtures.py) 记录现有两份模型的可重复创建/导出方法，用 Blender 4.5.16 LTS 的 `blender.exe --background --factory-startup --python scripts/create-character-fixtures.py` 执行；它会覆盖这两组源与导出文件，仅在明确要重建夹具时运行。Blender 是离线资产工具，不随游戏安装或参与 CPU/GPU 运行依赖。`assets.character_import` 通过生产入口读取两份实际 GLB，核对坐标、颜色、硬边法线、自有寿命、分件层级和肩髋轴，以及缺失/重复部件、零轴、奇异/非有限变换、循环层级、节点超限及静态兼容；执行结论仍归本 change 的 verification。
