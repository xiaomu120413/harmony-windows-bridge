# 桌面设置页交互规格（当前能力版）

> 本文描述 HarmonyOS 应用的桌面布局，不是已删除的 Web Demo。页面路径已按 common HSP 更新；分阶段实施记录保留，后文较新的日期记录优先于早期布局描述。

本文档对应 `docs/settings-desktop-current-capability.svg`，只覆盖当前代码里已经具备或已经有回调入口的能力，不新增日志导出、更新中心、高级诊断、安全策略等未实现功能。

## 0. 当前代码功能对应关系

这一版设计只映射当前工程里已经存在的页面、状态、回调和文案。

### 页面文件对应

| 设计模块 | 当前文件 | 当前职责 | 改造方式 |
| --- | --- | --- | --- |
| 主页设置入口 | `harmony/app/common/src/main/ets/pages/Index.ets` | 展示连接页、设置按钮、维护全局状态 | 只调整设置按钮视觉和打开设置前的刷新逻辑 |
| 设置容器 / 导航 | `harmony/app/common/src/main/ets/components/SettingsPage.ets` | 当前用 `pageName` 在三个设置页之间切换 | 桌面版改成左侧导航 + 右侧内容，继续复用 `pageName` |
| 基础设置 | `harmony/app/common/src/main/ets/components/settings/BasicSettingsPage.ets` | 外观模式、本机网络 IP | 桌面版只重排布局，不改能力 |
| 远控设置 | `harmony/app/common/src/main/ets/components/settings/RemoteControlSettingsPage.ets` | xrdp、录屏权限、注入权限、验证码、远程文件目录 | 桌面版使用五个功能面板 |
| 项目帮助 | `harmony/app/common/src/main/ets/components/settings/ProjectHelpPage.ets` | 关于项目、使用说明、排查说明 | 桌面版改成分组知识区 |
| 公共 UI | `harmony/app/common/src/main/ets/components/settings/SettingsUi.ets` | Header、ListItem、Card、颜色、阴影 | 增加桌面行、状态 chip、状态面板组件 |
| 文案常量 | `harmony/app/common/src/main/ets/components/settings/SettingsConstants.ets` | 当前设置页所有中文文案 | 继续复用，缺少的状态短文案再补 |

### 当前全局状态对应

这些状态都在 `Index.ets` 里维护，然后通过 `SettingsPage` 传给子页面。

| 设计字段 | 当前状态 / 函数 | 当前用途 |
| --- | --- | --- |
| xrdp 是否运行 | `xrdpServerRunning` | 控制远控服务显示“已启动/未启动” |
| xrdp 原始状态 | `xrdpServerState` | 显示 `Stopped`、`Listening`、`ActiveSession`、`Exited` 等 |
| xrdp 端口 | `xrdpServerPort` | 当前默认 `3390` |
| xrdp 状态消息 | `xrdpServerMessage` | 使用 `SettingsText.REMOTE_SERVER_MESSAGE_*` |
| xrdp 忙碌态 | `xrdpServerBusy` | 启动/重启时禁用重复操作 |
| 验证码 | `remoteAccessCode` | 门禁开启后展示当前验证码 |
| 门禁开关 | `remoteAccessCodeGateEnabled` | 控制验证码门禁开启/关闭 |
| 录屏权限 | `screenRecordingPermissionGranted` | 控制“已授权/未授权” |
| 注入权限 | `inputInjectionPermissionGranted` | 控制 API 26 `CONTROL_DEVICE` 的“已授权/未授权” |
| 外观模式 | `@StorageLink('settingsAppearanceMode') appearanceMode` | 主界面和设置页深浅色 |
| 系统深色 | `@StorageLink('settingsSystemDark') systemDark` | 跟随系统模式时判断实际颜色 |

### 当前回调对应

这些回调已经从 `Index.ets` 透传到 `SettingsPage.ets`，再传给 `RemoteControlSettingsPage.ets`。

| 设计动作 | 当前回调 | 当前实现位置 |
| --- | --- | --- |
| 刷新 xrdp 状态 | `onRefreshXrdpServerStatus` | `Index.refreshXrdpServerDiagnostics()` |
| 启动 xrdp 服务 | `onStartXrdpServer` | `Index.startXrdpServerFromSettings()` |
| 切换验证码门禁 | `onRemoteAccessCodeGateChange` | `Index.setRemoteAccessCodeGateFromSettings(enabled)` |
| 重新生成验证码 | `onRemoteAccessCodeRegenerate` | `Index.regenerateRemoteAccessCodeFromSettings()` |
| 请求录屏权限 | `onRequestScreenRecordingPermission` | `Index.requestScreenRecordingPermissionFromSettings()` |
| 刷新录屏权限状态 | `onRefreshScreenRecordingPermission` | `Index.refreshScreenRecordingPermissionState()` |
| 请求注入权限 | `onRequestInputInjectionPermission` | `Index.requestInputInjectionPermissionFromSettings()` |
| 刷新注入权限状态 | `onRefreshInputInjectionPermission` | `Index.refreshInputInjectionPermissionState()` |
| 打开共享目录 | `onOpenRemoteFilesDirectory` | `Index.openRemoteFilesDirectoryFromSettings()` |
| 关闭设置页 | `onClose` | `Index` 中设置 `showSettings = false` |

### 当前文案对应

| 设计模块 | 当前文案常量 |
| --- | --- |
| 设置页标题 | `SettingsText.SETTINGS_TITLE`、`SettingsText.SETTINGS_SUBTITLE` |
| 基础设置入口 | `BASIC_SETTINGS_ENTRY_TITLE`、`BASIC_SETTINGS_ENTRY_DESC` |
| 远控设置入口 | `REMOTE_CONTROL_ENTRY_TITLE`、`REMOTE_CONTROL_ENTRY_DESC` |
| 项目帮助入口 | `PROJECT_HELP_ENTRY_TITLE`、`PROJECT_HELP_ENTRY_DESC` |
| xrdp 服务 | `REMOTE_SERVER_*` |
| 验证码门禁 | `REMOTE_ACCESS_*` |
| 录屏权限 | `REMOTE_SCREEN_PERMISSION_TITLE`、`REMOTE_SCREEN_PERMISSION_DESC` |
| 注入权限 | `REMOTE_INPUT_PERMISSION_TITLE`、`REMOTE_INPUT_PERMISSION_DESC` |
| 通用权限状态 | `REMOTE_PERMISSION_GRANTED/MISSING/ACTION/BUSY` |
| 远程文件 | `REMOTE_FILES_*` |
| 本机网络 | `NETWORK_*` |
| 使用说明 | `USAGE_*` |
| 关于项目 | `ABOUT_*` |

### 当前还没有的能力

这些不应该出现在当前版本 UI 里，最多作为后续规划，不做按钮入口：

- 日志导出
- 更新中心
- 高级诊断中心
- 多共享目录管理
- 安全策略配置页
- 证书策略切换 UI
- 剪贴板/麦克风/定位权限的独立设置页
- 远程会话列表管理

## 1. 页面结构

桌面版设置页采用「左侧导航 + 右侧内容」。

- 左侧导航固定显示：`基础设置`、`远控设置`、`项目帮助`。
- 右侧默认显示 `设置概览`。
- 点击左侧导航或概览卡片后，右侧切换对应子页面，不关闭设置页。
- 设置页左上角保留返回/关闭按钮，点击后回到主页连接界面。

建议路由值：

```ts
SettingsRoute.SETTINGS = 'settings'          // 概览
SettingsRoute.BASIC = 'basic'                // 基础设置
SettingsRoute.REMOTE_CONTROL = 'remoteControl'
SettingsRoute.PROJECT_HELP = 'projectHelp'
```

## 2. 主页设置入口

应用浮窗最小尺寸声明为 `660 × 540vp`。该下限仍允许进入 Compact 布局；共享首页头部使用 Flex 剩余空间居中，不再依赖左右固定占位。全屏、分屏和窗口恢复仍由系统窗口管理器处理。

主页右上角设置入口保留为按钮，但弱化视觉。

点击行为：

1. 调用 `refreshScreenRecordingPermissionState()`。
2. 调用 `refreshXrdpServerDiagnostics()`。
3. 设置 `showSettings = true`。
4. 设置页默认进入 `SettingsRoute.SETTINGS` 概览。

状态点规则：

| 条件 | 状态点 |
| --- | --- |
| `xrdpServerBusy === true` | 蓝色，表示处理中 |
| 首页权限卡未授权 | 灰色，表示当前未具备该权限 |
| `xrdpServerRunning === false` | 灰色，表示服务未启动 |
| `xrdpServerRunning && screenRecordingPermissionGranted` | 绿色，表示可用 |

不要在主页入口展示过多文本。详细状态放到设置页。

首页底部“被控服务”卡片只显示“已启动 / 未启动 / 启动中 / 已退出 / 启动失败”，不拼接监听端口；端口只在设置概览和远控设置中展示。

连接详情中的用户名字段使用“Windows 用户名”作为标签，占位符提供“设备名\\用户名”示例；完整的本地账号、域账号和微软账号填写规则放在项目帮助页，避免表单正文过重。

### 2.1 连接配置切换与账户提示

- Change ID：`HOME-CONNECTION-CREDENTIAL-001`
- 状态：`Implemented / BuildVerified / DevicePending`
- 目标：切换已保存连接时消除密码框先清空再回填的闪烁，并防止凭据尚未读取时用空密码发起连接；同时让 Windows 账户填写格式和设备列表中的账户身份可直接辨认。
- 状态规则：未保存密码的配置立即切换；保存了密码的配置先异步读取 Asset 凭据，读取成功后一次性应用 host、port、username、password、rememberPassword 和 certPolicy。读取期间保留当前表单、禁止 Password 编辑和 Connect；快速连续切换时只允许最后一次请求提交结果。
- 展示规则：用户名标签继续为“Windows 用户名”，占位符明确写成“远程主机名\\用户名，例如：DESKTOP-ABC\\zhangsan”；设备卡首行显示完整 `profile.username`，不得把 `DESKTOP-ABC\\zhangsan` 截成 `DESKTOP-ABC`。Host 可能是 IP 或 DNS 地址，因此不得自动把 Host 输入值拼成 Windows 凭据。
- 不变项：不修改“记住密码”Toggle 的系统动效，不修改 Connect 的按压缩放，不改变 Asset 存储格式、FreeRDP settings、Native ABI 或连接成功后保存配置的时序。
- 代码范围：`pages/Index.ets`、`components/home/HomePage.ets`、`HomeConnectionDetails.ets`、`HomeDeviceList.ets`、`HomeText.ets`、`tools/run_tablet_arkts_tests.ps1`；测试只扩展既有 ArkTS 策略检查。
- 验收：`AC-CRED-01` 在两个保存密码的配置间切换时不出现空密码中间态；`AC-CRED-02` 凭据读取期间 Password 和 Connect 不可操作，旧异步结果不能覆盖最后选中的配置；`AC-USER-01` 输入提示出现远程主机名和用户名示例，设备卡保留完整反斜杠账户名；`AC-REG-01` 记住密码与 Connect 动效代码无行为变化，ArkTS 测试和 HAP 构建通过。
- 实施证据：2026-08-06 已按 generation 门禁实现保存凭据的原子切换，Password/Connect 只在读取期间禁用；用户名输入使用统一提示常量，设备卡不再截断反斜杠后的账户名。`tools/run_tablet_arkts_tests.ps1` 通过，模块单测和 ArkTS 编译成功；`harmony/app/build_hap.bat debug` 完整构建、打包和签名通过，signed HAP 为 35,843,146 bytes。尚未在设备上执行两个保存密码配置的连续切换，因此保持 `DevicePending`。

## 3. 设置概览页

概览页负责让用户先看到当前关键状态，再决定进入哪个子页。

进入概览页时刷新：

- 屏幕录制权限：`onRefreshScreenRecordingPermission`
- xrdp 状态：`onRefreshXrdpServerStatus`
- 外观模式：`SettingsTheme.getStoredAppearanceMode()`

概览状态区显示：

| 字段 | 数据来源 |
| --- | --- |
| 连接地址 | `SettingsPage.localIpAddress` 与 `xrdpServerPort` |
| xrdp | `xrdpServerRunning`、`xrdpServerPort`、`xrdpServerState` |
| 录屏权限 | `screenRecordingPermissionGranted` |
| 键鼠权限 | `inputInjectionPermissionGranted` |

连接地址不再复用基础设置页状态。`SettingsPage` 统一遍历当前所有网络，优先 Ethernet/Wi-Fi 和常见局域网 IPv4，VPN 地址只作为最后回退；连接地址仅在远控设置中展示，项目帮助只讲连接流程。

“当前状态”整卡复用设置卡片的 hover/press 动效：悬浮时抬升、增加阴影并切换 hover 背景，按下时轻微缩放。

概览卡片点击：

- `基础设置` 卡片：`pageName = SettingsRoute.BASIC`
- `远控设置` 卡片：`pageName = SettingsRoute.REMOTE_CONTROL`
- `项目帮助` 卡片：`pageName = SettingsRoute.PROJECT_HELP`

## 4. 基础设置页

当前只承载外观模式和通用偏好。本机 IP 属于“别人如何连接本机”的任务信息，只在远控设置展示，不再放在基础设置或项目帮助。

### 外观模式

三个选项：

- 跟随系统
- 浅色模式
- 深色模式

点击任一选项：

1. 先记录旧值。
2. 乐观更新本地选中态。
3. 调用 `SettingsTheme.applyAppearanceMode(this.getUIContext(), mode)`。
4. 成功：调用 `onModeChange(mode)`。
5. 失败：恢复旧值。

选中态：

- 选中的行显示蓝色边框或勾选图标。
- 未选中的行保持普通白底。

## 5. 远控设置页

当前可做内容分为“被动控制”和“主动控制”。

进入页面时：

1. 将 props 同步到 local state。
2. 调用 `refreshScreenRecordingState()`。
3. 调用 `refreshInputInjectionState()`。
4. 调用 `refreshXrdpServerStatus()`。

当前展示顺序：

1. 被动控制：即用连接卡（服务状态、只读连接地址、启动/刷新）
2. 被动控制：录屏权限
3. 被动控制：注入权限
4. 主动控制：鸿蒙共享目录
5. 主动控制：连接验证码

连接地址仅用于查看，不提供复制操作或“已复制”状态。验证码卡保留既有门禁、当前验证码和重新生成能力，归入主动控制并排列在鸿蒙共享目录下方。

### xrdp 服务卡

显示字段：

- 状态：`localXrdpServerState`
- 端口：`localXrdpServerPort`
- 消息：`localXrdpServerMessage`
- 运行状态：`localXrdpServerRunning`
- 连接地址：优先物理网络的本机 IP 与 `localXrdpServerPort`

按钮规则：

| 当前状态 | 主按钮文本 | 点击行为 |
| --- | --- | --- |
| `localXrdpServerBusy` | 处理中 | 禁止重复点击 |
| `localXrdpServerRunning === true` | 刷新 | `refreshXrdpServerStatus()` |
| `localXrdpServerRunning === false` | 启动服务 | `startXrdpServer()` |

`startXrdpServer()` 行为：

1. 若 busy，直接返回。
2. 设置 `localXrdpServerBusy = true`。
3. 调用 `onStartXrdpServer()`。
4. 当前 `Index.startXrdpServerFromSettings()` 会调用 `ensureXrdpServerStarted('remote control settings button', false, true)`，这意味着点击启动会立即触发录屏权限请求，而不是只做静默检查。
5. 成功：`applyXrdpServerStatus(status)`，并刷新录屏权限。
6. 失败：刷新当前 xrdp 状态。
7. finally：`localXrdpServerBusy = false`。

### 录屏权限卡

显示字段：

- 当前授权状态：`localScreenRecordingGranted`
- 忙碌状态：`screenPermissionBusy`

按钮规则：

| 状态 | 按钮文本 | 点击行为 |
| --- | --- | --- |
| busy | 处理中 | 禁止重复点击 |
| 已授权 | 已授权 | 可禁用，或点击刷新状态 |
| 未授权 | 去授权 | `requestScreenRecordingPermission()` |

`requestScreenRecordingPermission()` 行为：

1. 若 `screenPermissionBusy`，直接返回。
2. 设置 `screenPermissionBusy = true`。
3. 调用 `onRequestScreenRecordingPermission()`。
4. 成功：更新 `localScreenRecordingGranted`，并刷新 xrdp 状态。
5. 失败：调用 `onRefreshScreenRecordingPermission()` 回读状态。
6. finally：`screenPermissionBusy = false`。

### 注入权限卡

交互复用录屏权限卡的状态、按钮和系统设置页授权模式，但状态源独立：

- 当前授权状态：`localInputInjectionGranted`
- 忙碌状态：`inputInjectionPermissionBusy`
- 未授权时点击“去授权”，调用 `onRequestInputInjectionPermission()` 打开系统权限设置。
- 返回后通过 `onRefreshInputInjectionPermission()` 回读 `CONTROL_DEVICE`。
- 该卡与录屏权限共同说明被控端是否具备完整控制能力。

### 远程文件卡

该卡属于“主动控制”：本机作为主控端连接 Windows 时使用。当前已有打开共享目录能力，不要只做说明，也不要扩展成多目录管理。

显示：

- Windows 侧路径：`\\tsclient\\Downloads`
- 说明：连接到 Windows 后，可通过该路径访问鸿蒙侧固定共享目录。

按钮：

- `打开共享目录`：调用 `onOpenRemoteFilesDirectory()`。

不要展示“选择目录”“新增共享目录”“权限管理”等当前没有的能力。

## 6. 项目帮助页

2in1 首屏优先回答“别人如何连接本机”：

1. 连接设备卡：只显示被控服务状态和前往远控设置，不重复展示连接地址。
2. 三步说明：本机保持服务运行、Windows 打开 `mstsc`、输入地址并连接。
3. 常见排查：按“主控端 / 被控端”分别列出连接信息、权限、服务和端口检查；不再显示开头的安全提示句。

“本机连接 Windows”作为次级入口，展开后保留以下内容：

- 关于项目
- GitHub 链接
- FreeRDP 适配链接
- xrdp 适配链接
- MIT license
- 使用说明
- 连接前检查与 Windows 远程桌面开关
- Windows 用户名类型、填写格式和 `whoami` 核对方法
- Windows 远程桌面 H.264/AVC 硬件编码策略（可选）
- Host、Port、Username、Password 填写说明
- 证书策略
- 常见排查
- 远程文件说明

项目帮助分为：

1. 被控端即用连接指南（仅 2in1）
2. 主控端 Windows 连接指南
3. 安全与排查
4. 项目信息

tablet 继续 fail-closed：不显示远控导航、即用连接卡或被控端帮助，只显示“本机连接 Windows”和项目信息。

Compact 下三个步骤纵向排列；Expanded 下步骤三列排列。连接设备卡和每个步骤卡均复用统一 hover/press 动效，两种布局不复制业务页面。

连接指南必须按实际操作顺序编号。硬件加速说明使用 Windows 本地组策略路径“计算机配置 > 管理模板 > Windows 组件 > 远程桌面服务 > 远程桌面会话主机 > 远程会话环境”，同时说明专业版/企业版/教育版适用、家庭版通常没有本地组策略编辑器，以及硬件编码失败时系统会回退软件编码。

搜索框可以先只做 UI 预留；如果要实现搜索，规则如下：

- 输入为空：显示全部分组。
- 输入非空：按标题和正文做本地包含匹配。
- 无结果：显示空状态“没有找到相关内容”。

## 7. 状态视觉规则

只设计当前已有状态，不增加新能力。

| 状态 | 颜色 | 使用位置 |
| --- | --- | --- |
| Running / Granted / Ready | 绿色 | xrdp 已启动、录屏已授权、本机 IP 成功 |
| Stopped / Empty / Permission Missing | 灰色 | xrdp 未启动、无网络、首页权限卡未授权 |
| Busy / Loading | 蓝色 | 正在启动服务、正在请求权限、正在读取 IP |
| Failed | 红色 | 状态读取失败、权限请求失败 |
| Warning | 黄色 | 门禁关闭提示、无可用 IP 等需要注意但不是失败的状态 |
| ActiveSession | 蓝色 | xrdp 有活跃远程会话 |

首页状态卡的图标前景、图标底色和右侧状态点必须读取同一个实时 tone；异步刷新服务或权限后，三处颜色需同时更新，不能保留首帧的灰色图标底。

## 8. 桌面布局规则

建议断点：

- 宽度 `>= 1000vp`：左侧导航 + 右侧内容。
- 宽度 `< 1000vp`：退回当前手机式单页下钻。

桌面尺寸建议：

- 左侧导航宽度：`260-288vp`
- 设置内容最大宽度：不超过 `1280vp`
- 卡片圆角：`10-14`
- 主按钮高度：`34-40`
- 行高：`42-56`
- 卡片 hover：只变背景和边框，不做大幅上移动画。

## 9. 开发优先级

第一阶段建议只做：

1. 桌面设置壳：左侧导航 + 右侧内容。
2. 设置概览页。
3. 远控设置页重排。
4. 状态颜色和 busy/failed/missing 展示。

第二阶段再做：

1. 基础设置页桌面重排。
2. 项目帮助页分组。
3. 搜索框真正过滤。
4. 小屏回退适配。
