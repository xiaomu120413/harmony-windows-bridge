# Agent Notes

## 强制工作顺序：文档先行

- 任何代码修改前，必须先识别并更新所有受影响的设计/架构文档；文档缺少目标、边界、接口、状态机、文件清单或验收口径时，先补文档，再修改代码。禁止先写代码、最后补文档。
- “代码修改”包括 ArkTS、C/C++、N-API、manifest/config、构建/打包脚本、资源、测试、FreeRDP/xrdp 子模块及父仓 submodule SHA，不限于业务源文件。
- 每个代码任务或提交都必须能映射到文档中的设计章节、文件级修改项和验收项。若设计已经覆盖，也必须在对应设计文档的实施台账中先登记 Change ID、代码范围、验收 ID，并把设计状态标为 `DesignReady`，之后才能编辑代码。
- 实施过程中一旦发现代码所需行为与设计不一致，立即暂停该部分代码修改，先更新设计文档和验收条件，审阅文档 diff 后再继续；不能用实现现状反向默默改变架构。
- 小改动允许文档和代码进入同一提交，但编辑与审阅顺序仍必须是“文档 -> 代码 -> 测试 -> 文档状态/证据”。架构、公共 ABI、功能隔离、包配置或社区子模块变更，优先先提交独立设计基线。
- 目标设计文档可以在实现前记录 `Planned/DesignReady`，但不得把未完成能力写成已实现。`docs/freerdp-ohos-feature-matrix.md`、当前交互文档和验证基线属于当前事实来源，只有代码和验收真实通过后才能更新为完成。
- 代码完成后必须回写同一实施台账：记录实际文件、偏差、测试命令、结果和证据位置，并将状态依次更新为 `Implemented`、`Verified`；没有文档回写和验收证据，不视为完成。

文档路由规则：

- 平板/2in1 布局、字体、Icon、能力隔离、XComponent、旋转、输入和 IME：先更新 `docs/harmonyos-tablet-adaptation-architecture-and-acceptance.md`。
- Native 模块所有权、N-API 边界或文件拆分规则变化：同步 `docs/ohos-native-cpp-module-guidelines.md`；具体功能设计仍写入对应设计文档。
- FreeRDP 客户端公共 ABI、channel、codec、resize、DPI 或 fallback：先在对应设计文档写目标接口和兼容策略；验证后同步 `docs/freerdp-ohos-feature-matrix.md`，公共集成方式变化时再同步 `docs/freerdp-ohos-sdk-quickstart.md`。
- xrdp 服务端行为：先更新对应 xrdp 设计/整改文档，并以 xrdp 官方 server/module ABI 为依据；验证后再更新当前能力和验证文档。
- 构建、HAP/HNP、运行库同步、签名、安装和真机步骤：同步 `docs/freerdp-ohos-validation-baseline.md` 及受影响的 README。
- 不确定更新哪份文档时，先查 `docs/README.md` 的事实来源；仍缺文档就先创建/补齐设计文档，不得直接开始代码修改。

## Native 和协议边界

- 后续新增 HarmonyOS native C++ 逻辑必须放进对应模块，不能继续堆到主入口或 N-API 导出文件里。
- `module_init.cpp` / `napi_exports.cpp` 只负责注册、参数转换和转发；FreeRDP、输入、渲染、剪贴板、rdpgfx、音频等逻辑必须进入各自模块。
- 参考 `docs/ohos-native-cpp-module-guidelines.md` 维护模块边界和文件行数限制。
- 分项目参考已有逻辑：xrdp/HarmonyOS 远控线必须优先对照 xrdp 自身的 server、module ABI、`xup`、`vnc`、`neutrinordp`、`libxrdp` 回调和 xrdp 官方配置；FreeRDP/HarmonyOS 客户端线才参考 FreeRDP 客户端平台实现。不要把 FreeRDP 客户端渲染、输入或会话逻辑当成 xrdp 服务端实现模板。
- xrdp 线不要自编协议逻辑、线程时序、格式转换或状态兜底；缺少 xrdp 侧依据时先补调研和日志，再做 OHOS 适配层改动。
