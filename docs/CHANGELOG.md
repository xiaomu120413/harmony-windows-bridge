# 仓库修改记录

这里是仓库级统一修改台账，用来快速回答“哪一笔改了什么、影响哪里、到哪里看证据”。

Git 历史仍是提交内容和文件差异的最终事实来源；设计目标、状态机和详细验收继续由对应专项文档维护。本台账只保留便于回看的一层摘要，不复制专项文档全文。

## 维护规则

- 一笔 Git 提交对应一条记录，文档提交也必须登记；同一提交不要拆成多条，也不要把多笔提交合并成一条。
- 新记录加在对应日期表格最上方，Change ID 使用 `CHG-YYYYMMDD-NNN`，同一天序号只增不减。
- “提交”优先填写短哈希。记录与实现同提交、尚无哈希时，先填写唯一的提交主题；提交完成后可在后续整理时补短哈希。
- “改了什么”写行为变化和主要范围，不罗列所有文件；精确文件差异使用 `git show <提交>` 查看。
- “验证/关联”写实际执行的测试或证据入口。未执行的验证必须明确写“未执行”，不能写成已通过。
- 未提交的试验性修改不进入历史表；使用 `git diff` 查看。准备形成提交的完整修改，应在提交前登记。
- 设计先行、实施状态和证据回写仍按 [平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md) 等专项文档执行。
- 不记录密码、令牌、证书私钥、用户输入正文或其他敏感数据。

初始台账回填范围是 2026-08-04 当前分支相对上游的 32 笔提交（`6000284` 至 `5937f8e`）。更早的历史不反向补写摘要，仍通过 `git log` / `git show` 查询；从本台账建立后产生的每笔提交都必须持续登记。

## 新记录模板

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| `CHG-YYYYMMDD-NNN` | `type(scope): subject` | 功能/模块 | 用一两句话说明行为变化、兼容性和主要影响范围 | 测试命令、结果或专项文档/证据链接；未执行要明示 |

## 2026-08-04

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| `CHG-20260804-033` | `docs: add repository change ledger` | 文档/流程 | 新增本统一修改台账，在文档索引增加入口，并要求后续每笔提交同步登记。 | 文档链接、提交映射、UTF-8 和 Markdown 表格检查通过；本条随该主题提交。 |
| `CHG-20260804-032` | `5937f8e` | 文档/旋转与 IME 架构 | 将显示方向和 IME 的核心所有权移至 API 22 Native：ArkTS 只传宿主窗口身份并展示键盘控制，Native 负责方向监听、IME client 和远端输入提交。 | [平板专项台账 TAB-B-02/TAB-F-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 5937f8e`。 |
| `CHG-20260804-031` | `86996fa` | 文档/IME 架构 | 明确键盘环境由 `EntryAbility` 唯一启停，键盘高度只经 AppStorage 镜像给会话页，不进入 Native、Surface 尺寸或远端 resize。 | [平板专项台账 TAB-F-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 86996fa`。 |
| `CHG-20260804-030` | `ec9440e` | 修复/RDP 旋转 | 将 ArkTS 侧真实显示方向传入 Native 会话，由会话统一持有方向，并在远端 resize 时发送正确的 RDP orientation；补充方向策略测试。 | [平板专项台账 TAB-B-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show ec9440e`。 |
| `CHG-20260804-029` | `f41cdb6` | 文档/RDP 旋转 | 明确显示方向由会话统一持有，补齐 ArkTS、N-API、Native 的接口边界和验收口径。 | [平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show f41cdb6`。 |
| `CHG-20260804-028` | `5759423` | 文档/旋转与 IME | 定义旋转、远端 resize 和虚拟键盘/中文输入的后续适配方案，登记对应实施项。 | [平板专项台账 TAB-B-02/TAB-F-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 5759423`。 |
| `CHG-20260804-027` | `3159f47` | 修复/ArkUI 兼容 | 将三个状态圆点从 `Circle` 改为带圆角的 `Column`，消除 API 22 构建兼容问题，同时保持原尺寸和颜色语义。 | [平板专项台账 TAB-D-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 3159f47`。 |
| `CHG-20260804-026` | `a1a3b06` | 维测/RDP 会话 | 为登录、会话阶段、非敏感输入活动、resize 与断开结果增加低频结构化关键日志；统一字段并避免打印密码和高频逐事件日志。 | [平板专项台账 TAB-A-04](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show a1a3b06`。 |
| `CHG-20260804-025` | `c41b3fd` | 修复/RDP resize | 新增 resize 协调器和结构化结果，限制远端桌面 resize 等待时间，并在超时后执行有界 fallback，避免窗口尺寸变化长期卡住。 | [平板专项台账 TAB-B-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[Native 模块规范](ohos-native-cpp-module-guidelines.md)；`git show c41b3fd`。 |
| `CHG-20260804-024` | `0326453` | 文档/RDP resize | 定义平板 resize 的结构化结果、两秒等待上限和 fallback 边界，明确 Native 模块拆分要求。 | [平板专项台账 TAB-B-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[Native 模块规范](ohos-native-cpp-module-guidelines.md)；`git show 0326453`。 |
| `CHG-20260804-023` | `0a64531` | 修复/首页设备列表 | 取消设备列表文本的固定高度约束，使平板大字体下列表项能够随文本自然增长。 | [平板专项台账 TAB-D-07](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 0a64531`。 |
| `CHG-20260804-022` | `9548d70` | 重构/RDP 会话 UI | 将远程会话 Surface/XComponent UI 从首页协调器拆入独立 `RdpSessionPage`，保持连接状态与生命周期由首页协调。 | [平板专项台账 TAB-A-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 9548d70`。 |
| `CHG-20260804-021` | `b329f75` | 修复/Expanded 首页 | 让 Expanded 模式下的连接表单在矮窗口中可滚动，避免下方操作区域不可达。 | [平板专项台账 TAB-D-06](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show b329f75`。 |
| `CHG-20260804-020` | `f052817` | 功能/平板能力隔离 | 增加设备能力策略并从初始化、服务、路由和展示四层隔离 tablet 上不支持的 XRDP 服务端能力，同时保留 RDP 客户端路径。 | [平板专项台账 TAB-E-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show f052817`。 |
| `CHG-20260804-019` | `e22adac` | 功能/单 HAP 安装 | 调整产品配置、构建脚本和模块声明，使 tablet 与 2in1 复用同一 HAP，并解除 tablet 安装对产品级 HNP 的依赖。 | [平板专项台账 TAB-C-04](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[验证基线](freerdp-ohos-validation-baseline.md)；`git show e22adac`。 |
| `CHG-20260804-018` | `173626f` | 功能/设备声明 | 在 Entry HAP 中声明 tablet 设备支持，使同一应用包可被平板识别和安装。 | [平板专项台账 TAB-C-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 173626f`。 |
| `CHG-20260804-017` | `3e75a8d` | 功能/Compact 验收 | 放开应用最小窗口限制，并调整设置页组件的触控热区和文本高度，使 Compact 窗口能进入验收。 | [平板专项台账 TAB-C-01/TAB-D-05](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[平板验收结果](../artifacts/tablet-acceptance/2026-08-04/result.md)；`git show 3e75a8d`。 |
| `CHG-20260804-016` | `8594b5a` | 功能/连接表单 | 连接表单按组件实际宽度重排，并允许文本自然增高，改善窄窗和大字体下的可读性与可达性。 | [平板专项台账 TAB-D-04](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[平板验收结果](../artifacts/tablet-acceptance/2026-08-04/result.md)；`git show 8594b5a`。 |
| `CHG-20260804-015` | `a25f564` | 修复/2in1 验收 | 修正 Expanded 首页布局以完成 2in1 自适应 UI 冒烟验证，并回写布局矩阵和截图证据。 | [平板验收结果](../artifacts/tablet-acceptance/2026-08-04/result.md)；`git show a25f564`。 |
| `CHG-20260804-014` | `2384b74` | 文档/签名 | 补充自动签名材料的生成、检查和使用说明，更新仓库入口与 HAP 验证基线。 | [验证基线：自动签名材料检查](freerdp-ohos-validation-baseline.md)；`git show 2384b74`。 |
| `CHG-20260804-013` | `d38abd6` | 修复/ArkUI 兼容 | 将状态圆点的着色方式由 `fill` 改为 API 22 可用的背景色，覆盖首页状态和设置状态组件。 | [平板专项台账 TAB-D-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show d38abd6`。 |
| `CHG-20260804-012` | `db66966` | 文档/ArkUI 兼容 | 登记 API 22 状态圆点兼容修复的目标、范围和验收项。 | [平板专项台账 TAB-D-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show db66966`。 |
| `CHG-20260804-011` | `52bb6b2` | 功能/自适应首页 | 拆分首页头部、设备列表、连接详情和状态区，并增加 Compact/Expanded 两套外层布局。 | [平板专项台账 TAB-D-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 52bb6b2`。 |
| `CHG-20260804-010` | `1daee02` | 文档/首页架构 | 设计自适应首页的组件拓扑、共享状态和 Compact/Expanded 布局边界。 | [平板专项台账 TAB-D-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 1daee02`。 |
| `CHG-20260804-009` | `add8bd7` | 功能/自适应设置页 | 拆分设置页共享原语和布局，隔离 Compact/Expanded 导航拓扑，并保持业务状态共享。 | [平板专项台账 TAB-D-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show add8bd7`。 |
| `CHG-20260804-008` | `5f4266a` | 文档/可访问性 | 为设置页返回按钮补充最小 48vp 触控目标要求和验收标准。 | [平板专项台账 TAB-D-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 5f4266a`。 |
| `CHG-20260804-007` | `083f9db` | 文档/设置页架构 | 设计设置页 Compact/Expanded 拓扑隔离、共享内容和路由边界。 | [平板专项台账 TAB-D-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 083f9db`。 |
| `CHG-20260804-006` | `b2b58dd` | 功能/窗口断点 | 在运行时监听窗口尺寸变化，并用统一策略驱动页面 Compact/Medium/Expanded 布局状态。 | [平板专项台账 TAB-A-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show b2b58dd`。 |
| `CHG-20260804-005` | `e26d8d1` | 文档/窗口断点 | 登记窗口断点运行时接入的文件范围、行为不变量和验收项。 | [平板专项台账 TAB-A-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show e26d8d1`。 |
| `CHG-20260804-004` | `de8b60f` | 功能/窗口策略 | 新增平板窗口断点纯策略、单元测试和本地测试脚本，为自适应页面提供统一布局模式。 | [平板专项台账 TAB-A-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show de8b60f`。 |
| `CHG-20260804-003` | `2b063f4` | 文档/窗口策略 | 登记平板断点策略任务，明确输入、输出、边界值和验收命令。 | [平板专项台账 TAB-A-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 2b063f4`。 |
| `CHG-20260804-002` | `86aa844` | 文档/研发流程 | 在仓库规则中强制设计先行、实施台账和验收证据回写，并在平板方案中定义同步流程。 | [仓库协作规则](../agent.md)、[平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 86aa844`。 |
| `CHG-20260804-001` | `6000284` | 文档/平板基线 | 新增 HarmonyOS 单包平板/2in1 适配架构、文件清单和验收方案，并注册到文档索引。 | [平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 6000284`。 |
