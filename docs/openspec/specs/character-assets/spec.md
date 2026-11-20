---
tags:
  - area/spec
---

# character-assets Specification

## Purpose

定义从 Blender 交付的 micro-voxel 角色 GLB 到中立 CPU 角色网格及刚性分件数据的契约。绘制、程序动作与实例寿命由 [renderer 增量](../renderer/spec.md)负责。

## Requirements

### Requirement: Validated static character asset import

项目 SHALL 交付可辨认的人类及僵尸验证模型的可编辑 Blender 源和实际导出的 GLB。导入 SHALL 使用 cgltf 读取、校验并转换已支持的静态数据，输出自有浮点角色网格，保留顶点色和硬边法线。可用网格 SHALL 无需纹理贴图、蒙皮或动画播放。导入失败 SHALL 具名报告资产及原因，不发布半成品。

交付的演示模型 SHALL 进一步提供躯干、头、左右臂和左右腿的独立刚性部件、局部绑定变换及明确的肩/髋旋转轴，供简单动作驱动。导入 SHALL 保留自有部件与变换数据，不将可动部件合并为无法独立驱动的整体；仍兼容此前支持的整体静态输入。部件身份、层级、变换及关节数据 SHALL 在发布前完成校验，拒绝缺失、重复、成环、非有限或超出已声明输入上限的数据。该扩展不要求骨骼、蒙皮或动画片段。

#### Scenario: NPC-MVP-A16 Production import of Blender fixtures
- **WHEN** 使用正式导入入口在无窗口环境读取交付的两份 GLB，并释放原文件缓冲或解析器对象
- **THEN** 得到仍然有效的自有角色网格，几何和顶点色可区分人类与僵尸，硬边法线可用于后续绘制

#### Scenario: NPC-MVP-A17 Missing, malformed or unsupported assets
- **WHEN** 请求缺失或损坏的 GLB，或资产包含无法按已支持契约解释的数据
- **THEN** 报告具体资产及失败原因，不返回可供提交的半成品模型

#### Scenario: NPC-MVP-A32 Owned multipart character data
- **WHEN** 正式 CPU 入口导入交付的分件人类和僵尸 GLB 后释放解析器及原文件缓冲，并另行读取此前支持的整体静态夹具
- **THEN** 分件模型的各身体部件、绑定变换及关节数据仍有效，可独立改变一个肢体而不改变其余网格；静态夹具仍可导入

#### Scenario: NPC-MVP-A33 Invalid multipart metadata
- **WHEN** 分件资产缺少必需部件、部件身份重复、父子成环，或提供非法变换/关节值及超限数据
- **THEN** 具名拒绝资产并报告原因，不返回缺少部件或关节的可用半成品
