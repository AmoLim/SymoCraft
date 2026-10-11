---
type: 查验归档
status: R2顺序3开发验证，非运行包或R3验收
project: Symocraft
module: M3-T3-Renderer
created: 2026-10-09
tags:
  - area/evidence
  - topic/rendering
---

# R2 顺序3查验归档

主报告：[统一游戏调用](milestones/m3-t3/r2/r2-step3-implementation.md)。本节点包含 **785份原字节只读副本、33,397,051 bytes、549项定域输入身份**，详见 [验证清单](validation-manifest.json)。清单 SHA-256 为 `9246eeed80bfd3f7d5b0a7cd36bc43ee17cb4c74a7ed9b2bdb29fb9b10cf6879`。

原始 `out/` 未移动或覆盖；本归档不包含exe/dll/lib/obj/PDB，不是生产运行包，JSON中的原始绝对地址是运行来源记录。下方产物链接全部位于Obsidian的 `docs/` 库内。源码和资产副本用于查验，不自动取得公开API或人工验收资格。

## 实际入口

| 材料 | 用途 |
| --- | --- |
| [最终Debug游戏](raw/out/m3-t3/r2-step3-game-debug-004/verification.json)、[最终Release游戏/GL对照](raw/out/m3-t3/r2-step3-game-release-004/verification.json) | D3D12各三组完整世界短测；Release另三组旧GL对照，各自真实帧/GPU/上传/摘要和退出码 |
| [Debug静态摘要](raw/out/m3-t3/r2-step3-game-debug-004/d3d12-static/summary.yaml)、[Release编辑摘要](raw/out/m3-t3/r2-step3-game-release-004/d3d12-edit/summary.yaml) | 实际设备、1920×1080/4x/Immediate/validation、441区块、计时与上传归属 |
| [Debug编辑CSV](raw/out/m3-t3/r2-step3-game-debug-004/d3d12-edit/frames.csv)、[Release编辑CSV](raw/out/m3-t3/r2-step3-game-release-004/d3d12-edit/frames.csv) | 稳定frame ID、GPU迟到回填、每帧网格发布/上传、35次计划编辑 |
| [像素观察](raw/out/m3-t3/r2-step3-game-release-004/image-observations.json) | 九图非空像素范围；D3D12双配置RGB一致及GL差异实数，无预定义验收容差 |
| [GL-only选项预检](raw/out/m3-t3/r2-step3-gl-selection-001/verification.json) | 实际关闭D3D12的Release游戏：未编入后端建窗前具名拒绝，无fallback；六项通过，不是全量GL验收 |
| [无采样器玩法短测](raw/out/m3-t3/r2-step3-no-collector-001/verification.json) | 两配置请求400帧后正常退出，无独立实际帧数/调试输出流统计 |
| [旧证据与最终输入复核](raw/out/m3-t3/r2-step3-final-check-001/final-check.json) | R1冻结12份、查验114份、顺序1 raw108份、顺序2 raw822份及相应原始out未变；最终GPU输入/产物hash复核 |

## 画面查验

九张最终图已逐张视觉复核。静态/编辑画面有选框，walk的最终画面没有选框；不将短测说成人工完整玩法。

| 工作负载 | D3D12 Debug | D3D12 Release | OpenGL Release |
| --- | --- | --- | --- |
| static | [PNG](raw/out/m3-t3/r2-step3-game-debug-004/d3d12-static/final-frame.png) | [PNG](raw/out/m3-t3/r2-step3-game-release-004/d3d12-static/final-frame.png) | [PNG](raw/out/m3-t3/r2-step3-game-release-004/opengl-static/final-frame.png) |
| walk | [PNG](raw/out/m3-t3/r2-step3-game-debug-004/d3d12-walk/final-frame.png) | [PNG](raw/out/m3-t3/r2-step3-game-release-004/d3d12-walk/final-frame.png) | [PNG](raw/out/m3-t3/r2-step3-game-release-004/opengl-walk/final-frame.png) |
| edit | [PNG](raw/out/m3-t3/r2-step3-game-debug-004/d3d12-edit/final-frame.png) | [PNG](raw/out/m3-t3/r2-step3-game-release-004/d3d12-edit/final-frame.png) | [PNG](raw/out/m3-t3/r2-step3-game-release-004/opengl-edit/final-frame.png) |

## 构建与原失败

完整源/target/命令清单、cache、构建日志与各轮CTest均保留于 `raw/out/m3-t3/r2-step3-*-001/`。CPU完整构建各通过；native Debug全量仍有 [Win32 1175失败](raw/out/m3-t3/r2-step3-d3d12-debug-001/ctest-001.log)，不据Release通过关闭T2遗留项。

最终重点回归：[CPU Debug](raw/out/m3-t3/r2-step3-cpu-debug-001/ctest-focused-003.log)、[CPU Release](raw/out/m3-t3/r2-step3-cpu-release-001/ctest-focused-003.log)、[native Debug](raw/out/m3-t3/r2-step3-d3d12-debug-001/ctest-focused-003.log)、[native Release](raw/out/m3-t3/r2-step3-d3d12-release-001/ctest-focused-003.log)。最终公开边界与资产回归：[native Debug](raw/out/m3-t3/r2-step3-d3d12-debug-001/ctest-boundary-004.log)、[native Release](raw/out/m3-t3/r2-step3-d3d12-release-001/ctest-boundary-004.log)，各42/42。

增量：[CPU Debug](raw/out/m3-t3/r2-step3-cpu-debug-incremental-001/result.json)、[CPU Release](raw/out/m3-t3/r2-step3-cpu-release-incremental-001/result.json) 各3 probes；[native Debug](raw/out/m3-t3/r2-step3-d3d12-debug-incremental-002/result.json)、[native Release](raw/out/m3-t3/r2-step3-d3d12-release-incremental-002/result.json) 各5 probes。仅声明所探头文件的内容/mtime恢复与真实编译边界，不冒称旧可变exe始终匹配最终字节。

原失败入口：[CPU沙箱C1902](raw/out/m3-t3/r2-step3-cpu-debug-001/configure-001.log)、[GL-only沙箱C1902](raw/out/m3-t3/r2-step3-gl-release-001/configure-001.log)、[启动参数/WX](raw/out/m3-t3/r2-step3-startup-tests-001/build.log)、[game001 runner错误](raw/out/m3-t3/r2-step3-game-debug-001/verification.json)、[game002摘要探针错误](raw/out/m3-t3/r2-step3-game-debug-002/verification.json)、[未稳定增量基线](raw/out/m3-t3/r2-step3-d3d12-debug-incremental-001/result.json)、[沙箱资产预检/打包拒绝](raw/out/m3-t3/r2-step3-d3d12-debug-001/ctest-boundary-003.log)。修正/正常环境复测使用另轮日志，不覆盖原记录。

本节点未制作生产包、完成人工14项/15分钟、动态触发实际设备丢失/提交失败/在途timeout或批准R3；默认仍GL。人工核查只在 [唯一主表](milestones/m3-t3/manual-verification.md) 维护，顺序4交付候选后才新增编号及实际命令。
