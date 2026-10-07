---
type: 类设计
status: 自动验证完成（T1范围）
project: Symocraft
module: world
class_name: "SymoCraft::World::Detail::ChunkMesher"
inheritance: []
created: 2026-10-06
updated: 2026-10-06
tags:
  - area/architecture
---

# ChunkMesher 串行工作区与候选网格

关联：[World功能/统一验收](World-功能.md)、[发布owner](World-类设计.md)、[输入Chunk](Chunk-类设计.md)、[CPU输出](../scene/Scene-共享场景数据-数据设计.md)。源码：[私有头](../../../../game/modules/world/src/chunk_mesher.h)、[实现](../../../../game/modules/world/src/chunk_mesher.cpp)。

## 当前设计

每份World私有拥有一个具体Mesher。它只读当前Chunk、四个本次邻块与定义，返回完整自有MeshData；不编辑/发布World，不缓存长期邻居，不调度线程或GPU。

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `vector<Face>` | faces_ | 空；容量按需要增长 | 跨调用独占可复用面工作列，退出Build清size不清capacity |
| `Face`（私有值） | position/texture/direction | BlockCoord/uint16/uint8，direction0..5 | 一面世界坐标/层/方向；不存Block/Chunk指针 |
| `bool` | building_ | false | 同步重入检查，异常guard恢复，不是锁 |
| `MesherStatistics` | statistics | size_t字段均0 | 内部诊断，见下表，不属于World公开协议 |
| 私有模块测试入口函数指针 | probe、candidate_probe、probe_context | nullptr | 生产默认无调用；只经窄测试访问注入阶段/候选错误，不向app公开 |

| MesherStatistics字段 | 真实计量口径 |
| --- | --- |
| builds | Build完整返回的次数；不等于World成功发布数 |
| scratch_growths | 退出一次Build时capacity较入口增长的调用次数，含异常；不是vector分配总次数 |
| scratch_capacity_bytes | faces_.capacity()*sizeof(Face)，含备用容量 |
| candidate_bytes | 最近已成功reserve候选顶点的capacity字节；不是整世界峰值或全部分配流量 |
| peak_mesh_bytes | 本实例记录的scratch + 本次候选 + 此Chunk旧mesh容量峰值；不含其他Chunk/容器/驱动内存 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| ME-I1 | faces_独占；退出Build后size=0、building_=false，成功输出不引用scratch | 正常/异常退出 |
| ME-I2 | 输入邻居只借本次Build，结果自有vertices；Mesher不改Chunk最终mesh/版本 | Build期间及退出 |
| ME-I3 | y,x,z与六方向固定；每面六顶点，沿用28字节布局/纹理/绕序/normal=0 | 完整候选返回 |

### 公开接口预期行为

这里的public仅指私有模块类成员，不是world模块公开头API。

| 签名 / 入口 | 调用方与可见范围 | 预期行为：输出及状态变化 | 前提 / 边界 | 失败反馈及失败后状态 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| 默认构造/隐式析构 | World::Impl | 空工作区；析构容器释放 | 不存在活动Build | 不抛析构，不删除借用邻居 | chunk_mesher.h |
| copy/move ctor/assign = delete | 私有World/测试 | 工作区身份不复制迁移 | World串行独占 | 编译期拒绝 | chunk_mesher.h |
| `MeshData Build(const Chunk&, const std::array<const Chunk*,4>&, const BlockDefinition&)` | RebuildDirtyMeshes | 扫描面工作列→宽类型检查→一次reserve→展开完整候选 | 本World邻序+x,-x,+z,-z；定义完整，输入同步只读 | 重入logic_error；构建/分配异常传播，guard清scratch逻辑内容；最终旧mesh由World保留 | chunk_mesher.cpp / ME-I1-I3 |

### 私有函数预期行为

| 签名 / 入口 | 内部调用方 | 预期行为：处理规则及副作用 | 前提 / 边界 | 失败传播及清理责任 | 源码 / 约束 |
| --- | --- | --- | --- | --- | --- |
| read(x,y,z) lambda | Build六邻格 | y域外/缺邻返回NULL；本地越一轴选择当前四邻 | 只查询相邻一格，不作任意跨块寻路 | 不写输入，不保存借用 | chunk_mesher.cpp / ME-I2 |
| Restore::~Restore()局部guard | Build全部退出路径 | 更新容量/growth统计、clear faces_、恢复building_ | 容器仍活着 | 不抛；不清已发布网格 | chunk_mesher.cpp / ME-I1 |
| probe/candidate_probe回调 | 私有测试场景 | BeforeScratch/BeforeOutput/BeforeValidate或候选篡改 | 默认null；测试负责人结束借用 | 异常走同一恢复路径；不构成生产服务 | chunk_mesher.h |

### 生命周期、容量与代价

Build(A) → 清逻辑size但保留工作容量 → Build(B) → Build(A)。容量不足允许vector增长，不预分配全世界面表；世界销毁释放容量。保留的Face列是一个Chunk的实际可见面，候选独立一次reserve，发布时World交换vertices，旧网格随候选局部析构释放。

双遍历Face带来额外写读与scratch内存；复用只减少可能重复增长，不复用候选顶点存储，不保证零分配或变快。六邻格规则仍为“邻格非NULL且透明才出面”，不是相机可见性/遮挡剔除。固定面模板为constexpr，四角点为每面局部值。

## 本次变更

D08采用具体私有Mesher，移除共享g_normal/block_faces；失败恢复与按块发布分别由Mesher和World负责。ME-I1-I3对应T1-A05/A06/A08/A09/A15/A16，自动结果、分配故障配置及实际成本见 [功能验收](World-功能.md#验收案例)。

## 后续考虑

需并发时先定义每任务独占scratch与输入寿命；实测内存/分配瓶颈后再比较工作列/候选复用策略。
