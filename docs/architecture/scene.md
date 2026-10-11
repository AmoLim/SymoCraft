---
tags:
  - area/architecture
---

# Scene

## 边界与权威

`symocraft_scene` 是真实共享 CPU 数据的 `INTERFACE` target，不是 SceneManager、实体容器或空实现库。只依赖 foundation；公开头不携带 GPU/SDL/Windows SDK、私有 Chunk/ECS 类型、命令或全局容量。
公开 World 借用/发布行为以 [world spec](../openspec/specs/world/spec.md) 为准，现代消费/资源接受以 [renderer spec](../openspec/specs/renderer/spec.md) 为准，不在此另建同一行为清单。
落点：[CMake](../../game/modules/scene/CMakeLists.txt)、[mesh.h](../../game/modules/scene/include/symocraft/scene/mesh.h)、[camera.h](../../game/modules/scene/include/symocraft/scene/camera.h)。

## 值与布局

- `BlockVertex3D` 保存整数世界格顶点 `pos_coord`、UV 加纹理层的 `tex_coord` 与既有 float `normal`。当前 World mesher 仍写 `normal=0`；它不是可推定的新光照模型、材质对象或 GPU 句柄。
- 编译时检查 BlockVertex 标准布局且为 28 字节、LineVertex 的 float 位置为 12 字节。改变 GLM 配置/对齐会影响这些检查及实际消费者，不能局部换配置掩盖 ABI 差异。整数别名/GLM 入口统一见 [foundation](foundation.md)。
- `CameraView` 拥有 projection/view 两份矩阵，默认单位矩阵；不是相机 owner 或对实体姿态的借用。legacy GL 由 app 生成该值，现代 renderer 接收自己公开的中立 `CameraParameters`，不把未来 T4 镜头状态或第二相机实体迁入 scene。
- `MeshData` 是拥有连续顶点的 vector，默认空；复制深复制，移动/交换转移标准容器所有权。没有隐含 GPU/count 副本、全世界顶点上限或 provider 校验器，顶点数只有 `vertices.size()` 一个来源。
- `MeshView` 是只读非拥有 span。空 span 本身不能区分“首次未发布”和“已发布空结果”；该区别属于 World 的发布记录/版本，不添加 scene 全局状态。

布局断言不校验数值合法性、不证明各种编译器/平台字节稳定，也不将 28 字节 CPU 顶点声明为通用 GPU ABI；私有后端须显式打包和校验。

## 所有权与交接

World 的 Chunk 拥有最终 MeshData，私有 Mesher 返回独立候选，Face scratch 不被输出引用。发布交换 vector 后旧网格归局部候选清理；vector swap 本身不令元素引用悬空，但**不延长** World 规定的回调借用。
顶点借用受实际 provider 契约限制；扩容、替换、释放、赋值及 owner 销毁可能使地址失效，复制 span 不取得所有权。World 的 MeshView 最长当前 `VisitMeshes` callback，调用方不得在借用中销毁 World；长期缓存须在 callback 内取得自有数据。

当前 app 的两条消费路径不同：legacy `AppendMesh` 在 callback 内复制进帧批次；现代 [render_conversion](../../game/app/src/render_conversion.cpp) 复制顶点并做一次 V→1-V 转换，缓存以 World 身份/已发布版本维护。此转换不修改 World 源顶点、顺序、U/层/normal，不把 backend 坐标或纹理方向修正移入 scene。缓存失败/在途 GPU 寿命仍归 [renderer](renderer.md) 及 app，不由拥有 CPU vector 自动保证。

scene 无锁、任务或运行时同步；普通值/标准容器不保证同一数据并发读写安全。跨线程需另定义快照所有权及 provider 寿命，不能把 const span 当同步原语。

## 稳定不变量与验证边界

| 原身份 | 继续成立的边界 |
| --- | --- |
| Scene I1 | 当前 CPU 顶点布局/尺寸断言成立；不冒充 GPU ABI 或运行时有效性检查。 |
| Scene I2 | 消费者不得延长 provider 借用或提前终结其 owner。 |
| Scene I3 | 共享头无图形 SDK 或 World/ECS 私有实现依赖。 |
| Scene I4 | MeshData 自有；完整候选/最终结果不借用 Mesher scratch。 |

[world_contract_tests](../../test/unit/world/world_contract_tests.cpp) 覆盖完整发布、自有输出及 guard；[render_conversion_tests](../../test/unit/app/render_conversion_tests.cpp) 覆盖真实 atlas、V/层和面顺序交接。测试源码不是新运行成绩，不通过解引用超期 span 验证寿命。
历史 T0/T1 与现代候选的不同适用范围从 [VERIFICATION](../verification/VERIFICATION.md) 核对；本轮没有重新编译、C++ 测试或 GPU 验证。
