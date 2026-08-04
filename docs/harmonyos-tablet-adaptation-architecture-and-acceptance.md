# MuHub HarmonyOS 单包平板适配架构、修改清单与验收方案

> 状态：架构基线，尚未实施
> 文档版本：1.2
> 审阅日期：2026-08-04
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
| F. 输入能力 | 触控阈值、黑边拒绝、releaseAllInput、显式 IME/中文输入 | B 的 geometry 可用 | 坐标、手势、键鼠、IME 和失焦释放测试 |
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

一个 HAP 同时服务 tablet 和 2in1 时，xrdp.hnp 以及 module.json5 中声明的录屏权限仍会物理存在于同一个安装包中。若 D-01 采用本文推荐值，运行时需要保证：

- tablet 不启动 XRDP。
- tablet 不调用 XRDP N-API。
- tablet 不请求录屏权限。
- tablet 不创建 XRDP 状态卡和设置入口。
- tablet 即使收到旧路由或非法路由也不能进入 XRDP 页面。

但不能在同一个 HAP 中按设备物理删除 HNP 和权限声明。若未来要求安装包内容也按设备裁剪，就与“一个包”约束冲突，必须重新决策，不能靠运行时代码解决。

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
│     ├─ 浮动会话工具栏
│     └─ 显式 IME 输入入口
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
3. 会话页面可见且 Surface ready 时，createdCount - destroyedCount = 1；退出会话后为 0。
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
- 从实际 display rotation 得到 RDP orientation，不再永久写死 landscape；orientation 与 resize 结果走版本化兼容接口。
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
├─ 浮动会话工具栏
└─ IME 输入宿主
~~~

showSession 必须是 Index 最外层分支。Home/Settings 的 Compact/Expanded 分支只在非会话状态下存在。会话内工具栏可以根据可用宽度重排，但不能条件替换 XComponent 节点。

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
       LegacyUnknown -> 已调用旧 BOOL ABI，但不进入严格等待
  -> TargetPresented（匹配当前 surfaceGeneration、targetGeneration、normalized target）
       -> 发布新 geometryRevision
  -> SentTimeout <= 2s -> 回到 fallback，保留最后可用帧
~~~

关键要求：

- 现有 freerdp_ohos_session_resize 的 BOOL ABI 保留；新增版本化 _ex/result struct 区分 Sent、Deferred、Unchanged、Unsupported、Failed，并返回 normalized/sent target。
- 只有 Sent 才进入目标帧等待。
- Deferred、Unsupported、Failed、Unchanged 和 LegacyUnknown 不等待 2 秒，也不丢帧。
- App runtime 优先加载 `_ex`；只找到旧 BOOL ABI 时可以保持旧客户端兼容连接，但结果标为 LegacyUnknown、继续 fallback，不能把 `TRUE` 猜成 Sent。发布候选的旋转/分屏门禁要求 diagnostics 显示 `resizeApi=ex`。
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

- 默认触摸远程桌面不自动弹软键盘。
- 会话工具栏提供明确的键盘按钮。
- 键盘入口聚焦 ArkUI TextInput 输入宿主；TextInput 获焦期间 XComponent key callback 不再是按键来源，不能假定两边同时收事件。
- TextInput preview text 保持本地组合态；onDidInsert 只把已提交文本交给 sendCommittedText，不把拼音预编辑串重复发送。
- onDidDelete 转成退格/删除平台键，onSubmit 转成 Enter；TextInput.onKeyEvent 只转发方向键、修饰键等非打印物理键，打印字符仍以 onDidInsert 为准，避免双发。
- N-API 暴露 sendCommittedText 和 sendPlatformKey，分别薄转发到现有 RdpSession::SendCommittedText/SendPlatformKey。
- 进入会话保存当前 KeyboardAvoidMode，设置为 NONE；退出时恢复。
- IME 以 overlay 方式覆盖，会话 Surface 尺寸不能因键盘显示/隐藏而变化。
- 关闭 IME 时清空输入宿主，并用稳定 focus id 把焦点还给 XComponent；随后物理键盘继续由 XComponent callback 处理。
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
| 不改 | harmony/app/entry/src/main/module.json5（hnpPackages/requestPermissions） | 单包条件下保留，通过能力策略避免 tablet 调用 | tablet 冷启动无 XRDP/录屏行为 |

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
| P0，新文件 | harmony/app/entry/src/main/ets/rdp/NativeRdpGateway.ets | 成为 libentry.so 唯一 import 点；薄封装 connect/disconnect/callback/输入/XRDP/diagnostics | components 和 controllers 不直接 import libentry.so |
| P0，新文件 | harmony/app/entry/src/main/ets/rdp/RdpClientController.ets | 负责连接、断开、Native 回调注册、sessionId 和会话状态；通过回调/快照通知 Index | 不 import UI；同一会话 connectCount 为 1 |
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
| P0，新文件 | harmony/app/entry/src/main/ets/components/session/RdpSessionPage.ets | 从 Index 移出会话 Builder；接收已有 Controller；固定 Stack + XComponent + overlay；用 TextInput onDidInsert/onDidDelete/onSubmit/onKeyEvent 实现 IME，并处理 KeyboardAvoidMode 与焦点恢复 | 切布局/工具栏/字体不重建 Controller 或 session；中英文不双发 |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets | XComponentController 只在这里创建一次；showSession 保持最外层分支 | 静态只有一个创建点，运行时 controllerInstanceId 保持不变 |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets、harmony/app/entry/src/main/ets/rdp/RdpClientController.ets | 会话开始及 display change 时用 display.getDefaultDisplaySync() 读取 rotation 和本地 densityDPI，形成 P0 DisplayProfile，经 Controller 转发；display.on/off('change') 成对 | 旋转后 profile generation 更新，页面消失后无残留监听 |
| P1 | harmony/app/entry/src/main/ets/pages/Index.ets、harmony/app/entry/src/main/ets/rdp/RdpClientController.ets | DisplayProfile 再加入经校验的 xDPI/yDPI 和远端 scale 策略 | 不与 P0 旋转提交混合；无效值有明确 fallback |
| P0 | harmony/app/entry/src/main/cpp/input/xcomponent_input_registration.cpp | 不再无条件 SetNeedSoftKeyboard(true)；MakeNativePointer 默认不允许 clamp | 普通触摸不误弹键盘；down/click/hover 不夹到边缘 |
| P0 | harmony/app/entry/src/main/cpp/napi/napi_exports.cpp、harmony/app/entry/src/main/cpp/napi/napi_exports.h、harmony/app/entry/src/main/cpp/types/libentry/Index.d.ts | 增加 sendCommittedText、sendPlatformKey、surface orientation/input density、RDP diagnostics 薄接口；P1 才加 xDPI/yDPI/scale | 参数校验后转发，不承载算法 |
| P0 | harmony/app/entry/src/main/cpp/input/xcomponent_mouse.cpp、harmony/app/entry/src/main/cpp/input/xcomponent_touch_gesture.cpp | down/click/hover 的 allowClamp=false；仅已开始拖动的 move/up 可有限 clamp；原始 px 阈值改为 density 相关 | 不同密度手感一致；黑边不误触远端边缘 |
| P0 | harmony/app/entry/src/main/cpp/input/xcomponent_input_bridge.h、harmony/app/entry/src/main/cpp/input/xcomponent_input_internal.h、harmony/app/entry/src/main/cpp/session/rdp_session_input.cpp | 新增幂等 releaseAllInput：发送活动左/右/中/触控拖动 up，再释放所有键，最后清本地状态；在 Surface invalidation/blur/background/disconnect 调用 | diagnostics 的 buttonsDown/keysDown 均归零，远端无粘键/粘按钮 |
| P0 | harmony/app/entry/src/main/ets/pages/Index.ets、harmony/app/entry/src/main/ets/rdp/NativeRdpGateway.ets、harmony/app/entry/src/main/cpp/napi/napi_exports.cpp、harmony/app/entry/src/main/cpp/types/libentry/Index.d.ts | onPageHide/aboutToDisappear/断开调用 releaseAllInput；N-API 只转发 | 前后台和断开路径覆盖，不只依赖 XComponent blur |

若 RdpSessionPage 超过 300 行，再拆 SessionToolbar 或 SessionImeHost；首轮不预先拆出大量小文件。

### 10.7 Native resize、几何和输入

| 优先级 | 文件 | 修改点 | 文件完成条件 |
|---|---|---|---|
| P0，新文件 | harmony/app/entry/src/main/cpp/surface/display_geometry.h/.cpp | 唯一保存 surfacePx、desktopPx、最终 viewportPx、surfaceGeneration、geometryRevision；替换 SurfaceBridge 现有 viewport 字段；只负责 contain 几何和代次，不复制远端坐标映射 | GDI/AVC/input/diagnostics 共用同一对象；边界单测通过 |
| P0，新文件 | harmony/app/entry/src/main/cpp/session/rdp_display_resize_coordinator.h/.cpp | 从 N-API 文件移出；在 session/orchestration 层实现 trailing debounce、generation、明确结果状态、2秒 Sent 超时和 fallback，避免 surface 层反向依赖 session | unsupported/deferred/legacy/timeout 不黑屏 |
| P0 | harmony/app/entry/src/main/cpp/CMakeLists.txt | 注册新增源文件和可测试纯逻辑 | HAP 构建和测试 target 通过 |
| P0 | harmony/app/entry/src/main/cpp/napi/native_bridge_context.cpp | Surface 回调只转发；检查 resize 结果；非 Sent 不进入严格等待 | 文件不新增算法，目标不继续膨胀 |
| P0 | harmony/app/entry/src/main/cpp/surface/surface_bridge.h/.cpp | 删除自身 viewportX_/Y_/Width_/Height_ 副本，委托 display_geometry；Surface changed 后先重算旧 desktop 在新 Surface 的 fallback viewport | fallback present 后输入恢复；Bridge 与 diagnostics 不出现第二份几何 |
| P0 | harmony/app/entry/src/main/cpp/channels/rdpgfx_pipeline.h/.cpp | 把 viewport 发布传入 AVC420/AVC444 | 三渲染路径同一接口 |
| P0 | harmony/app/entry/src/main/cpp/surface/avc420_gpu_compositor*.cpp、harmony/app/entry/src/main/cpp/surface/avc444_gpu_compositor*.cpp | present 成功后发布实际 contain viewport；不再自留第二套输入几何 | 三种编码的 viewport 诊断与画面一致 |
| P0 | harmony/app/entry/src/main/cpp/session/rdp_session_input.cpp | 校验当前 geometryRevision 和黑边；通过后把 local pointer + viewport 交给 FreeRDP OHOS 映射；仅拖动 move/up 可有限 clamp | 黑边 down/click 不进入 FreeRDP |
| P0 | harmony/app/entry/src/main/cpp/freerdp/freerdp_runtime.h/.cpp | 优先动态加载 resize `_ex`；缺失时可调用旧 BOOL ABI但返回 App 级 LegacyUnknown，不猜测 Sent | diagnostics 暴露 resizeApi=ex/legacy；发布旋转矩阵只接受 ex |
| P0 | harmony/app/entry/src/main/cpp/session/rdp_session_channels.h/.cpp、harmony/app/entry/src/main/cpp/session/rdp_session_core.h/.cpp | 把内部 bool resize 链改为结构化结果并传给 coordinator；不通过日志字符串推断状态 | Deferred/normalized target 可端到端进入 diagnostics 和状态机 |
| P0，兼容扩展 | harmony/third_party/FreeRDP/client/OHOS/ohos_session.h、harmony/third_party/FreeRDP/client/OHOS/ohos_session_display.c、harmony/third_party/FreeRDP/client/OHOS/ohos_display.c | 保留原 BOOL ABI；新增带 structSize/version 的 resize `_ex` request/result，P0 返回 Sent/Deferred/Unchanged/Unsupported/Failed、normalized target 和真实 orientation | 新调用优先 `_ex`，旧 SDK/旧调用仍可连接；禁止解析日志推断状态 |
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
6. 先完成 FreeRDP resize `_ex` 兼容接口（含真实 orientation）、Native resize/fallback/geometry 和输入过渡期；用测试和可控 Surface change 验证不黑屏。
7. 再在未发布适配候选中启用 tablet deviceTypes、auto_rotation、split、字体 configuration，并临时把最小窗口降到 600×480以允许真机测试。
8. 改首页 Compact/Expanded。
9. 改设置 Compact/Expanded。
10. 清理固定高度、字体裁切、Icon 热区和焦点顺序。
11. 补显式 IME、sendCommittedText 和 density 触控阈值。
12. 完成 600×480 与 1.75 字体验收后，把 600×480 固化为发布 manifest 值；若产品另有更高下限，必须有独立产品决策，不能代替布局修复。
13. 最后以 P1 独立评估远端 auto DPI、手动 100/140/180 和增强手势。

建议提交边界：

- Commit A：纯策略、基础 diagnostics、测试，无 UI 行为变化。
- Commit B：会话页/Controller 抽取，无 UI 行为变化。
- Commit C：XRDP 能力隔离。
- Commit D：FreeRDP resize `_ex` + orientation 兼容 ABI。
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
| TAB-B | B 会话底座 | 第 8.3、9.2、9.3、10.6、10.7 节 | AC-XC、AC-RESIZE、AC-INPUT | DesignReady | NotStarted | `_ex` ABI 与旧 BOOL 兼容评审通过 |
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
  - 若接口返回 Deferred/Unsupported/Failed/LegacyUnknown，不进入 2 秒等待，立即保持 fallback。
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

- 普通触摸 XComponent 不自动弹键盘。
- 工具栏键盘按钮可显式打开/关闭。
- 中文拼音：预编辑不重复发送，选词后只提交一次。
- 英文、数字、退格、Enter 正常。
- IME 打开且 TextInput 获焦时，物理键盘打印字符只提交一次；方向键、修饰键等非打印键仍可到达远端。
- 关闭 IME 后 focus 返回 XComponent，随后物理键盘输入继续正常。
- 键盘开关 20 次，Surface width/height 不变化。
- IME 显示/隐藏不触发 RDP reconnect 或动态分辨率请求。

### 12.10 稳定性

- 远程会话持续 30 分钟。
- 期间执行 10 次旋转、20 次窗口 resize、20 次 IME 开关和 20 次前后台切换。
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
| harmony/third_party/FreeRDP client/OHOS | OHOS port fork/上游候选 | 兼容的 Display Control result、DPI、orientation、OHOS 输入映射 | MuHub 名称、页面断点、产品能力 |
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
