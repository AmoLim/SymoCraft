---
tags:
  - area/spec
---

## ADDED Requirements

### Requirement: Harvest properties in checked block definitions

World SHALL 为可采集方块提供受校验的基础采集秒数、一个资源 ID 和固定产出数量。基础采集秒数 MUST 有限且为正；未配置采集规则的方块 SHALL 明确不可采集。资源 ID 可独立于 BlockID。无采集规则的现有方块定义 SHALL 仍可加载。

#### Scenario: NPC-MVP-A13 Checked harvest definitions
- **WHEN** 装载合法采集规则、未配置采集规则的现有定义或参数非法的规则
- **THEN** 合法规则返回相应基础耗时和产出，未配置规则的方块不可采集，非法规则拒绝发布候选定义

### Requirement: Conditional removal of a harvest target

World SHALL 支持以原目标仍有效为前提的受控移除。原位移除或替换 MUST 使旧目标失效，即使该位置随后出现相同 BlockID。其他坐标的编辑和本格无变化编辑 SHALL 保持原目标有效。条件校验与对应移除在同一次受控提交内完成，成功移除沿用现行编辑状态及网格失效规则。

#### Scenario: NPC-MVP-A14 Same-type replacement
- **WHEN** 取得目标后将该方块移除，再在同一坐标放回同种方块，并以旧目标请求采集移除
- **THEN** 旧请求失败，保留新方块，且该请求本身不产生新的内容或网格版本变化

#### Scenario: NPC-MVP-A31 Unrelated and unchanged edits preserve target identity
- **WHEN** 持有有效目标时改变同一区块的其他格，再对目标格执行语义无变化的编辑
- **THEN** 两次编辑后原目标仍有效，之后可用该目标发起一次正常的条件移除
