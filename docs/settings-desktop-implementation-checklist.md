# 桌面设置页实施验收清单

> 本文描述 HarmonyOS 应用的桌面布局，不是已删除的 Web Demo。页面路径已按 common HSP 更新；分阶段实施记录保留，后文较新的日期记录优先于早期布局描述。

本文档用于实施 `docs/settings-desktop-current-capability.svg` 和 `docs/settings-desktop-current-interactions.md` 对应的当前能力版桌面设置页。目标是明确每个文件的修改点、验收点和风险点，避免开发过程中引入当前没有的能力。

## 1. 实施范围

本次只改设置相关 UI 和现有回调接线，不改 native/RDP 协议逻辑。

包含：

- 主页设置入口视觉和状态点。
- 设置页桌面布局：左侧导航 + 右侧内容。
- 设置概览页。
- 基础设置页桌面重排。
- 远控设置页桌面重排。
- 项目帮助页分组重排。
- 当前已有状态的视觉表达。

不包含：

- 日志导出。
- 更新中心。
- 高级诊断中心。
- 多共享目录管理。
- 证书策略切换 UI。
- 安全策略设置页。
- 剪贴板、麦克风、定位权限的独立设置页。
- 远程会话列表管理。

## 2. 文件修改点

### `harmony/app/common/src/main/ets/pages/Index.ets`

修改点：

- 调整主页设置按钮视觉。
- 增加设置入口状态点。
- 保留点击前刷新逻辑：
  - `refreshScreenRecordingPermissionState()`
  - `refreshXrdpServerDiagnostics()`
  - `showSettings = true`
- 不改连接表单字段和 `connectNative()` 逻辑。

状态点规则：

| 条件 | 展示 |
| --- | --- |
| `xrdpServerBusy === true` | 蓝色处理中 |
| `!screenRecordingPermissionGranted` | 黄色需要授权 |
| `xrdpServerRunning === false` | 灰色未启动 |
| `xrdpServerRunning && screenRecordingPermissionGranted` | 绿色可用 |

验收点：

- 点击设置按钮仍能进入设置页。
- 点击前仍会刷新录屏权限和 xrdp 状态。
- 连接表单不受影响。
- 状态点在 busy、未授权、未启动、可用四类状态下颜色正确。

风险点：

- 设置按钮缩小后触控/点击区域不足。按钮建议不小于 `40vp x 40vp`。
- 状态点颜色和深色模式对比度不足。

### `harmony/app/common/src/main/ets/components/SettingsPage.ets`

修改点：

- 保留 `pageName` 作为路由状态。
- 增加桌面壳布局：左侧导航 + 右侧内容。
- 新增或改造设置概览页。
- 左侧导航项：
  - 设置概览
  - 基础设置
  - 远控设置
  - 项目帮助
- 继续把当前 props 和回调透传给子页面：
  - `remoteAccessCode`
  - `remoteAccessCodeGateEnabled`
  - `screenRecordingPermissionGranted`
  - `xrdpServerRunning`
  - `xrdpServerState`
  - `xrdpServerPort`
  - `xrdpServerMessage`
  - `xrdpServerBusy`
  - `onRemoteAccessCodeGateChange`
  - `onRemoteAccessCodeRegenerate`
  - `onRequestScreenRecordingPermission`
  - `onRefreshScreenRecordingPermission`
  - `onRefreshXrdpServerStatus`
  - `onStartXrdpServer`
  - `onOpenRemoteFilesDirectory`

验收点：

- 设置页默认进入概览页。
- 左侧导航能切换四个页面。
- 子页面返回行为清晰：桌面下优先返回概览或直接通过左侧导航切换，不应关闭设置页。
- 关闭按钮仍调用 `onClose()` 回到主页。
- 远控页收到的 props 和回调不丢失。

风险点：

- 桌面布局和当前手机式下钻混在一起导致路由行为混乱。
- `appearanceMode` 更新后概览页没有同步。
- 左侧导航长文案在小窗口溢出。

### `harmony/app/common/src/main/ets/components/settings/SettingsUi.ets`

修改点：

建议新增公共组件或 Builder：

- `SettingsDesktopShell`：桌面设置壳。
- `SettingsDesktopNavItem`：左侧导航项。
- `SettingsStatusChip`：状态标签。
- `SettingsPanel`：右侧分组面板。
- `SettingsKeyValueRow`：键值状态行。
- `SettingsCompactButton`：设置页内次级操作按钮。

调整：

- 降低桌面 hover 的上移动画强度。
- 减弱卡片阴影，桌面工具页以边框和浅背景为主。
- 保留现有 `SettingsTheme` 色值体系，必要时只补状态色 helper。

验收点：

- 新组件同时支持浅色和深色。
- 长文本能省略或换行，不挤压按钮。
- hover、pressed、disabled 状态清晰。
- 组件不引入当前未用的图标或资源依赖。

风险点：

- ArkUI 组件抽象过度，导致状态传递复杂。
- 现有 `SettingsListItem` hover 上移过大，桌面端可能显得不稳。
- 深色模式下浅色状态 chip 对比不够。

### `harmony/app/common/src/main/ets/components/settings/RemoteControlSettingsPage.ets`

修改点：

- 重排为四个面板，顺序固定：
  1. xrdp 服务
  2. 录屏权限
  3. 验证码门禁
  4. 远程文件
- 保留现有状态同步逻辑：
  - `aboutToAppear()`
  - `refreshScreenRecordingState()`
  - `refreshXrdpServerStatus()`
- 保留现有行为：
  - 服务启动/刷新：`onStartXrdpServer()`、`onRefreshXrdpServerStatus()`
  - 录屏授权：`onRequestScreenRecordingPermission()`
  - 门禁开关：`onRemoteAccessCodeGateChange(enabled)`
  - 重新生成验证码：`onRemoteAccessCodeRegenerate()`
  - 打开共享目录：`onOpenRemoteFilesDirectory()`
- 当前 `Index.startXrdpServerFromSettings()` 会立即请求录屏权限，所以 UI 文案要明确录屏权限是服务启动前置条件。

验收点：

- 服务未启动时按钮显示“启动服务”。
- 服务运行中按钮显示“刷新”。
- `localXrdpServerBusy` 或 `screenPermissionBusy` 时禁止重复点击。
- 录屏未授权时显示黄色提示和“去授权”。
- 录屏已授权时显示已授权，可禁用按钮或允许刷新状态。
- 门禁关闭时显示“默认关闭”。
- 门禁开启时显示当前验证码。
- 切换门禁后仍会触发当前 `ensureXrdpServerStarted(..., true)` 路径。
- 重新生成验证码后 UI 更新。
- 远程文件按钮调用 `onOpenRemoteFilesDirectory()`。

风险点：

- `Toggle` 本地先更新，但后续重启服务失败时没有回滚状态。当前实现也是乐观更新，UI 要能显示 xrdp 失败状态。
- `onStartXrdpServer()` 触发权限弹窗，busy 状态和按钮文案要避免用户误点。
- `xrdpServerStatusText()` 当前有乱码风险或拼接文案不规范，需要检查最终中文显示。
- 门禁开启但验证码尚未初始化时，应调用现有 `ensureRemoteAccessCode()` 链路，不要在 UI 层另造验证码。

### `harmony/app/common/src/main/ets/components/settings/BasicSettingsPage.ets`

修改点：

- 保留外观模式切换：
  - 跟随系统
  - 浅色模式
  - 深色模式
- 保留 `refreshIp()` 网络读取逻辑。
- 桌面端重排为两个面板：
  - 外观
  - 本机网络
- 不增加默认分辨率、证书策略等未实现能力。

验收点：

- 页面进入时仍自动刷新 IP。
- 点击外观选项后仍调用 `SettingsTheme.applyAppearanceMode()`。
- `onModeChange(mode)` 仍能同步到设置容器。
- 网络刷新按钮可用。
- 网络状态能展示：
  - 查询中
  - 未连接网络
  - 未获取到可用 IP
  - 读取失败
  - 成功 IP 列表

风险点：

- 外观切换后右侧桌面壳没有同步刷新颜色。
- IP 地址过长时溢出。
- 多 IP 地址列表在横向面板内高度不足。

### `harmony/app/common/src/main/ets/components/settings/ProjectHelpPage.ets`

修改点：

- 保留现有帮助和关于信息。
- 按三组重排：
  - 连接指南
  - 安全与排查
  - 项目信息
- 当前阶段不实现搜索过滤，避免 UI 暗示不可用能力。

验收点：

- GitHub / FreeRDP / xrdp 链接仍可点击。
- MIT license 信息仍显示。
- 使用说明四项仍完整：
  - 连接前准备
  - 填写连接信息
  - 证书策略
  - 常见排查
- 远程文件说明仍能在帮助页找到。

风险点：

- 帮助正文较长，桌面分栏后可能截断过度。
- 链接在窄窗口中可能换行影响布局。

### `harmony/app/common/src/main/ets/components/settings/SettingsConstants.ets`

修改点：

- 只补当前 UI 需要的短文案。
- 不大规模改写现有文案。
- 优先复用已有：
  - `REMOTE_SERVER_*`
  - `REMOTE_ACCESS_*`
  - `REMOTE_SCREEN_*`
  - `REMOTE_FILES_*`
  - `NETWORK_*`
  - `USAGE_*`
  - `ABOUT_*`

建议新增文案：

- `SETTINGS_OVERVIEW_TITLE`
- `SETTINGS_CURRENT_STATE`
- `SETTINGS_NAV_OVERVIEW`
- `REMOTE_FILES_OPEN_ACTION`
- `STATUS_READY`
- `STATUS_BUSY`
- `STATUS_MISSING`
- `STATUS_FAILED`

验收点：

- 中文无乱码。
- 英文占位或旧乱码不得出现在最终 UI。
- 按钮文案长度在桌面和小屏都不溢出。

风险点：

- 文案常量已经多，新增时命名混乱。
- 旧文案中仍有异常字符，需要边改边检查。

## 3. 状态验收矩阵

### xrdp 服务

| 场景 | 输入状态 | 预期 UI |
| --- | --- | --- |
| 运行中 | `xrdpServerRunning=true`，`xrdpServerBusy=false` | 绿色状态，按钮“刷新” |
| 未启动 | `xrdpServerRunning=false`，`xrdpServerBusy=false` | 灰/黄状态，按钮“启动服务” |
| 处理中 | `xrdpServerBusy=true` | 蓝色状态，按钮“处理中”，禁止重复点击 |
| ActiveSession | `xrdpServerState='ActiveSession'` | 蓝色或绿色状态，提示已有远程会话 |
| Exited | `xrdpServerState='Exited'` | 黄色/红色状态，提示可重新启动 |
| Failed | 诊断返回失败消息 | 红色状态，保留刷新或启动入口 |

### 录屏权限

| 场景 | 输入状态 | 预期 UI |
| --- | --- | --- |
| 已授权 | `screenRecordingPermissionGranted=true` | 绿色状态，显示“已授权” |
| 未授权 | `screenRecordingPermissionGranted=false` | 黄色状态，显示“去授权” |
| 请求中 | `screenPermissionBusy=true` | 蓝色状态，按钮禁用 |
| 请求失败 | Promise catch | 回读权限状态，显示当前真实状态 |

### 验证码门禁

| 场景 | 输入状态 | 预期 UI |
| --- | --- | --- |
| 关闭 | `remoteAccessCodeGateEnabled=false` | 显示“默认关闭”，验证码显示禁用态 |
| 开启 | `remoteAccessCodeGateEnabled=true` | 显示验证码，状态为已开启 |
| 切换中 | 切换后 xrdp 重启中 | xrdp 服务显示处理中 |
| 重新生成 | 返回新验证码 | UI 显示新验证码 |

### 远程文件

| 场景 | 动作 | 预期 UI |
| --- | --- | --- |
| 打开共享目录 | 点击“打开共享目录” | 调用 `onOpenRemoteFilesDirectory()` |
| 打开失败 | Promise catch 写日志 | UI 不崩溃，后续可从日志看失败原因 |

### 本机网络

| 场景 | 输入状态 | 预期 UI |
| --- | --- | --- |
| 查询中 | `ipAddresses=['查询中']` | loading 文案 |
| 无网络 | `getDefaultNet()` 无有效网络 | 空状态 |
| 无 IP | linkAddresses 无有效地址 | 黄色提示 |
| 读取失败 | catch | 红色/错误文案 |
| 成功 | 有地址 | 显示接口名和 IP |

## 4. 验证清单

功能验证：

- 打开设置页。
- 关闭设置页。
- 左侧导航切换概览、基础设置、远控设置、项目帮助。
- 设置概览点击卡片可进入对应页面。
- 外观切换三种模式。
- 本机 IP 刷新。
- xrdp 状态刷新。
- xrdp 启动。
- 录屏权限授权。
- 验证码门禁开关。
- 重新生成验证码。
- 打开共享目录。
- 项目链接点击。

视觉验证：

- 浅色模式可读。
- 深色模式可读。
- 跟随系统后切换正常。
- 长文案不压住按钮。
- 状态 chip 不换行挤压。
- 小窗口下不出现明显重叠。
- hover 不产生夸张跳动。

回归验证：

- 主页连接表单仍可输入。
- `connectNative()` 不受影响。
- 已连接远程会话页面不受影响。
- 权限回调 `onMicrophonePermissionRequest`、`onClipboardPermissionRequest`、`onLocationPermissionRequest` 不受设置页改造影响。

## 5. 建议实施顺序

1. `SettingsUi.ets`：补公共桌面组件和状态 chip。
2. `SettingsPage.ets`：实现桌面壳和概览页。
3. `RemoteControlSettingsPage.ets`：按当前能力重排，并确保所有回调不变。
4. `BasicSettingsPage.ets`：重排外观和网络。
5. `ProjectHelpPage.ets`：重排帮助内容。
6. `Index.ets`：调整主页设置入口和状态点。
7. `SettingsConstants.ets`：补短文案并检查中文显示。
8. 构建和交互验收。

## 6. 主要风险和规避

| 风险 | 影响 | 规避 |
| --- | --- | --- |
| 响应式布局不稳 | 小窗口重叠或裁切 | 先做桌面宽度，窄宽回退旧单列结构 |
| 状态本地值和父状态不同步 | UI 显示旧状态 | 进入远控页时同步 props，关键操作后调用刷新回调 |
| 权限弹窗和 busy 状态冲突 | 用户重复点击 | busy 时禁用按钮 |
| 门禁切换重启失败 | 开关显示已变但服务未生效 | 服务卡显示失败/停止状态，保留刷新和启动入口 |
| 中文文案溢出 | 按钮或状态 chip 变形 | 重要按钮短文案，说明文本允许换行或省略 |
| 深色模式状态色过亮/过暗 | 可读性差 | 状态色通过 `SettingsTheme` 分深浅模式返回 |

