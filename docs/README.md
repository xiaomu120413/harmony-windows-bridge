# 文档索引

文档核对：2026-09-05。当前范围为 HarmonyOS 远程桌面客户端和 2in1 被控端。构建成功、功能接入、真机验收和商店分发分别记录，不能相互替代。

## 最新复核

[2026-09-05 文档、外链与历史证据复核](audit/2026-09-05-document-verification.md)：20个原始外链、40处图片引用、源码断言与完整App构建结果；明确缺失历史截图和端到端待验项。

## 开始使用与开发

- [项目 README](../README.md)：产品能力、目录、构建与安装入口。
- [HarmonyOS 工程说明](../harmony/README.md)：HSP/Entry 边界、权限和运行库同步。
- [构建与验收基线](freerdp-ohos-validation-baseline.md)：当前路径、签名配置、包门禁与真机清单。
- [仓库维护说明](repository-notes.md)：生成目录、工具链和连接入口。
- [Windows RDP 环境排查](windows-rdp-environment-setup.md)：客户端连接 Windows 的准备步骤。
- [xrdp HDC 连接说明](xrdp-windows-hdc-forwarding.md)：Windows MSTSC 连接 2in1 被控端。

## 当前范围、架构与能力

- [项目范围与会话工具栏](project-scope-and-session-controls.md)：已完成的范围清理；缩放工具栏仍为 Planned，尚未实现。
- [FreeRDP 功能矩阵](freerdp-ohos-feature-matrix.md)：已接入能力、默认策略、fallback 与待验收项目。
- [多设备与 HNP 打包方案](harmonyos-multidevice-hnp-packaging-plan.md)：common HSP、设备 Entry 和独立 xrdp 进程；旧 tablet 升级和市场分发仍有阻断项。
- [平板/2in1 适配设计与验收](harmonyos-tablet-adaptation-architecture-and-acceptance.md)：布局、输入、IME、XComponent 的阶段设计与证据；早期单 HAP 约束由多设备方案替代。
- [手写笔与多显示器设计](freerdp-ohos-pen-and-multimon-design.md)：接口、生命周期和动作级验收要求。
- [Native 模块边界](ohos-native-cpp-module-guidelines.md)：FreeRDP、xrdp、N-API 和渲染职责。
- [FreeRDP SDK 接入](freerdp-ohos-sdk-quickstart.md)：第三方 Native 调用方的接入流程；不代表应用已提供所有参数入口。
- [设置页交互规格](settings-desktop-current-interactions.md)：HarmonyOS 桌面布局及阶段更新。
- [设置页实施清单](settings-desktop-implementation-checklist.md)：对应设计的实施检查，不是新的完成状态声明。
- [第三方组件材料](release-third-party-notices.md)：组件来源及发布材料清单。

## 参考与历史

以下资料保留用于定位旧问题，不应覆盖上面的现行路径和能力状态。

- [修改记录](CHANGELOG.md)：按日期保留每次变更，后续删除不抹去历史。
- [xrdp 早期穿刺方案](xrdp-ohos-mstsc-penetration-plan.md)：dummy/内嵌阶段设计和后续阶段记录。
- [xrdp 早期整改方案](xrdp-ohos-commercial-remediation-plan.md)：旧职责划分与整改背景，文件大小和布局需以现代码核对。
- [MSTSC sizing 笔记](xrdp-mstsc-smart-sizing.md)：Windows 客户端缩放背景，不是鸿蒙客户端工具栏实现。
- [硬件编解码概念](hardware_codec_notes.md)：概念参考，不是当前能力矩阵。
- [AVC444 复盘](archive/avc444-gpu-compositor-retrospective.md)、[白帧复盘](archive/rdp-white-frame-case-study.md)、[旧真机记录](archive/ohos-device-validation-2026-05-15.md)：历史问题和当时证据。

`docs/` 下的 SVG 为阶段设计参考；运行时行为以现行源码及对应验收记录为准。
其他仓库文档：客户端 [Native 目录说明](../harmony/app/common/src/main/cpp/README.md)、[商店文案草稿](../harmony/app/store-assets/muhub/store-listing.md)、[旧设置页视觉验收](../design-qa.md)、[旧签名目录说明](../tools/hapsigner/README.md)、[2026-08-04 平板验收](../artifacts/tablet-acceptance/2026-08-04/result.md)。历史截图路径和草稿文案不作为当前交付验证。
