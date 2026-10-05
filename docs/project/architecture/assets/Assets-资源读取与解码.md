---
type: 功能
status: 已验证（T0范围）
project: Symocraft
module: assets
created: 2026-10-05
tags:
  - area/architecture
---

# Assets-资源读取与解码

关联类：[类设计](Assets-资源读取与解码-类设计.md)。统一验收：[M3-T0](../../../spec/M3-T0-模块软硬边界.md)。

最终验证：[M3-T0 交付与验收报告](../../../milestones/m3-t0/README.md)。

## 当前设计

assets 保留以可执行文件为根的路径策略，并接管原来分散在 application / texture 内的 CPU 图片解码。文件字节与 原通道像素归调用方所有，GPU 创建与释放仍由 renderer 负责。

实现位置：[模块源码](../../../../game/modules/assets/CMakeLists.txt)。当前为迁移实现，验证状态见本次变更；不将已有 M2 结果冒充新实现证据。

## 本次变更

### 目标与流程

T0 仅建立真实模块、公开数据契约及测试边界，维持原正常运行路径，不提前做领域算法重写。

```text
相对资源名 → Resolve → 可执行文件 assets 内的路径
完整路径 → ReadBytes → 文件字节 → DecodeImage → 自有 Image
路径越界 / 文件缺失 / 解码失败 → 具名异常 → 调用方停止初始化
```

### 关键约束与取舍

- Resolve 继续拒绝根路径、父目录越界、备用数据流及越界符号链接，不靠改变 CWD 修复资源问题。
- DecodeImage 保留源图片通道数（1 至 4），renderer 按原策略只接受 RGB / RGBA，行紧密排列；每次显式设置当前线程的翻转选项，避免调用间污染。
- 解码临时缓冲通过 RAII 释放；向 Image 的一次复制只发生于资源加载，不发生于逐帧渲染。
- 图片解码库与 Windows 路径实现均私有；模块不解释方块配置语义。

### 验收案例

| 状态 | 场景 | 预期行为 |
| --- | --- | --- |
| [x] | 独立公开头消费者 | 不需要聚合 core.h、SDK 或私有 include |
| [x] | assets.image + assets.contracts + assets.from_source_directory | 正常 / 失败路径通过正式库验证 |
| [x] | Debug / Release 与 CPU-only | 相同源码所有者；CPU 库无需窗口 |
| [x] | 安装包无关 CWD、资源错误 | 资源定位不依赖当前目录；真实损坏纹理探针以退出码 3 报错 |

2026-10-05 最终 Debug、Release、CPU-only 与安装包测试通过，源通道数/翻转契约和损坏图片诊断有独立测试及真实运行证据。最终日志、包身份和故障探针见阶段报告；不推定覆盖任意图片格式、超大文件或分配故障。

## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 边界成为实际瓶颈 | 先测量分配、复制或调用开销，再调整接口；不恢复私有跨模块访问 |
| 后续阶段修改内部算法 | 同步更新本笔记、类不变量与回归证据 |

