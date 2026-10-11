---
tags:
  - area/spec
---

# Telemetry Lifecycle Delta

## Purpose

D06最小分离与D08封闭/导出目标，当前Session尚无Seal。不得覆盖[foundation-files现役允许失败](../../../../specs/foundation-files/spec.md)或恢复[暂停Fix1](../../../m3-t2-publication-fix1/proposal.md)。

## ADDED Requirements

### Requirement: Controlled sealed measurement facts

Session SHALL 自有记录且不保留运行对象指针，串行非重入Recording→Sealed；采样完成、运行/关闭、观测有效性、导出结果四类事实分开。停止场景/模拟/新采样、交接已就绪GPU并完成Renderer/Window/Runtime关闭后 SHALL 汇入全部最终事实再Seal；启动部分失败记录仍存活至最终处理。封闭后 Add/GPU回填/metadata/startup/completed/invalid原因全部 SHALL 具名拒绝；相同最终事实重复Seal幂等、不同事实拒绝，不改原帧顺序。Session SHALL 不发GPU query/等待fence/采系统内存/控制玩法。

#### Scenario: M3-T4-A18 closed facts and late samples
- **WHEN** 无效运行、迟到/缺失GPU样本、重复或冲突Seal及封闭后任何记录写入
- **THEN** 无效仍可导出、缺失有原因不补0、frame ID关联不破坏；不为补指标额外等GPU，安全释放等待保留；迟到写/冲突事实拒绝、相同Seal不变

### Requirement: Explicit export retry and saved idempotence

Export SHALL 仅保存Sealed记录并独立表达NotAttempted/Failed/Saved及阶段/路径诊断，不改记录/观测有效性。失败 SHALL 保持Sealed/内存/部分文件，允许同进程同会话专属目录显式重试、不自动无限重试，不承诺文件组回滚/崩溃恢复；Saved重复 SHALL 返回原成功结果、不再写文件。全部导出及最终状态发布成功才 SHALL 宣布finished。记录寿命覆盖最后显式尝试，析构不Export/宣布完成/清部分结果，新测量创建新Session/目录。

#### Scenario: M3-T4-A19 export failure and repeat
- **WHEN** CSV/summary/状态发布失败、显式重试成功或Saved后重复Export
- **THEN** 失败保留具体阶段/路径/首诊断及原封闭结论，文件存在不当完成；成功后无重写，不重开旧记录或以导出修复1175

#### Scenario: M3-T4-A19 A21 final status and exit category
- **WHEN** 运行/关闭/有效性与保存结果汇合
- **THEN** 有效运行且保存成功为0，正常结束但观测无效为4，运行/关闭/导出错误为3，次要错误不盖首因；原解析64/资产2分类和文件列/统计/终态协议不漂移

### Requirement: Scenario and observation are independent

app私有BenchmarkScenario SHALL 自有既有路线游标并由实际经过时间及必要状态值产生意图/模拟前编辑，不借Session存在启停，不拥有World/Registry或采样统计。D06 SHALL 只交付封闭/导出及场景最小分离，保留当前启动命令/能力，不新增Off/Basic/Deep/热注入/裁剪包或无窗口性能文件协议。

#### Scenario: M3-T4-A17 A20 existing workload and scope
- **WHEN** 普通运行记录或static/walk/edit场景无完整记录，并接收基准窗口事实
- **THEN** 场景动作/顺序/预条件/实际时间与旧协议不变；玩家隔离/allow-unfocused/Escape局部锁存/最小化无效由app策略成立，观测不成为玩法前提
