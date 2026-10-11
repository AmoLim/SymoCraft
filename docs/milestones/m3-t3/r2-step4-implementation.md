---
type: 实施报告
status: R2顺序4候选已交付，人工及R3整组验收未完成
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
updated: 2026-10-09
tags:
  - area/milestones
  - topic/rendering
---

> 2026-10-11 文档浓缩后已退役复核源：现行入口见 [权威位置](../../verification/VERIFICATION.md#原位置保留的历史材料)；原文保留点时反馈、历史身份和复核依据，不再维护为当前机制或活动计划，旧勾选不产生新批准。

# R2 顺序4生产候选交付

按用户批准完成同源构建、真实安装/打包、完整游戏与窗口复核，提供固定人工入口。**本轮交付候选，不批准R3，不切OpenGL默认，不交付Vulkan，不恢复核显专项。** 依据 [R2入口](r2-entry.md)、[T3 spec](../../openspec/changes/m3-t3-renderer/proposal.md)；唯一人工主表及逐项命令为 [T3-M08至M15](manual-verification.md#当期人工核验主表r2-至-r3)。

查验材料已复制到 [节点归档](../../verification/archive/t3-r2/step4-20261009-001/README.md)，不使用库外相对out链接。raw共4430份、只读；[清单](../../verification/archive/t3-r2/step4-20261009-001/validation-manifest.json) SHA为 `fe63e4bef9407648b4ea52ede631549c83678cf6fb75fa69eaae508acf09a008`。原始输出和可运行包仍在工程out，旧轮与失败不覆盖。五份R1/顺序1至3历史清单SHA仍一致，不把旧来源改为当前源码。

## 实施范围

| 范围 | 本轮实现与约束 |
| --- | --- |
| 构建/安装 | native游戏构建私有发现Python3.10+；实际链接后POST_BUILD记录工具、3282项生产输入和12份构建产物；不从现成旧exe生成伪“当前源码”身份 |
| 生产打包 | `scripts/package-d3d12-candidate.py`先核对实际源码/工具/exe/DXIL/CMake/Ninja身份，再执行真实cmake install；新输出、精确相对文件清单/SHA/x64 PE、实际dumpbin imports与CRT来源检查 |
| 增量准备 | Ninja仅允许glob检查及既有asset同步；出现配置/编译/链接/身份重录步骤即拒绝打包，不在验证中偷偷重建；资产同步后再核对已记录产物 |
| 生命周期诊断 | 主入口保留原RenderError文字并追加category/native/diagnostic，普通循环记录暂停原因变化、恢复和实际rendered_frames；只增加观测，不改变玩法/同步算法 |
| 运行验证 | Python复制真实安装包到带空格目录，在无关CWD、system-only PATH与移除SDK/编译变量的子进程中运行；原包精确文件集和字节前后守卫 |
| 窗口验证 | Python标准库ctypes、严格PID/HWND，正常可见启动；真实最小化12秒、恢复/缩放、只给目标WM_CLOSE；有界正常关闭，强制结束不算通过 |
| 自动化政策 | 新增文件/编排/测试/归档默认Python标准库，不新增PS1；PowerShell仅加载MSVC厂商环境后启动Python。规则见 [project-scope](../../scope/project-scope.md#开发自动化约定) 与 [脚本规范](../../scope/C-Scripting-Policy.md) |

六个新增交付脚本为packager、build/verify candidate、verify window、archive和Python单测；构建/验证要求Python3.10+，实际使用3.14.5。Python仅为构建/查验依赖，包内游戏不依赖Python。截图数值分析单独使用已提供的Pillow/NumPy，不新增项目运行依赖；通用文件处理可跨平台，D3D12/MSVC/Win32流程明确限Windows，不声称Linux/macOS实测。

## 候选身份

实际包根为 `F:/GameDevelop/OpenGLProject/Symocraft/out/m3-t3/package/`；两者都是完整生产过渡游戏，不是R1实验或测试fixture，不含测试exe/库/PDB。

| 字段 | Debug | Release |
| --- | --- | --- |
| 当前候选 | `d3d12-game-debug-002` | `d3d12-game-release-002` |
| 有效载荷文件数，不含manifest自身 | 58 | 56 |
| manifest SHA256 | `b3db1103f9241f926a5dcf5f0df11eb0ec55315f4b302e5b0e03fb1c8e9ce8f8` | `dbf317beb24ad0ece7d9535216bbf919999bb2501f479adf19a4c8a3464ff086` |
| 实际exe SHA256 | `766081a2462a58f645d7b4d538b73b135ad80c6a527f2ecd4c7906e66dd686a2` | `e31d353a3b61f5c348493c5b07602d62eb9940e7a62a40d286642fa1092d1726` |
| CRT政策 | 本机开发诊断，Debug_NonRedist/debug UCRT不可对外再分发 | 匹配选中VC143工具集的app-local Release CRT；不等于新机器部署通过 |

两配置的3282项生产输入逐项相同且与归档时当前源码/资产一致；涵盖game/cmake/assets、选定vendor、root CMake及packager，不包含整个SDK/驱动/操作系统，也不把并发测试/通用脚本改动冒充生产来源未变。最终producer来自matrix004目录，经final-delivery002实际重建链接并重录安装/工具身份；不是把首次matrix004旧记录当成最终metadata。

实际工具为MSVC19.38.33145/tools14.38.33130、SDK10.0.22621.0、DXC1.6.2112.16、CMake4.3.5、Ninja1.13.2。没有用最小支持版本做兼容实测。

## 构建与边界

| 最终完整CTest | Debug | Release |
| --- | --- | --- |
| CPU-only，final-cpu001 | **59/60启用项；foundation.files Win32 1175失败**；world.allocations原有禁用 | 61/61 |
| native，final-delivery002 | 69/69启用项；world.allocations原有禁用 | 70/70 |
| 本轮候选Python单测 | 24/24，最终交付前重跑 | 同一脚本套件，不另相加计数 |

公开头、CPU门面/寿命、生产转接、图形SDK隔离、目标/负向边界均在对应完整套件内。四套目录最初全量fresh构建完成，后因许可证install与并发Python测试注册刷新；最终表按实际JUnit统计，不混加初轮成绩。CPU-only请求D3D12不发现或链接图形SDK；实际native是 **D3D12 + 隔离旧GL过渡组合**，不是现代独占包。

后端私有头、公开契约、cache和HLSL的增量依据见 [顺序3](r2-step3-implementation.md#边界与原失败)，保持其独立来源/范围，不声称本轮另跑增量探测。关闭测试native、benchmark-only、benchmark+native禁止组合的补充001均在configure因沙箱C1902退出；提权002审批被中断、未执行，无构建/CTest/具名失败成绩，见 [收尾记录](../../verification/archive/t3-r2/step4-20261009-001/raw/out/m3-t3/r2-step4-notests-matrix-001/closure.json)。现代独占及游戏全后端关闭组合还未交付；A02/A03不整项勾选。

Win32 1175在matrix004两native、final-links002 nativeDebug和最终CPU-onlyDebug均有原始失败，最终native两配置通过不表示根因修复、Fix1恢复或Q06通过。正式同源Q06/首次可操作时间预算继续保持T2遗留状态。

## 完整游戏

最终真实包的 [Debug004](../../verification/archive/t3-r2/step4-20261009-001/raw/out/m3-t3/r2-step4-candidate-debug-004/verification.json) 与 [Release002](../../verification/archive/t3-r2/step4-20261009-001/raw/out/m3-t3/r2-step4-candidate-release-002/verification.json) 分别14/14、11/11检查通过，包括assets、普通400帧、三组D3D12、隔离故障/未交付backend；Debug另有三组GL，负载报告均由真实C++探针校验。400帧普通运行没有Session，日志实际rendered_frames=400，不只看请求值。

本机实际RTX5070Ti，LUID `00000000:0001435b`、PCI `10de:2c05`、driver `32.0.16.1664`；FL12_0/SM6_0、1920×1080、4x/Immediate、validation开启。固定radius10、441区块、seed424242、361初始非外围网格，启动58,818,904 bytes/362calls；terrain `fnv1a64:cd80ebb0446c15c6`、scene `fnv1a64:bddd435ea737fa62`。

| D3D12短负载 | Debug正式帧/有效GPU | Release正式帧/有效GPU | 资源行为 |
| --- | --- | --- | --- |
| static，2秒 | 278/278 | 112/112 | cache几何bytes/calls=0；selection staging另计 |
| walk，3秒 | 295/295 | 418/418 | 实际位置变化，cache几何bytes/calls=0 |
| edit，9秒 | 726/726 | 385/385 | 两配置均35次计划编辑、105次更新、25,137,000bytes；final `fnv1a64:dc99578b13c5bcf2` |

六次native GPU缺失为0，不承诺所有运行都零缺失。Debug旧GL static183/183、walk241/240并保留1个缺失、edit1070/1070；不把旧GL逐帧重传当现代持久缓存指标。短负载只证明功能/关联/上传，不作60秒预热+180秒正式基准或同条件性能预算判定，后台构建等因素也未作为性能控制消除。

九张最终PNG逐张视觉复核、1920×1080且非空；D/R三个native解码RGB均相同。static GL/native差10,082像素、RGB平均字节差0.068451；edit差10,075/0.068542。walk GL与native最终CSV x为-7.593261719与-7.299926758，不能将非同位姿图按静态栅格容差判断等价。正式参考图/容差未批准，观测不代替A06/A07或人工材质/玩法验收。

## 生命周期与故障

最终完整游戏窗口 [Debug003](../../verification/archive/t3-r2/step4-20261009-001/raw/out/m3-t3/r2-step4-window-debug-003/verification.json) 与 [Release004](../../verification/archive/t3-r2/step4-20261009-001/raw/out/m3-t3/r2-step4-window-release-004/verification.json) 均正常exit0：连续最小化12508.125/12067.107ms，暂停/恢复rendered_frames129/129、131/131，最终376/982。OS实际minimize/client/outer矩形与生产暂停日志独立记录；普通启动4x/Fifo。日志extent1280×720不是OS client矩形，报告保留二者，不假称已测恢复像素/Renderer framebuffer或native fence/Present计数。实际Esc仍为人工项。

原生backend夹具Debug正常002、Release正常001与两配置缺/坏shader共六项最终通过，范围是固定几何/实际图集/空网格、资源/句柄、4x物理resolve、数值极值、VSync/query/readback、真实最小化与重复Shutdown；不是完整世界。无截图提交后立即逻辑destroy未强制GPU重叠，不扩为真实在途停滞/回收证明。

| 完整包隔离故障 | 两配置实际结果 |
| --- | --- |
| 缺blocks.vs.dxil | preflight exit2，选中D3D12/资源具名，未建窗 |
| 坏blocks.vs.dxil | 构造exit3，保留初始化category/native HRESULT/诊断，已创建Window正常清理 |
| 坏atlas / YAML | exit3，保留真实解码/解析错误、请求后端及清理 |
| 无效renderer | 参数exit64；不建窗，不隐式回退 |
| 未实施Vulkan | SelectBackend Unsupported exit3；不进入D3D12/GL假成功 |

原失败原样保留：沙箱C1902/资源weakly_canonical拒绝/临时文件WinError5；packager首次误把Ninja glob和asset同步当stale（已改白名单并测）；验证器SystemRoot大小写错误（已修并测）；Debug窗口001被提前恢复，Release窗口001/002隐藏启动焦点失败（修为正常可见启动，复测通过）；native Debug首次DXGI Occluded被旧夹具的“仅Presented可有GPU样本”断言拒绝，隐藏子进程控制台复跑通过，未修改生产renderer或伪造截图；final-delivery001新增ZIP测试被Windows ZipInfo构造规范化造成假阴性，并发修正夹具后002全量通过。没有原生崩溃归因或未实测修复宣称。

## 发布与协议

安装包带实际游戏/assets/four DXIL、app-local匹配CRT、来源/工具/imports、第三方notices；不携带DXC/开发SDK/Debug Layer，不动态依赖SDL。GL过渡资产及opengl32仍保留。额外补齐SDL3 libm/Sun、qsort、rotate、CC0、内置stb/miniz、Brian Paul/VMware OpenGL与KHR/Vulkan桥接的本地完整notice来源；平台Vulkan桥接不等于生产Vulkan后端。yaml-cpp原始vendor版本仍未知，SOURCES明确保留，不虚构版本或批准正式商业发布。

从带空格包路径、无关CWD、system-only PATH/移除编译SDK变量实际运行通过；这是同一开发主机环境裁剪，不是卸载SDK或新无SDK主机实测。图形运行时/驱动/Graphics Tools仍由机器提供。Release CRT再分发政策不等于完整部署兼容或许可审核已全部关闭。

现代报告保持中立字段/frame ID/实际设备，不冒充gl；真实C++探针与Python候选验证校验PNG/CSV/YAML及缺失原因。旧GUI benchmark仍硬编码GL协议，尚不能消费现代CSV或选择现代后端；不能把本轮独立验证器当成GUI工具已适配。

## 下一步边界

可按 [唯一人工入口](manual-verification.md#附录-b-r2-生产候选命令) 开展已交付候选的核查，但八项均待人工，不代填14项玩法或15分钟。实际DeviceLost/提交失败/真正在途timeout/分配失败、现代独占/全关闭组合、未执行的关闭测试/工具补充配置、GUI现代协议及正式视觉/性能标准继续单列。**顺序4交付不使A01–A19、R3、默认切换、Vulkan或整个T3通过**；后续工程和节点授权独立确认。
