---
tags:
  - area/architecture
---

# Assets

## 边界与权威

`symocraft_assets` 提供以可执行文件为根的资源定位、自有字节/CPU 图片解码与 PNG 写入，不解释方块规则、持有 GPU 或改变 CWD。Windows 路径实现、stb decoder/writer 私有，公开依赖 foundation；边界见 [module-boundaries](../openspec/specs/module-boundaries/spec.md)。GPU 接受/截图值与生命周期由 [renderer spec](../openspec/specs/renderer/spec.md) 管理。
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
