# MuHub HarmonyOS 单包平板适配架构、修改清单与验收方案

> 状态：核心平板适配已验证；TAB-A-06 架构拆分已实施，等待业务动作回归后升级
> 文档版本：2.2
> 审阅日期：2026-08-05
> 适用工程：MuHub HarmonyOS 应用，目标/兼容 API 22
> 本文目标：先固定修改边界、实施顺序、验收口径和架构门禁，再开始改业务代码。
> 变更控制：任何相关代码修改前，先同步本文设计和实施台账；实现及验证后再回写状态与证据。

## 1. 结论

本应用不复杂，不需要拆包，也不需要建立一套重型多端框架。合理方案是：

- 保持一个 bundleName：com.muhub.desktop。
- 保持一个 entry 模块、一个 default product、一个 HAP。
- 同一个 HAP 声明 tablet 和 2in1。
- 首页和设置页只实现两种页面拓扑：
  - Compact：窗口宽度小于 840vp。
  - Expanded：窗口宽度大于等于 840vp。
- 设备类型只决定产品能力，不决定页面布局。
- API 22 WidthBreakpoint（由当前窗口宽度产生）只决定页面布局，不决定产品能力。
- RDP 会话页独立于首页和设置页的响应式布局；布局切换不得重建 XComponent、Controller 或 RDP 会话。
- 本地 ArkUI 字体缩放、图标尺寸、XComponent Surface 像素、远端桌面尺寸和远端 DPI 分开处理，不能用一个“全局缩放比例”混在一起。
- Native 侧建立唯一的显示几何和 resize 状态机；GDI、AVC420、AVC444 渲染与输入必须使用同一个 viewport。
- 推荐 tablet 首版只作为 RDP 客户端，XRDP 被控服务只在 2in1 开启；这是待确认的产品决策 D-01，不是当前已实现事实。

这套方案与 HarmonyOS 官方的“一次开发、多端部署”方向一致：共享状态和业务，通过断点、Navigation、Grid 等进行重排，而不是复制平板页面。官方 ResponsiveLayout 示例也以断点组合单列、双列、侧栏和 GridRow/GridCol。

### 1.1 整体改造工作包

| 工作包 | 主要改动 | 前置关系 | 完成证据 |
|---|---|---|---|
| A. 架构基线 | 单包身份、Layout/Capability 策略、Gateway/Controller/SessionPage 抽取、diagnostics | 无行为变化，先完成 | 纯策略测试、生命周期快照、baseline 门禁 |
| B. 会话底座 | FreeRDP resize `_ex`、orientation、唯一 geometry、fallback、三渲染路径 viewport | 必须先于开放旋转/分屏 | Native 单测、codec 路径证明、resize 状态链 |
| C. 系统能力 | tablet 声明、auto_rotation、split、600×480 候选窗口、系统字体比例 | B 通过后启用 | 同一 HAP 的 tablet/2in1 真机矩阵 |
| D. UI 重排 | Home/Settings Compact/Expanded、内部 Grid、Scroll、字体/Icon/焦点 | A/C 基线稳定后 | 布局截图 + snapshot + 839/840 状态连续性 |
| E. 功能隔离 | D-01 下 XRDP 初始化/服务/路由/展示四层隔离，保留 RDP 客户端远程文件 | D-01 已确认 | tablet Native mock 零调用 + 2in1 回归 |
| F. 输入能力 | 触控阈值、黑边拒绝、releaseAllInput、XComponent focus 驱动的 Native IME/中文输入 | B 的 geometry 可用 | 坐标、手势、键鼠、IME 和失焦释放测试 |
| G. 交付门禁 | 自动测试、架构检查、Debug/Release、证据包 | 各工作包同步建设 | 脚本退出码、canonical HAP、完成定义 |

### 1.2 本文怎么用

1. 产品先确认第 7 节 D-01；未确认时可以做无行为变化的抽取和布局/resize 基础，但不能宣称功能隔离完成。
2. 开发按第 10 节文件清单改，严格遵循第 11 节顺序和提交边界；不得先开放 rotation/split 再补 resize。
3. 每个 PR 执行第 14 节对应阶段的测试与 baseline/strict 架构门禁，并提交本阶段 smoke 证据。
4. 测试按第 12 节矩阵执行，判定以第 13 节 snapshot/diagnostics 的数值为准，截图和肉眼观察只作补充。
5. 发布评审逐项核对第 18 节完成定义；任一 P0 项没有证据就不能标记“平板适配完成”。

### 1.3 文档先行与同步流程

本方案实施期间，每个代码任务必须按以下顺序执行：

1. **定位设计**：确认改动对应本文的工作包、架构章节、文件级修改项和验收 ID。
2. **先改文档**：补齐目标行为、非目标、所有权、调用/数据流、状态机或 API、兼容策略、实际文件、验收阈值和回退条件；不适用的字段明确写“不适用”，不能省略后让代码自行决定。
3. **登记台账**：在第 11.1 节新增/更新 Change ID，状态达到 `DesignReady`，并审阅文档 diff。未达到该状态不得编辑代码。
4. **再改代码**：代码范围必须落在台账和第 10 节文件清单内。发现需要新增文件、跨模块调用或改变行为时，先暂停实现并返回步骤 2。
5. **验证并回写**：记录实际文件、测试命令、diagnostics、截图/日志证据和设计偏差；先标 `Implemented`，验收通过后再标 `Verified`。
6. **同步当前事实**：只有 `Verified` 后，才把 feature matrix、当前交互、验证基线或 README 中的能力状态改成“已支持”。

文档状态分工：

| 文档类型 | 实现前怎么写 | 验证后怎么写 | 禁止事项 |
|---|---|---|---|
| 本文目标架构/验收方案 | 写清 Planned/DesignReady 设计、代码范围和验收 | 回写 Implemented/Verified、偏差和证据 | 未改本文就直接改平板相关代码 |
| freerdp-ohos-feature-matrix.md | 仅可增加明确标注的 planned/gap 链接 | 真实测试通过后更新支持状态 | 提前把目标能力写成已完成 |
| settings-desktop-current-interactions.md | 只记录仍然成立的当前行为；目标变化链接本文 | UI 真机验收后更新当前交互 | 用目标稿覆盖尚未实现的行为 |
| ohos-native-cpp-module-guidelines.md | 仅在模块所有权/通用规则变化时先更新 | 验证文件归属和规模符合规则 | 把具体产品 UI 逻辑写成通用 Native 规范 |
| freerdp-ohos-validation-baseline.md / README | 先补计划采用的新命令、参数和候选流程，并标待实现 | 命令实际可执行后改为当前步骤 | 发布不存在的脚本或未验证命令 |
| FreeRDP/xrdp 子模块文档 | 先写公共 ABI、协议行为、兼容和上游边界 | 子模块提交与父仓 SHA 可复现后记录版本 | 只改父仓调用而不记录社区接口变化 |

“设计已写过”不等于可以跳过同步：如果本次代码完全符合既有设计，仍需在实施台账中引用对应章节和验收 ID；如果不完全符合，必须先修改本文。文档和代码可以在同一提交，但文档 diff 必须先形成并完成审阅；公共 ABI、功能隔离、manifest/单包边界和子模块变更优先采用独立文档提交。

## 2. 固定约束和非目标

### 2.1 必须保持不变

| 项目 | 固定值/规则 | 验收方式 |
|---|---|---|
| bundleName | com.muhub.desktop | 解包或查看 HAP profile |
| module | entry | 构建产物检查 |
| product/target | default | build-profile.json5 和构建产物检查 |
| native 模块名 | libentry.so | ArkTS import 和 HAP 内容检查 |
| 产物 | 唯一正式安装产物 entry-default-signed.hap | 允许同一 target 的 unsigned 中间产物；禁止产生 tablet/2in1 两个产品 HAP |
| 升级关系 | 不改包名和签名身份 | 覆盖安装验证 |

### 2.2 本轮明确不做

- 不新增 HAR、HSP、Feature HAP 或第二个 entry。
- 不新增 tablet/2in1 两套 product flavor。
- 不复制 TabletHomePage、DesktopHomePage 等两套页面。
- 不做五套完全不同的 XS/SM/MD/LG/XL 页面。
- 不把所有固定 vp 机械替换为百分比。
- 不新增 Redux、全局路由框架或完整 Design System。
- 不在首版实现多显示器、跨设备流转、复杂捏合缩放、手写笔专用协议。
- 不因为应用布局修改 FreeRDP 通用协议核心。
- 不把“隐藏按钮”当成功能隔离。

### 2.3 单包条件下的现实边界

一个 HAP 同时服务 tablet 和 2in1 时，不能假设所有产品都支持 HNP。2026-08-04 的 tablet 真机验证表明：产品未启用 `const.startup.hnp.install.enable` 时，内嵌 `xrdp.hnp` 会以 HNP API `0x2009`（十进制 8201）失败，BMS 对外返回 9568407；仅删除 HNP 文件但保留 `hnpPackages` 声明则返回 9568409。因此“单 HAP + tablet 可安装”的发布基线必须满足：

- canonical HAP 不声明 `hnpPackages`，也不执行 HNP 二次封包。
- RDP 客户端和 XComponent 继续使用标准 HAP Native `.so`。
- XRDP 被控服务若保留，必须使用标准 HAP 可承载的进程内 `.so` 和应用资源，不依赖产品级 HNP 安装开关。
- 在进程内 XRDP 的配置/share 资源迁移和 2in1 回归完成前，不能把“单包被控服务兼容”标为 Verified。

若 D-01 采用本文推荐值，运行时还需要保证：

- tablet 不启动 XRDP。
- tablet 不调用 XRDP N-API。
- tablet 不请求录屏权限。
- tablet 不创建 XRDP 状态卡和设置入口。
- tablet 即使收到旧路由或非法路由也不能进入 XRDP 页面。

录屏权限声明仍会物理存在于同一 HAP 中，但 HNP 不能再作为该单包的发布依赖。若未来要求权限声明也按设备物理裁剪，就与“一个包”约束冲突，必须重新决策，不能靠运行时代码解决。

## 3. 当前工程基线和风险

以下是本次代码审阅发现的现状，不代表已经修改：

| 范围 | 当前状态 | 风险 |
|---|---|---|
| 包配置 | bundleName 已是 com.muhub.desktop；一个 default product、一个 entry target | 符合单包目标 |
| 设备声明 | module.json5 当前只声明 2in1 | tablet 不能作为正式目标设备验收 |
| 最小窗口 | 1280 × 760 | 无法覆盖平板竖屏、分屏和小浮窗 |
| 首页 | 永久左右 Row；设备列表 31%；标签宽 190vp；多处固定高度 | 窄窗口、1.75 字体下会挤压或裁切 |
| 设置 | 永久 196vp 左侧栏 | Compact 下内容空间不足 |
| 按钮 | 多处 34/36/38vp 高度 | 触控热区偏小，字体放大后易裁字 |
| 字体 | 使用 fp，但 AppScope 未配置跟随系统 | 实际不完整跟随系统字体缩放 |
| 图标 | 功能图标为 48×48 viewBox 的 SVG，视觉尺寸约 16～24vp | 资源本身合理，不应改成百分比 |
| Index | 约 1218 行，同时负责 UI、连接、权限、XRDP、Native 回调和会话 | 响应式改造容易把业务与布局耦合 |
| XComponent | 已使用稳定 XComponentController，SURFACE 填满 Stack | 基础正确，但会话 Builder 仍在 Index 内 |
| resize | ResizeCoordinator 内联在 native_bridge_context.cpp；请求结果被忽略；等待精确尺寸帧无超时 | 旋转/分屏时存在永久黑屏风险 |
| viewport | GDI 已有 contain/letterbox 映射；AVC420/AVC444 没有统一发布同一 viewport | 画面和点击坐标可能不一致 |
| 黑边输入 | 当前输入路径允许 clamp | 点击黑边可能被夹到远端边缘并误触 |
| 触控阈值 | 8px、24px 等使用原始像素 | 不同密度设备手感不一致 |
| 虚拟键盘 | Native XComponent 无条件请求软键盘；已有 SendCommittedText 能力但未暴露到 N-API/ArkTS | 易误弹键盘，中文组合输入链不完整 |
| 远端显示参数 | 方向固定 landscape、scale 固定 100，物理宽高语义不完整 | 远端 Windows UI 尺寸和旋转信息不准确 |

当前最危险的不是首页百分比，而是 resize 严格等待没有超时。UI 大改前必须先把会话生命周期基线和诊断日志固定，否则旋转黑屏会很难区分是 ArkUI 重建、Surface 改变、FreeRDP resize，还是解码器 viewport 的问题。

## 4. 目标架构的四个单一事实来源

| 状态 | 唯一来源 | 禁止来源 |
|---|---|---|
| Compact/Expanded | API 22 WidthBreakpoint；XS/SM/MD = Compact，LG/XL = Expanded | 再用 widthVp 判断、deviceType、横竖屏名称 |
| XRDP 等产品能力 | DeviceCapabilityPolicy 生成的不可变能力快照 | 窗口宽度、某个按钮是否显示 |
| RDP 会话状态 | RdpClientController/Index 协调层 | Home、Settings 展示组件 |
| 画面和输入坐标 | Native 最终渲染 viewport + geometryRevision | ArkUI 百分比、vp 推算、各解码器各算一遍 |

这四项是架构验收的核心。任何代码出现第二个来源，都应在评审中阻止。原始 `deviceInfo.deviceType` 只能由 `DeviceCapabilityPolicy` 的系统输入适配器读取一次，并产出包含 `capabilitySnapshotId/sourceDeviceType` 的不可变能力快照；EntryAbility、Index、Controller 和验收快照都传递/序列化这同一对象，禁止再次读取设备类型后自行判断。

## 5. 目标架构

### 5.1 最小组件关系

~~~text
Index（页面协调器，不再承载底层算法）
├─ WindowLayoutPolicy
│  └─ WidthBreakpoint -> Compact / Expanded
├─ DeviceCapabilityPolicy
│  └─ deviceType -> 不可变产品能力快照
├─ RdpClientController
│  └─ 连接、断开、Native 回调、sessionId、会话状态
├─ XrdpServerController
│  └─ 2in1 的 XRDP 生命周期；tablet 直接 unavailable
├─ XComponentController（每个会话只创建一次）
│
├─ showSession = true
│  └─ RdpSessionPage
│     ├─ 常驻 XComponent
│     ├─ 状态/错误 overlay
│     └─ XComponent focus 驱动的 Native IME
│
└─ showSession = false
   └─ AdaptiveShell
      ├─ HomePage：同一状态，Compact/Expanded 重排
      └─ SettingsPage：同一路由，Compact/Expanded 重排
~~~

不额外建立全局状态框架。Index 继续拥有表单值、选中配置、当前设置路由等页面级状态；连接协议和 Native 回调抽到 RdpClientController，N-API 调用统一经 NativeRdpGateway。

### 5.2 依赖方向

~~~text
components/*
    ↓ 只接收状态和回调
Index / 页面协调器
    ↓
RdpClientController / XrdpServerController
    ↓
NativeRdpGateway
    ↓
libentry.so
    ↓
session / surface / input / channels
    ↓
FreeRDP OHOS 平台层
~~~

依赖规则：

- components 不直接 import libentry.so。
- adaptive 不依赖 RDP、XRDP 或 libentry.so。
- capability 不依赖 UI 组件。
- rdp 业务层不判断 Compact/Expanded。
- N-API 文件只负责注册、参数校验、类型转换和转发。
- Surface 几何、resize、输入映射、IME 转发的算法不放入 N-API 入口。
- FreeRDP 负责协议语义、Display Control、DPI/scale、orientation 和 RDP 输入。
- HAP 负责 UI、权限、路由、能力过滤、XComponent 生命周期和 ArkTS/Native 传输。

### 5.3 必须长期成立的架构不变量

1. 窗口断点变化不改变 sessionId。
2. 会话期间 XComponentController 不重建。
3. 会话页面可见且 Surface ready 时，createdCount - destroyedCount = 1；用户关闭系统窗口后由应用进程退出统一销毁 Surface 和会话资源。
4. NativeWindow 可以因 Surface changed 重建，但不触发 RDP reconnect。
5. Surface 尺寸只取 Native XComponent 的物理像素，不从 ArkUI vp 推算。
6. 渲染队列关联 targetGeneration，viewport 发布 geometryRevision，输入只消费当前 geometryRevision；旧请求不能覆盖新目标。
7. 黑边区域不发送远端 pointer down/click。
8. 只有已 Sent 的 resize 最长等待 2 秒；其余状态立即使用 fallback，继续显示最后可用画面。
9. 多个 resize 采用 last-write-wins，旧请求完成不能覆盖最终目标。
10. D-01 采用推荐值且 tablet 能力快照为 XRDP unavailable 时，XRDP start 次数和录屏权限请求次数都必须为 0。
11. 表单值、选中配置、设置路由都由响应式布局上层持有，839/840 切换不能丢状态。
12. Native RdpSession 的 sessionId 是权威值；ArkTS Controller 只能镜像，不能生成第二个协议 sessionId。

## 6. UI 布局整体方案

### 6.1 断点

API 22 SDK 定义的宽度断点为：

| 官方断点 | 窗口宽度 |
|---|---|
| XS | 小于 320vp |
| SM | 320～599vp |
| MD | 600～839vp |
| LG | 840～1439vp |
| XL | 大于等于 1440vp |

本应用正式支持的最小窗口目标为 600 × 480vp，因此首版只需要两种拓扑；策略仍要对 XS/SM 做 fail-safe 映射：

~~~text
XS/SM/MD（<840vp） -> Compact
LG/XL（>=840vp）   -> Expanded
~~~

XL 不新增第三套页面，只设置内容最大宽度并扩大留白。初始值使用 API 22 的 UIContext.getWindowWidthBreakpoint()，变化监听使用 UIContext.getUIObserver().on/off('windowSizeLayoutBreakpointChange', callback)；实际 widthVp/heightVp 只进入诊断和截图证据，不参与第二次布局判断。不使用 API 26 才提供的 ContainerReader。

### 6.2 页面行为

| 页面 | Compact，小于 840vp | Expanded，大于等于 840vp |
|---|---|---|
| 首页 | 设备列表进入连接详情，单栏 | 设备列表 + 连接详情分栏 |
| 连接表单 | 标签在输入框上方，表单可滚动 | 标签/输入框按 Grid 分列 |
| 设置 | 设置列表进入子页 | 左侧导航 + 右侧内容 |
| 状态卡 | 单列或 2×2 | 四列，按能力过滤 |
| 会话 | 独立全屏会话页 | 同一个独立全屏会话页 |

首页和设置页不是两套业务组件。可以有 Compact/Expanded 两个小型布局 Builder，但它们接收同一份状态和同一组回调，禁止各自持有表单值、选择状态或 Native 调用。

### 6.3 哪些尺寸应该自适应

| 类型 | 推荐做法 | 示例 |
|---|---|---|
| 主内容、列表剩余空间、XComponent | 100%、layoutWeight、Grid | XComponent 充满会话 Stack |
| 页面拓扑 | Navigation、GridRow/GridCol、条件 Builder | 首页单列/双列，设置单栏/分栏 |
| 超宽窗口 | maxWidth + 居中 | XL 不无限拉长表单 |
| 侧栏 | minWidth/maxWidth 或 Navigation 分栏 | 不用永久 196vp，也不用任意百分比 |
| 文本按钮、卡片、表单行 | minHeight + padding + 自然高度 | 避免 1.75 字体裁切 |
| 图标、圆角、边框、基础间距 | 固定 vp/token | 20vp 图标、48vp 热区 |
| 字体 | fp | 跟随系统，设置最大比例 |

“固定值”不是都错。图标、圆角和触控热区应保持可控的 vp；需要去掉的是会裁切文本、锁死页面或按窗口无限放大的固定值。

窗口 WidthBreakpoint 只控制首页/设置的页面拓扑。Expanded 分栏后，详情组件自身可能仍小于 600vp；表单、卡片等内部重排使用 GridRow 的 component-size breakpoint（BreakpointsReference.ComponentSize），按组件可用宽度决定标签横排或置顶，但不得反向修改全局 Compact/Expanded 状态。

### 6.4 安全区和窗口装饰

- Home/Settings 默认在可用安全区内布局，不用固定顶部高度抵消状态栏。
- 2in1 浮窗下，标题和 Header 操作不能与系统窗口按钮区域重叠。
- P0 不启用沉浸式：会话 XComponent 填充系统提供的可用内容区，工具栏也位于该内容区，避免首版再维护一套 avoid-area 监听。
- 键盘避让单独按会话 IME 方案处理，不能把键盘高度混入永久 safe-area token。
- 若后续为了增加远端画面面积启用沉浸式，必须作为独立 P1，补 avoid-area/window-title-button 监听及对应验收，不能只调用 expandSafeArea。

## 7. 功能隔离方案

### 7.1 决策门禁 D-01 与推荐能力矩阵

| 决策 | 当前状态 | 推荐值 | 实施门禁 |
|---|---|---|---|
| D-01：tablet 是否提供 XRDP 被控服务 | 待产品确认 | tablet 不提供；2in1 保持提供 | 第 11 节步骤 5 开始前必须确认并记录 |

决策落地时必须在本文或关联决策记录中写明：`result`、`owner`、`decisionDate`、`effectiveVersion` 和理由；缺任一字段视为未确认。

在 D-01 未确认前，可以完成布局策略、诊断和无行为变化的会话抽取，但不能把 XRDP 隔离行为发布为完成。若最终决定 tablet 也提供 XRDP，只修改 DeviceCapabilityPolicy 和对应验收矩阵，不能增加第三套布局。

未知、空值或未来新增的 deviceType 对 XRDP 必须 fail-closed：remoteControlServer = unavailable；RDP 客户端基础能力不因此关闭。

以下是 D-01 推荐值对应的首版能力矩阵：

| 能力 | tablet | 2in1 |
|---|---:|---:|
| RDP 客户端 | 支持 | 支持 |
| RDP 客户端磁盘重定向（`\\tsclient\Downloads`） | 支持 | 支持 |
| XComponent 远程桌面 | 支持 | 支持 |
| 触控 | 支持 | 支持 |
| 物理键盘/鼠标/触控板 | 支持 | 支持 |
| 显式虚拟键盘入口 | 支持 | 支持 |
| XRDP 被控服务 | 默认关闭/不可用 | 支持 |
| 录屏权限请求 | 不触发 | XRDP 按需触发 |
| XRDP 状态卡、访问码、设置入口 | 不创建 | 展示 |

触控、鼠标、键盘和手写笔属于输入方式，不用于判断页面布局。运行时收到哪类输入就走对应适配；不能简单认为 tablet 只有触控、2in1 只有鼠标。

### 7.2 XRDP 四层隔离

XRDP 必须同时满足：

1. 初始化隔离：tablet 冷启动不刷新录屏权限、不读取 XRDP diagnostics、不启动 HNP。
2. 服务隔离：XrdpServerController 在 unsupported 时直接返回 unavailable，不进入 NativeRdpGateway。
3. 路由隔离：Settings 收到 remoteControl 旧路由、深链或恢复状态时回退到设置概览。
4. 展示隔离：Home Footer、设置入口、访问码和 XRDP 状态组件不创建。

UI 层和服务层都要保护。只隐藏入口不能防止冷启动自动调用，也不能防止旧状态恢复进入功能。

`RemoteFilesDirectory`、`EntryAbility.ensureDownloadDriveDirectory()` 以及 FreeRDP 的 Downloads drive 属于 **RDP 客户端**磁盘重定向，不属于 XRDP 服务端。tablet 仍支持 RDP 客户端，因此这些初始化不能用 `remoteControlServer` 能力关闭，也不计入 XRDP Native 调用次数。当前“远程文件”卡位于 `RemoteControlSettingsPage` 内；若 tablet 隐藏该 XRDP 页面，必须把该卡移到两类设备均可达的 RDP 客户端/基础设置区域，不能把功能一并隐藏。

## 8. 字体、Icon 和缩放

### 8.1 本地 ArkUI 字体

- 字体继续使用 fp，不改成百分比。
- AppScope 通过 configuration profile 设置 followSystem，最大比例 1.75。
- 所有含文本的固定高度改为 minHeight + 上下 padding。
- 页面内容在小高度和 1.75 字体下必须可滚动到达。
- 正文允许换行；关键地址、端口等需要保持单行时使用省略和可查看完整值的交互。
- 中文和英文都验收，不能只测中文。

拟新增配置：

~~~json
{
  "configuration": {
    "fontSizeScale": "followSystem",
    "fontSizeMaxScale": "1.75"
  }
}
~~~

### 8.2 图标

当前功能图标主要是 SVG，48×48 viewBox，继续保留：

- 视觉尺寸通常为 16/20/24vp。
- 点击热区与视觉尺寸分离；本轮所有可点击 UI 控件硬门槛为不小于 48×48vp，可用外层容器/responseRegion 扩大而不放大 SVG。
- 图标不随字体 1.75 倍同比放大；扩大容器和热区。
- 继续使用 fillColor/currentColor 支持深浅色。
- 应用 layered icon 的 PNG 不因平板布局重做。
- 只有会话工具栏缺少键盘、退出或缩放图标时才新增 SVG。

### 8.3 三套缩放不能混用

| 缩放 | 单位/来源 | 负责层 |
|---|---|---|
| ArkUI 页面布局和字体 | vp/fp、系统字体比例 | ArkUI |
| XComponent Surface | Native XComponent physical px | Surface/渲染层 |
| 远端桌面和 Windows DPI | RDP DesktopWidth/Height、Display Control、DPI/scale | FreeRDP OHOS 平台层 |

RenderFit.CENTER 只是 ArkUI 组件呈现策略，不等于远端桌面缩放。远端 resize、DPI 和输入坐标必须依据 Native Surface 像素和最终 viewport。

远端显示参数分两阶段，避免把“旋转可用”与“远端 UI 缩放增强”绑成一个发布阻塞项：

P0（平板旋转/分屏必须完成）：

- Surface resize 稳定后，按最终 Native Surface 物理像素请求动态分辨率。
- 从实际 display rotation 得到 RDP orientation，不再永久写死 landscape；orientation 与 resize 结果走唯一的版本化 `_ex` 接口，不保留旧接口兼容层。
- 本地 `densityDPI` 只用于触控阈值换算，不冒充远端 Windows DPI。

P1（独立增强）：

- 使用有效的 xDPI/yDPI 换算 physicalWidth/physicalHeight；无效时采用协议安全默认值并记录 fallback，不能用 Surface 像素冒充毫米。
- 提供远端 auto scale，并独立评估手动 100%/140%/180%。
- 本地字体比例变化始终不触发远端桌面 DPI 变化。

## 9. XComponent、旋转、resize、输入和 IME

### 9.1 会话页隔离

RdpSessionPage 固定为：

~~~text
Stack
├─ XComponent（唯一、100% × 100%、SURFACE、稳定 Controller）
├─ 连接状态/错误 overlay
└─ IME 输入宿主
~~~

showSession 必须是 Index 最外层分支。Home/Settings 的 Compact/Expanded 分支只在非会话状态下存在。当前产品不提供应用内返回、断开按钮或浮动会话工具栏：用户点击系统窗口 `X` 退出应用并结束会话，远端主动断开时回到首页。该决策不要求新增 `disconnect` N-API；若未来支持沉浸式全屏或隐藏系统标题栏，必须先新增可达的应用内退出路径并单独验收，不能沿用本决策。

### 9.2 resize 状态机

~~~text
Stable(surfaceGeneration=s, geometryRevision=r)
  -> SurfaceChanged(s+1)
  -> 用旧 desktop/frame 在新 Surface 上 contain
  -> FallbackPresented(geometryRevision=r+1，输入可恢复)
  -> TrailingDebounce（固定 200ms）
  -> Request
       Sent        -> AwaitingNormalizedTarget（从实际发送时开始计时）
       Deferred    -> 保持 fallback；channel ready 后可重试
       Unchanged   -> 保持 fallback，直接 Stable
       Unsupported -> 保持 fallback，直接 Stable
       Failed      -> 保持 fallback，直接 Stable
  -> TargetPresented（匹配当前 surfaceGeneration、targetGeneration、normalized target）
       -> 发布新 geometryRevision
  -> SentTimeout <= 2s -> 回到 fallback，保留最后可用帧
~~~

关键要求：

- `freerdp_ohos_session_resize_ex` 是唯一 resize ABI；版本化 request/result 区分 Sent、Deferred、Unchanged、Unsupported、Failed，并返回 normalized/sent target。
- 只有 Sent 才进入目标帧等待。
- Deferred、Unsupported、Failed 和 Unchanged 不等待 2 秒，也不丢帧。
- App runtime 只加载 `_ex`；符号缺失时标Unsupported并继续fallback，不调用旧BOOL。发布候选打包门禁要求内置运行库导出该符号。
- 等待期间继续按 contain 显示最后可用帧，不允许清空后永久黑屏。
- 若会话尚无任何可用首帧，则显示明确的连接/调整中 overlay，保持输入无效，直到首个 viewport 发布；不能把空黑 Surface 伪装成 fallback 已成功。
- 使用 trailing debounce 200ms：连续 20 个、间隔 50ms 的事件期间发送 0 次，最后事件后 200～500ms 内只发送最终目标一次。
- 连续 resize 只让最新 surfaceGeneration/targetGeneration 生效。
- 目标帧必须同时匹配当前 `surfaceGeneration`、当前 `targetGeneration` 和实际 sent 的 `normalizedWidth/normalizedHeight`，才能进入 TargetPresented；仅因帧在请求后入队或只带 generation 标签，不足以判定目标已呈现。
- 服务器不支持 Display Control 时仍可使用旧远端尺寸。
- 任何超时或 fallback 都要有诊断计数，不逐帧刷日志。

### 9.3 唯一显示几何

新增 display_geometry，统一保存：

~~~text
surfacePx:  Native Surface width/height
desktopPx:  当前远端桌面 width/height
viewportPx: 实际 contain 后 x/y/width/height
surfaceGeneration: 每次 Surface changed 递增
geometryRevision: 每次成功 present 并发布 viewport 后递增
targetGeneration: resize 请求的 last-write-wins 代次
~~~

GDI、AVC420、AVC444 每次成功 present 后都发布实际 viewport。远端帧不修改 FreeRDP 协议结构来携带 generation；应用渲染队列保存提交时的 surfaceGeneration/targetGeneration 快照和实际 frame desktop 尺寸，目标帧判定仍必须校验三者。resize 失败时，最后可用远端帧必须在当前 surfaceGeneration 的新 Surface 上重新 contain，成功 present 后发布 fallback geometryRevision，输入才能恢复。

`display_geometry` 替换并接管 `SurfaceBridge` 现有的 `viewportX_/viewportY_/viewportWidth_/viewportHeight_` 状态；SurfaceBridge 只委托读写和发布事件，不再保留第二份 viewport。这样渲染、输入和 diagnostics 才会消费同一对象。

- App Native 只校验当前 surfaceGeneration 的 geometryRevision 和黑边；viewport 到远端 desktop 的坐标变换继续由 FreeRDP OHOS ohos_pointer.c 负责。
- pointer down/click 在黑边外直接拒绝。
- 拖动已经开始后，move/up 可以有限 clamp，避免远端按钮粘住。
- Surface changed 到 fallback/new viewport present 之间拒绝 pointer down。
- 旋转/Surface invalidation、失焦、切后台、断开前先用最后有效 geometry 发送所有活动左/右/中键和触控拖动的 button-up，再 release all keys，最后清理本地手势/按钮状态；releaseAllInput 必须幂等。只清 ResetNativeTouchGesture 而不发远端 up 不算完成。

映射纯函数按整数舍入误差不超过 1px；端到端坐标精度使用物理鼠标或自动输入和远端坐标探针验证。手指触控只验收目标命中与手势，不用肉眼宣称 2px 精度。

### 9.4 触控

首版必须支持：

- 单指点击 = 左键点击。
- 无移动长按 = 右键点击。
- 移动超过密度换算后的拖动阈值 = 左键按下并拖动，抬手时释放。
- 双指滚动 = 远端滚轮。
- 物理鼠标/触控板 hover、点击、滚轮保持可用。

8px、24px 等阈值改为由 density 换算的 vp 语义。首版不强制捏合缩放、惯性滚动和手写笔专用映射；这些在基础点击和 resize 正确后再做。

### 9.5 虚拟键盘和中文输入

- XComponent 获得 Native focus 时由 `RemoteImeClient` 自动 attach/show 系统输入法；失焦、Surface 销毁或会话退出时自动 hide/detach，不提供工具栏按钮或配置开关。若用户通过系统返回键临时隐藏键盘但 XComponent 仍保持焦点，下一次触摸 XComponent 时由 Native 根据 keyboard status 恢复 show，不新建第二输入链。
- ArkTS 不创建隐藏 `TextInput`，不维护 IME active 状态，也不暴露逐字符或 open/close N-API；仅向 Native 提供宿主 `windowId/displayId`，并在会话页设置 `KeyboardAvoidMode.NONE`。
- Native `InputMethod_TextEditorProxy` 保存 preview text 本地组合态，只把 committed insert 交给现有 `RdpSession::SendCommittedText`，不把拼音预编辑串重复发送。
- delete forward/backward 与 Enter 由 Native 直接转成平台键；物理键盘、鼠标和触控仍由 XComponent callback 处理，避免 ArkTS 输入宿主与 XComponent 双输入源。
- N-API 暴露 sendCommittedText 和 sendPlatformKey，分别薄转发到现有 RdpSession::SendCommittedText/SendPlatformKey。
- 进入会话保存当前 KeyboardAvoidMode，设置为 NONE；退出时恢复。
- IME 以 overlay 方式覆盖，会话 Surface 尺寸不能因键盘显示/隐藏而变化。
- 关闭 IME 时清理 Native preview 状态但不转移 XComponent 焦点；随后物理键盘继续由 XComponent callback 处理。
- 需要验证中文拼音组合、候选选择、英文、数字、退格、Enter 和连续开关键盘。

## 10. 文件级修改清单

所有修改均是计划项，本文落盘不表示已经改代码。

本节所有路径均从仓库根目录开始，表格中的路径可以直接定位到工程文件。

### 10.1 包、设备和窗口配置

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0 | harmony/app/entry/src/main/module.json5 | 适配候选构建中把 deviceTypes 改为 tablet + 2in1、orientation 设为 auto_rotation、supportWindowMode 加 split，并把 minWindowWidth/minWindowHeight 临时降到 600×480；只有矩阵通过才保留为发布值 | 同一个 HAP 在两类设备安装；横竖屏、分屏、浮窗可进入；最小尺寸无不可达内容 |
| P0 | harmony/app/AppScope/app.json5 | 只新增 configuration profile 引用；不改 bundleName、版本身份和图标 | bundleName 仍为 com.muhub.desktop |
| P0，新文件 | harmony/app/AppScope/resources/base/profile/configuration.json | followSystem，fontSizeMaxScale 1.75 | API 22 schema 校验和 1.75 真机验证通过 |
| 不改 | harmony/app/build-profile.json5 | 保持一个 default product、一个 entry target | 构建仍只有一个 HAP |
| P0 | harmony/app/entry/src/main/module.json5（hnpPackages） | 删除产品级 HNP 声明，canonical 单 HAP 改用标准 Native `.so`/应用资源；权限声明仍由能力策略避免 tablet 调用 | tablet 可安装启动；包内无 HNP；2in1 进程内 XRDP 资源迁移和回归另验 |
| 不改 | harmony/app/entry/src/main/module.json5（requestPermissions） | 单包条件下保留，通过能力策略避免 tablet 请求录屏权限 | tablet 冷启动无录屏请求 |

当前 1280×760 manifest 会阻止真机进入 600×480，因此不能要求“先真机通过再改 manifest”。正确流程是：在未发布的适配候选中临时改为 600×480，构建同一个 default/entry HAP 做真机矩阵；通过后保留该发布值，失败则继续修布局并重测，不能直接发布，也不能用抬高下限掩盖问题。

### 10.2 布局和能力基础

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0，新文件 | harmony/app/entry/src/main/ets/adaptive/WindowLayoutPolicy.ets | 定义 LayoutMode 和纯函数 layoutModeForWidthBreakpoint；把 XS/SM/MD 映射 Compact，LG/XL 映射 Expanded | 五个 breakpoint 映射单测通过，839/840 由真机边界验收 |
| P0，新文件 | harmony/app/entry/src/main/ets/capability/DeviceCapabilityPolicy.ets | 唯一读取一次 deviceInfo.deviceType，调用纯映射后产出含 capabilitySnapshotId/sourceDeviceType 的不可变能力快照；首版只定义必要的 remoteControlServer 能力 | tablet/2in1/unknown/空值单测通过；文件不 import UI；全仓其他文件不读 deviceType |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets | 监听/注销 windowSizeLayoutBreakpointChange；持有 layoutMode 和同一 capability snapshot；所有状态位于布局分支上层 | 839/840 往返 20 次无状态丢失；不二次读取 deviceInfo |
| P1，按需新文件 | harmony/app/entry/src/main/ets/adaptive/AdaptiveTokens.ets | 只集中页面 padding、最大内容宽度、侧栏约束、48vp 热区等少量 token | 不建立完整 Design System；无重复魔法值扩散 |

### 10.3 ArkTS 业务边界

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0，新文件 | harmony/app/entry/src/main/ets/rdp/NativeRdpGateway.ets | 成为 libentry.so 唯一 import 点；薄封装 connect/callback/输入/XComponent/IME/XRDP/diagnostics；当前产品决策不导出主动 disconnect | components 和 controllers 不直接 import libentry.so |
| P0，新文件 | harmony/app/entry/src/main/ets/rdp/RdpClientController.ets | 负责连接、Native 状态/错误回调注册和输入释放，通过回调通知 Index；系统窗口关闭依赖进程退出清理，不虚构 disconnect | 不 import UI；同一会话只注册一次回调 |
| P0 | harmony/app/entry/src/main/ets/rdp/XrdpServerController.ets | 接收 capability；unsupported 时 start/diagnostics 返回 unavailable，不进入 gateway | D-01 采用推荐值时，tablet native mock 调用数为 0 |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets | 保留页面协调、表单和路由状态；移出 Native 调用细节；能力保护所有 XRDP 回调 | 不再包含 resize/输入算法；目标控制在 500 行左右 |

现有 Index 超过 1200 行。不能只把 Builder 换个文件名而继续把所有 Native 回调塞在 Index。拆分目标是按现有职责边界减负，不是引入全局状态框架。

### 10.4 首页

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0 | harmony/app/entry/src/main/ets/components/home/HomePage.ets | 接收 layoutMode、capabilities、统一状态和回调；Compact 用 Navigation/Stack 单页切换，Expanded 用 Row/Grid 同时承载同一列表与详情 | 两种布局不复制业务状态；两种拓扑使用 API 22 可用组件 |
| P0 | harmony/app/entry/src/main/ets/components/home/HomeDeviceList.ets | 删除组件自身 31% 宽；宽度由上层决定；卡片固定高度改 minHeight + padding；删除按钮热区 48vp | 600vp、1.75 字体无裁切 |
| P0 | harmony/app/entry/src/main/ets/components/home/HomeConnectionDetails.ets | 删除 190vp 标签列和 52vp 固定行；用 GridRow + BreakpointsReference.ComponentSize 按详情组件宽度决定标签置顶/横排；表单可滚动；按钮 minHeight 48vp | 即使 Expanded 的详情 pane 小于 600vp 也不挤压；600×480 全部字段可达 |
| P0 | harmony/app/entry/src/main/ets/components/home/HomeHeader.ets | 删除两侧 260vp 和 84vp 固定高度；Compact 重排标题/操作；设置按钮热区 48vp | 中英文、1.75 字体无重叠 |
| P0 | harmony/app/entry/src/main/ets/components/home/HomeStatusFooter.ets | Compact 单列/2×2，Expanded 四列；删除 70vp 固定高度；按能力不创建 XRDP 卡 | D-01 采用推荐值时，tablet 不出现 XRDP 状态 |
| P1 | harmony/app/entry/src/main/ets/components/home/HomeResources.ets、harmony/app/entry/src/main/ets/components/home/HomeText.ets | 只补新增的平板/会话文案 | 不顺带迁移全部旧字符串 |

### 10.5 设置

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0 | harmony/app/entry/src/main/ets/components/SettingsPage.ets | 永久 196vp 侧栏改为 Compact 单页导航/Expanded 分栏；按能力过滤；非法 remoteControl 路由回退 | D-01 采用推荐值时，tablet 无入口且不可强行进入 |
| P0 | harmony/app/entry/src/main/ets/components/settings/SettingsPrimitives.ets | Back、导航项、列表项热区至少 48vp；文本容器改 minHeight | 键鼠、触控、焦点均可达 |
| P0 | harmony/app/entry/src/main/ets/components/settings/BasicSettingsPage.ets | 34vp 刷新按钮改 48vp 热区；页面加 Scroll | 600×480、1.75 字体可达 |
| P0 | harmony/app/entry/src/main/ets/components/settings/RemoteControlCards.ets | 36vp 按钮改 minHeight 48vp；卡片自然增高 | 2in1 行为不回退 |
| P0，移动职责 | harmony/app/entry/src/main/ets/components/settings/RemoteControlCards.ets、harmony/app/entry/src/main/ets/components/settings/RdpClientSettingsCards.ets（新建）、harmony/app/entry/src/main/ets/components/settings/BasicSettingsPage.ets | 把 RemoteFilesCard 从 XRDP 专属文件移到客户端设置文件，并在两类设备均可达的基础设置区渲染；SettingsPage 继续传递打开目录回调 | 两类设备都能打开共享目录；隐藏 XRDP 页面不影响客户端 drive redirection |
| 边界保护，不按 XRDP 隔离 | harmony/app/entry/src/main/ets/entryability/EntryAbility.ets、harmony/app/entry/src/main/ets/rdp/RemoteFilesDirectory.ets | 保持 Downloads 客户端共享目录准备；不得读取 remoteControlServer 后跳过 | tablet 冷启动仍准备 `\\tsclient\Downloads` 对应目录；该调用不计入 XRDP 指标 |
| D-01 推荐值下保留但 tablet 不可达 | harmony/app/entry/src/main/ets/components/settings/RemoteControlSettingsPage.ets | 不复制 tablet 版本；依赖路由和服务双重保护 | tablet 路由测试通过 |
| P0 | harmony/app/entry/src/main/ets/components/settings/SettingsTheme.ets | 增加 TOUCH_TARGET = 48 等少量 token；保留图标视觉尺寸 | 热区与图标尺寸分离 |

### 10.6 会话和 XComponent

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0，新文件 | harmony/app/entry/src/main/ets/components/session/RdpSessionPage.ets | 从 Index 移出会话 Builder；接收已有 Controller；固定 Stack + XComponent + overlay；会话期间设置并在退出时恢复 `KeyboardAvoidMode.NONE`，不创建 TextInput 或 IME 按钮 | 切布局/字体/键盘显示状态不重建 Controller、session 或 Surface |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets | XComponentController 只在这里创建一次；showSession 保持最外层分支 | 静态只有一个创建点，运行时 controllerInstanceId 保持不变 |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets、harmony/app/entry/src/main/ets/rdp/RdpClientController.ets | 会话开始及 display change 时用 display.getDefaultDisplaySync() 读取 rotation 和本地 densityDPI，形成 P0 DisplayProfile，经 Controller 转发；display.on/off('change') 成对 | 旋转后 profile generation 更新，页面消失后无残留监听 |
| P1 | harmony/app/entry/src/main/ets/pages/Index.ets、harmony/app/entry/src/main/ets/rdp/RdpClientController.ets | DisplayProfile 再加入经校验的 xDPI/yDPI 和远端 scale 策略 | 不与 P0 旋转提交混合；无效值有明确 fallback |
| P0 | harmony/app/entry/src/main/cpp/input/xcomponent_input_registration.cpp | 显式 `SetNeedSoftKeyboard(false)`，由 XComponent focus/blur 唯一驱动 `RemoteImeClient`；MakeNativePointer 默认不允许 clamp | 获焦自动显示、失焦自动隐藏且无第二输入链；down/click/hover 不夹到边缘 |
| P0 | harmony/app/entry/src/main/cpp/napi/napi_exports.cpp、harmony/app/entry/src/main/cpp/napi/napi_exports.h、harmony/app/entry/src/main/cpp/types/libentry/Index.d.ts | 增加 sendCommittedText、sendPlatformKey、surface orientation/input density、RDP diagnostics 薄接口；P1 才加 xDPI/yDPI/scale | 参数校验后转发，不承载算法 |
| P0 | harmony/app/entry/src/main/cpp/input/xcomponent_mouse.cpp、harmony/app/entry/src/main/cpp/input/xcomponent_touch_gesture.cpp | down/click/hover 的 allowClamp=false；仅已开始拖动的 move/up 可有限 clamp；原始 px 阈值改为 density 相关 | 不同密度手感一致；黑边不误触远端边缘 |
| P0 | harmony/app/entry/src/main/cpp/input/xcomponent_input_bridge.h、harmony/app/entry/src/main/cpp/input/xcomponent_input_internal.h、harmony/app/entry/src/main/cpp/session/rdp_session_input.cpp | 新增幂等 releaseAllInput：发送活动左/右/中/触控拖动 up，再释放所有键，最后清本地状态；在 Surface invalidation/blur/background/disconnect 调用 | diagnostics 的 buttonsDown/keysDown 均归零，远端无粘键/粘按钮 |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets、harmony/app/entry/src/main/ets/rdp/NativeRdpGateway.ets、harmony/app/entry/src/main/cpp/napi/napi_exports.cpp、harmony/app/entry/src/main/cpp/types/libentry/Index.d.ts | onPageHide/aboutToDisappear/断开调用 releaseAllInput；N-API 只转发 | 前后台和断开路径覆盖，不只依赖 XComponent blur |

若 RdpSessionPage 超过 300 行，再拆 SessionToolbar 或 SessionImeHost；首轮不预先拆出大量小文件。

### 10.7 Native resize、几何和输入

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0，新文件 | harmony/app/entry/src/main/cpp/surface/display_geometry.h/.cpp | 唯一保存 surfacePx、desktopPx、最终 viewportPx、surfaceGeneration、geometryRevision；替换 SurfaceBridge 现有 viewport 字段；只负责 contain 几何和代次，不复制远端坐标映射 | GDI/AVC/input/diagnostics 共用同一对象；边界单测通过 |
| P0，新文件 | harmony/app/entry/src/main/cpp/session/rdp_display_resize_coordinator.h/.cpp | 从 N-API 文件移出；在 session/orchestration 层实现 trailing debounce、generation、明确结果状态、2秒 Sent 超时和 fallback，避免 surface 层反向依赖 session | unsupported/deferred/timeout 不黑屏 |
| P0 | harmony/app/entry/src/main/cpp/CMakeLists.txt | 注册新增源文件和可测试纯逻辑 | HAP 构建和测试 target 通过 |
| P0 | harmony/app/entry/src/main/cpp/napi/native_bridge_context.cpp | Surface 回调只转发；检查 resize 结果；非 Sent 不进入严格等待 | 文件不新增算法，目标不继续膨胀 |
| P0 | harmony/app/entry/src/main/cpp/surface/surface_bridge.h/.cpp | 删除自身 viewportX_/Y_/Width_/Height_ 副本，委托 display_geometry；Surface changed 后先重算旧 desktop 在新 Surface 的 fallback viewport | fallback present 后输入恢复；Bridge 与 diagnostics 不出现第二份几何 |
| P0 | harmony/app/entry/src/main/cpp/channels/rdpgfx_pipeline.h/.cpp | 把 viewport 发布传入 AVC420/AVC444 | 三渲染路径同一接口 |
| P0 | harmony/app/entry/src/main/cpp/surface/avc420_gpu_compositor*.cpp、harmony/app/entry/src/main/cpp/surface/avc444_gpu_compositor*.cpp | present 成功后发布实际 contain viewport；不再自留第二套输入几何 | 三种编码的 viewport 诊断与画面一致 |
| P0 | harmony/app/entry/src/main/cpp/session/rdp_session_input.cpp | 校验当前 geometryRevision 和黑边；通过后把 local pointer + viewport 交给 FreeRDP OHOS 映射；仅拖动 move/up 可有限 clamp | 黑边 down/click 不进入 FreeRDP |
| P0 | harmony/app/entry/src/main/cpp/freerdp/freerdp_runtime.h/.cpp | 只动态加载 resize `_ex`；缺失时返回App级Unsupported并立即fallback | diagnostics暴露resizeApi=ex/unsupported；发布打包只接受ex |
| P0 | harmony/app/entry/src/main/cpp/session/rdp_session_channels.h/.cpp、harmony/app/entry/src/main/cpp/session/rdp_session_core.h/.cpp | 把内部 bool resize 链改为结构化结果并传给 coordinator；不通过日志字符串推断状态 | Deferred/normalized target 可端到端进入 diagnostics 和状态机 |
| P0，破坏性升级 | harmony/third_party/FreeRDP/client/OHOS/ohos_session.h、harmony/third_party/FreeRDP/client/OHOS/ohos_session_display.c、harmony/third_party/FreeRDP/client/OHOS/ohos_display.c | 以带structSize/version的resize `_ex` request/result替换旧BOOL会话接口，P0返回Sent/Deferred/Unchanged/Unsupported/Failed、normalized target和真实orientation | App与内置运行库同步升级；旧resize符号引用为0；禁止解析日志推断状态 |
| P1 | harmony/third_party/FreeRDP/client/OHOS/ohos_session.h、harmony/third_party/FreeRDP/client/OHOS/ohos_session_display.c、harmony/third_party/FreeRDP/client/OHOS/ohos_display.c | 以同一版本化结构扩展真实 physical size、xDPI/yDPI、auto/manual scale | 改动通用、可同步 OHOS port，不含 MuHub UI 规则 |
| 归属不变，尽量小改 | harmony/third_party/FreeRDP/client/OHOS/ohos_pointer.c | 继续作为 viewport 到远端 desktop 坐标映射的唯一实现；只在映射本身有通用缺陷时修复 | App Native 不实现第二套远端坐标变换 |

现有 AVC420/AVC444 实现文件已经明显超出仓库模块行数约束。此次不能继续往大文件堆几何/resize 逻辑；应把共享几何抽到新文件。若必须改超大文件，只做调用接入，不在其中新增第二套算法。

### 10.8 诊断、测试、构建和架构检查

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0，新脚本 | tools/check_tablet_architecture.ps1 | 提供 baseline/strict 两种模式；检查 import/call/path allowlist 和 Controller 创建位置 | CI/本地违规时返回非 0；Commit B 后启用 strict |
| P0，新测试/脚本 | harmony/app/entry/src/test/List.test.ets、harmony/app/entry/src/test/WindowLayoutPolicy.test.ets、harmony/app/entry/src/test/DeviceCapabilityPolicy.test.ets、tools/run_tablet_arkts_tests.ps1 | 使用现有 @ohos/hypium 运行纯策略和能力测试；脚本定位 DevEco Hvigor | hvigor test 退出码为 0 |
| P0，新测试/脚本 | harmony/app/entry/src/main/cpp/tests、harmony/third_party/FreeRDP/client/OHOS/test/TestOhosPointer.c + tools/run_tablet_native_tests.ps1 | 建立不依赖 OHOS UI 的 host target；App 测 geometry/resize，FreeRDP 测 ohos_pointer 映射 | ctest 失败时脚本返回非 0 |
| P0 | harmony/app/build_hap.bat | 参数化为 build_hap.bat debug 或 release；把 buildMode 传给 Hvigor，并输出本次 buildId/product/module/buildMode/canonical HAP | 两种模式均可复现，不能靠历史 outputs 数量判定 |
| P0 | harmony/app/entry/src/main/cpp/types/libentry/Index.d.ts、harmony/app/entry/src/main/cpp/napi/napi_exports.h、harmony/app/entry/src/main/cpp/napi/napi_exports.cpp | 增加 getRdpDiagnostics | 验收可得到不含 ArkUI 状态的 Native JSON 快照 |
| P0 | docs/freerdp-ohos-feature-matrix.md | 只有功能真实通过后更新 resize/IME/viewport 状态 | 文档不提前宣称完成 |
| P0 | docs/freerdp-ohos-validation-baseline.md | 复用原 RDP 基线，不复制；新增链接到本文平板矩阵 | 通用 RDP 回归和设备适配都执行 |

## 11. 实施顺序和提交边界

必须按顺序实施，每一步独立构建、独立验收、可单独回退：

1. 固定单包、包名、当前连接和 XComponent 生命周期基线。
2. 新增 WindowLayoutPolicy、DeviceCapabilityPolicy 和单测；暂不改 UI。
3. 先无行为变化地抽出 NativeRdpGateway、RdpClientController、RdpSessionPage。
4. 验证 connectCount、sessionId、Surface created/changed/destroyed 与改造前一致。
5. 确认并记录 D-01；若采用推荐值，完成 XRDP 初始化、服务、路由、展示四层隔离，同时验证 RemoteFilesDirectory 仍作为 RDP 客户端能力可用。
6. 先完成 FreeRDP resize 唯一 `_ex` 接口（含真实 orientation）、Native resize/fallback/geometry 和输入过渡期；用测试和可控 Surface change 验证不黑屏。
7. 再在未发布适配候选中启用 tablet deviceTypes、auto_rotation、split、字体 configuration，并临时把最小窗口降到 600×480以允许真机测试。
8. 改首页 Compact/Expanded。
9. 改设置 Compact/Expanded。
10. 清理固定高度、字体裁切、Icon 热区和焦点顺序。
11. 补 XComponent focus 驱动的 Native IME、sendCommittedText 和 density 触控阈值。
12. 完成 600×480 与 1.75 字体验收后，把 600×480 固化为发布 manifest 值；若产品另有更高下限，必须有独立产品决策，不能代替布局修复。
13. 最后以 P1 独立评估远端 auto DPI、手动 100/140/180 和增强手势。

建议提交边界：

- Commit A：纯策略、基础 diagnostics、测试，无 UI 行为变化。
- Commit B：会话页/Controller 抽取，无 UI 行为变化。
- Commit C：XRDP 能力隔离。
- Commit D：FreeRDP resize 唯一 `_ex` ABI + orientation，删除旧 resize ABI。
- Commit E：App Native geometry/resize/fallback/input。
- Commit F：manifest、旋转/分屏和字体配置。
- Commit G：首页响应式。
- Commit H：设置响应式、字体和热区。
- Commit I：IME 和触控阈值。
- Commit J（P1）：远端 DPI/scale。

不要把布局、IME、FreeRDP DPI 和 XRDP 隔离放入同一个大提交。

### 11.1 实施台账与状态流转

状态只能按以下方向流转：

~~~text
Planned -> DesignReady -> Implemented -> Verified
Planned/DesignReady -> DecisionPending / Blocked -> DesignReady
~~~

- `DesignReady`：本文已写清实际代码范围、接口/状态、兼容、验收 ID 和回退条件，文档 diff 已审阅；这是开始代码修改的唯一合法状态。
- `Implemented`：代码和测试已落地、可构建，但完整验收尚未结束；不能据此更新当前能力为“支持”。
- `Verified`：对应验收 ID 全部通过，证据已归档，当前事实文档已经同步。
- `DecisionPending/Blocked`：缺产品决策、服务端、设备或外部 ABI；不得用假设直接实现越过门禁。

初始父级台账：

| Change ID | 工作包 | 设计/文件清单 | 验收 ID | 设计状态 | 实现状态 | 开始代码前的额外门禁 |
|---|---|---|---|---|---|---|
| TAB-A | A 架构基线 | 第 4、5、10.2、10.3、10.8 节 | AC-ARCH、AC-XC | DesignReady | NotStarted | 固定现状 diagnostics 基线 |
| TAB-B | B 会话底座 | 第 8.3、9.2、9.3、10.6、10.7 节 | AC-XC、AC-RESIZE、AC-INPUT | DesignReady | NotStarted | `_ex` 唯一ABI破坏性升级评审通过，App与运行库同步交付 |
| TAB-C | C 系统能力 | 第 6.4、8.1、10.1 节 | AC-PKG、AC-LAYOUT、AC-FONT | DesignReady | NotStarted | TAB-B 至少完成 resize/fallback 验证 |
| TAB-D | D UI 重排 | 第 6、8.1、8.2、10.4、10.5 节 | AC-LAYOUT、AC-FONT | DesignReady | NotStarted | 目标设备/窗口范围已确认 |
| TAB-E | E 功能隔离 | 第 7、10.3、10.5 节 | AC-CAP | DecisionPending | NotStarted | D-01 完整决策记录 |
| TAB-F | F 输入能力 | 第 9.3～9.5、10.6、10.7 节 | AC-INPUT、AC-IME | DesignReady | NotStarted | TAB-B 的唯一 geometry 已可用 |
| TAB-G | G 交付门禁 | 第 12～14 节 | 全部 AC-* | DesignReady | NotStarted | 测试/构建脚本先按第 10.8 节落地 |

#### TAB-A-01：窗口断点纯策略与本地单测

| 字段 | 内容 |
|---|---|
| Change ID | TAB-A-01 |
| 设计版本/章节 | v1.2；第 4、6.1、10.2、10.8、12.3、14.2、14.3 节 |
| 目标 | 建立唯一的 `WidthBreakpoint -> LayoutMode` 纯函数和可在本机执行的单测入口，不接入 Index、不改变现有 UI |
| 计划代码文件 | 新建 `harmony/app/entry/src/main/ets/adaptive/WindowLayoutPolicy.ets`、`harmony/app/entry/src/test/List.test.ets`、`harmony/app/entry/src/test/WindowLayoutPolicy.test.ets`、`tools/run_tablet_arkts_tests.ps1` |
| 公共 API/状态 | ArkTS 导出 `LayoutMode` 与 `layoutModeForWidthBreakpoint()`；XS/SM/MD -> Compact，LG/XL -> Expanded，未知枚举值 fail-safe 为 Compact；无 Native/FreeRDP ABI，无运行时状态 |
| 非目标 | 本项不监听窗口、不修改 Index/Home/Settings、不读取 deviceInfo、不实现 DeviceCapabilityPolicy、不修改 manifest |
| 兼容与回退 | 只使用本机 API 22 已声明的 WidthBreakpoint；删除四个新增文件即可完整回退，不影响当前应用行为 |
| 验收 ID | AC-LAYOUT：五个官方枚举映射单测；AC-ARCH：测试脚本退出码与纯策略无 UI/RDP import |
| 设计状态 | DesignReady |
| 实现状态 | Verified |
| 实际代码文件 | `harmony/app/entry/src/main/ets/adaptive/WindowLayoutPolicy.ets`、`harmony/app/entry/src/test/List.test.ets`、`harmony/app/entry/src/test/WindowLayoutPolicy.test.ets`、`tools/run_tablet_arkts_tests.ps1` |
| 设计偏差及原因 | 无；实现文件、导出 API、映射规则、非目标和回退方式均与 DesignReady 记录一致 |
| 测试命令/结果/证据 | 2026-08-04 执行 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\run_tablet_arkts_tests.ps1`，退出码 0；5/5 通过、0 Failure、0 Error；结果文件 `harmony/app/entry/.test/default/intermediates/test/coverage_data/test_result.txt`；策略文件行/函数/分支覆盖率均为 100%；`rg` 静态检查确认策略和测试无 Index/Home/Settings/RDP/Session/XComponent/deviceInfo/DeviceCapability import。构建存在一条既有 `SettingsPrimitives.ets` API 26 兼容性警告，与本项无关 |
| 关联提交 | 设计先行提交 `2b063f4`；实现与本台账回写包含在同一后续提交（以 Git 历史为准） |

#### TAB-A-02：窗口断点运行时接入

| 字段 | 内容 |
|---|---|
| Change ID | TAB-A-02 |
| 设计版本/章节 | v1.2；第 4、5.3、6.1、10.2、12.3、14.2、14.3 节 |
| 目标 | 在 Index 建立唯一、响应式的 `layoutMode` 页面状态：首次显示时读取当前窗口官方宽度断点，窗口跨断点时更新，页面销毁时使用同一个回调注销；本项不改变 Home/Settings/Session 的现有拓扑 |
| 计划代码文件 | 修改 `harmony/app/entry/src/main/ets/pages/Index.ets`；复用 `harmony/app/entry/src/main/ets/adaptive/WindowLayoutPolicy.ets` 和现有测试，不新增运行时文件 |
| 公共 API/状态 | Index 新增私有 `@State layoutMode`，初始 fail-safe 为 Compact；`aboutToAppear()` 先通过 `UIContext.getWindowWidthBreakpoint()` 初始化，再通过 `UIContext.getUIObserver().on('windowSizeLayoutBreakpointChange', callback)` 注册；回调只调用 `layoutModeForWidthBreakpoint()`；`aboutToDisappear()` 使用同一 callback 调用 `off` |
| 所有权与数据流 | HarmonyOS UIContext/observer -> `WidthBreakpoint` -> WindowLayoutPolicy -> Index.layoutMode；Index 是页面拓扑状态所有者，adaptive 仍为无 UI/RDP 依赖的纯策略；EntryAbility、Home、Settings、RDP 和 Native 均不产生第二份布局状态 |
| 非目标 | 不修改 Home/Settings 布局、不把 layoutMode 传入子组件、不改 XComponent、不监听像素宽高、不读 deviceInfo、不实现 capability、不修改 manifest/EntryAbility/Native |
| 兼容与回退 | 仅使用当前 compile/compatible SDK 22 已声明 API；注册或读取异常时保留 Compact 并记录日志；删除 Index 的状态、初始化和 on/off 即可回退，不影响连接与会话状态 |
| 验收 ID | AC-LAYOUT：初始值和 observer 事件都经过同一纯策略；AC-ARCH：on/off 使用同一回调、无 pixel/deviceType 第二来源、ArkTS 单测与模块编译通过；AC-XC：showSession 最外层分支和 XComponent 节点无改动 |
| 设计状态 | DesignReady |
| 实现状态 | Verified |
| 实际代码文件 | `harmony/app/entry/src/main/ets/pages/Index.ets` |
| 设计偏差及原因 | 无；layoutMode、初始化、observer on/off、异常回退和非目标均与 DesignReady 记录一致 |
| 测试命令/结果/证据 | 2026-08-04 执行 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\run_tablet_arkts_tests.ps1`，退出码 0、5/5 通过、模块 ArkTS 编译成功；静态检查结果：callback 符号恰好出现 3 次（定义/on/off），`getWindowWidthBreakpoint()` 恰好 1 次，无 `getWindowProperties`、`windowSizeChange`、`deviceInfo/deviceType` 第二来源；`git diff --unified=0` 证明 showSession/XComponent/Home/Settings 构建分支无改动。仍有一条既有 `SettingsPrimitives.ets` API 26 `fill` 兼容警告，与本项无关 |
| 关联提交 | 设计先行提交 `e26d8d1`；实现与本台账回写包含在同一后续提交（以 Git 历史为准） |

#### TAB-D-01：设置页 Compact/Expanded 拓扑隔离

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-01 |
| 设计版本/章节 | v1.2；第 5.1、6.2、10.2、10.5、12.3、14.2、14.3 节 |
| 目标 | 让同一个 SettingsPage、同一个 `pageName` 和同一组回调根据 Index.layoutMode 切换两种拓扑：Compact 概览单页进入子页，Expanded 保留左侧导航与右侧内容；切换断点不重建业务状态源 |
| 计划代码文件 | 修改 `harmony/app/entry/src/main/ets/pages/Index.ets`、`harmony/app/entry/src/main/ets/components/SettingsPage.ets`、`harmony/app/entry/src/main/ets/components/settings/SettingsPrimitives.ets`；复用 `adaptive/WindowLayoutPolicy.ets`，不新增页面副本 |
| 公共 API/状态 | SettingsPage 新增 `@Prop layoutMode: LayoutMode`；Index 传递现有唯一 layoutMode；SettingsPage 抽取一个共享内容 Builder。Expanded 渲染 desktop nav + 共享内容，Compact 在概览页渲染可关闭设置的顶栏 + 共享内容，子页继续使用现有 onBack 回到同一概览路由；既有 SettingsBackButton 的真实 Button 点击区域由 44×44vp 调整为 48×48vp，图标仍保持 18vp |
| 所有权与数据流 | UIContext -> WindowLayoutPolicy -> Index.layoutMode -> SettingsPage 展示分支；`pageName`、appearanceMode、XRDP 状态、权限状态和全部回调仍只存在一份；Compact/Expanded Builder 不拥有业务状态，不调用 Native |
| Compact 布局 | 不创建 196vp desktop nav；概览顶部使用真实点击区域 48×48vp 的 SettingsBackButton 作为返回/关闭入口，概览内容保持 Scroll；Basic/RemoteControl/ProjectHelp 子页沿用既有页面及其返回回调 |
| Expanded 布局 | 保留 196vp desktop nav 和右侧共享内容；不改变现有路由、回调、颜色及功能入口 |
| 非目标 | 本项不决定 tablet 是否显示 XRDP（D-01 仍待决）、不拆设置内部卡片、不修改 Basic/RemoteControl/ProjectHelp 和 BackButton 以外的 Primitives、不改字体/Icon 视觉尺寸或全局 token、不改 Home/Session/XComponent/Native/manifest |
| 兼容与回退 | 只依赖现有 LayoutMode；删除 layoutMode Prop、Index 传参和两个拓扑 Builder 后可恢复原 Row；BackButton 可独立恢复 44vp；断点切换只条件重排 SettingsPage 容器，不重置 `pageName` |
| 验收 ID | AC-LAYOUT：Compact 不创建 desktop nav，Expanded 创建且两者复用同一内容 Builder；AC-ARCH：只存在一个 pageName 和一组回调，无 TabletSettingsPage/DesktopSettingsPage；AC-XC：Index 的 showSession/XComponent 分支无改动；本机先完成编译与静态结构检查，600/839/840/1440vp 截图及往返状态证据留到真机验收后再升 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（本机结构与编译通过；等待真机矩阵后升 Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/ets/pages/Index.ets`、`harmony/app/entry/src/main/ets/components/SettingsPage.ets`、`harmony/app/entry/src/main/ets/components/settings/SettingsPrimitives.ets` |
| 设计偏差及原因 | 无；实现前发现 BackButton 仅 44vp 后已先通过文档提交补入 48vp 真实点击区域要求，再按更新后的 DesignReady 范围实现 |
| 测试命令/结果/证据 | 2026-08-04 两次执行 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\run_tablet_arkts_tests.ps1`，退出码均为 0、5/5 通过、模块 ArkTS 编译成功；静态结构检查：`@State pageName` 仅 1 处、desktop nav 调用仅在 Expanded Builder 1 处、共享内容调用 1 处、无 TabletSettingsPage/DesktopSettingsPage 文件；Index diff 仅新增 layoutMode 传参，showSession/XComponent 无改动；SettingsBackButton 的真实 Button 为 48×48vp，内部图标仍为 18vp。API 26 `fill` 警告已由 `TAB-D-03` 消除。API 26 2in1 真机 Expanded 概览、基础设置、远控设置、项目帮助四个路由均可达，截图和 layout dump 位于 `artifacts/tablet-acceptance/2026-08-04/`；但当前 `minWindowWidth=1280` 阻塞 Compact，且字体 1.75、839/840 往返及 tablet 真机仍待补，因此保持 Implemented |
| 关联提交 | 设计先行提交 `083f9db`，48vp 设计补充提交 `5f4266a`；实现与本台账回写包含在同一后续提交（以 Git 历史为准） |

#### TAB-D-02：首页 Compact/Expanded 外层拓扑与可达性

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-02 |
| 设计版本/章节 | v1.2；第 5.1、6.2、6.3、8.2、10.2、10.4、12.3、12.4、14.2、14.3 节 |
| 目标 | 在不复制表单/配置状态和业务回调的前提下，把首页固定桌面外壳改成真正的 Compact/Expanded 两种拓扑，并保证 600×480vp 下设备列表、连接详情、设置入口和四个状态入口均可到达 |
| 计划代码文件 | 修改 `harmony/app/entry/src/main/ets/pages/Index.ets`、`components/home/HomePage.ets`、`HomeHeader.ets`、`HomeDeviceList.ets`、`HomeConnectionDetails.ets`、`HomeStatusFooter.ets`；复用 `adaptive/WindowLayoutPolicy.ets` 和 48vp SettingsBackButton，不新增 Tablet/Desktop 页面副本 |
| 公共 API/状态 | Index 向 HomePage 传唯一 layoutMode；HomePage 向 Header/List/Details/Footer 传同一 layoutMode；HomePage 只新增展示路由 `compactPage = devices/details`，不复制 host/port/username/password/profile/服务状态。选中配置或新建配置后进入详情，详情顶部返回设备列表；Expanded 忽略 compactPage 并同时显示列表和详情 |
| Header | Expanded 保留居中标题、状态、设置；Compact 删除左右 260vp 占位，标题/设置与服务状态自然换成两行；设置入口真实点击高度由 38vp 提升到 48vp，19vp 图标不放大 |
| 主内容 | Expanded 保持 Row；Compact 使用单页 devices/details。列表宽度由上层拓扑决定，不再永久 31%；新建设备入口真实高度由 44vp 提升到 48vp。详情在 Compact 取消 `layoutWeight + 100% height`，由外层 Scroll 承载，确保小高度窗口字段可达 |
| Footer | Expanded 保持四列；Compact 使用 2×2 GridRow，卡片 52vp 且整体自然增高，不再固定 70vp 横向挤压；本项不按设备能力过滤卡片，等待 D-01 决策后的 TAB-E 子项 |
| 所有权与数据流 | WindowLayoutPolicy -> Index.layoutMode -> HomePage 展示拓扑 -> 子组件尺寸；所有连接值和回调仍由 Index 单向传入，Home 展示组件不 import libentry.so，不读取 deviceInfo，不产生第二断点 |
| 非目标 | 不处理 HomeConnectionDetails 内部 190vp 标签列和所有固定行高（后续内部 Grid/字体任务）；不决定 XRDP 隔离；不修改 Session/XComponent/Native/manifest；不声称字体 1.75 已完成 |
| 兼容与回退 | Expanded 保持当前结构和操作；切换 839/840 只改变展示节点，不修改 Index 表单或连接状态；删除 layoutMode/compactPage 和 Compact Builders 可恢复原布局；按钮高度和列表宽度改动可独立回退 |
| 验收 ID | AC-LAYOUT：Compact 无 260vp 占位/31% 列表/70vp footer，设备与详情有明确往返路径且详情可滚动；Expanded 仍同时显示两 pane；AC-FONT：设置/新建/返回点击区至少 48vp，视觉图标不缩放；AC-ARCH：业务状态仍只在 Index，无 TabletHomePage/DesktopHomePage；AC-XC：showSession/XComponent diff 为零。本机完成编译与静态结构检查后标 Implemented，真机 600/839/840/1440vp、字体与状态往返证据通过后升 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（本机结构与编译通过；等待真机矩阵及内部表单任务后升 Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/ets/pages/Index.ets`、`components/home/HomePage.ets`、`HomeHeader.ets`、`HomeDeviceList.ets`、`HomeConnectionDetails.ets`、`HomeStatusFooter.ets` |
| 设计偏差及原因 | 无；Compact/Expanded 外层拓扑、展示路由、真实点击区、详情 Scroll 和 2×2 Footer 均按 DesignReady 记录实现；190vp 标签列等明确非目标未在本项夹带修改 |
| 测试命令/结果/证据 | 2026-08-04 三次执行 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\run_tablet_arkts_tests.ps1`，均退出码 0、5/5 通过、模块 ArkTS 编译成功；完整 HAP 构建最初发现 `HomePage.ets` 缺少已使用的 `HomeText` import，补齐同一计划文件内的 import 后 `CompileArkTS`、`SignHap`、HNP 重签和真机安装通过。静态检查：`@State compactPage` 仅 1 处且为 devices/details 联合类型，Home 子组件断点监听 0 处、libentry/deviceInfo 依赖 0 处、TabletHomePage/DesktopHomePage 0 个；Index diff 仅新增 HomePage.layoutMode 传参，showSession/XComponent 无改动；Compact 分支使用 100% 列表、可滚动详情、48vp 设置/新建/返回和 2×2 GridRow，260vp/31%/70vp 仅保留在 Expanded 分支。API 26 2in1 真机 Expanded 首页、真实触控 Host 输入和系统输入面板通过，证据位于 `artifacts/tablet-acceptance/2026-08-04/`；但当前 `minWindowWidth=1280` 阻塞 Compact，且字体、839/840 往返和 tablet 真机仍待补，因此保持 Implemented |
| 关联提交 | 设计先行提交 `1daee02`；实现与本台账回写包含在同一后续提交（以 Git 历史为准） |

#### TAB-D-03：状态圆点 API 22 兼容

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-03 |
| 设计版本/章节 | v1.2；第 2.1、8.2、10.4、10.5、14.1、14.3 节 |
| 目标 | 消除当前 compile/compatible SDK 22 下由 Circle `fill` 引起的 API 26 兼容警告，同时保持状态圆点尺寸、颜色语义和布局不变 |
| 计划代码文件 | 修改 `components/settings/SettingsPrimitives.ets`、`components/home/HomeHeader.ets`、`components/home/HomeStatusFooter.ets` 中全部 3 处 Circle 状态圆点 |
| API/行为 | 将 Circle 的 API 26 `fill(color)` 替换为 API 22 已支持的通用 `backgroundColor(color)`；继续使用同一个 SettingsTheme 状态颜色函数；不改变 width/height、文字、回调或布局拓扑 |
| 非目标 | 不修改 SVG/Image 的 `fillColor`，不改 Icon 资源、不改主题色、不改其他组件、不引入 apiAvailable 分支、不改 Native/manifest |
| 兼容与回退 | 三处属性可逐行恢复；API 22 行为为同尺寸实心圆背景，API 26+ 无功能差异 |
| 验收 ID | AC-FONT：圆点视觉尺寸和颜色来源不变；AC-ARCH：ArkTS 模块编译中不再出现 `fill API is supported since SDK version 26`，全仓 ArkTS `.fill(` 计数为 0，现有 5 个策略测试通过 |
| 设计状态 | DesignReady |
| 实现状态 | Verified |
| 实际代码文件 | `components/settings/SettingsPrimitives.ets`、`components/home/HomeHeader.ets`、`components/home/HomeStatusFooter.ets` |
| 设计偏差及原因 | 无；仅替换 3 个 Circle 的着色属性，尺寸、颜色函数、布局和交互均未改变 |
| 测试命令/结果/证据 | 2026-08-04 执行 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\run_tablet_arkts_tests.ps1`，退出码 0、5/5 通过、模块 ArkTS 编译成功且警告输出为空；执行 `rg -n '\.fill\(' harmony/app/entry/src/main/ets -g '*.ets'` 无匹配，计数 0；`git diff --check` 通过 |
| 关联提交 | 设计先行提交 `db66966`；实现与本台账回写包含在同一后续提交（以 Git 历史为准） |

#### TAB-D-04：连接表单组件宽度重排与字体自然高度

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-04 |
| 设计版本/章节 | v1.2；第 6.3、8.1、8.2、10.4、12.3、12.4、14.3 节 |
| 目标 | 消除 `HomeConnectionDetails` 内部永久 190vp 标签列和 36～52vp 固定文本行高；表单根据组件自身可用宽度重排，使 Compact 页面以及较窄的 Expanded 详情 pane 都不会挤压、裁字或把操作移出可达区域 |
| 计划代码文件 | 仅修改 `harmony/app/entry/src/main/ets/components/home/HomeConnectionDetails.ets`；不修改 Index、HomePage、Settings、manifest、Session/XComponent 或 Native |
| 组件宽度策略 | 每个表单行使用 12 列 `GridRow/GridCol`，`breakpoints.reference = BreakpointsReference.ComponentSize`，唯一内部阈值为 600vp；xs 下标签与输入各占 12 列并上下排列，sm 下标签占 3 列、输入占 9 列。该内部断点只影响表单排版，不读取或修改全局 Compact/Expanded 状态 |
| 字体与热区 | Windows host、Port、Username、Password、记住密码、Connect 和设备操作行取消固定 `height`，改用 `minHeight + padding + 自然高度`；标题允许两行；三个设备操作按钮真实最小高度提升到 48vp，图标仍保持 20/24vp，不随字体同比放大 |
| 状态与行为 | host/port/username/password/rememberPassword、反馈文本和全部回调继续使用现有 Link/Prop；组件断点状态只用于标签对齐，不复制表单值，不触发连接、Native 或路由行为 |
| 非目标 | 不修改 Home 外层 Compact/Expanded 拓扑，不处理系统字体 configuration，不新增第三套页面，不修改文本内容/表单校验/剪贴板/连接行为，不开放 tablet/旋转/分屏，不改 RDP/XRDP |
| 兼容与回退 | GridRow、GridCol、BreakpointsReference.ComponentSize 均为当前 API 22 已支持能力；移除组件断点状态并恢复原 Row 即可回退，业务状态不受影响 |
| 验收 ID | AC-LAYOUT：静态结构证明内部使用 ComponentSize 而非第二个窗口监听，xs 为纵向、sm 为 3/9；AC-FONT：文本容器无 36/40/44/52 固定高度，设备操作按钮至少 48vp，图标视觉尺寸不变；AC-ARCH：仅改计划文件且无业务/Native 依赖变化；本机编译和策略测试通过后标 Implemented，600/839/840vp 与 1.75 字体真机矩阵后再升 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（本机编译、签名、安装及2in1 Expanded 回归通过；等待 Compact 与1.75字体矩阵后升 Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/ets/components/home/HomeConnectionDetails.ets` |
| 设计偏差及原因 | API 22 ArkTS 类型不提供直接 `.minHeight()` 修饰器，完整 HAP 首次编译据此失败；在不改变设计语义的前提下改用 API 22 支持的 `.constraintSize({ minHeight: 48 })`，仍是最小高度而非固定高度。其余组件断点、12列 span、状态边界和非目标无偏差 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_arkts_tests.ps1` 退出码 0，5/5 通过；首次完整 `assembleHap` 精确暴露8处 `.minHeight()` API 兼容错误，改用 `constraintSize` 后 `CompileArkTS`、`SignHap` 和完整构建通过；HNP 重签产物 41788752 字节，设备 `3QC0124C11000711` 覆盖安装成功。静态检查：`ComponentSize` 2处、内部阈值仅 `600vp`，190/170vp 标签宽与 36/40/44/52 固定高度均为0，文件无 window observer/deviceInfo/Native 新依赖。2in1 Expanded 截图 `artifacts/tablet-acceptance/2026-08-04/muhub-d04-expanded.jpeg` 无回退；Compact/1.75 字体仍被现有 manifest 最小窗口和设备条件阻塞 |
| 关联提交 | 实现与本台账回写包含在同一提交（以 Git 历史为准） |

#### TAB-D-05：设置页触控热区与文本自然高度

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-05 |
| 设计版本/章节 | v1.2；第 6.3、8.1、8.2、10.5、12.4、14.3 节 |
| 目标 | 清理设置页中低于48vp的真实点击区域，并把状态/键值文本行从固定高度改为 API 22 可用的最小高度约束，使图标视觉尺寸不变而触控、字体放大和自然换行更稳健 |
| 计划代码文件 | 修改 `components/settings/SettingsPrimitives.ets`、`components/settings/BasicSettingsPage.ets`、`components/settings/RemoteControlCards.ets`；不修改 SettingsPage 路由、Index、manifest、Native 或 XRDP 行为 |
| 点击区域 | `SettingsDesktopNavItem` 从固定46vp改为最小48vp；本机网络刷新按钮从34vp改为最小48vp；XrdpServer、RemoteFiles、RemoteAccess、ScreenRecording 四个操作按钮从36vp改为最小48vp。统一使用 `.constraintSize({ minHeight: 48 })`，不把图标放大到48vp |
| 文本自然高度 | `SettingsStatusChip` 的28vp和 `SettingsKeyValueRow` 的32vp固定高度改为对应最小高度约束并增加上下 padding；键值文本允许最多两行，容器可随系统字体增长，不通过缩小 fp 保持一行 |
| 状态与行为 | 所有按钮 enabled/disabled、onClick、hover/pressed、Toggle、刷新/授权/目录/验证码回调保持原样；本项只修改布局约束，不改变能力、权限或服务状态 |
| 非目标 | 不重排远控卡片为另一套 Compact 拓扑，不移动 RemoteFilesCard，不处理 D-01 能力隔离，不改颜色、文案、图标资源或 Icon 视觉尺寸，不启用系统 font configuration |
| 兼容与回退 | API 22 不支持直接 `.minHeight()`，统一使用已验证可编译的 `constraintSize`；每处可独立恢复原固定高度，不影响业务状态 |
| 验收 ID | AC-FONT：6处代码约束覆盖桌面导航、本机网络刷新和4个远控操作按钮，实测点击高度至少48vp，原34/36/46固定高度计数为0；状态/键值行不再固定高度且可两行；Icon 的17/18及 token 尺寸不变。AC-ARCH：仅改计划文件，无回调/能力/Native 依赖变化；本机测试和完整 HAP 通过后标 Implemented，1.75字体真机后升 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（API 26 2in1 的 Expanded/约756×647vp Compact、构建签名安装通过；等待1.75字体矩阵后升 Verified） |
| 实际代码文件 | `components/settings/SettingsPrimitives.ets`、`components/settings/BasicSettingsPage.ets`、`components/settings/RemoteControlCards.ets` |
| 设计偏差及原因 | 无；6处固定点击高度改为 `constraintSize({ minHeight: 48 })`，状态/键值行按计划改为最小高度与自然换行，回调、状态和图标视觉尺寸未改 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_arkts_tests.ps1` 退出码0、5/5通过；完整 `assembleHap` 的 `CompileArkTS`、`SignHap` 通过；HNP重签产物41788509字节并覆盖安装成功。密度1.9真机 dump 中，Expanded桌面导航为91～92px，Compact本机网络刷新及4个远控按钮的原始边界高度均为91px（取整对应48vp）；约756×647vp下基础设置和远控设置可滚动到全部操作。证据：`muhub-layout-d05-settings-expanded.json`、`muhub-layout-d05-basic-compact.json`、`muhub-layout-d05-remote-compact.json`及对应截图 |
| 关联提交 | 实现与本台账回写包含在同一提交（以 Git 历史为准） |

#### TAB-D-06：首页 Expanded 矮窗表单可滚动

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-06 |
| 设计版本/章节 | v1.3；第 6.2、8.1、10.4、12.3、12.4 节 |
| 触发证据 | 2026-08-04 MatePad Pro多窗口约855×420vp时，宽度断点为Expanded但可用高度不足；首页右侧Password以下被Footer遮挡/截断，Connect不可直接到达 |
| 目标 | WidthBreakpoint仍只决定Compact/Expanded拓扑；Expanded右侧详情pane增加独立纵向Scroll，连接详情组件改为自然高度，使矮窗和字体放大时所有字段、Connect和设备操作可滚动到达 |
| 计划代码文件 | 修改`components/home/HomePage.ets`和`HomeConnectionDetails.ets`；不新增高度断点、不复制表单、不修改Index状态/回调/XComponent/Native |
| 所有权 | HomePage拥有pane尺寸和滚动容器；HomeConnectionDetails只拥有表单自然内容高度和100%组件宽度，内部600vp ComponentSize断点保持不变 |
| 兼容与回退 | Expanded高窗视觉宽度和Row拓扑不变，只在内容超高时出现滚动；Compact继续复用同一详情组件和已有Scroll。删除Expanded Scroll并恢复100%强制高度可回退，但会恢复矮窗不可达问题 |
| 验收 ID | AC-LAYOUT：约855×420vp和600×480vp下Connect/设备操作可滚动到达；AC-FONT：自然高度兼容1.75字体；AC-ARCH：无第二套表单/高度断点/业务状态；AC-XC：showSession分支diff为0 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（纯策略测试、完整构建、MatePad Pro约855×420vp真机滚动可达通过；600×480和1.75字体矩阵待补） |
| 实际代码文件 | `components/home/HomePage.ets`、`components/home/HomeConnectionDetails.ets` |
| 设计偏差及原因 | 无；未增加高度断点，Expanded和Compact继续复用同一连接详情组件；只把pane高度约束和滚动所有权留在HomePage |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_arkts_tests.ps1`退出码0，9项通过；`harmony/app/build_hap.bat`的CompileArkTS、PackingCheck、SignHap通过，HAP34457637字节；MatePad Pro PCE-W30覆盖安装后在约855×420vp多窗口，从右侧详情向上滑动可显示完整Password、记住密码和Connect，Footer仍可见，设备列表未被联动滚走 |
| 关联提交 | 实现与本台账回写包含在同一提交（以Git历史为准） |

#### TAB-D-07：首页设备列表字体自然高度

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-07 |
| 设计版本/章节 | v1.3；第 6.2、8.1、8.2、10.4、12.3、12.4 节 |
| 触发证据 | `HomeDeviceList.ets` 的新建设备按钮固定136×48vp、设备标签和标签Row固定18vp、设备卡固定72vp、空状态固定90vp且两行文字都限制1行；系统字体放大到1.75时存在文字裁切或横向挤压风险 |
| 目标 | 保持Icon视觉vp尺寸和现有布局拓扑，把承载文本的固定宽高改成最小约束与自然增长；可点击设备卡和新建设备操作至少48vp，Compact/Expanded继续复用同一列表 |
| 计划代码文件 | 仅修改`harmony/app/entry/src/main/ets/components/home/HomeDeviceList.ets`；不修改Index/HomePage/Settings/Session/XComponent/Native/manifest、业务回调或连接数据 |
| 尺寸策略 | 新建设备按钮去掉固定宽度，使用水平padding与`minWidth:136/minHeight:48`；18vp标签及标签容器改`minHeight:18`；设备卡改`minHeight:72`并允许内容自然增高；空状态改`minHeight:90`、增加内边距并允许标题/正文最多2行。20/22/36/18vp Icon视觉尺寸保持不变 |
| 兼容与回退 | 标准字体下最小尺寸等于原视觉基线，只有文本需要更多空间时增长；Scroll继续吸收纵向增长。恢复固定width/height即可回退，但会恢复字体裁切风险 |
| 验收 ID | AC-FONT：目标文本容器固定高度计数归零，Icon尺寸不变，1.75字体下无裁字；AC-LAYOUT：Compact/Expanded列表可滚动且新建、搜索、选择均可达；AC-ARCH：仅改展示约束，无业务状态/回调/Native依赖变化。本机测试和完整HAP通过后标Implemented，系统1.75真机矩阵后升Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（标准字体tablet构建、安装和首页回归通过；系统1.75字体及存在已保存设备时的卡片增长矩阵待补后升Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/ets/components/home/HomeDeviceList.ets` |
| 设计偏差及原因 | 无；只替换文本容器的固定尺寸，Icon的20/22/36/18vp视觉尺寸、搜索输入48vp最小可用高度、布局拓扑、状态和回调均未改变 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_arkts_tests.ps1`退出码0，9项策略测试通过；完整`harmony/app/build_hap.bat`的CompileArkTS、PackingCheck、SignHap通过，HAP34470550字节；明确指定MatePad Pro `5JB0223804000371`覆盖安装、启动成功，标准字体首页截图`%TEMP%/muhub-d07-home.jpeg`显示新建设备、搜索和空状态无回退。静态diff确认136×48按钮、18标签/Row、72设备卡和90空状态的固定文本容器已改为min约束，Icon尺寸未改。设备当前无保存项且系统字体未切到1.75，因此不伪造对应证据 |
| 关联提交 | 实现与本台账回写包含在同一提交（以Git历史为准） |

#### TAB-D-08：首页连接详情移除复制地址操作

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-08 |
| 设计版本/章节 | v1.3；第 6.2、8.1、10.4、12.3 节 |
| 目标 | 从首页连接详情的设备操作区移除“复制地址”按钮及其专用剪贴板调用，只保留“删除”和“清除密码”；Host、Port 展示编辑、连接流程及 RDP 剪贴板同步能力保持不变 |
| 计划代码文件 | 修改 `components/home/HomeConnectionDetails.ets`、`HomeText.ets`、`HomeResources.ets`；不修改 Pasteboard 权限协调器、RDP cliprdr、连接状态、Native 或 FreeRDP |
| 布局策略 | 继续使用现有 12 列 ComponentSize Grid；sm 下标签、删除、清除密码各占 3 列，保留末尾空白，不扩大按钮或引入条件占位；xs 下各项仍按 12 列纵向排列 |
| 兼容与回退 | 删除仅由该按钮使用的 ArkTS pasteboard import、地址拼接方法、文案和图标常量；媒体资源文件暂保留，不影响其他能力。恢复上述入口即可回退 |
| 验收 ID | AC-LAYOUT：连接详情设备操作区不再构建“复制地址”，删除和清除密码仍可达；AC-ARCH：全仓业务代码无 `copyAddress`/`COPY_ADDRESS_ACTION` 引用，RDP Pasteboard/cliprdr diff 为 0；ArkTS 策略测试及 Debug HAP 构建通过后标 Implemented |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（按钮及专用 ArkTS 剪贴板逻辑已移除，静态检查、ArkTS 测试和 Debug HAP 构建通过；未做真机视觉复核，因此不标 Verified） |
| 实际代码文件 | `components/home/HomeConnectionDetails.ets`、`components/home/HomeText.ets`、`components/home/HomeResources.ets` |
| 设计偏差及原因 | 无；保留了未再引用的 `home_copy` 媒体文件，避免把本次交互删除扩大为资源文件删除，业务和 RDP 剪贴板链路未改 |
| 测试命令/结果/证据 | 2026-08-05：静态 `rg` 确认首页组件无 `copyAddress`、`COPY_ADDRESS_ACTION`、`COPY_ICON`、专用 pasteboard import 或地址拼接方法；`git diff --check` 通过；`tools/run_tablet_arkts_tests.ps1` 退出码 0；`harmony/app/build_hap.bat debug` 完整 Native/ArkTS/打包/签名成功，signed HAP 35,532,987 bytes。首次构建调用因外层 1 秒超时产生 EPIPE，随后以正常时限重跑成功，不计为编译失败 |
| 关联提交 | 实现与本台账回写包含在同一工作区变更（未创建提交） |

#### TAB-D-09：首页设备操作双按钮对齐优化

| 字段 | 内容 |
|---|---|
| Change ID | TAB-D-09 |
| 设计版本/章节 | v1.3；第 6.2、8.1、10.4、12.3、12.4 节 |
| 触发证据 | 2026-08-05 MatePad Pro 约 1680×1318 窗口截图 `artifacts/design-audit/2026-08-05-copy-address-layout/03-recheck.png`：外层 12 列中标签、删除、清除密码各占 3 列，末尾遗留 3 列空白，按钮组缩在表单中部且与上方输入框右边界不齐；较窄窗口截图 `02-actions-before.png` 中两个按钮纵向堆叠，占用过多高度 |
| 目标 | 设备操作标签继续遵循表单 3/9 对齐；剩余 9 列作为统一操作组，内部将删除和清除密码等分并排，使按钮组与输入框左右边界一致，消除无意义空白和窄窗纵向堆叠 |
| 计划代码文件 | 仅修改 `harmony/app/entry/src/main/ets/components/home/HomeConnectionDetails.ets`；不改按钮文案、颜色、图标、回调、连接表单、Native 或 RDP 能力 |
| 响应式策略 | 外层保持 `xs:12/sm:3` 标签和 `xs:12/sm:9` 操作组；操作组内部固定 12 列、两个按钮各 6 列。xs 下标签独占一行、两个按钮在下一行并排；sm 下标签与操作组同排，操作组边界与 Host/Port/Username/Password 输入框一致 |
| 交互与无障碍 | 两按钮继续使用既有 `buildActionButton`，真实点击高度至少 48vp，危险/警告色、图标和 hover/press 状态不变；只改变布局容器，不改变操作确认语义 |
| 验收 ID | AC-LAYOUT：宽窗按钮组与输入框左右边界一致且无末尾 3 列空白；窄窗两个按钮同排、文字不裁切、操作区高度不超过单个按钮行；AC-FONT：1.75 字体下允许按钮自然增高但不重叠；AC-ARCH：仅布局容器 diff，回调/Native/RDP diff 为 0。ArkTS 测试、Debug HAP 构建及同设备前后截图通过后标 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（标准字体下宽窗与窄窗真机布局、测试及完整构建通过；1.75 字体矩阵待补后升 Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/ets/components/home/HomeConnectionDetails.ets` |
| 设计偏差及原因 | 无；外层 3/9 表单栅格和操作组内部 6/6 等分均按 DesignReady 记录实现，按钮文案、色调、图标和回调未改变 |
| 测试命令/结果/证据 | 2026-08-05：`tools/run_tablet_arkts_tests.ps1` 退出码 0；`harmony/app/build_hap.bat debug` 完整 Native/ArkTS/打包/签名成功，signed HAP 35,592,072 bytes；HAP 已覆盖安装并启动到 MatePad Pro `5JB0223804000371`。窄窗截图 `artifacts/design-audit/2026-08-05-copy-address-layout/06-actions-after.png` 显示标签换行后两按钮等分并排；最大化截图 `07-maximized-after.png` 及 UI tree 显示操作组与表单输入边界一致，删除/清除密码按钮宽度分别 651/651px、高 82px，无末尾空白；恢复窗口截图 `08-restored-after.png` 证明窗口模式切换无布局崩溃。1.75 字体尚未执行，因此不升 Verified |
| 关联提交 | 实现与本台账回写包含在同一工作区变更（未创建提交） |

#### TAB-C-01：适配候选解除应用声明的最小窗口限制

| 字段 | 内容 |
|---|---|
| Change ID | TAB-C-01 |
| 设计版本/章节 | v1.2；第 6.1、10.1、11、12.3 节 |
| 目标 | 删除 `EntryAbility` 的 `minWindowWidth`、`minWindowHeight`，让窗口管理器按系统可达范围缩放，从而在当前 2in1 真机进入 Compact 并暴露真实的小窗布局问题 |
| 计划代码文件 | 仅修改 `harmony/app/entry/src/main/module.json5`；保留同文件中现有且与本项无关的打印扩展改动，不修改包名、product、module、deviceTypes、orientation、supportWindowMode 或业务代码 |
| 配置语义 | 本项是未发布适配候选，不再由应用声明 1280×760vp 下限；实际最小尺寸由当前 HarmonyOS 设备和窗口管理器决定，不把“字段删除”误写成已保证 600×480vp |
| 依赖偏差 | 父计划原定 TAB-B 会话 resize/fallback 验证后再开放小窗；按 2026-08-04 用户要求先解除限制以推进 UI 真机验收。未完成 TAB-B 前，远程会话小窗、XComponent 几何和输入映射仍是发布阻塞项，不因本次 Home/Settings 通过而放行发布 |
| 状态与行为 | 只改变可达窗口范围；Compact/Expanded 仍由唯一 WidthBreakpoint 决定，首页/设置共享原状态与回调，连接、XRDP、打印和签名行为不变 |
| 回退 | 恢复 `minWindowWidth: 1280`、`minWindowHeight: 760` 即可回退；如设备允许过小窗口导致不可达，发布前可按完整矩阵证据改为设计目标 600×480vp，而不是无证据抬高限制 |
| 验收 ID | AC-PKG：仍为同一 `default/entry` HAP 和 `com.muhub.desktop`；AC-LAYOUT：真机可跨过 840vp 进入 Compact，Home/Settings 可达且无固定桌面侧栏；AC-XC/AC-INPUT：仅记录阻塞，不在本项宣称小窗远程会话已通过 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（2in1真机已进入Compact；精确600/839/840vp、tablet和小窗远程会话仍待完整矩阵） |
| 实际代码文件 | `harmony/app/entry/src/main/module.json5`（仅删除 `EntryAbility.minWindowWidth/minWindowHeight`） |
| 设计偏差及原因 | 无实现偏差；依赖顺序偏差已在设计阶段明确记录。实际最小窗口由系统管理，未宣称字段删除等于固定600×480vp |
| 测试命令/结果/证据 | 2026-08-04：完整 `assembleHap` 的 `CompileArkTS`、`SignHap` 通过；HNP重签产物41788522字节，`hdc install -r`成功；包名仍为`com.muhub.desktop`。HAD-W32密度1.9真机窗口缩至1437×1229px（约756×647vp），Home切为单页设备列表，Settings概览无桌面侧栏，基础/远控子页可达。证据：`muhub-c01-home-compact.jpeg`、`muhub-c01-settings-compact.jpeg`及对应layout dump。XComponent会话、600vp下限和tablet未在本项验证 |
| 关联提交 | 实现与本台账回写包含在同一提交（以 Git 历史为准） |

#### TAB-C-02：同一 HAP 增加 tablet 安装声明

| 字段 | 内容 |
|---|---|
| Change ID | TAB-C-02 |
| 设计版本/章节 | v1.2；第 3、6.1、8.1、10.1、12.2、12.5 节 |
| 目标 | 在不增加 product、module、HAP 或 bundleName 的前提下，把 entry 模块 `deviceTypes` 从仅 `2in1` 改为 `2in1 + tablet`，使同一签名 HAP 具备在两类设备安装的清单声明 |
| 计划代码文件 | 仅修改 `harmony/app/entry/src/main/module.json5` 的 `deviceTypes`；保留同文件中现有且与本项无关的打印扩展改动和已删除的最小窗口限制 |
| 包与身份 | 继续使用 `default/entry`、`com.muhub.desktop` 和单 HAP；不建立 tablet product、tablet module、tablet bundleName 或条件分包 |
| 能力与 UI 边界 | 本项只开放安装声明，不把设备类型判断写进 Home/Settings 布局。Compact/Expanded 仍只由 WidthBreakpoint 决定；tablet 的 XRDP 能力过滤仍属于 D-01/TAB-E，未完成前不得宣称功能隔离已 Verified |
| 非目标 | 本项不同时启用 `auto_rotation`、`split` 或字体 configuration，不修改 Native/XRDP/XComponent/输入/IME，不伪造无 tablet 真机的安装与旋转证据 |
| 兼容与回退 | 删除 `tablet` 字符串即可恢复仅2in1声明；构建清单必须同时包含且仅包含 `2in1`、`tablet`，Hvigor清单校验、签名和现有2in1覆盖安装均需通过 |
| 验收 ID | AC-PKG：同一HAP、同一包名且清单包含两类设备；AC-CAP：2in1现有启动/Compact回归不退化，tablet真机安装与能力隔离保留为明确待测；AC-ARCH：无新增product/module/bundle和业务设备判断 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（清单、签名产物和2in1安装回归通过；等待tablet真机安装与能力隔离后升 Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/module.json5`（`deviceTypes`增加`tablet`） |
| 设计偏差及原因 | 无；仍是一个default product、一个entry module、一个bundleName和一个HAP，未夹带旋转、分屏或业务设备判断 |
| 测试命令/结果/证据 | 2026-08-04：完整`assembleHap`的`ProcessProfile`、`PackingCheck`、`CompileArkTS`、`SignHap`通过；`intermediates/hap_metadata/default/output_metadata.json`及打包`module.json`均为`["2in1","tablet"]`。HNP重签产物41788502字节并在HAD-W32覆盖安装、启动成功；设备端`bm dump -n com.muhub.desktop`显示entry/ability的`deviceTypes`同时包含`2in1`和`tablet`。无tablet真机，因此未执行tablet安装、旋转和能力隔离验收 |
| 关联提交 | 实现与本台账回写包含在同一提交（以 Git 历史为准） |

#### TAB-C-03：同一应用启用系统字体缩放上限

| 字段 | 内容 |
|---|---|
| Change ID | TAB-C-03 |
| 设计版本/章节 | v1.2；第 8.1、10.1、10.5、12.4 节 |
| 目标 | 使用 AppScope configuration profile 让 ArkUI fp 字体跟随系统设置，并把应用最大字体比例限制为1.75；不以缩小字体或全局几何缩放掩盖布局问题 |
| 计划代码文件 | 修改 `harmony/app/AppScope/app.json5`，新增 `harmony/app/AppScope/resources/base/profile/configuration.json`；不修改module清单、页面业务、Native、XComponent或远端DPI |
| 配置 | `app.configuration="$profile:configuration"`；profile内`fontSizeScale="followSystem"`、`fontSizeMaxScale="1.75"`。两项均使用本机API 22 modulecheck schema声明的字符串枚举值 |
| 缩放边界 | 只影响本地ArkUI fp；Icon继续使用既有vp视觉尺寸和独立48vp热区，XComponent Surface继续使用物理px，远端Windows DPI不读取该字体比例 |
| 兼容与回退 | 删除app引用和profile即可恢复系统默认行为；构建必须通过profile/resource/schema检查。未在系统设置实际切到1.75并截图前只标Implemented，不标Verified |
| 验收 ID | AC-FONT：打包配置为followSystem/1.75，默认字体下Home/Settings无回退，后续补1.75中英文/Compact/Expanded矩阵；AC-PKG：bundleName、deviceTypes、单HAP不变；AC-ARCH：无Native/XComponent/DPI依赖 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（profile/schema、标准字体平板首屏和单HAP安装已通过；系统1.75字体矩阵未执行，因此不标Verified） |
| 实际代码文件 | `harmony/app/AppScope/app.json5`、`harmony/app/AppScope/resources/base/profile/configuration.json` |
| 设计偏差及原因 | 无；安装验证同时发现并按TAB-C-04记录HNP产品门禁，字体配置本身未扩大到Native/XComponent/DPI |
| 测试命令/结果/证据 | 2026-08-04：API22本机modulecheck schema确认`followSystem`/`1.75`为合法枚举；`harmony/app/build_hap.bat`的ProcessProfile、CompileResource、CompileArkTS、PackingCheck、SignHap通过；MatePad Pro PCE-W30标准字体横屏首屏启动且无裁切。尚未把系统字体切换到1.75，保留Implemented |
| 关联提交 | 实现与本台账回写包含在同一提交（以Git历史为准） |

#### TAB-C-04：单 HAP 移除产品级 HNP 安装依赖

| 字段 | 内容 |
|---|---|
| Change ID | TAB-C-04 |
| 设计版本/章节 | v1.3；第 2.3、7、10.1、10.5、12.2 节 |
| 触发证据 | 2026-08-04 tablet 真机 `5JB0223804000371` 的 `const.product.devicetype=tablet`；带 HNP 的 HAP 安装返回9568407，hilog中`NativeInstallHnp ret: 8201`；标准HAP仍保留`hnpPackages`时返回9568409。OpenHarmony HNP API源码将8201定义为`HNP_API_ERRNO_HNP_INSTALL_DISABLED` |
| 目标 | 保持一个default product、一个entry module、同一bundleName和一个canonical HAP；发布构建不再声明或二次封装HNP，使tablet先具备可安装/可启动基线 |
| 计划代码文件 | 修改`harmony/app/entry/src/main/module.json5`删除`hnpPackages`；修改`harmony/app/build_hap.bat`停止默认调用HNP打包和二次封包。保留HNP工具脚本作为迁移期诊断工具，但不进入canonical构建 |
| XRDP兼容边界 | 根目录`libs/arm64-v8a`已有`libxrdpserver.so`、`libxrdpohos.so`及依赖，可继续作为进程内加载候选；config/share迁移到标准HAP资源和2in1被控服务回归另设后续子项。在此之前tablet按D-01推荐策略隔离被控服务，2in1被控服务不得宣称Verified |
| 非目标 | 本项不修改FreeRDP/xrdp社区代码、不修改XComponent/RDP客户端、不伪造2in1被控服务回归、不新增第二个product/module/HAP |
| 兼容与回退 | 清单删除HNP后tablet可走标准HAP安装；如需临时诊断旧HNP可手工调用现有脚本，但该产物不是单包发布候选。回退HNP声明会重新导致不支持HNP的tablet安装失败，不能作为发布回退 |
| 验收 ID | AC-PKG：标准HAP中无`hnp/`条目且打包module无`hnpPackages`，tablet安装/启动成功，bundleName/deviceTypes不变；AC-ARCH：仍为单product/module且构建入口不自动repack；AC-CAP：tablet XRDP四层隔离留给TAB-E，2in1进程内XRDP资源迁移留给后续子项 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（tablet单HAP安装启动已通过；2in1进程内XRDP资源迁移/回归和tablet功能隔离未完成，因此不标Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/module.json5`、`harmony/app/build_hap.bat` |
| 设计偏差及原因 | 无；保留既有HNP工具脚本供诊断，但canonical构建入口不再调用。真机首屏仍显示XRDP状态卡，明确证明TAB-E功能/UI隔离尚未完成 |
| 测试命令/结果/证据 | 2026-08-04：MatePad Pro PCE-W30（tablet、OpenHarmony-6.0.2.130）上，旧HNP HAP返回9568407/NativeInstallHnp 8201，缺HNP但有声明返回9568409；删除声明并停止repack后完整构建成功，canonical HAP 34441489字节、`hnp/`条目0，`hdc -t 5JB0223804000371 install -r`和`aa start`成功；包名`com.muhub.desktop`、entry、`[2in1,tablet]`不变 |
| 关联提交 | 实现与本台账回写包含在同一提交（以Git历史为准） |

#### TAB-E-01：tablet XRDP 初始化、服务、路由与展示四层隔离

| 字段 | 内容 |
|---|---|
| Change ID | TAB-E-01 |
| 设计版本/章节 | v1.3；第 5.1、7、10.2、10.3、10.4、12.5、14.2 节；采用D-01推荐值 |
| 决策 | 单HAP需在不支持HNP的MatePad Pro安装，tablet首版仅提供RDP客户端；2in1保留XRDP能力入口。布局仍只按WidthBreakpoint，设备类型只进入能力策略 |
| 计划代码文件 | 新增`harmony/app/entry/src/main/ets/capability/DeviceCapabilityPolicy.ets`及其纯策略测试；修改`pages/Index.ets`、`rdp/XrdpServerController.ets`、`components/home/HomePage.ets`、`HomeHeader.ets`、`HomeStatusFooter.ets`、`components/SettingsPage.ets`、`settings/SettingsConstants.ets`和测试入口 |
| 能力数据 | 唯一策略输入读取`deviceInfo.deviceType`并生成不可变语义快照：`2in1 -> remoteControlServer=available`，`tablet/unknown/空值 -> unavailable`。Home/Settings只接收布尔展示能力，不自行读取设备类型 |
| 初始化/服务隔离 | tablet的Index冷启动和onPageShow不检查录屏、不读取XRDP diagnostics、不启动服务；所有设置回调先做能力保护。XrdpServerController同时接收能力并在unavailable时直接返回Unavailable状态，不调用`libentry.so`，形成第二道保护 |
| 路由/UI隔离 | tablet非法或旧`remoteControl`路由回退设置概览；Home Header不显示被控状态，Footer只保留客户端共享目录卡；Settings不创建远控导航、XRDP状态卡和远控页，改为可直接打开共享目录的客户端入口 |
| 保留能力 | Windows RDP连接、XComponent、输入/IME、剪贴板/音频/摄像头/位置通道和`RemoteFilesDirectory`不受XRDP能力策略拦截 |
| 非目标 | 不在本项迁移2in1进程内XRDP config/share，不启用rotation/split，不修改Native/FreeRDP/xrdp，不复制Tablet页面，不把WidthBreakpoint和deviceType混用 |
| 兼容与回退 | 2in1快照保持available并走原回调；策略异常或未知设备按最小能力unavailable。删除能力传参可恢复旧UI，但会破坏tablet安装后的安全隔离，不能作为发布回退 |
| 验收 ID | AC-CAP：四类纯策略测试；tablet冷启动hilog无XRDP start/diagnostics/录屏请求，首页和设置截图无XRDP UI但共享目录可达；2in1原行为待设备重新在线回归。AC-ARCH：deviceInfo只在policy文件，Home/Settings无Native导入；AC-PKG：同HAP/包名不变；AC-XC：会话分支无改动 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（tablet四层隔离、纯策略测试、完整构建与真机UI/日志验收通过；2in1回归待设备重新在线，因此不标Verified） |
| 实际代码文件 | `capability/DeviceCapabilityPolicy.ets`、`pages/Index.ets`、`rdp/XrdpServerController.ets`、`components/home/HomePage.ets`、`HomeHeader.ets`、`HomeStatusFooter.ets`、`components/SettingsPage.ets`、`components/settings/SettingsConstants.ets`、`src/test/DeviceCapabilityPolicy.test.ets`、`src/test/List.test.ets` |
| 设计偏差及原因 | 无功能偏差；真机当前处于约855×420vp多窗口，额外发现首页Expanded表单在矮窗被底部截断，作为布局后续项处理，不在本项用能力UI改动掩盖 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_arkts_tests.ps1`退出码0，原5项断点测试加4项能力测试全部通过；`harmony/app/build_hap.bat`的CompileArkTS、PackingCheck、SignHap通过，HAP34457850字节；MatePad Pro PCE-W30覆盖安装/冷启动成功，启动前`hilog -r`后过滤`RdpBridge|xrdp|screen recording`为0行；真机首页仅保留共享目录卡，Header无被控状态，Settings无远控导航/XRDP状态卡且共享目录入口仍可达。静态检查deviceInfo仅在policy文件，components下libentry导入为0 |
| 关联提交 | 实现与本台账回写包含在同一提交（以Git历史为准） |

#### TAB-A-03：RDP 会话 UI 与首页协调器隔离

| 字段 | 内容 |
|---|---|
| Change ID | TAB-A-03 |
| 设计版本/章节 | v1.3；第 4、5.2、9.1、10.2、10.6、12.6、14.2 节 |
| 目标 | 把现有 RDP 会话的 `Stack + XComponent + 状态提示层` 从 `Index` 抽取为唯一的 `RdpSessionPage`；`Index` 继续唯一创建并持有 `XComponentController`、连接状态和焦点回调，布局组件不拥有或重建会话 |
| 计划代码文件 | 新增 `harmony/app/entry/src/main/ets/components/session/RdpSessionPage.ets`；修改 `harmony/app/entry/src/main/ets/pages/Index.ets`；不修改 Native、FreeRDP/xrdp、manifest、Home、Settings 或输入协议 |
| 公共 API/状态 | `RdpSessionPage` 接收已有 `XComponentController`、提示标题/副标题、等待状态和 `onSurfaceLoad` 回调；组件内部只构建一个固定 `XComponent` 节点和展示提示层，不创建业务状态、Controller、Native import、断点或设备类型判断 |
| 所有权与数据流 | `Index.surfaceController` -> `RdpSessionPage.surfaceController` -> XComponent；Native 连接回调 -> Index 的 notice 状态 -> `@Prop` 提示字段；XComponent `onLoad` -> 页面回调 -> Index 现有延迟聚焦逻辑。`showSession` 仍是 Index.build 的最外层分支 |
| 行为不变量 | XComponent 的 id、libraryname、SURFACE 类型、100% 尺寸、focusable/focusOnTouch/defaultFocus、黑色背景和 `RenderFit.CENTER` 保持不变；提示层颜色、边框、尺寸和文本截断保持不变；抽取不启用 IME、rotation、split 或新的 resize 请求 |
| 兼容与回退 | 仅为 ArkUI 组件所有权整理，无 Native ABI/包配置变化；把组件 Builder 原样移回 Index 并删除新文件即可回退。若 ArkTS 编译、静态唯一性检查或真机连接首屏任一失败，不继续开放 rotation/split |
| 验收 ID | AC-ARCH：Index 不再直接构建 XComponent，session 组件无 Native/能力/断点依赖；AC-XC：ArkTS 全局仅一个 XComponent 构造、仅一个 `new XComponentController`，`showSession` 仍为最外层分支，既有 XComponent 属性逐项保持；本机单测和完整 HAP 构建通过后标 Implemented，实际 RDP 连接时 Controller 身份、Surface 和提示层回归通过后升 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（组件隔离、静态唯一性、完整构建及 tablet 安装启动通过；实际 RDP 连接的 Controller/SURFACE/提示层回归待可用服务端凭据后升 Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/ets/components/session/RdpSessionPage.ets`、`harmony/app/entry/src/main/ets/pages/Index.ets` |
| 设计偏差及原因 | 无行为偏差。完整构建首次发现普通必传字段不满足 ArkTS 严格初始化；未在子组件创建备用 Controller，而是使用可空初始化并由 `requireSurfaceController()` 在构建 XComponent 前 fail-fast，父级仍是唯一实际 Controller 创建和传入点 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_arkts_tests.ps1`退出码0，9项策略测试与模块编译通过；静态检查确认全 ArkTS 仅 `RdpSessionPage.ets` 1个XComponent构造、仅`Index.ets` 1个`new XComponentController`，session目录无`libentry.so`/deviceInfo/WidthBreakpoint/LayoutMode依赖；`harmony/app/build_hap.bat`的CompileArkTS、PackingCheck、SignHap通过，HAP 34471202字节；明确指定MatePad Pro `5JB0223804000371`覆盖安装和EntryAbility启动成功，首屏截图`%TEMP%/muhub-a03-home.jpeg`无首页布局回退。无可用RDP测试凭据，本项不伪造实际Surface连接证据 |
| 关联提交 | 实现与本台账回写包含在同一提交（以Git历史为准） |

#### TAB-A-04：RDP 会话关键路径结构化诊断

| 字段 | 内容 |
|---|---|
| Change ID | TAB-A-04 |
| 设计版本/章节 | v1.3；第 10.8、12.6、13、14.3 节 |
| 目标 | 用低频、可关联且不含凭据的 public 日志覆盖连接开始、TCP/FreeRDP失败、图形回退、登录成功、首帧、登录阶段停帧、输入首次使用、resize请求和会话结束，替换重复健康轮询日志，为后续IME和rotation真机验收提供会话级证据 |
| 计划代码文件 | `session/rdp_session_core.cpp`、`session/freerdp_session_runner.h/.cpp`、`napi/native_bridge_context.cpp`；不修改协议、连接参数、渲染、输入队列、UI和manifest |
| 数据与日志 | Native为每次Start分配单调`diagnosticSessionId`并贯穿runner；统一使用`RDP_CORE sid=... event=...`和`RDP_DISPLAY event=resize_request ...`。只记录阶段、图形模式、尺寸、耗时、计数和分类后的失败原因；不得记录用户名正文、密码、证书、输入文本、剪贴板或远端像素内容 |
| 行为不变量 | 日志不改变连接/回退顺序、等待阈值、Surface/Controller所有权、resize状态机或输入发送。登录阶段连续15秒无帧只产生一次`frame_stalled`状态，恢复后由`first_frame recovered=yes`闭环；删除每30秒健康刷屏和每10秒重复停帧日志 |
| 验收 ID | AC-ARCH：同一连接的关键事件sid一致、无定时健康刷屏、日志字段无敏感内容；AC-XC/AC-RESIZE/AC-INPUT：只增加观测，不改变会话、Surface、resize和输入路径；完整HAP构建及真机连接首帧通过后升Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（代码和完整HAP已通过；现有真机会话可正常恢复XComponent画面，新的完整断开重连事件链待下一次可控连接验收） |
| 实际代码文件 | `session/rdp_session_core.cpp`、`session/freerdp_session_runner.h/.cpp`、`napi/native_bridge_context.cpp` |
| 设计偏差及原因 | 无功能行为偏差；resize沿用当前App级`DisplayResizeStatus`名称，未引入第二套诊断状态对象 |
| 测试命令/结果/证据 | 2026-08-04：`harmony/app/build_hap.bat`退出码0；signed HAP在tablet `5JB0223804000371`安装启动并恢复现有RDP XComponent画面，进程无FATAL/SIGABRT。为避免中断用户当前远程会话，本次未强制断线重连，故保持Implemented |
| 关联提交 | 实现与本台账回写包含在同一提交（以Git历史为准） |

#### TAB-B-01：结构化 resize 结果与两秒 fallback

| 字段 | 内容 |
|---|---|
| Change ID | TAB-B-01 |
| 设计版本/章节 | v1.3；第 8.3、9.2、9.3、10.7、12.6、12.7、14.2、14.3 节 |
| 用户决策 | 2026-08-04确认resize等待允许以超时结束，且无需兼容旧resize接口。实现语义固定为：会话断开/Surface销毁立即取消；Deferred、Unchanged、Unsupported和Failed立即使用fallback；只有明确Sent最多等待2000ms，超时后主动请求最后可用GDI帧重绘，禁止永久黑屏 |
| FreeRDP公共ABI | 按用户2026-08-04追加决策，不保留旧resize ABI；以`freerdp_ohos_session_resize_ex()`作为唯一会话resize接口，request/result均带`structSize/version`。request包含width、height、orientation；result返回Sent/Deferred/Unchanged/Unsupported/Failed、normalized/sent尺寸和实际orientation。BOOL只表示调用及result写入是否合法，调用方必须读取status，不能从message解析状态 |
| FreeRDP内部数据流 | session resize_ex -> display-control resize_ex -> normalize -> channel/caps/lastSent/send结果；monitor layout使用request真实orientation（0/90/180/270），非法orientation返回Failed，不静默写死landscape。旧`freerdp_ohos_session_resize()`声明、实现和App动态加载入口删除；同时删除无调用者且缺少orientation/result的旧display包装`freerdp_ohos_display_build_monitor_layout()`、`freerdp_ohos_display_send_monitor_layout()`和`freerdp_ohos_display_control_request_resize()`，不保留默认landscape入口 |
| App运行时接口 | `FreerdpRuntimeApi`只动态加载`freerdp_ohos_session_resize_ex`；`RdpSessionChannels`返回App级`DisplayResizeResult`，缺少_ex直接标Unsupported并立即fallback，不调用旧BOOL、不产生LegacyUnknown |
| Native状态机 | 新建`session/rdp_display_resize_coordinator.*`拥有Idle/WaitingForTarget/Fallback、targetGeneration、目标尺寸、2000ms deadline和超时回调；只有Sent进入Waiting。匹配目标帧结束等待；不匹配帧在等待期暂拒绝；超时切Fallback并回调`RequestCurrentFrameRender("resize timeout")`。新请求last-write-wins，旧generation的定时完成不得覆盖新目标 |
| 生命周期 | SurfaceChanged先取得结构化resize结果，再决定是否BeginSent；SurfaceCreated/Destroyed、RDP Disconnected/连接失败均Reset并清除等待；display-control断开即使无即时通知，也最多由2000ms deadline退出。停止/析构时工作线程可join，不使用捕获全局对象的detached线程 |
| 渲染边界 | 本项先修复GDI严格等待与重绘；AVC420/AVC444仍使用现有Surface target contain路径，不新增第二套等待算法。未完成三codec真机矩阵前，不开放manifest的auto_rotation/split，也不把TAB-B父项标Verified |
| 计划代码文件 | FreeRDP子模块：`client/OHOS/ohos_session.h`、`ohos_session_display.c`、`ohos_display.h/.c`；App：`freerdp/freerdp_runtime.h/.cpp`、新建无OHOS/FreeRDP依赖的`session/rdp_display_resize_types.h`、`session/rdp_session_channels.h/.cpp`、`session/rdp_session_core.h/.cpp`、新建`session/rdp_display_resize_coordinator.h/.cpp`、`napi/native_bridge_context.cpp`、`cpp/CMakeLists.txt`、新建`cpp/tests/rdp_display_resize_coordinator_test.cpp`和`tools/run_tablet_native_tests.ps1`。保留当前`rdp_session_core.cpp`中未提交的诊断改动，不覆盖或混入本项语义 |
| 兼容与回退 | 本项是App与内置FreeRDP运行库同步升级，不兼容旧运行库；`_ex`缺失时应用仍可保持会话，但动态分辨率只走Unsupported即时fallback，发布打包检查必须确认内置运行库导出`_ex`。完整回退需同步回退FreeRDP子模块提交和父仓SHA。任何构建/ABI/timeout测试失败均保持rotation/split关闭 |
| 验收ID | AC-RESIZE：Sent匹配、Sent超时、Deferred、Unchanged、Unsupported、Failed、last-write-wins、disconnect/reset均有确定结果；AC-XC：Surface变化不重建Controller/session；AC-INPUT：等待期不接受旧geometry输入，fallback成功present后恢复；AC-ARCH：N-API入口无状态机/线程/日志解析，App/FreeRDP无旧session resize符号且FreeRDP无上述三个旧display包装引用；本机单测、FreeRDP构建、完整HAP通过后标Implemented，GDI/AVC420/AVC444真机旋转/分屏矩阵后升Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（结构化ABI、2000ms超时和fallback已落地；真实rotation接入、DisplayGeometry/input门禁及平板三codec矩阵未完成，不能升Verified） |
| 实际代码文件 | FreeRDP：`client/OHOS/ohos_session.h`、`ohos_session_display.c`、`ohos_display.h/.c`；App：`freerdp/freerdp_runtime.h/.cpp`、`session/rdp_display_resize_types.h`、`rdp_session_channels.h/.cpp`、`rdp_session_core.h/.cpp`、`rdp_display_resize_coordinator.h/.cpp`、`napi/native_bridge_context.cpp`、`cpp/CMakeLists.txt`、`cpp/tests/rdp_display_resize_coordinator_test.cpp`、`tools/run_tablet_native_tests.ps1` |
| 设计偏差及原因 | 本次完成结构化结果、无兼容层、Sent目标等待/超时、即时fallback和生命周期Reset；尚未实现第9.3节完整DisplayGeometry及输入门禁，App当前仍传`ORIENTATION_LANDSCAPE`，待DisplayProfile/rotation工作包提供真实方向。未加入200ms SurfaceChanged debounce，避免在缺少真机旋转事件序列证据时引入额外时序；这些缺口使AC-INPUT和三codec旋转矩阵保持未通过 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_native_tests.ps1`退出码0，覆盖Deferred即时fallback、Sent目标匹配/超时、last-write-wins和reset取消；`tools/run_tablet_arkts_tests.ps1`退出码0；`build-freerdp-ohos.sh`完整OHOS arm64交叉构建退出码0（198.3s）；源码`rg`确认旧session接口及三个旧display包装均0引用；runtime与最终HAP内`readelf -Ws`仅见`freerdp_ohos_session_resize_ex`、`display_*_ex`；`build_hap.bat`退出码0，signed HAP 35,389,685 bytes；在线2in1 `3QC0124C11000711`安装成功、EntryAbility前台、首帧完成且无FATAL/SIGABRT。平板`5JB0223804000371`离线，按用户决策超时收口；平板旋转/分屏/触控/IME及真实RDP GDI/AVC420/AVC444矩阵待补 |
| 关联FreeRDP提交/父仓提交 | FreeRDP `3c2aa31a1`；设计先行父仓提交`0326453`；实现父仓提交待本项提交后以Git历史为准 |

#### TAB-B-02：真实显示方向接入远端 resize

| 字段 | 内容 |
|---|---|
| Change ID | TAB-B-02 |
| 设计版本/章节 | v1.4；第 8.3、9.2、9.3、10.7、12.7、13、14.2、14.3 节 |
| 用户决策 | 2026-08-04确认浮窗字体/任务栏/点击目标偏小不纳入本轮；本项只修复远程 resize 固定 `ORIENTATION_LANDSCAPE` 的问题，并实际覆盖正横屏、正竖屏、反横屏和反竖屏 |
| 方向来源与映射 | 2026-08-04按用户复核改为使用 API 22 Native Display Manager：`OH_NativeDisplayManager_CreateDisplayById()` 读取主窗口所在 Display 的 `NativeDisplayManager_Orientation`，Native 映射为 RDP 角度：`LANDSCAPE -> 0`、`PORTRAIT -> 90`、`LANDSCAPE_INVERTED -> 180`、`PORTRAIT_INVERTED -> 270`。禁止用 Surface width/height 猜方向，也不把 rotation 序号直接当 RDP 角度 |
| 所有权与时序 | ArkTS 不监听 display change、不计算方向、也不向 Native 传 displayId。`RdpDisplayOrientationMonitor` 使用 `OH_NativeDisplayManager_GetDefaultDisplayId()` 取得当前默认 Display，注册 `OH_NativeDisplayManager_RegisterDisplayChangeListener()` 并更新 `RdpSessionChannels` 唯一 orientation；SurfaceChanged、session connected、display-control connected 和 Native display change 四条 resize 路径读取同一状态。XComponent 四向 Surface 变化仍由系统负责，应用不控制或重建它。当前单屏 tablet/2in1 范围不包含窗口跨外接屏迁移；该能力以后独立设计，不能重新复用 IME windowId 接口夹带 displayId |
| N-API/Native 边界 | 显示方向没有 ArkTS/N-API 配置入口。Native Display Manager 独立拥有显示枚举、方向映射、合法性、last-known 状态和主动 resize，结构化日志记录 displayId、native orientation、requested/sent RDP orientation。API 22 Native IME 所需的主窗口 ID 通过独立 `bindImeHostWindow(windowId)` 传递，不属于方向模块 |
| 计划代码文件 | `cpp/session/rdp_display_orientation_monitor.h/.cpp` 保持 Native 默认 Display 监听；删除 `ets/rdp/HostWindowTracker.ets` 的 display 监听职责并改为单次 IME 窗口绑定；修改 `EntryAbility.ets`、`cpp/types/libentry/Index.d.ts`、`cpp/napi/napi_exports.cpp`、`cpp/napi/native_bridge_context.h/.cpp`。此前 ArkTS `DisplayOrientationPolicy.ets`、`DisplayOrientationTracker.ets`及其测试保持删除 |
| 非目标 | 不在本项开放/修改浮窗、最大化、全屏、远端缩放或 manifest 窗口策略；不实现第二套 resize coordinator；不以方向变化重建 XComponent、Controller 或 RDP session |
| 兼容与回退 | 目标和兼容 API 均为 22，不提供旧行为兼容开关；监听/读取异常时保留最后一次合法方向并记录日志。若未来支持窗口跨外接屏迁移，必须新增明确的窗口/Display 关联设计；禁止使用需要 `CUSTOM_SCREEN_CAPTURE` 权限的全设备主窗口枚举来反查当前应用窗口 |
| 验收ID | AC-RESIZE：四个 Native display orientation 枚举映射、日志 orientation、Surface 尺寸与远端方向一致；AC-XC：四向旋转不重连、不重建 Controller；AC-ARCH：全仓 App resize 路径不存在固定 `ORIENTATION_LANDSCAPE`，系统方向只由 Native monitor 读取。HAP 构建通过后标 Implemented，tablet 真机四向旋转及远端 Windows 方向/画面证据通过后升 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（四向映射、主窗口 Display 跟踪、唯一 Session orientation 和四条 resize 路径已接入；tablet 真机四向远端 Windows 证据未完成，不能升 Verified） |
| 实际代码文件 | `harmony/app/entry/src/main/ets/rdp/HostWindowTracker.ets`、`entryability/EntryAbility.ets`、`cpp/session/rdp_display_orientation_monitor.h/.cpp`、`cpp/types/libentry/Index.d.ts`、`cpp/napi/api_exports.cpp`、`cpp/napi/native_bridge_context.h/.cpp`、`cpp/session/rdp_session_core.h/.cpp`、`cpp/session/rdp_session_channels.h/.cpp`、`cpp/CMakeLists.txt`；删除 ArkTS `DisplayOrientationPolicy.ets`、`DisplayOrientationTracker.ets` 及其测试 |
| 设计偏差及原因 | 无功能偏差；此前已提交的 ArkTS orientation policy/tracker 按用户复核被完整删除，改由 API 22 Native Display Manager 持有监听、枚举映射和刷新；ArkTS 只传窗口身份。session connected 和 display-control connected 两条既有内部 resize 旁路也读取同一 `RdpSessionChannels` orientation，未保留固定 landscape 入口 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_native_tests.ps1`退出码0；`tools/run_tablet_arkts_tests.ps1`退出码0；完整 `build_hap.bat` 的 Native/ArkTS/签名均成功，signed HAP 35,458,600 bytes。平板 `5JB0223804000371` 覆盖安装成功，Native Display 日志实际出现 `current=0`、`90`、`180` 且真实 RDP `login_success`、`first_frame` 成功，无 FATAL/SIGABRT；270° 及四向各自的远端 Windows 画面证据仍待人工旋转补齐，因此保持 Implemented |
| 关联提交 | 设计先行提交 `d4ef334`；所有权补充提交 `4acb9c3`；实现与本台账回写包含在同一后续提交（以 Git 历史为准） |

#### TAB-F-01：远程会话虚拟键盘焦点与提交隔离

| 字段 | 内容 |
|---|---|
| Change ID | TAB-F-01 |
| 设计版本/章节 | v1.4；第 9.5、10.6、10.7、12.9、13、14.2、14.3 节 |
| 用户决策 | 2026-08-04确认虚拟键盘获焦后的适配是本轮重点；浮窗字体/点击目标和其余审计项不作为本项问题 |
| 交互与焦点 | 2026-08-04用户明确要求不做键盘开关，IME 完全绑定 XComponent。XComponent Native focus callback 先释放活动远端按键，再由 `RemoteImeClient` attach/show；blur、Surface destroyed、会话退出时 hide/detach 并清理 preview。Native keyboard-status callback 记录 show/hide；系统返回键仅隐藏但不改变 XComponent focus 时，下一次 XComponent touch-down 自动恢复 show。ArkTS 不创建 TextInput、不持有 IME active 状态，也不提供手动开关 |
| IME 数据流 | 2026-08-04按用户复核改用 API 22 Native IME C API。新建 `RemoteImeClient`，通过 `OH_InputMethodController_Attach` + `InputMethod_TextEditorProxy` 接收 committed insert、delete forward/backward、enter 和 preview 生命周期；preview 只保存在本地组合态，不发远端，insert 才调用现有 `RdpSession::SendCommittedText`，delete/enter直接调用 `SendPlatformKey`。XComponent 保持焦点并继续处理物理键盘/鼠标/触控，避免 ArkTS TextInput 与 XComponent 双输入源 |
| 键盘避让 | 进入会话保存当前 `KeyboardAvoidMode` 并设置 `NONE`，离开时恢复；不再监听或镜像 keyboardHeight，因为没有浮动键盘控件需要移动。键盘以 overlay 覆盖且不改变 XComponent width/height、不触发远端 resize。远端 caret 位置不可由当前 RDP 通道获得，因此不伪造自动滚动或远端光标跟随 |
| N-API/Native 边界 | N-API 只通过公共 `configureHostWindow()` 传 windowId/displayId，不暴露 open/close 或逐字符接口。IME attach/proxy、XComponent focus/blur、预编辑、提交、删除、Enter 和状态清理由 Native `RemoteImeClient` 持有；ArkTS SessionPage 只保存/设置/恢复 `KeyboardAvoidMode.NONE`。`SetNeedSoftKeyboard` 不作为第二条输入链，显式设为 false，由 RemoteImeClient 唯一 attach/show |
| 计划代码文件 | 新建 `cpp/input/remote_ime_client.h/.cpp`；修改 `components/session/RdpSessionPage.ets`（仅键盘避让，无 TextInput/按钮）、`entryability/EntryAbility.ets`、`cpp/types/libentry/Index.d.ts`、`cpp/napi/api_exports.cpp`、`cpp/napi/native_bridge_context.h/.cpp`、`input/xcomponent_input_bridge.h`、`xcomponent_input_registration.cpp`、`xcomponent_key.cpp`、`cpp/CMakeLists.txt`；删除 ArkTS `SessionImePolicy`、`SessionKeyboardEnvironment` 及测试 |
| 非目标 | 不实现远端 caret 探测、候选窗自绘、手写输入、远端桌面缩放、Surface 随键盘缩放或一套 tablet 专用 SessionPage；不顺带修改浮窗/全屏策略和其他会话工具栏功能 |
| 兼容与回退 | 目标和兼容 API 均为 22；退出路径必须恢复原 KeyboardAvoidMode，Native blur/Surface destroyed/disconnect 必须 hide/detach 并清理 preview。任一 Native 输入调用失败仅记录可诊断错误并保留本地焦点，不崩溃、不重复补发。回退需同时删除 `RemoteImeClient`、XComponent 生命周期接入和宿主窗口 N-API，不能留下已 attach 但无生命周期所有者的输入链 |
| 验收ID | AC-IME：XComponent focus 自动显示、blur 自动隐藏，系统返回键隐藏后再次触摸可自动恢复，中文拼音选词仅一次提交，英文/数字/退格/Delete/Enter及20次 focus/blur 循环；AC-INPUT：IME 显示时物理打印字符不双发且非打印键可达；AC-XC/AC-RESIZE：IME 显示隐藏前后 Surface width/height 不变、无 reconnect/resize_request；AC-ARCH：无按钮、无 TextInput、无 open/close 或逐字符 N-API，IME 状态不进入 ArkTS。HAP 构建及本机策略测试通过后标 Implemented，tablet 真机中文 IME+物理键盘+Surface 日志证据通过后升 Verified |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（Native IME 生命周期、XComponent focus/touch 恢复、提交链和键盘 overlay 已接入；中文选词、Delete/Enter、物理键盘及20次循环完整矩阵未完成，不能升 Verified） |
| 实际代码文件 | `cpp/input/remote_ime_client.h/.cpp`、`cpp/input/xcomponent_input_bridge.h`、`xcomponent_input_internal.h`、`xcomponent_input_registration.cpp`、`xcomponent_key.cpp`、`xcomponent_touch_gesture.cpp`、`cpp/napi/native_bridge_context.h/.cpp`、`cpp/napi/api_exports.cpp`、`cpp/types/libentry/Index.d.ts`、`cpp/CMakeLists.txt`、`ets/components/session/RdpSessionPage.ets`、`ets/entryability/EntryAbility.ets`、`ets/rdp/HostWindowTracker.ets` |
| 设计偏差及原因 | 无功能偏差；真机发现系统“完成”可隐藏键盘但不会让 XComponent blur，因此先补充 DesignReady，再增加 Native keyboard-status 状态和下一次 touch-down 恢复 show；仍无按钮、开关、TextInput 或 IME N-API。ArkTS 只承担 `KeyboardAvoidMode.NONE` 和宿主窗口身份 |
| 测试命令/结果/证据 | 2026-08-04：Native/ArkTS 测试均退出码0，完整 HAP Native/ArkTS/签名成功，signed HAP 35,458,600 bytes；平板 `5JB0223804000371` 覆盖安装、真实 RDP 登录和首帧成功。XComponent focus 后系统键盘自动显示；点击系统“完成”隐藏后再次触摸 XComponent 自动恢复；三张截图为 `artifacts/tablet-acceptance/2026-08-04/muhub-native-ime-before-done.jpeg`、`muhub-native-ime-after-done.jpeg`、`muhub-native-ime-after-touch.jpeg`。全过程无 `RDP_DISPLAY event=resize_request`、FATAL 或 SIGABRT，键盘仅覆盖远端画面。中文选词、Delete/Enter、物理键盘和20次循环待补 |
| 关联提交 | 设计先行提交待本项提交后以 Git 历史为准；实现提交待回写 |

#### TAB-F-02：远程会话按需虚拟键盘与宿主窗口解耦

| 字段 | 内容 |
|---|---|
| Change ID | TAB-F-02 |
| 设计版本/章节 | v1.5；第 9.5、10.6、10.7、12.9、13、14.1、14.2 节 |
| 用户决策与平台依据 | 2026-08-04用户指出 XComponent focus 不能代表远端正在输入，并进一步明确“按需”不是增加单独键盘按钮。Microsoft Windows App、FreeRDP Android/iOS 和 Guacamole 的显式入口仅作为对比，不采用其 UI；本应用按用户要求用 Native 远端 Pointer Shape + 直接触屏时序判断输入需求，ArkTS 不出现键盘控件 |
| 交互状态机 | 默认 `Hidden`。XComponent focus、session connected、鼠标 hover/click 和普通非文本触摸均不打开 IME。FreeRDP PostConnect 注册 Native pointer prototype，缓存远端当前光标是否为 I-beam 候选；只有 touchscreen-like pointer down/up 形成最近直接触屏意图，且当前或随后短窗口内的远端光标被识别为 I-beam，才由 Native `RemoteImeClient` attach/show。系统返回/完成键隐藏后，只有再次触摸远端文本候选才恢复。XComponent blur、Surface destroyed、disconnect 和会话退出必须 hide/detach、清理 preview 和 pending touch |
| 远端信号边界 | RDP Pointer Shape 返回位图而非控件语义，不能声称获得了远端真实文本焦点。Native 将 pointer masks 解码为 RGBA，按非透明包围盒、纵向主干和上下横杠识别 I-beam 候选；再与 touchscreen-like 最近触摸短窗口求交，避免鼠标经过静态文字或单纯 XComponent focus 自动弹出。误判/漏判通过结构化计数和真机样本调参，不把 bitmap hash 写死为单一 Windows 主题。单纯鼠标移动或远端光标变化不关闭键盘；仅当存在新的 touchscreen-like 直接触摸意图，且随后远端光标切换为非文本候选时由 Native 隐藏，避免编辑中抖动并保证离开输入区域后不常驻 |
| N-API/Native 边界 | `bindImeHostWindow(windowId)` 只传 API 22 `OH_TextConfig_SetWindowId()` 必需的主窗口 ID；XComponent 的 `OHNativeWindow` 是渲染 Surface，不能反查 Window Manager ID。不存在 keyboard show/hide/request 或逐字符 N-API，IME 决策、attach/proxy、show/hide、组合态和 RDP 提交全部属于 Native。删除通用 `configureHostWindow({windowId, displayId})`，显示方向不再经过该边界 |
| 计划代码文件 | `docs/harmonyos-tablet-adaptation-architecture-and-acceptance.md`；删除 `ets/rdp/HostWindowTracker.ets`，新建 `ets/rdp/ImeHostWindowBinder.ets`；修改 `EntryAbility.ets`、`cpp/types/libentry/Index.d.ts`、`cpp/napi/napi_exports.cpp`、`cpp/napi/native_bridge_context.h/.cpp`、`session/rdp_display_orientation_monitor.h/.cpp`、`freerdp/freerdp_runtime.h/.cpp`、`freerdp/freerdp_gdi_bridge.cpp`、`input/remote_ime_client.h/.cpp`、`input/xcomponent_input_bridge.h`、`input/xcomponent_input_internal.h`、`input/xcomponent_input_registration.cpp`、`input/xcomponent_key.cpp`、`input/xcomponent_touch_gesture.cpp`；新建 `input/remote_pointer_text_policy.h/.cpp`、`input/remote_pointer_text_detector.h/.cpp` 和 `tests/remote_pointer_text_policy_test.cpp`，修改 `cpp/CMakeLists.txt`、`tools/run_tablet_native_tests.ps1`。撤回试验性的 `pages/Index.ets`、`components/session/RdpSessionPage.ets` 键盘按钮改动；不修改 FreeRDP/xrdp 子模块 |
| 兼容与回退 | 目标/兼容 API 22，不兼容旧 focus 自动弹出行为，也不保留 `configureHostWindow` 别名。pointer 注册/解码失败时保持键盘隐藏并记录诊断，不退回“XComponent focus 即输入”。API 23 以后可单独评估 `OH_InputMethodController_AttachWithUIContext()`，但本项不得提高 SDK 或申请 `CUSTOM_SCREEN_CAPTURE` 权限 |
| 验收ID | AC-IME：连接、XComponent focus、鼠标操作和非文本触摸后键盘保持隐藏；直接触摸远端文本框后显示；系统关闭后触摸非文本不恢复、再次触摸文本框可恢复；静态可选择文字样本不得持续误弹；中文拼音只提交一次。AC-INPUT：物理键鼠不触发软键盘且输入不受影响。AC-XC/AC-RESIZE：键盘显示隐藏不改变 Surface、不重连、不发送 resize。AC-ARCH：无键盘按钮/TextInput、`configureHostWindow`、ArkTS displayId 监听、focus/touch 无条件 Open 或 IME N-API；pointer 分类有纯逻辑测试，N-API 只做 windowId 参数转换/转发 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（Native pointer shape 分类、直接触摸短窗口、IME 按需显示/隐藏及宿主窗口解耦已完成；完整中文组合态、物理键盘和多主题光标矩阵未完成，不能升 Verified） |
| 实际代码文件 | `cpp/input/remote_pointer_text_policy.h/.cpp`、`cpp/input/remote_pointer_text_detector.h/.cpp`、`cpp/tests/remote_pointer_text_policy_test.cpp`、`cpp/input/remote_ime_client.h/.cpp`、`cpp/input/xcomponent_input_registration.cpp`、`cpp/input/xcomponent_key.cpp`、`cpp/input/xcomponent_touch_gesture.cpp`、`cpp/freerdp/freerdp_runtime.h/.cpp`、`cpp/freerdp/freerdp_gdi_bridge.cpp`、`cpp/napi/napi_exports.cpp`、`cpp/napi/native_bridge_context.h/.cpp`、`cpp/session/rdp_display_orientation_monitor.h/.cpp`、`cpp/types/libentry/Index.d.ts`、`cpp/CMakeLists.txt`、`ets/entryability/EntryAbility.ets`、`ets/rdp/ImeHostWindowBinder.ets`、删除 `ets/rdp/HostWindowTracker.ets`、`tools/run_tablet_native_tests.ps1` |
| 设计偏差及原因 | 触摸后不立即以旧光标状态作最终判断，而是等待 120ms 内的新 Pointer Shape；若服务器未重复发送相同 shape，才使用触摸前缓存作为回退。这样可避免从文本区触摸到非文本区时旧 I-beam 造成键盘常驻，同时支持系统“完成”隐藏后再次触摸同一文本框恢复。无 ArkTS 键盘按钮、开关或 `RequestRemoteKeyboard` 接口 |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_native_tests.ps1` 退出码0，新增 I-beam/arrow/block/null 分类样本通过；`tools/run_tablet_arkts_tests.ps1` 退出码0；`harmony/app/build_hap.bat debug` 完整 Native/ArkTS/打包/签名成功，最终 signed HAP 35,488,925 bytes；`rg` 确认无 `RequestRemoteKeyboard`、`requestRemoteKeyboard` 或键盘按钮 symbol。平板 `5JB0223804000371` 覆盖安装成功；冷启动、连接前和连接成功后键盘保持隐藏，真实 RDP 文本区直接触摸后系统 IME 显示，日志出现 `ShowCurrentInput` 且无崩溃。非文本触摸关闭、中文选词、物理键盘和多主题 Pointer Shape 完整矩阵待补 |
| 关联提交 | 待实现后回写 |

#### TAB-F-03：远程 IME 收起与触屏双击修正

| 字段 | 内容 |
|---|---|
| Change ID | TAB-F-03 |
| 设计版本/章节 | v1.5；第 10.7、12.8、12.9、14.1、14.2 节 |
| 问题与根因 | 2026-08-04 真机反馈：远程 IME 弹出后“完成”无法收起，触屏双击不稳定。Native `OnSendEnter` 只发送远端 Enter，没有响应 `IME_ENTER_KEY_DONE` 隐藏软键盘；单指手势没有双击时间/距离状态，且 8px 拖动阈值在平板高密度触控上过小，第二次轻触容易退化为拖动 |
| 交互决策 | `IME_ENTER_KEY_DONE` 先向远端发送 Enter，再只调用当前 `InputMethodProxy` 隐藏键盘并更新 Native visibility；不 detach editor proxy，下一次远端文本候选触摸可复用当前 attach 快速显示。普通触屏点击仍即时发送第一组 left down/up，不为等待双击而增加单击延迟；第二次 tap 在 350ms、32px 范围内时，在第二次 touch-down 立即发送 left-down、touch-up 发送 left-up，缩短远端两击间隔并保留真实按压时长，由 Windows/RDP 既有双击语义处理。拖动启动阈值调整为 18px，长按和双指滚动保持原语义 |
| 架构边界 | 不增加 ArkTS 按钮、键盘状态或手势识别；IME hide、visibility 与 Enter 提交属于 `RemoteImeClient`，tap/double-tap/drag 判定属于 Native XComponent input。双击没有独立 RDP Pointer flag，正确线协议仍是两组带相同按钮和邻近坐标的 pointer down/up |
| 计划代码文件 | 本文；`cpp/input/remote_ime_client.h/.cpp`、`cpp/input/xcomponent_input_internal.h`、`cpp/input/xcomponent_touch_gesture.cpp`；新建 `cpp/input/xcomponent_touch_policy.h/.cpp`、`cpp/tests/xcomponent_touch_policy_test.cpp`；修改 `cpp/CMakeLists.txt`、`tools/run_tablet_native_tests.ps1` |
| 验收ID | AC-IME：文本区触摸显示后点系统“完成”，远端收到一次 Enter 且键盘隐藏；XComponent 保持焦点时再次触摸文本区可恢复。AC-TOUCH：两次 tap 间隔 50/200/350ms 且距离不超过32px时远端收到两组 left down/up；351ms或距离超过32px按两个普通单击；10px手抖不启动 drag，超过18px启动一次 drag；双击不产生 right-click/wheel。AC-ARCH：无 ArkTS 键盘/双击入口，无双击专用 RDP 私有协议 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（IME“完成”收起已真机通过；双击 Native 判定和线协议发送已实现，远端主机恢复后补文件/窗口动作证据再升 Verified） |
| 实际代码文件 | `cpp/input/remote_ime_client.h/.cpp`、`cpp/input/xcomponent_input_internal.h`、`cpp/input/xcomponent_touch_gesture.cpp`、`cpp/input/xcomponent_touch_policy.h/.cpp`、`cpp/tests/xcomponent_touch_policy_test.cpp`、`cpp/CMakeLists.txt`、`tools/run_tablet_native_tests.ps1` |
| 测试命令/结果/证据 | 2026-08-04：`tools/run_tablet_native_tests.ps1` 退出码0，50/350/351ms及32/32.1px双击边界通过；第二击改为 touch-down/down、touch-up/up 后再次通过 Native 测试；`harmony/app/build_hap.bat debug` 完整构建、打包、签名成功，最新 signed HAP 35,494,385 bytes并覆盖安装到平板 `5JB0223804000371`。此前真实 RDP 文本区按需显示键盘后点击“完成”，系统日志 `HidePanel success`/`OnPanelStatus type=hide`，截图确认键盘消失。最新动作复测时远端主机持续 `connect_timed_out`，双击打开文件/窗口的最终真机动作证据待主机恢复后补，不误报 Verified |
| 关联提交 | 待实现后回写 |

#### TAB-F-04：XComponent 输入手势 P0/P1 收敛

| 字段 | 内容 |
|---|---|
| Change ID | TAB-F-04 |
| 设计版本/章节 | v1.6；第 10.7、12.8、14.1、14.2 节 |
| 问题与根因 | 当前触摸识别依赖多组全局布尔状态，长按只有收到 MOVE/UP 才能触发；多指按数组下标取点而非 pointer ID；拖动、双击、双指滚动阈值使用物理 px；鼠标、触屏、触控板和 UNKNOWN 输入未严格分流。Axis 使用通用 action API、按符号直接发送固定滚轮步长，且 Blur/旋转/Surface 失效只清本地状态或只释放键盘，可能造成远端按钮粘滞。新按下允许坐标 clamp，黑边点击可能落到远端边缘。 |
| 交互决策 | 目标 API 22 且不保留旧行为兼容分支。XComponent 固定采用远端鼠标模拟：单击=左键单击；双击=两组标准左键 down/up；长按 500ms=右键单击；单指移动超过 5vp=按住左键拖动；双指中心位移按 12vp 量化滚动。双击采用 300ms/60vp，与 API 22 Native Gesture 默认值对齐。当前没有本地 zoom transform，因此 pinch、旋转手势和惯性缩放不伪装成远端输入。鼠标保持原生按钮语义；触屏使用 Touch 流；触控板滚动只使用 Axis 流；UNKNOWN 不同时进入两套手势状态机。 |
| Native 状态机 | 将 tap/double-tap/long-press/drag/two-finger-scroll 收敛为可纯测的 Native reducer，显式状态为 Idle、Pressed、Dragging、DoubleSecondDown、LongPressRecognized、Scrolling；追踪 active pointer ID，不依赖 touchPoints 顺序。长按使用可取消的单一调度器，在手指静止时到期触发；UP/CANCEL/多指切换/生命周期失效均取消计时并保证已下发按钮只释放一次。阈值先以 vp 表达，再用 `defaultDisplayDensityDpi / 160` 转为 px。 |
| 坐标与释放 | 新 down/click 默认 `allowClamp=false`：Surface 未 ready、有效 viewport 为零、stale geometry 或落在 contain 黑边时直接拒绝。只有已经成功按下后的 drag move/button up 可 clamp。新增幂等 `ReleaseAllXComponentInput(reason)`，按最后有效坐标释放鼠标和触摸已按下按钮、释放键盘、取消长按与 Axis 余量，再清本地状态；Blur、方向变化、Surface change/destroy、页面隐藏和断连都必须在清 viewport/session 前调用。 |
| Axis 策略 | 使用 `OH_ArkUI_AxisEvent_GetAxisAction` 区分 BEGIN/UPDATE/END/CANCEL；按 source/tool 分流。鼠标滚轮以 15° 为一个远端 notch，触控板以 12vp 为一个 notch，横纵轴分别累计并保留不足一个 notch 的余量；大幅滚动可产生多步，不再退化为 sign-only。pinch scale 事件不作为滚轮。Touch 流不再处理 touchpad 双指滚动，避免 Touch/Axis 重复发送。 |
| 公共 ABI | 删除 `releaseAllKeys` N-API/声明，替换为 `releaseAllInput`；不提供别名。ArkTS 只在页面生命周期通知一次统一释放，不持有手势或按钮状态。 |
| 计划代码文件 | 本文；`cpp/input/xcomponent_touch_policy.h/.cpp`、`cpp/input/xcomponent_touch_gesture.cpp`、`cpp/input/xcomponent_input_internal.h`、`cpp/input/xcomponent_input_bridge.h`、`cpp/input/xcomponent_input_registration.cpp`、`cpp/input/xcomponent_mouse.cpp`、`cpp/input/xcomponent_axis.cpp`、`cpp/input/xcomponent_key.cpp`、`cpp/session/rdp_session_input.cpp`、`cpp/napi/native_bridge_context.cpp`、`cpp/napi/napi_exports.cpp`、`cpp/types/libentry/Index.d.ts`、`ets/pages/Index.ets`、`cpp/tests/xcomponent_touch_policy_test.cpp` |
| 兼容与回退 | 仅 API 22 最优实现；不保留旧阈值、旧 `releaseAllKeys`、错误 Axis action 或 touchpad 双路发送。未取得密度时仅以 1.0 作为安全诊断回退；Surface/viewport 无效时宁可拒绝新输入，不映射到远端边缘。无需修改 FreeRDP/xrdp 通用 core，远端仍接收标准 pointer down/up/wheel 序列。 |
| 验收ID | AC-INPUT-P0：静止长按 500ms 无 MOVE 也只产生一组右键；CANCEL、Blur、旋转、Surface change/destroy、页面隐藏和断连后远端无粘键/粘按钮；黑边与零 viewport 的新按下不产生远端事件；拖动中的 UP 仅释放一次。AC-INPUT-P1：300/301ms、60/60.1vp 双击边界；5vp 拖动边界；不同 density 下手势物理语义一致；pointer 数组换序后双指滚动方向和余量不跳变；鼠标、触屏、触控板各只走一条路径；Axis BEGIN/UPDATE/END/CANCEL、横纵轴、小量累积和大幅多 notch 全序列可纯测。AC-ARCH：Native reducer 无 ArkUI 依赖，统一释放幂等，ArkTS 无手势状态，旧 N-API symbol 不存在。 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（普通指针事件已在 TAB-F-05 修正后恢复；2026-08-05 真机反馈双击仍不生效，不能升 Verified，后续必须按独立子项重新定位真实事件序列） |
| 实际代码文件 | `cpp/input/xcomponent_touch_policy.h/.cpp`、`cpp/input/xcomponent_touch_gesture.cpp`、`cpp/input/xcomponent_input_internal.h`、`cpp/input/xcomponent_input_bridge.h`、`cpp/input/xcomponent_input_registration.cpp`、`cpp/input/xcomponent_mouse.cpp`、`cpp/input/xcomponent_axis.cpp`、`cpp/input/xcomponent_key.cpp`、`cpp/session/rdp_session_input.cpp`、`cpp/napi/native_bridge_context.cpp`、`cpp/napi/napi_exports.cpp`、`cpp/types/libentry/Index.d.ts`、`ets/pages/Index.ets`、`cpp/tests/xcomponent_touch_policy_test.cpp` |
| 设计偏差及原因 | 无协议或交互偏差。Axis 量化策略与触摸 reducer 共用纯策略文件，避免为很小的无 ArkUI 逻辑新增模块；既有 CMake/test runner 已包含该策略源和测试目标，因此无需修改。断连回调发生在传输已断后，只能保证本地幂等清理；主动隐藏、页面退出、Blur、旋转和 Surface invalidation 均在会话/geometry 有效时先发送释放。鼠标 CANCEL 同样释放活动左/右/中键。 |
| 测试命令/结果/证据 | 2026-08-05：`tools/run_tablet_native_tests.ps1` 退出码0，覆盖 300/301ms、60/60.1vp、5vp、静止长按、CANCEL 幂等、pointer 换序、双指余量、Axis begin/update/end、source 切换与多 notch；`tools/run_tablet_arkts_tests.ps1` 退出码0；`harmony/app/build_hap.bat debug` 完整 Native/ArkTS/打包/签名成功。TAB-F-05 包覆盖安装后用户确认普通输入恢复，但双击仍不生效，说明纯策略测试尚未覆盖真实 XComponent 事件形态或远端时序；本项不得标记 Verified。文档中预告的 `tools/check_tablet_architecture.ps1` 在仓库不存在，未伪报通过。 |
| 关联提交 | 待实现后回写 |

#### TAB-F-05：跨渲染路径的远端内容几何与输入逆变换

| 字段 | 内容 |
|---|---|
| Change ID | TAB-F-05 |
| 设计版本/章节 | v1.7；第 10.5、10.7、12.6、12.8 节 |
| 回归根因 | TAB-F-04 在 App 输入队列入口要求 `SurfaceSnapshot.viewportWidth/Height` 非零，但该字段只由 GDI/RGBA `RenderRgbaFrame` 成功路径回写；AVC420/AVC444 直接向 Decoder Surface/GPU present，虽已显示画面却保持 viewport 为零，导致鼠标、触摸和 Axis 在进入 FreeRDP 前全部被拒绝。旧版之所以可用，是 FreeRDP OHOS mapper 在 viewport 为零时退回整个 Surface；该回退无法正确拒绝 contain 黑边，不能作为最终方案。 |
| 跨平台依据 | FreeRDP Android 由 View 的 draw matrix 及 inverse matrix 成对处理渲染与输入；X11 共用 scaledWidth/scaledHeight/offset；SDL 共用 window rect/offset 与 GDI 尺寸；Wayland 在事件发生时由 drawing-buffer geometry 与远端桌面尺寸解析变换。共同原则是输入消费渲染几何的逆变换，几何有效性不绑定某种 codec 或 CPU frame callback。 |
| 通用能力 | 新增无 ArkUI、FreeRDP、codec 依赖的 `RemoteContentGeometry` 纯策略：输入为 target Surface、remote content 尺寸和可选已发布 content rect，输出唯一 contain rect、有效性与双向坐标关系。GDI、AVC420、AVC444 的 fit 计算和输入 viewport 构造必须调用同一策略；删除 SurfaceBridge 私有重复算法和 AVC420 私有 snap 分支，避免渲染与输入出现 1～16px 偏差。 |
| 几何所有权 | `SurfaceBridge` 仍持有 Surface 生命周期与已发布 viewport；输入优先使用当前 Surface generation 内已发布 viewport。对不经过 CPU 回写的直接 GPU 路径，按 FreeRDP Wayland 模式从当前 ready Surface 尺寸与当前远端桌面尺寸解析同一 contain geometry。Surface/desktop 任一为零时才判定无效；不能再以“显式 viewport 为零”代表画面无效。后续若引入裁剪、平移或局部 zoom，必须由 renderer 发布显式 rect 覆盖派生值。 |
| 输入边界 | 新 down/click/wheel 在 content rect 外拒绝；内部按逆变换映射。活动 drag move/button-up 可 clamp，保证跨黑边和生命周期中断仍能释放。Surface change/rotation 前仍使用旧有效 geometry release，之后新事件使用新 Surface 与 desktop geometry。FreeRDP 零 viewport 全 Surface fallback 不再进入正常路径：App 必须始终传入已发布或统一派生的非零 content rect。 |
| 计划代码文件 | 本文；新建 `cpp/surface/remote_content_geometry.h/.cpp`、`cpp/tests/remote_content_geometry_test.cpp`；修改 `cpp/surface/native_rgba_copy.cpp`、`cpp/surface/surface_bridge.cpp`、`cpp/surface/avc420_gpu_compositor_internal.cpp`、`cpp/session/rdp_session_input.cpp`、`cpp/CMakeLists.txt`、`tools/run_tablet_native_tests.ps1`、`docs/CHANGELOG.md` |
| 兼容与回退 | 目标 API 22 单一路径，不保留“viewport=0 即整个 Surface”的正常行为，也不按 GDI/AVC codec 分支输入。派生几何仅适用于当前无 pan/zoom/crop 的 contain 模式；未来出现额外视觉变换时必须显式发布 geometry，而不是在输入层猜测。 |
| 验收ID | AC-GEOMETRY：16:9→4:3、4:3→16:9、同尺寸、奇数尺寸、零尺寸均与 GDI/AVC fit 一致；content 四角映射到远端四角，黑边拒绝。AC-CODEC-INPUT：GDI、AVC420、AVC444 画面出现后，即使 CPU viewport 未回写，鼠标移动/点击、触摸点击/拖动/长按/双击和 Axis 均生效；三 codec 同一点映射一致。AC-LIFECYCLE：旋转/Surface change 前释放，变更后不得复用旧 content rect。 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（共享几何、自动测试、构建和安装完成；GDI/AVC 远端动作级输入待真机操作后升 Verified） |
| 实际代码文件 | `cpp/surface/remote_content_geometry.h/.cpp`、`cpp/tests/remote_content_geometry_test.cpp`、`cpp/surface/native_rgba_copy.cpp`、`cpp/surface/surface_bridge.h/.cpp`、`cpp/surface/avc420_gpu_compositor_internal.cpp`、`cpp/session/rdp_session_input.cpp`、`cpp/CMakeLists.txt`、`tools/run_tablet_native_tests.ps1`、本文、`docs/CHANGELOG.md` |
| 设计偏差及原因 | 无架构偏差。显式 published rect 增加对应 remoteWidth/remoteHeight 身份，只有与当前远端尺寸一致且完全位于 Surface 内时采用；AVC/过渡期使用同一纯策略派生 contain rect。没有为 codec 增加输入分支，也未恢复 FreeRDP 的零 viewport 全 Surface 回退。 |
| 测试命令/结果/证据 | 2026-08-05：`tools/run_tablet_native_tests.ps1` 退出码0，新增 16:9→4:3、4:3→16:9、16px 近一比一、奇数尺寸、零尺寸、黑边边界、published/stale/越界 rect 用例；`harmony/app/build_hap.bat debug` 完整 Native/ArkTS/打包/签名成功，signed HAP 35,547,212 bytes；平板 `5JB0223804000371` 覆盖安装并冷启动成功，PID 49235。用户随后确认此前全部失效的普通输入事件已恢复，双击仍不生效并继续归属 TAB-F-04；GDI/AVC420/AVC444 完整矩阵仍待补。 |
| 关联提交 | 待实现后回写 |

#### TAB-F-06：触屏双击抖动隔离与远端坐标稳定

| 字段 | 内容 |
|---|---|
| Change ID | TAB-F-06 |
| 设计版本/章节 | v1.9；第 10.5、10.7、12.8、14.2 节 |
| 真机根因 | 平板 XComponent 的真实 touchscreen 流在一次按下与抬起之间会产生 6～15 个 MOVE 样本，观测到的两击间隔约 110～240ms，时序满足系统双击窗口。现有 reducer 自行重复实现 tap/double-tap 竞争，并在第二次 DOWN 后把每个 MOVE 都作为 held move 发送；远端因此可能把第二击按拖动处理。纯策略测试只覆盖理想序列，没有覆盖系统真实事件仲裁。 |
| 官方能力与平台依据 | API 22 `ArkUI_NativeGestureAPI_1` 原生提供 Tap、LongPress、Pan、Pinch、Rotation、Swipe 识别器以及 group/精确手指数限制；双击规则明确为两击间隔 300ms、位置距离 60vp。单击与双击不能放在 Exclusive 同类型组：第一击会先满足单击，导致双击失败；应并行识别并由业务翻译层把 single ACCEPT 作为首击、double ACCEPT 作为第二击。不同语义的 Tap/LongPress/Pan 再由外层 Exclusive 竞争。Android/桌面远控同样优先使用平台 GestureDetector/touch-slop，再转换为标准鼠标事件。 |
| 系统手势映射 | 1指 Tap(1)/Tap(2) 使用系统 Parallel 子组：single ACCEPT 立即发送首组 left down/up 并保存坐标，double ACCEPT 只补第二组且固定使用首击坐标，避免三击和远端坐标阈值漂移。1指 LongPress(500ms) ACCEPT=右键单击。1指 Pan(5vp) ACCEPT=在起点 left-down，UPDATE=held move，END/CANCEL=left-up。2指 Pan(12vp) 使用精确2指限制，按系统累计 offset 的增量量化为横/纵 wheel。外层 Exclusive 让 Tap、LongPress、1指 Pan、2指 Pan 只成功一条；所有 recognizer 调用 `OH_ArkUI_SetGestureRecognizerLimitFingerCount(true)`，避免1指 Pan被两指触发。 |
| 原始事件与非映射手势 | 原始 XComponent Touch callback 只保留直接触摸通知，用于远端文本光标/IME 按需判断，不再识别或发送 tap、drag、long-press、scroll。Pinch、Rotation、Swipe 当前没有已定义的 RDP/本地缩放语义，不绑定也不伪装成滚轮；未来引入本地 zoom transform 时单独设计 Pinch。鼠标按钮、Hover、键盘和触控板 Axis 本身已有系统原生事件接口，继续直通对应 Native callback，不重复挂 Gesture。 |
| Native/ArkTS 边界 | 不再由 ArkTS 声明 XComponent 后反传 FrameNode。Native 通过 `ArkUI_NativeNodeAPI_1::createNode(ARKUI_NODE_XCOMPONENT)` 直接创建 XComponent node，以 `OH_NativeXComponent_GetNativeXComponent(node)` 获取 Surface/Input 接口，在同一 node 上注册 Surface、鼠标、键盘、Axis 和系统 Gesture。ArkTS 仅创建 SDK 要求的 `NodeContent` 宿主对象并用 `ContentSlot` 占位，将 NodeContent 交给 Native 挂载/卸载；ArkTS 不持有 XComponentController、node、gesture 参数、状态或回调。 |
| 节点属性与生命周期 | Native node 显式设置 100% 宽高、黑色背景、focusable、focus-on-touch、default-focus 和既有组件 ID，保持当前布局/焦点语义。attach 时依次创建 node、取得 `OH_NativeXComponent`、注册既有 Surface/Input callback、挂系统手势、加入 NodeContent；任一步失败按逆序回滚。detach 时先统一释放输入，再从 NodeContent 移除、卸载/销毁手势和 node。重复 attach 同一 content 幂等，不保留 ArkTS 声明 XComponent 的兼容路径。 |
| 计划代码文件 | 本文；新建 `cpp/input/xcomponent_native_gesture.h/.cpp`、`cpp/surface/xcomponent_native_host.h/.cpp`；修改 `cpp/input/xcomponent_touch_policy.h/.cpp`、`cpp/input/xcomponent_touch_gesture.cpp`、`cpp/input/xcomponent_input_internal.h`、`cpp/input/xcomponent_input_registration.cpp`、`cpp/napi/native_bridge_context.h/.cpp`、`cpp/napi/napi_exports.cpp`、`cpp/types/libentry/Index.d.ts`、`ets/components/session/RdpSessionPage.ets`、`cpp/CMakeLists.txt`、`cpp/tests/xcomponent_touch_policy_test.cpp`、`tools/run_tablet_native_tests.ps1` |
| 公共 ABI | 新增内部 N-API `attachXComponentContent(nodeContent)` / `detachXComponentContent()`，仅承载 Native node 的 UI 宿主生命周期；删除未发布的 `bindXComponentGestures(frameNode)` / `unbindXComponentGestures()` 草案。无 ArkTS 手势动作 ABI，无 FreeRDP/xrdp ABI 变化。 |
| 兼容与回退 | 仅保留 API 22 Native XComponent node + Native Gesture 单一路径，不保留 ArkTS XComponent 或手写双击兼容分支；node 创建、callback 注册、addGesture 或 NodeContent add 失败必须记录明确错误并完整回滚。异常 CANCEL/Blur/Surface 生命周期仍走统一释放。 |
| 验收ID | AC-GESTURE-UNIT：raw Touch 不产生远端动作；single ACCEPT=1击，single+double ACCEPT=首击同坐标的2击且不产生第3击；1指 Pan begin/update/end/cancel 按钮严格配对；2指 Pan 小量累计、大量多 notch、横纵增量正确；精确手指数、Parallel Tap 子组和外层 Exclusive 的创建结果均检查。AC-GESTURE-DEVICE：文件/标题栏双击10/10；单击、拖动、长按右键、双指纵横滚动各10次成功且互不串动作；Pinch/Rotation 不产生远端滚轮/按钮。AC-LIFECYCLE：Blur、旋转、页面销毁和 Surface 失效可释放系统 Pan 的活动左键与滚动余量；重复 attach 无重复回调、悬空 node、Surface 或粘键。AC-ARCH：ArkTS 只有 NodeContent/ContentSlot 宿主，无 XComponentController、手势参数/状态/回调，无双击专用协议，FreeRDP/xrdp 通用 core 无修改。 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（全 Native XComponent node 及系统 Tap/LongPress/1指Pan/2指Pan 已完成；等待远端动作级真机验收后升 Verified） |
| 实际代码文件 | `cpp/input/xcomponent_native_gesture.h/.cpp`、`cpp/surface/xcomponent_native_host.h/.cpp`、`cpp/input/xcomponent_touch_policy.h/.cpp`、`cpp/input/xcomponent_touch_gesture.cpp`、`cpp/input/xcomponent_input_internal.h`、`cpp/input/xcomponent_input_registration.cpp`、`cpp/napi/native_bridge_context.h/.cpp`、`cpp/napi/napi_exports.cpp`、`cpp/types/libentry/Index.d.ts`、`ets/components/session/RdpSessionPage.ets`、`ets/pages/Index.ets`、`cpp/CMakeLists.txt`、`cpp/tests/xcomponent_touch_policy_test.cpp`、`tools/run_tablet_native_tests.ps1`、本文 |
| 设计偏差及原因 | 无功能边界偏差。`ContentSlot` 本身不支持 width/height/onAppear 通用属性，因此用 100% `Stack` 承载显示槽与 attach 生命周期；NodeContent 仍只作为 Native node 的系统宿主，不持有 XComponent 或手势逻辑。系统 Pan offset 单位为 px，双指滚轮量化使用 display density 将12vp换算为px。Pinch/Rotation/Swipe 按“无已定义远端语义”明确不绑定。 |
| 测试命令/结果/证据 | 2026-08-05：`tools/run_tablet_native_tests.ps1` 退出码0，覆盖 single+double 仅两击且锚定首击坐标、Pan accept/update/end/cancel 按钮配对、双指累计 offset 横纵滚轮、Axis/geometry 回归，并静态检查 Parallel Tap 子组、外层 Exclusive、精确手指数及 ArkTS/Raw Touch 无手势所有权；`tools/run_tablet_arkts_tests.ps1` 退出码0；`harmony/app/build_hap.bat debug` 完整 Native/ArkTS/打包/签名成功，signed HAP 35,542,202 bytes。HAP 已覆盖安装到平板 `5JB0223804000371` 并以 bundle `com.muhub.desktop` 冷启动成功，PID 2371；远端双击、长按、拖动和双指滚动动作矩阵待用户在会话页验证。 |
| 关联提交 | 待实现后回写 |

#### TAB-F-07：权限请求 N-API 收敛与类型化

| 字段 | 内容 |
|---|---|
| Change ID | TAB-F-07 |
| 设计版本/章节 | v2.0；第 11、14.2 节 |
| 问题 | 麦克风、摄像头、剪贴板和位置权限各暴露一组 `on*PermissionRequest` 与 `complete*PermissionRequest`，8个 ABI 仅 type 和 Native bridge 不同；ArkTS 已用同一注册表处理，却仍重复包装。`attachXComponentContent` 和权限完成参数使用 `Object`，编译期无法约束字段。 |
| 设计决策 | 删除8个专用权限 ABI，不保留别名；新增 `onPermissionRequest(callback)`，统一回调 `{ type, requestId }`；新增 `completePermissionRequest({ type, requestId, granted })`，Native 按严格 type 路由到既有4个独立 `PermissionRequestBridge`。各通道 pending ID、等待条件和超时保持隔离，不能合并为共享 pending 状态。未知 type、错误参数或非 pending ID 返回明确失败。 |
| 类型边界 | 新增 `NativePermissionType` 字面量联合、`NativePermissionRequest`、`NativePermissionResult`；`attachXComponentContent` 参数改为 ArkUI `NodeContent`。状态/错误回调及 connect/xrdp/IME/XComponent/release ABI 保持不变。统一权限回调内部可为4个独立线程安全 sink 注册同一 JS callback，但 ArkTS 只注册一次。 |
| 计划代码文件 | 本文；`cpp/napi/napi_event_sink.h/.cpp`、`cpp/napi/napi_exports.cpp`、`cpp/types/libentry/Index.d.ts`、`ets/pages/Index.ets`、相关 ArkTS 测试/架构检查 |
| 兼容与回退 | 目标单包/API 22，不保留旧8接口；Native 各权限 bridge 和 FreeRDP/xrdp callback ABI 不变。任一 sink 注册失败时统一接口返回失败，不把部分注册误报为成功。 |
| 验收ID | AC-PERM-ABI：d.ts/N-API exports/ArkTS 均只存在2个统一权限接口，旧8 symbol 不存在；NodeContent/permission 参数无裸 `Object`。AC-PERM-ROUTE：4种 type 分别进入正确 bridge，未知 type 拒绝；requestId/granted 原样回传，非 pending ID 失败。AC-PERM-REG：ArkTS 只注册一次回调，4通道请求仍各自触发正确 UI 权限流程。AC-REGRESSION：Native/ArkTS 测试和 Debug HAP 构建通过。 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented；AC-PERM-ABI、静态 AC-PERM-ROUTE、编译与安装冒烟已通过；4类真实 RDP 通道触发的 AC-PERM-REG 待联机会话验收后升级为 Verified。 |
| 实际代码文件 | 本文；`harmony/app/entry/obfuscation-rules.txt`；`cpp/napi/napi_event_sink.h/.cpp`；`cpp/napi/napi_exports.cpp`；`cpp/types/libentry/Index.d.ts`；`ets/pages/Index.ets`；`tools/run_tablet_arkts_tests.ps1`；`tools/run_tablet_native_tests.ps1` |
| 设计偏差及原因 | 无接口和状态边界偏差。沿用既有4个 `EventSink` 和4个 `PermissionRequestBridge`，仅由统一注册入口给各 sink 绑定同一 ArkTS callback 并附加 type；不合并 pending 状态。 |
| 测试命令/结果/证据 | 2026-08-05：`git diff --check` 通过；`tools/run_tablet_arkts_tests.ps1` 退出码0，检查强类型统一接口、单次 ArkTS 注册及旧8接口/裸 Object 消失，ArkTS 单测通过；`tools/run_tablet_native_tests.ps1` 退出码0，检查4类 type 路由、2个统一 N-API export、旧8 export 消失并通过既有 native 回归；`harmony/app/build_hap.bat debug` 完整 Native/ArkTS/打包/签名成功，signed HAP 35,543,326 bytes。HAP 已覆盖安装到平板 `5JB0223804000371`，bundle `com.muhub.desktop` 启动成功，进程 PID 12608。真实 microphone/camera/clipboard/location channel 请求仍需对应 RDP 服务端触发验收。 |
| 关联提交 | 待实现后回写 |

#### TAB-A-05：Native Gateway 与 RDP 客户端协调器收口

| 字段 | 内容 |
|---|---|
| Change ID | TAB-A-05 |
| 设计版本/章节 | v2.1；第 5.1、5.2、10.3、10.6、11、14.4 节 |
| 问题 | `Index.ets`、`RdpSessionPage.ets`、`ImeHostWindowBinder.ets`、`XrdpServerController.ets` 均直接 import `libentry.so`；Index 同时承担 Native 状态/错误回调注册、连接调用和输入释放，违反既定依赖方向，也使后续平板会话能力难以独立验收。 |
| 设计决策 | 新建 `NativeRdpGateway.ets` 作为 ArkTS 唯一 `libentry.so` import 点，逐项薄转发当前实际 ABI，不添加主动 disconnect。新建 `RdpClientController.ets` 负责一次性注册 state/error 回调、发起 connect、维护 Native 连接状态和 releaseAllInput；Index 保留表单、路由、提示文案、持久化和权限 UI 协调。XComponent、IME 和 XRDP 控制器只能调用 Gateway，展示组件不直接接触 Native 模块。 |
| 状态与生命周期 | Controller 将 `Connected/RemoteLoginWaiting/RemoteDesktopReady` 归为 connected；Native state 与同步 connect result 都更新同一状态。Index 的 `aboutToAppear` 注册一次回调，`onPageHide/aboutToDisappear` 通过 Controller 幂等释放输入。系统窗口 `X` 关闭由进程退出结束会话，不增加断开按钮或未实现 ABI。 |
| 计划代码文件 | 本文；新增 `ets/rdp/NativeRdpGateway.ets`、`ets/rdp/RdpClientController.ets`；修改 `ets/pages/Index.ets`、`ets/rdp/XrdpServerController.ets`、`ets/rdp/ImeHostWindowBinder.ets`、`ets/components/session/RdpSessionPage.ets`、`tools/run_tablet_arkts_tests.ps1`。 |
| 兼容与回退 | N-API、FreeRDP/xrdp ABI、连接参数、状态字符串、权限流程、XComponent node 与 UI 均不变；Gateway 不吞异常，Controller 不持有 ArkUI Context。可按调用点逐项恢复直接 import 回退，但发布门禁要求最终仅 Gateway 可 import `libentry.so`。 |
| 验收ID | AC-ARCH：ArkTS 全仓只有 `NativeRdpGateway.ets` import `libentry.so`，components 为0；Index 不出现 `rdpNative.`；Controller/Gateway 不 import UI组件、Capability或断点策略；Gateway 只薄转发。AC-REGRESSION：ArkTS测试、Debug HAP构建和真机安装启动通过；可用远端下连接/首帧/输入链不回退后升 Verified。 |
| 设计状态 | DesignReady |
| 实现状态 | Verified（用户于2026-08-05确认可升级；唯一 Native import、Controller 状态所有权、展示组件回调隔离、构建、平板安装启动及实际使用均无回退） |
| 实际代码文件 | 本文；`ets/rdp/NativeRdpGateway.ets`、`ets/rdp/RdpClientController.ets`、`ets/pages/Index.ets`、`ets/rdp/XrdpServerController.ets`、`ets/rdp/ImeHostWindowBinder.ets`、`ets/components/session/RdpSessionPage.ets`、`tools/run_tablet_arkts_tests.ps1` |
| 设计偏差及原因 | 无 Native 边界偏差。`RdpSessionPage` 通过 NodeContent attach/detach 回调保持展示层零 Native 依赖；Index 仍是现有表单、持久化、XRDP权限和页面路由的页面协调器，文件规模尚未达到长期约500行目标，但不再持有 `rdpNative`、连接状态或 Native callback 注册细节，后续业务域拆分不得重新穿透 Gateway。Controller 显式传递回调前的 `wasConnected`，避免异步 UI 更新丢失首次 Connected 转换。 |
| 测试命令/结果/证据 | 2026-08-05：`tools/run_tablet_arkts_tests.ps1`退出码0，静态确认ArkTS全仓仅Gateway一处`libentry.so` import、components无Gateway/Native依赖、Index无`rdpNative.`且Controller无UI/能力/断点依赖；`tools/run_tablet_native_tests.ps1`退出码0；`git diff --check`通过；`harmony/app/build_hap.bat debug`完整Native/ArkTS/打包/签名成功，signed HAP 35,549,099 bytes；平板`5JB0223804000371`覆盖安装并启动成功，PID 27536；用户确认实际会话使用通过并同意升级Verified。 |
| 关联提交 | 待实现后回写 |

#### TAB-B-03：Native 显示监听首次方向同步

| 字段 | 内容 |
|---|---|
| Change ID | TAB-B-03 |
| 设计版本/章节 | v2.1；第 9.2、9.3、10.7、12.7 节 |
| 问题 | Native 已注册 display change listener，但启动后未读取默认显示的当前 orientation；若应用首次进入时设备为竖屏或倒置方向，会话仍从默认 landscape 开始，只有下一次 display change 才纠正。 |
| 设计决策 | `RdpDisplayOrientationMonitor::Start` 在监听注册成功且释放内部 mutex 后立即执行一次 `Refresh("native_display_initial")`；首次查询失败只记录明确诊断并保留监听，不把已成功注册的 monitor 误报为启动失败。回调继续进入既有 `UpdateDisplayOrientation` 与 resize 路径，不新增 ArkTS 显示监听或第二方向来源。 |
| 计划代码文件 | 本文；`cpp/session/rdp_display_orientation_monitor.cpp`及 Native 测试/静态门禁。 |
| 兼容与回退 | 无公共 ABI 变化；删除首次 Refresh 即可回退为仅监听后续 change 的旧行为。第5项200ms resize尾随防抖按用户决策不在本轮处理。 |
| 验收ID | AC-ROTATION：静态检查 Start 注册后存在唯一 `native_display_initial` Refresh，调用发生在 mutex 释放后；Native测试、Debug HAP构建通过。真机冷启动四方向及连接后方向矩阵通过后升 Verified。 |
| 设计状态 | DesignReady |
| 实现状态 | Verified（用户于2026-08-05确认四向XComponent渲染及本轮方向能力可升级；首次方向查询、Native回归、完整构建和平板安装启动均通过） |
| 实际代码文件 | 本文；`cpp/session/rdp_display_orientation_monitor.cpp`、`tools/run_tablet_native_tests.ps1` |
| 设计偏差及原因 | 无；首次Refresh在注册成功并释放mutex后执行，失败只写诊断且保留已注册监听。按用户决策未实现第5项200ms resize尾随防抖。 |
| 测试命令/结果/证据 | 2026-08-05：Native门禁确认唯一`Refresh("native_display_initial")`，`tools/run_tablet_native_tests.ps1`退出码0；Debug HAP完整构建成功并覆盖安装启动到平板`5JB0223804000371`，PID 27536；结合用户此前确认XComponent四向渲染正常并明确同意升级，标记Verified。 |
| 关联提交 | 待实现后回写 |

#### TAB-C-05：开启自动旋转与分屏窗口声明

| 字段 | 内容 |
|---|---|
| Change ID | TAB-C-05 |
| 设计版本/章节 | v2.1；第 10.1、12.1、12.7、14.5 节 |
| 设计决策 | 保持一个 bundle、一个 entry、一个 default product 和同一 HAP；在 EntryAbility 增加 `orientation: auto_rotation`，并把 `split` 加入既有 `fullscreen/floating` 窗口模式。tablet/2in1 声明和当前最小窗口值不在本项重复调整。 |
| 计划代码文件 | 本文；`harmony/app/entry/src/main/module.json5`、`tools/run_tablet_arkts_tests.ps1`。 |
| 兼容与回退 | 目标设备/API 22，不保留“仅横屏”兼容分支；移除新增字段可回退。若 schema、签名、安装或启动失败则不得交付该候选。 |
| 验收ID | AC-PKG：构建产物清单含 `auto_rotation` 与 `fullscreen/floating/split`，bundleName仍为`com.muhub.desktop`且只有一个entry HAP；AC-ROTATION/AC-LAYOUT：真机横竖四方向、50/50分屏和窗口恢复无崩溃、无不可达内容后升 Verified。 |
| 设计状态 | DesignReady |
| 实现状态 | Verified（用户于2026-08-05确认可升级；schema、生成清单、完整构建、平板覆盖安装启动及实际窗口使用通过） |
| 实际代码文件 | 本文；`harmony/app/entry/src/main/module.json5`、`tools/run_tablet_arkts_tests.ps1` |
| 设计偏差及原因 | 无；保持同一bundle、entry、default product及tablet/2in1声明，仅增加自动旋转与split能力。 |
| 测试命令/结果/证据 | 2026-08-05：ArkTS门禁与Hvigor schema处理通过；生成的`build/default/intermediates/package/default/module.json`含`orientation=auto_rotation`及`supportWindowMode=[fullscreen,floating,split]`，bundleName为`com.muhub.desktop`、deviceTypes为`2in1/tablet`；Debug HAP 35,549,099 bytes并在平板`5JB0223804000371`覆盖安装、启动成功，PID 27536；用户确认实际窗口使用并同意升级Verified。 |
| 关联提交 | 待实现后回写 |

#### TAB-A-06：Index 页面协调器按业务域拆分

| 字段 | 内容 |
|---|---|
| Change ID | TAB-A-06 |
| 设计版本/章节 | v2.2；第 5.1、5.2、10.3、11、14.4 节 |
| 问题 | `Index.ets` 仍超过1200行，同时包含连接地址校验、配置存储异步编排、Native权限请求路由、XRDP权限/启动状态机和页面装配；虽然Native入口已收口，但页面协调器职责仍过宽，修改任一业务域都容易影响会话UI。 |
| 设计决策 | 继续保持单HAP且不引入全局状态框架。抽出四个非UI职责：`RdpConnectionValidator`只做连接字段纯校验；`WindowsConnectionProfileCoordinator`只编排Store异步操作；`RdpPermissionRequestCoordinator`只路由四类Native权限请求；`RemoteControlCoordinator`只拥有XRDP权限、访问码、启动请求和状态快照。Index只保留ArkUI响应式字段、页面路由、组件回调装配、会话提示文案及把Coordinator快照应用到`@State`。 |
| 状态与依赖 | Validator不得import UI/Native/Context；Profile Coordinator只依赖Store和日志，通过结果回调返回快照，不写ArkUI状态；Permission Coordinator依赖PermissionManager与Gateway，不持有UIContext；RemoteControl Coordinator依赖PermissionManager、XrdpServerController和文件目录provider，通过不可变快照通知Index。所有Native调用仍只经Gateway，components继续零Native依赖。 |
| 计划代码文件 | 本文；新增`ets/rdp/RdpConnectionValidator.ets`、`WindowsConnectionProfileCoordinator.ets`、`RdpPermissionRequestCoordinator.ets`、`RemoteControlCoordinator.ets`、`RdpSurfaceContentHost.ets`及展示层映射`components/home/HomeConnectionValidation.ets`；新增`src/test/RdpConnectionValidator.test.ets`并修改测试入口、`ets/pages/Index.ets`和`tools/run_tablet_arkts_tests.ps1`。 |
| 行为不变量 | 表单校验文案与顺序、TOFU默认值、配置选择/密码加载/删除/保存、四类权限路由、tablet XRDP fail-closed、2in1权限与启动时序、连接状态、XComponent/IME/手势及全部UI布局不变。拆分不新增兼容分支、不复制页面。 |
| 兼容与回退 | 无N-API、FreeRDP/xrdp、manifest、资源和公共UI契约变化；各Coordinator可按业务域独立内联回Index回退。若Index仍直接出现Store方法、连接校验正则、权限type注册表或XRDP启动Promise状态机，则AC-ARCH不通过。 |
| 验收ID | AC-ARCH：Index显著缩小并只保留页面协调；四类实现分别位于唯一文件；Validator/Controller无UI组件、能力断点依赖；全仓仍仅Gateway import `libentry.so`。AC-REGRESSION：ArkTS/Native测试、Debug HAP、平板覆盖安装启动及现有会话使用通过。 |
| 设计状态 | DesignReady |
| 实现状态 | Implemented（职责拆分、单测、架构门禁、完整构建和平板安装启动通过；远端连接、配置增删/密码存储及2in1 XRDP动作回归后升Verified） |
| 实际代码文件 | 本文；`ets/rdp/RdpConnectionValidator.ets`、`WindowsConnectionProfileCoordinator.ets`、`RdpPermissionRequestCoordinator.ets`、`RemoteControlCoordinator.ets`、`RdpSurfaceContentHost.ets`、`components/home/HomeConnectionValidation.ets`、`ets/pages/Index.ets`、`src/test/RdpConnectionValidator.test.ets`、`src/test/List.test.ets`、`tools/run_tablet_arkts_tests.ps1` |
| 设计偏差及原因 | Index从1256行降至899行（减少357行，约28%），没有强行压到旧文档约500行目标。剩余主体是40余个响应式字段、连接配置表单状态应用、路由/生命周期、会话提示文案和Home/Settings参数装配；继续压缩需引入可观察ViewModel并改写大量UI绑定，超出“职责拆分且行为不变”的合理边界。新增文件最大为RemoteControlCoordinator 260行，均低于300行协调器预算；其余职责文件20～113行。 |
| 测试命令/结果/证据 | 2026-08-05：新增5项连接校验单测，与原断点/能力测试一起通过；`tools/run_tablet_arkts_tests.ps1`退出码0并强制Index不超过950行、无Store/XRDP Controller/权限type/校验正则/Gateway细节；`tools/run_tablet_native_tests.ps1`退出码0；`git diff --check`通过；`harmony/app/build_hap.bat debug`完整Native/ArkTS/打包/签名成功，signed HAP 35,592,754 bytes；平板`5JB0223804000371`覆盖安装和EntryAbility启动成功，PID 34716。 |
| 关联提交 | 待实现后回写 |

父级台账不能代替每次代码变更登记。开始具体实现前，在本文追加子项（例如 `TAB-B-01`），至少填写：

~~~text
Change ID:
设计章节/设计版本:
计划代码文件:
公共 ABI 或数据/状态变化:
兼容与回退:
验收 ID:
设计状态: DesignReady
实际代码文件:                # 实现后回写
设计偏差及原因:              # 无偏差也写“无”
测试命令/结果/证据路径:       # 验证后回写
实现状态: NotStarted | Implemented | Verified
关联工程提交/子模块提交/SHA:
~~~

当前工作区中未登记到子项的既有改动，不自动视为本方案的实现证据；必须先核对归属、补台账和设计，再继续修改。

## 12. 验收方案

代码、测试、截图和实施台账统一引用以下稳定验收 ID；不得只写“已测试”而不指明用例：

| 验收 ID | 对应范围 | 本文位置 |
|---|---|---|
| AC-PKG | 单包、身份、安装和升级 | 第 12.2 节 |
| AC-LAYOUT | Compact/Expanded、窗口、方向、安全区 | 第 12.3 节 |
| AC-FONT | 字体、Icon、触控热区和焦点 | 第 12.4 节 |
| AC-CAP | tablet/2in1 功能隔离和回归 | 第 12.5 节 |
| AC-XC | XComponent/Controller/session 生命周期 | 第 12.6 节 |
| AC-RESIZE | resize、fallback、viewport 和 codec | 第 12.7 节 |
| AC-INPUT | 坐标、黑边、触控、键鼠和释放 | 第 12.8 节 |
| AC-IME | 虚拟键盘、中文提交和焦点恢复 | 第 12.9 节 |
| AC-STABILITY | 30 分钟组合稳定性 | 第 12.10 节 |
| AC-ARCH | diagnostics、静态边界、自动测试和构建 | 第 13、14 节 |

最低验收环境：至少 1 台目标 API 22 tablet 真机、1 台目标 API 22 2in1 真机；可实际操作的触屏、物理键盘和鼠标/触控板；至少 1 个支持 Display Control 的 RDP 服务端和 1 个不支持/可关闭该能力的服务端。GDI、AVC420、AVC444 可以由不同服务端或配置提供，但每项必须用 diagnostics 证明真实协商路径。模拟器可用于布局和纯策略冒烟，不能替代 Surface、旋转、密度、物理输入和 codec 真机验收。

| 责任 | 必须交付 |
|---|---|
| 产品 | D-01 完整决策记录、目标设备/最小窗口范围 |
| 开发 | 自动测试、架构门禁、Debug/Release 构建、diagnostics snapshot |
| 测试 | 布局/字体/输入/IME/resize/稳定性矩阵及截图证据 |
| 架构评审 | 单一事实来源、依赖方向、ABI 兼容和社区边界复核 |
| 发布负责人 | canonical HAP 身份、升级、P0 证据包和第 18 节签收 |

### 12.1 验收证据包

每次候选版本保存：

~~~text
artifacts/tablet-acceptance/<date>/<build-id>/
├─ manifest.txt
├─ build-debug.log
├─ build-release.log
├─ layout-matrix.csv
├─ screenshots/
├─ acceptance-snapshot-before.json
├─ acceptance-snapshot-after.json
├─ hilog-filtered.txt
└─ result.md
~~~

产物可以保存在 CI 或测试归档中，不要求提交大型截图到 Git。result.md 必须列出设备型号、系统版本、窗口尺寸、字体比例、RDP 服务器、编码路径和失败项。

### 12.2 单包与安装

| 用例 | 操作 | 通过条件 |
|---|---|---|
| 单 HAP | Debug/Release 构建并检查 outputs | 唯一正式安装产物为 entry-default-signed.hap；可存在同 target 的 unsigned 中间产物，但不得有设备分包 |
| 身份 | 读取 HAP profile | bundleName = com.muhub.desktop，module = entry |
| tablet 安装 | 安装同一 HAP | 可安装、启动 |
| 2in1 安装 | 安装同一 HAP | 可安装、启动 |
| 升级 | 使用旧版覆盖安装到候选版 | 数据和升级关系保持 |

实施后使用参数化构建命令；在脚本尚未按第 14.3 节改造前，该项只能标记为“待实现”，不能写已通过：

~~~text
harmony/app/build_hap.bat debug
harmony/app/build_hap.bat release
~~~

### 12.3 布局矩阵

- 宽度：600、720、839、840、1024、1280、1440vp；目标设备可达时补 1920vp 验证 XL maxWidth。
- 高度：480、600、800vp。
- 方向/窗口：横屏、竖屏、分屏、浮窗拖动。

每张截图同时保存 acceptance snapshot 中的实际 widthVp、heightVp 和 WidthBreakpoint，不能只凭窗口外观标注尺寸。若真机窗口管理器无法精确停在 839/840vp，使用最接近的跨断点尺寸做真机连续性验证，五个 breakpoint 的精确映射由策略单测保证。

通过条件：

- 839vp 为 Compact，840vp 为 Expanded。
- 1440vp 仍为 Expanded，不出现第三套结构。
- 1920vp（设备可达时）表单/卡片受 maxWidth 约束，不被无限拉宽。
- 839↔840 往返 20 次，表单值、已选设备、设置路由和滚动可达性不丢失。
- 无横向溢出、重叠、不可点击或不可滚动内容。
- Compact 不保留桌面固定侧栏。
- Expanded 首页同时可见设备列表和连接详情；Expanded 设置同时可见导航和当前内容。
- 高度 480vp 时所有操作通过 Scroll 可达。
- 状态栏、圆角、挖孔、系统手势区和 2in1 窗口按钮不遮挡关键操作。

### 12.4 字体、Icon、焦点

字体比例：1.0、1.3、1.75；语言：zh_CN、en_US。

通过条件：

- 无裁字、重叠、按钮消失。
- 所有内容可滚动到达。
- SVG 清晰、不变形、不按窗口拉伸。
- 图标视觉尺寸合理；所有可点击 UI 控件的实测交互热区不小于 48×48vp。
- Tab/Shift+Tab 能遍历所有可交互控件，焦点顺序与视觉顺序一致。
- 鼠标 hover 有明确反馈；触控和鼠标操作结果一致。

### 12.5 功能隔离（D-01 采用推荐值时）

tablet 冷启动：

- 录屏权限弹窗次数 = 0。
- XRDP runtime/HNP 启动调用次数 = 0。
- XRDP diagnostics Native 调用次数 = 0。
- Home 无 XRDP 状态卡、访问码。
- Settings 无远控服务入口。
- 通过旧状态、非法初始路由或回调调用都不能进入/启动 XRDP。
- RDP 客户端连接正常。
- RemoteFilesDirectory 仍完成客户端共享目录准备，基础/RDP 客户端设置中仍可打开目录；该行为不计作 XRDP 调用。
- 启用客户端 drive redirection 的连接中，远端 Windows 可通过 `\\tsclient\Downloads` 列出并读写约定测试文件。

2in1 回归：

- 原 XRDP 设置入口仍在。
- 原录屏权限流程按需工作。
- XRDP 启停、诊断、访问码功能不回退。
- 客户端 `\\tsclient\Downloads` 仍可用。

### 12.6 XComponent 生命周期

| 场景 | 次数 | 通过条件 |
|---|---:|---|
| 进入会话后显隐工具栏 | 20 | connectCount = 1，nativeSessionId 不变；indexInstanceId、controllerInstanceId、sessionPageInstanceCount 前后不变 |
| Home/Settings 839/840 切换 | 20 | 不在会话时状态不丢；会话节点不受影响 |
| 横竖屏旋转 | 10 | 3秒内画面稳定，无 reconnect；controllerInstanceId 不变 |
| 分屏连续拖动 | 20 个、间隔 50ms 的事件 | 事件期间 sent delta = 0；最后事件后 200～500ms 内 sent delta = 1，目标为最后尺寸 |
| 退出会话 | 1 | destroyed 与 created 配平；releaseAllInputCount 增加，activePointerButtons/buttonsDown/keysDown 均为 0 |

### 12.7 resize 和渲染

分别测试 GDI、AVC420、AVC444：

- 路径证明：
  - requestedGraphicsMode、negotiatedCodec、renderOwner 必须明确显示目标路径。
  - 该路径 presentedFrames 增加后才能判通过；协商回退不能冒充目标 codec 通过。
- 支持 Display Control：
  - diagnostics 出现 requested -> sent -> accepted。
  - accepted desktop size = 实际 sent normalized target。
  - requested/sent orientation 与该次 displayProfileGeneration 的真实 rotation 一致，画面不侧转或倒转。
  - normalized target 与稳定 Surface 的差异符合 diagnostics 中记录的 clamp/alignment 规则。
- 不支持 Display Control：
  - 若接口返回 Deferred/Unsupported/Failed/Unchanged，不进入 2 秒等待，立即保持 fallback。
  - 只有已经 Sent 但未收到目标帧时，才在发送后 2 秒内 timeout/fallback。
  - 最后可用画面继续显示，不永久黑屏。
- 连续 resize：
  - 最后 targetGeneration 生效。
  - 旧请求结果不能覆盖新目标。
  - TargetPresented 的 frame 必须同时匹配当前 surfaceGeneration、当前 targetGeneration 和 sent normalized target 尺寸；任一不符只能作为旧帧/fallback 处理。
- 画面：
  - 比例正确，无非预期拉伸。
  - viewport 与实际黑边一致。

### 12.8 输入和触控

- FreeRDP OHOS 坐标纯函数测试覆盖四角和中心，整数舍入误差不超过 1px。
- 端到端使用物理鼠标或自动输入和远端坐标探针；误差不超过 2px 或 0.5%，取较宽松者。
- 手指触控验证目标命中和手势结果，不用肉眼判定 2px 精度。
- 黑边 pointer down：pointerDownSentCount/pointerClickSentCount delta = 0，blackBarRejectedDownCount delta = 1；hover/move 计数不参与该断言。
- 当前 surfaceGeneration 尚无有效 geometryRevision 时点击：down/click sent delta = 0，staleGeometryRejectedDownCount delta = 1。
- fallback 在当前 Surface 成功 present 并发布新 geometryRevision 后，旧 desktop 上的有效区域点击可恢复；不能因 target resize 失败而永久拒绝输入。
- 单击、长按、拖动、双指滚动通过。
- 物理鼠标点击、hover、滚轮通过。
- 物理键盘字母、组合键、方向键、Enter、Backspace 通过。
- 旋转/Surface invalidation、失焦、切后台和断开均使 releaseAllInputCount 增加，activePointerButtons/buttonsDown = 0、keysDown = 0；重复调用仍为 0，远端无粘键/粘鼠标按钮。

### 12.9 虚拟键盘

- 进入远程会话、连接成功、XComponent 获得 Native focus、鼠标操作和非文本 touch-down 后，虚拟键盘均保持隐藏。
- 只有 touchscreen-like 直接触摸与远端 I-beam 候选光标在限定短窗口内同时成立时，Native 才显示键盘；单独 focus、单独 I-beam hover 或单独触摸均不得触发。
- 系统返回/完成键隐藏键盘后，触摸非文本区域不恢复；再次触摸远端文本候选可恢复。
- ArkTS 组件树不存在隐藏 TextInput、键盘按钮或键盘状态；N-API 不存在 IME show/hide/request 或逐字符入口。
- 中文拼音：预编辑不重复发送，选词后只提交一次。
- 英文、数字、退格、Enter 正常。
- IME 打开且 XComponent 保持焦点时，中文选词只提交一次；物理键盘、方向键和修饰键继续由 XComponent 到达远端且不双发。
- XComponent 失焦并关闭 IME 后，Native preview、pending touch 和按键状态完成清理；再次获焦本身不 attach/show，必须再次触摸远端文本候选。
- 键盘显示/隐藏 20 次，Surface width/height 不变化。
- IME 显示/隐藏不触发 RDP reconnect 或动态分辨率请求。

### 12.10 稳定性

- 远程会话持续 30 分钟。
- 期间执行 10 次旋转、20 次窗口 resize、20 次键盘动作/系统隐藏/再次动作循环、20 次 XComponent focus/blur 和 20 次前后台切换。
- 无崩溃、永久黑屏、输入失配、粘键或持续增长的 pending resize；结束时 activePointerButtons/buttonsDown/keysDown 均为 0。

## 13. 诊断字段、验收快照和日志

Native 新增 getRdpDiagnostics()，只返回 RDP/Surface/渲染/输入状态。Native RdpSession 的 sessionId 是权威值，ArkTS 只镜像：

~~~text
session:
  nativeSessionId, phase, connectCount, disconnectCount
surface:
  ready, surfaceGeneration, widthPx, heightPx
  createdCount, changedCount, destroyedCount
geometry:
  surfaceGeneration, targetGeneration, geometryRevision
  desktopWidthPx, desktopHeightPx
  viewportX, viewportY, viewportWidth, viewportHeight
resize:
  resizeApi, state, requestedTargetGeneration
  displayProfileGeneration, requestedOrientation, sentOrientation
  requestedWidth, requestedHeight
  normalizedWidth, normalizedHeight
  requestedCount, sentCount, deferredCount
  unchangedCount, unsupportedCount, failedCount, legacyUnknownCount
  acceptedCount, timeoutCount, fallbackCount, pendingMs
render:
  requestedGraphicsMode, negotiatedCodec, renderOwner
  presentationKind, lastPresentedSurfaceGeneration, lastPresentedTargetGeneration
  lastFrameDesktopWidth, lastFrameDesktopHeight
  presentedFrames, droppedFrames, lastPresentedAt
input:
  geometryRevision, pointerDownSentCount, pointerClickSentCount
  blackBarRejectedDownCount, staleGeometryRejectedDownCount
  keySentCount, keysDown, activePointerButtons, buttonsDown
  releaseAllInputCount
~~~

ArkTS 另提供 buildAcceptanceSnapshot()，在验收边界合并 Native 诊断与 UI/产品状态；Native 不读取 layout 或 capability：

~~~text
build:
  buildId, product, module, buildMode, bundleName
device:
  capabilitySnapshotId, sourceDeviceType, remoteControlServer
window:
  widthBreakpoint, layoutMode, widthVp, heightVp
  rotation, densityDPI
  xDPI, yDPI  # P1 未启用时为 null/absent
ui:
  fontScale, locale, inputMode
  indexInstanceId, controllerInstanceId, sessionPageInstanceCount
rdp:
  <getRdpDiagnostics result>
~~~

状态变化日志：

~~~text
RDP_SURFACE sid=... surfaceGeneration=... event=created|changed|destroyed size=...
RDP_RESIZE sid=... targetGeneration=... event=requested|sent|deferred|unchanged|unsupported|failed|legacy_unknown|accepted|timeout|fallback target=...
RDP_GEOMETRY sid=... geometryRevision=... desktop=... viewport=...
RDP_INPUT sid=... geometryRevision=... event=down_sent|black_bar_rejected|stale_rejected
APP_CAPABILITY snapshotId=... sourceDeviceType=... remoteControlServer=...
APP_LAYOUT breakpoint=... mode=...
~~~

验收判定以 diagnostics/snapshot 为主，不依赖 release 构建可能过滤的 private hilog。需要保留的状态事件使用明确、无敏感字段的 public 安全日志；禁止逐帧打印，也禁止记录密码、访问码、剪贴板内容、输入文本和证书敏感信息。

## 14. 架构合理性门禁

### 14.1 静态边界

tools/check_tablet_architecture.ps1 分两阶段：

- baseline：Commit A/B 迁移期间记录当前违规 allowlist，只允许减少，不允许新增。
- strict：Commit B 完成并确认 NativeRdpGateway/RdpSessionPage 已落地后启用，目标规则全部强制。

可自动化规则：

| 规则 | 自动检查 |
|---|---|
| 文档先行门禁 | 本次 diff 只要修改第 10 节覆盖的代码/config/脚本/子模块范围，就必须同时包含本文实施台账中的子级 Change ID；架构/ABI/manifest/子模块变更必须引用对应设计版本，未被现有设计精确覆盖时还必须先包含设计段落 diff |
| Native import allowlist | harmony/app/entry/src/main/ets/**/*.ets 中，libentry.so 只允许 harmony/app/entry/src/main/ets/rdp/NativeRdpGateway.ets import |
| XComponent allowlist | XComponent 构造只允许 harmony/app/entry/src/main/ets/components/session/RdpSessionPage.ets |
| Controller 创建点 | harmony/app/entry/src/main/ets/**/*.ets 只有一个 new XComponentController 创建点；运行时身份另由 snapshot 验证 |
| 布局无设备判断 | harmony/app/entry/src/main/ets/adaptive 及 components/home、SettingsPage 禁止 import deviceInfo，也禁止调用能力探测 API |
| 能力读取 allowlist | deviceInfo.deviceType 只允许 harmony/app/entry/src/main/ets/capability/DeviceCapabilityPolicy.ets 的系统输入适配器读取；acceptance snapshot 只能序列化已存在的 capability snapshot |
| 组件无服务调用 | harmony/app/entry/src/main/ets/components 中禁止调用 NativeRdpGateway、ensureXrdpServerStarted 或录屏权限请求 |
| 社区变更路径 | 本次 FreeRDP/xrdp diff 默认只能位于各自 OHOS 平台目录；通用 core 变更必须显式 waiver |

以下项目不能靠全仓关键词可靠判断，列入强制 code-review checklist：

- showSession 是否确实位于 Compact/Expanded Builder 外层。
- N-API 是否只做注册、参数转换和转发，是否夹带 resize/DPI/几何/IME 算法。
- FreeRDP/xrdp 改动是否为通用 OHOS 问题，而不是 MuHub 产品策略。
- 响应式分支是否共享同一状态和回调。

禁止用“全仓是否出现 tablet/Compact 文字”代替边界检查；脚本只检查明确路径、import/call allowlist 或本次 diff。

### 14.2 自动测试

最少覆盖：

~~~text
layoutModeForWidthBreakpoint(WIDTH_XS) = compact
layoutModeForWidthBreakpoint(WIDTH_SM) = compact
layoutModeForWidthBreakpoint(WIDTH_MD) = compact
layoutModeForWidthBreakpoint(WIDTH_LG) = expanded
layoutModeForWidthBreakpoint(WIDTH_XL) = expanded

capabilitiesForDeviceType("tablet").remoteControlServer  = unavailable
capabilitiesForDeviceType("2in1").remoteControlServer    = available
capabilitiesForDeviceType("unknown").remoteControlServer = unavailable
capabilitiesForDeviceType("").remoteControlServer        = unavailable
~~~

能力路由测试（D-01 采用推荐值时）：

- tablet 的 remoteControl 初始路由回退。
- tablet 调用 XrdpServerController.start/diagnostics 不触发 Native mock。
- tablet 的 RemoteFilesDirectory/Downloads drive 初始化不被能力策略拦截。
- 2in1 保持原行为。

Native 纯逻辑测试：

- contain viewport：等比、宽黑边、高黑边、奇数尺寸、零尺寸保护。
- App Native：surfaceGeneration/targetGeneration/geometryRevision、三条件 TargetPresented、fallback present 后输入恢复、present 前 stale geometry 拒绝。
- FreeRDP OHOS：ohos_pointer 的四角、中心、clamp 和整数舍入。
- resize：trailing debounce、Sent、Deferred、Unsupported、Sent timeout、normalized target、last-write-wins。

### 14.3 实施后必须存在的可执行命令

当前仓库尚无 ArkTS test 源目录、Native test target，build_hap.bat 也尚未参数化；因此本文不把它们误报为“已通过”。Commit A～E 必须按文件清单补齐以下命令，并以退出码 0 为通过：

~~~powershell
# ArkTS 纯策略/能力测试
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_tablet_arkts_tests.ps1

# App Native geometry/resize + FreeRDP OHOS pointer 纯逻辑测试
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_tablet_native_tests.ps1

# 架构边界；Commit A/B 先 baseline，Commit B 完成后 strict
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_tablet_architecture.ps1 -Mode baseline
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_tablet_architecture.ps1 -Mode strict

# 同一 default product/entry target 的两种 buildMode
harmony\app\build_hap.bat debug
harmony\app\build_hap.bat release
~~~

build_hap.bat 内部统一调用 assembleHap，并显式传 product=default、module=entry@default、buildMode=debug|release。每次构建生成 buildId 元数据并指定本次 canonical signed HAP；单包判定基于 product/module/buildMode/profile，不统计 outputs 中可能残留的旧文件或 unsigned 中间产物。

### 14.4 文件规模

Native C++ 遵循 docs/ohos-native-cpp-module-guidelines.md；ArkTS 数值是本文为本次改造设置的评审预算，不是该 Native 规范的原文要求：

- 本文预算：新 ArkTS 页面/协调器目标不超过 500 行。
- 本文预算：新 ArkTS 展示组件目标不超过 300 行。
- Native 规范：新 C++ 源文件目标不超过 500 行，硬上限 1000 行。
- 已经超大的 AVC 文件不得继续承载共享几何或 resize 算法。

### 14.5 分阶段合并门禁

每个实现 PR：

1. 第一处代码 diff 产生前，本文已存在对应子级 Change ID，设计状态为 DesignReady；PR 描述引用设计章节和验收 ID。
2. 计划/实际代码文件、公共 ABI、兼容和回退与台账一致；有偏差时先产生并审阅文档 diff，再继续代码，并在 PR 中记录此次设计同步。
3. 受影响的 ArkTS/Native 纯逻辑测试成功。
4. Commit A/B 使用 baseline，Commit B 之后使用 strict 架构检查，文档先行门禁始终启用。
5. Debug 构建成功。
6. 无未说明的 FreeRDP/xrdp 通用 core 修改。
7. 对应阶段的 smoke 证据；不要求每个小 PR 都跑 30 分钟完整矩阵。
8. 合并前台账至少回写为 Implemented；若声称能力完成，必须为 Verified 并同步当前事实文档。

启用 tablet/rotation/split、修改 Native/FreeRDP ABI 或形成发布候选时，额外要求：

1. Release 构建成功。
2. canonical HAP 的 bundleName/module/product/buildMode/签名身份检查成功。
3. 对应设备与会话真机矩阵成功。
4. 发布候选执行第 12.10 节 30 分钟稳定性。
5. 文档只在功能真实通过后更新状态。

## 15. 工程与 FreeRDP/xrdp OHOS port 同步边界

当前 .gitmodules 将 FreeRDP 和 xrdp 都固定到 xiaomu120413 fork 的 ohos-port 分支。为了后续与工程和上游同步，改动分四层：

| 层 | 归属 | 可包含 | 不可包含 |
|---|---|---|---|
| ArkUI/HAP | MuHub 工程 | 布局、能力过滤、权限、路由、IME 宿主 | RDP 协议语义 |
| App Native adapter | MuHub 工程 | Surface 生命周期、最终 viewport/generation、诊断、N-API 转发 | 复制 FreeRDP 的远端坐标映射 |
| harmony/third_party/FreeRDP client/OHOS | OHOS port fork/上游候选 | 版本化 Display Control result、DPI、orientation、OHOS 输入映射；本工程同步升级时不保留旧resize ABI | MuHub 名称、页面断点、产品能力 |
| harmony/third_party/xrdp | OHOS port fork/上游候选 | 通用的 OHOS 服务端、编码、会话或平台问题 | tablet 是否显示/启动 XRDP 的产品策略 |

同步规则：

- tablet 的 XRDP 隔离只修改 HAP 能力、路由、权限调用和服务入口，不为该产品策略修改 xrdp 子模块。
- FreeRDP/xrdp 通用 OHOS 问题分别独立提交，先写通用问题和协议行为。
- 保持 OHOS 平台文件范围，避免修改通用 core。
- 提供无 MuHub UI 依赖的测试/日志。
- 工程侧通过稳定接口调用，不在工程和社区各复制一套算法。
- 改动先落对应 fork 的 ohos-port 分支并更新父仓 submodule SHA，使工程有可复现同步点；推动官方上游是后续独立流程，不要求工程等待上游合入。
- 上游最终接受同功能实现后，在 OHOS port 中做对齐并更新父仓 SHA，不长期保留重复私有实现。

## 16. 与现有文档的关系

| 文档 | 继续作为事实来源 | 本文覆盖/修正 |
|---|---|---|
| settings-desktop-current-interactions.md | 设置状态、回调和现有交互 | 旧 1000vp 布局断点不再作为适配依据 |
| settings-desktop-implementation-checklist.md | 历史实施记录和可复用回归项 | 不再是现行实施计划；不再采用“桌面优先、窄屏兜底”的总体策略 |
| settings-desktop-current-capability.svg | 信息架构参考 | 不作为尺寸和断点规范 |
| home-settings-desktop-design-v3.svg | Expanded 视觉方向 | 当前代码结构和本文架构优先 |
| freerdp-ohos-feature-matrix.md | 当前 RDP channel/codec/fallback 状态 | resize 严格等待风险是发布门禁，修复后才更新矩阵 |
| freerdp-ohos-validation-baseline.md | 构建、运行时同步、HAP 和通用 RDP 回归 | 本文增加 tablet/2in1 布局、输入、旋转和 IME 矩阵 |
| ohos-native-cpp-module-guidelines.md | Native 模块所有权和文件规模 | 本文必须服从该规范 |

本文只作为平板/2in1 目标架构、修改计划和适配验收基线。当前运行能力仍以代码和 freerdp-ohos-feature-matrix.md 为准；Native 所有权仍以 ohos-native-cpp-module-guidelines.md 为准；通用构建/RDP 回归仍以 freerdp-ohos-validation-baseline.md 为准。若旧设置文档与本文在响应式断点、侧栏固定宽度或触控热区上冲突，以本文为平板适配来源。

## 17. 官方依据

- HarmonyOS 官方响应式示例：[ResponsiveLayout README](https://gitee.com/harmonyos_samples/ResponsiveLayout/blob/master/README.en.md)
- HarmonyOS 官方 Native XComponent 示例：[NdkXComponent README](https://gitee.com/harmonyos_samples/ndk-xcomponent/blob/master/README.md)
- [Navigation 组件与页面路由](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/arkts-navigation-navigation)
- [Navigation 分栏开发](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/arkts-navigation-split-mode)
- [GridRow/GridCol 栅格布局](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-V13/arkts-layout-development-grid-layout-V13)
- [设备兼容规则](https://developer.huawei.com/consumer/cn/doc/doccenter-architecture/device-compatible)
- [应用 UX 体验标准：vp/fp、点击热区等](https://developer.huawei.com/consumer/cn/doc/design-guides/ux-guidelines-overview-0000001760867048)
- [焦点导航](https://developer.huawei.com/consumer/cn/doc/design-guides/hmi-focus-0000001748650376)
- [HarmonyOS 多设备设计入口](https://developer.huawei.com/consumer/cn/design)
- [窗口沉浸式和安全区](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/arkts-develop-apply-immersive-effects)
- [应用配置文件概述](https://developer.huawei.com/consumer/cn/doc/doccenter-getting-started/application-configuration-file-overview-stage)

同时以本机 API 22 SDK 声明为实施校验源：

- $DEVECO_SDK_HOME/default/openharmony/ets/api/@ohos.arkui.UIContext.d.ts
- $DEVECO_SDK_HOME/default/openharmony/ets/component/enums.d.ts
- $DEVECO_SDK_HOME/default/openharmony/toolchains/modulecheck/configuration.json
- $DEVECO_SDK_HOME/default/openharmony/toolchains/modulecheck/module.json

## 18. 完成定义

只有以下条件全部满足，才能称为“平板适配完成”：

1. 一个 HAP、bundleName 和升级身份不变。
2. tablet、2in1 均能安装和启动。
3. 600～1440vp 布局矩阵通过，839/840 状态连续。
4. 1.75 系统字体、中英文无裁切且所有内容可达。
5. D-01 已形成书面决策；若采用推荐值，tablet 的 XRDP 在初始化、服务、路由、展示四层均隔离。
6. XComponent 旋转、分屏和 resize 不重连、不永久黑屏。
7. GDI、AVC420、AVC444 的画面和输入使用同一 viewport。
8. 黑边、resize 过渡期和四角坐标验收通过。
9. 虚拟键盘不改变 Surface，中文提交不重复。
10. 物理键鼠、触控、焦点导航和失焦释放通过。
11. 静态架构检查、纯逻辑测试、Debug/Release 构建通过。
12. 截图、diagnostics、hilog、设备矩阵形成可复核证据包。
13. 所有实现提交都有 DesignReady 先行记录；实施台账、实际代码文件、设计偏差、验收 ID、证据和当前事实文档已经同步。
