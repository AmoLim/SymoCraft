---
tags:
  - area/spec
---

## ADDED Requirements

### Requirement: NPC computation and presentation dependency isolation

NPC 动作、AI 与 World 采集交接 SHALL 使用正式 CPU 模块，能够在无窗口、原生图形后端和外部编辑工具的配置下运行。BehaviorTree.CPP 和 cgltf 的类型 SHALL 留在各自实现边界，公开交接只使用中立数据。角色资产解析 SHALL 与 GPU 资源创建分开，renderer 不读取 World 或 ECS 私有状态。

分件几何、关节和只读动作/结果交接 SHALL 继续使用自有中立值。程序姿态、演示控制和可见反馈由应用/显示组合负责，CPU 模拟不依赖其文字、窗口或图形实现；显示不可写入 World、ECS 或资源计数。

#### Scenario: NPC-MVP-A20 CPU-only and public consumers
- **WHEN** 配置 CPU-only、关闭测试或编译 NPC/角色导入的公开消费者
- **THEN** 适用的生产 CPU 能力及分件导入可用，公开接口不传播第三方解析器、树运行库、文字显示实现或图形 SDK 类型；生产目标不依赖测试夹具或策划工具
