# repository-licensing Specification

## Purpose

#area/spec

定义以 `a6620be9e3e6fbc4a06244686d971b6e6ce754a9` 为分界的项目代码许可及分发随附材料目标。历史 MIT 授权保留，后续有权贡献者的新增原创代码采用 Apache-2.0；本增量不表示已实施换证或完成权属审查。

## Requirements

### Requirement: Preserved historical MIT code and provenance

项目 SHALL 保留基线及其历史版本中原受 MIT 授权的项目代码的 MIT 许可，并提供基线根许可的完整原字节副本 `LICENSE-MIT`，保持原版权、许可和免责声明。移动、复制、拆分、重命名、格式化及保留旧表达的重构 MUST 不被当作新增原创代码重新许可。项目 MUST 不撤销旧副本的 MIT 授权，不代表历史作者新增 Apache 专利许可；第三方及非代码资产按各自来源处理。

#### Scenario: LIC-A01 historical release or relocated source
- **WHEN** 用户取得基线旧版本，或后续源码包含从基线移动/复制/重构保留的 MIT 代码
- **THEN** 相关历史部分继续适用 MIT、保留适用声明；新根 Apache 许可不能被解释为已经覆盖或替换该历史授权

### Requirement: Apache licensing of authorized new contributions

项目 SHALL 在 README 许可说明及贡献规则中明确：分界后由有权贡献者提供并按新规则接受的新增原创项目代码及原创修改采用 Apache-2.0，全新自有源文件带有对应许可标记，版权仍归实际权利人。当前未提交改动、显式其他许可的输入及来源不明材料 MUST 先核对来源/权限，不能仅因后续提交或新增路径自动视为 Apache；贡献政策不构成版权转让或历史贡献者补签义务。

#### Scenario: LIC-A02 wholly new authorized source
- **WHEN** 贡献者有权提供分界后新写的原创自有源文件，并按新规则提交及被接受
- **THEN** 文件明确标记 Apache-2.0，接收者能取得完整 Apache 许可，保留实际版权归属，不声称该贡献也可任选 MIT

#### Scenario: LIC-A03 pre-existing dirty changes or uncertain input
- **WHEN** 处理分界时已有的未提交改动、复制的外部代码或缺少授权依据的素材
- **THEN** 核对并记录适用范围，未判明项明确报告并保留原声明；不能把本政策当作已取得授权或统一重许可的证据

### Requirement: Explicit mixed-file scope and modification notices

同时含有历史 MIT 部分及后续 Apache 原创贡献的文件 SHALL 保留适用的原版权/归属声明，使用 `MIT AND Apache-2.0` 并用可读说明界定两部分；对 Apache 文件的后续修改 SHALL 明示修改事实。独立源码包 MUST 能识别许可范围，不能只依赖联网或本地 Git 历史，也不能用 `MIT OR Apache-2.0` 或单独 Apache 标记让人误以为整个混合文件可任选或已统一换证。

#### Scenario: LIC-A04 original feature added to a legacy file
- **WHEN** 在历史 MIT 文件中新增原创函数或修改原函数，并将该文件作为源码分发
- **THEN** 旧代码的 MIT 仍适用，新原创贡献的 Apache 范围和修改事实在文件中可读，包内有两份完整许可，接收者无需 Git 即可理解范围

### Requirement: Informational NOTICE and complete distribution materials

项目 SHALL 提供与基线和贡献政策一致的根 `NOTICE`，注明原项目版权、历史 MIT 范围、新增 Apache 范围及第三方/非代码资产例外；NOTICE MUST 仅作说明，不添加许可限制、不代替完整许可正文或声称已核清素材权利。后续源码包及游戏安装包 SHALL 随附可离线阅读的 `LICENSE`、`LICENSE-MIT`、`NOTICE`，保留实际配置所需的全部既有第三方许可和通知，不以项目 NOTICE 替代它们。

#### Scenario: LIC-A05 independent source and game packages
- **WHEN** 生成后续源码包或通过游戏安装规则生成包含项目代码的发行包
- **THEN** 接收者可离线读取完整 Apache 正文、原字节 MIT 副本及 NOTICE；三者适用范围一致，游戏包原第三方许可仍齐全，不必取得源码仓库或联网查找全文

#### Scenario: LIC-A06 third-party and asset exceptions
- **WHEN** 阅读项目 NOTICE，或分发含 vendor、纹理、参考图片及其他外部输入的材料
- **THEN** 对应材料继续遵守各自适用许可；项目声明不扩大未获权利，不把已知素材来源或依赖版本缺口写成已解决，不覆盖原第三方归属
