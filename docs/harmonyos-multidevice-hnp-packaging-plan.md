# MuHub HarmonyOS 多设备 HNP 分包架构、修改清单与验收方案

> 当前状态（2026-09-05 核对）：多设备拆分及独立 HNP 进程已实现；旧 tablet 覆盖升级和市场分发仍未闭环，不能视为发布 Verified。
> 文档版本：1.0
> 审阅日期：2026-08-06
> 适用工程：`harmony/app`
> 本文范围：tablet/2in1 的 HAP/HSP/HNP、权限、独立进程、构建分发和升级验收
> 非本文范围：重新设计首页、设置页或 RDP 会话交互

> 外部依据已于 2026-09-05 复查：访问受限的固定版本文档改用同提交官方 GitHub 原文；旧 HSP 文档改用现行 in-app HSP 页面，HNP 链接改为官方开发指南。原地址与核验方式保留在 [审计清单](audit/external-links.json)。

## 1. 初始方案与当前状态

当前工程验证结构采用 **一个 bundle、一个 App Pack、两个按设备隔离的 Entry HAP、一个应用内 HSP**：

```text
com.muhub.desktop
├─ common.hsp
│  ├─ 现有首页、设置页、RDP 会话页和响应式布局
│  ├─ RDP 客户端业务、ArkTS 公共代码和资源
│  └─ FreeRDP 客户端 Native bridge 与运行库
├─ entry.hap
│  ├─ deviceTypes: ["2in1"]
│  ├─ 薄 EntryAbility / PrintExtensionAbility 壳
│  ├─ XRDP HNP 启停桥和 2in1 能力实现
│  ├─ hnp/arm64-v8a/xrdp.hnp + hnpPackages
│  └─ 2in1 公共权限 + CUSTOM_SCREEN_RECORDING + CONTROL_DEVICE
└─ entry_tablet.hap
   ├─ deviceTypes: ["tablet"]
   ├─ 薄 EntryAbility / PrintExtensionAbility 壳
   ├─ tablet 不可用的 RemoteControlPort 实现
   ├─ 不含 HNP、hnpPackages 和 XRDP 服务端运行库
   └─ 只声明 RDP 客户端实际需要的权限
```

设备实际安装集合：

| 设备 | 应用市场应下发的模块 | 必须不存在 |
|---|---|---|
| tablet | `entry_tablet.hap + common.hsp` | `entry.hap`、HNP、`hnpPackages`、PC 专属权限 |
| 2in1 | `entry.hap + common.hsp` | tablet 专属入口 |

固定决策如下：

1. 保持 `bundleName=com.muhub.desktop`、版本身份和用户数据身份不变。
2. 保留现有 `entry` 模块名给 2in1，降低原 PC/2in1 安装的升级风险；新增 `entry_tablet`。
3. 不新增第二套 UI，不把远控设置迁移到新的 Feature UIAbility，不改变现有页面、路由和交互顺序。
4. `common.hsp` 是 UI 和 RDP 客户端的唯一实现；两个 Entry 只做系统入口、权限声明和设备能力注入。
5. XRDP 独立进程只由 2in1 Entry 中的私有 HNP 提供，最终不再使用 `dlopen(libxrdpserver.so) + std::thread` 作为发布路径。
6. tablet 的安装包从物理内容上不包含 HNP、PC 权限和 XRDP 服务端，而不仅是运行时隐藏按钮。

## 2. 为什么必须改变当前单 HAP 方案

当前单 HAP 同时声明 `2in1` 和 `tablet`，也同时声明 RDP 客户端权限和 XRDP 服务端权限。XRDP 当前由应用进程 `dlopen(libxrdpserver.so)` 后在线程中调用 `xrdp_ohos_server_main()`，不是独立进程。

已验证的约束：

- MatePad Pro tablet 产品未启用 `const.startup.hnp.install.enable`。
- HAP 内含 HNP 时，安装返回 BMS `9568407`，底层 HNP 返回 `8201/HNP_INSTALL_DISABLED`。
- 删除 HNP 文件但保留 `hnpPackages` 时，安装返回 `9568409`。
- 同一 HAP 删除 HNP 文件和 `hnpPackages` 后，tablet 可以安装、启动。

因此以下四个目标不能由一个通用 HAP 同时满足：

1. tablet 可安装；
2. 2in1 携带 HNP；
3. XRDP 使用独立可执行进程；
4. tablet 不携带 PC 专属权限。

运行时 `deviceType` 判断只能阻止调用，不能改变 HAP 已经包含的 `hnp/`、`hnpPackages` 和 `requestPermissions`。必须把物理包边界改为按设备分发。

官方模型依据：

- 一个 App Pack 中，每种设备类型只能有一个 Entry HAP；不同设备类型可以各自具有 Entry HAP，应用市场按 HAP 拆分分发：<https://raw.githubusercontent.com/openharmony/docs/43d836fe05a882d386c6c42e3827221cd2051256/en/application-dev/quick-start/hap-package.md>
- App Pack 是发布单元，云端和设备端按 HAP 安装：<https://raw.githubusercontent.com/openharmony/docs/ecf01f0dd5c6fc9797d60a61b288f00bb68f24de/en/application-dev/quick-start/application-package-structure-stage.md>
- HSP 可以共享 ArkTS、资源和 C++ 库，多 HAP 引用时只保留一份：<https://raw.githubusercontent.com/openharmony/docs/master/en/application-dev/quick-start/in-app-hsp.md>
- HAP 和进程不是一一对应；普通三方应用不能依靠 `process` 配置获得独立进程：<https://raw.githubusercontent.com/openharmony/docs/e99082d84adcc46cde561d79b04fbf2fad68a723/en/application-dev/quick-start/multi-hap-principles.md>
- HNP 由 HAP 分发，并通过 `fork/execv` 等方式执行 Native 二进制：<https://raw.githubusercontent.com/openharmony/startup_appspawn/master/service/hnp/README_zh.md>

## 3. 不改变的产品体验

本次是安装包和运行时边界改造，不是 UI 改版。

必须保持：

- 现有首页、设置页、远控设置页、RDP 会话页和 Compact/Expanded 响应式规则；
- 现有页面路由、返回行为、表单状态、主题、字体和窗口适配；
- 2in1 上当前远控入口、状态展示、验证码和权限操作位置；
- tablet 上现有 RDP 客户端、远程文件、剪贴板、音频、摄像头、定位和打印能力边界；
- RDP 会话期间不因 Entry/HSP 拆分重建 XComponent、Controller 或 sessionId。

禁止：

- 为了分包增加第二个可见 UIAbility；
- 从当前页面跳转到 Feature HAP 页面；
- 复制 `TabletHomePage`、`DesktopHomePage` 等两套业务页面；
- 用构建目标替代窗口断点，或用窗口宽度推断 XRDP 能力；
- 只隐藏按钮但仍把 HNP/PC 权限打进 tablet HAP。

UI 回归以拆分前同一设备、同一窗口尺寸的截图和自动化 layout dump 为基准。除 tablet 本来就不提供 XRDP 外，不接受新增步骤、页面跳转、重复任务卡或状态丢失。

## 4. 模块职责和依赖方向

### 4.1 `common.hsp`

`common.hsp` 负责共享实现，但不声明 Ability、ExtensionAbility、HNP 或设备专属权限。

应迁入：

- `entry/src/main/ets/pages`、`components`、`adaptive`；
- RDP 客户端 Controller、配置、持久化、权限请求协调和 XComponent host；
- 与 UI 无关的公共能力接口和设备能力模型；
- 共享资源、主题、字符串、图标、页面 profile；
- FreeRDP 客户端 Native C++：`channels`、`freerdp`、`input`、`session`、`surface` 和客户端 N-API；
- FreeRDP/WinPR 客户端运行库。

不应迁入：

- `EntryAbility`、`PrintExtensionAbility` 的系统声明；
- `CUSTOM_SCREEN_RECORDING`、`CONTROL_DEVICE`；
- `xrdp.hnp`、`hnpPackages`；
- XRDP 进程启动、PID 管理和服务端运行库。

### 4.2 2in1 `entry.hap`

保留当前模块名 `entry`，最终只支持 `deviceTypes: ["2in1"]`。

负责：

- 薄 `EntryAbility`，安装 2in1 `RemoteControlPort` 后加载 `common.hsp` 的同一 UI 根组件；
- 必要的 `PrintExtensionAbility` 薄壳；
- 2in1 完整权限清单和受限权限 ACL 对应的签名材料；
- `xrdp.hnp` 和 `hnpPackages`；
- `libxrdpcontrol.so`：解析 HNP 私有安装路径、`fork/execv`、PID/退出码、停止和健康检查；
- 应用显式启动/停止 XRDP 时的生命周期所有权。

### 4.3 tablet `entry_tablet.hap`

只支持 `deviceTypes: ["tablet"]`。

负责：

- 薄 `EntryAbility`，安装不可用的 `RemoteControlPort` 后加载相同 UI 根组件；
- tablet 需要的 `PrintExtensionAbility` 薄壳；
- tablet 权限清单；
- 对旧路由、旧持久化状态和非法远控调用 fail-closed。

tablet 模块不能引用 `libxrdpcontrol.so`，也不能包含 XRDP 头文件、服务端 `.so`、配置、share 或 HNP。

### 4.4 共享接口

`common.hsp` 不能反向依赖任一 Entry。通过小接口注入设备能力：

```text
RemoteControlPort
├─ capability(): available | unavailable
├─ start(config): result
├─ stop(): result
├─ snapshot(): state/pid/lastExit
└─ refreshPermissionState(): permission state

2in1 Entry  -> HnpXrdpRemoteControlPort -> libxrdpcontrol.so -> fork/execv
tablet Entry -> UnavailableRemoteControlPort -> fail-closed result
```

建议在 `common.hsp` 提供一次性 `AppFeatureRegistry.install()`。两个 Entry 在加载页面前注入实现；共享 UI 只读取语义能力和回调，不读取模块名，也不直接 import 设备 Entry 代码。

现有 `DeviceCapabilityPolicy` 保留为运行时防御，但最终可用条件必须同时满足：

```text
packagedRole == 2in1
AND systemDeviceType == 2in1
AND HNP executable health check passed
```

任何未知设备类型、HNP 缺失或健康检查失败都返回 unavailable。

## 5. Native 和 XRDP 进程模型

### 5.1 最终调用链

```text
现有远控设置 UI
  -> RemoteControlCoordinator / XrdpServerController
  -> RemoteControlPort
  -> 2in1 Entry 的 libxrdpcontrol.so
  -> 校验私有 HNP 路径、配置和权限
  -> fork()
  -> child: execve(<HNP>/bin/xrdp, argv, env)
  -> parent: 保存 pid，异步 waitpid，发布状态
```

最终发布路径禁止继续调用：

```text
dlopen(libxrdpserver.so)
  -> dlsym(xrdp_ohos_server_main)
  -> std::thread(mainFn)
```

在新路径真机验证完成前，旧进程内实现可以保留在分支中作对照，但不能和新路径自动 fallback。独立进程启动失败时应向 UI 返回明确错误，不能静默退回线程模式。

### 5.2 进程生命周期

必须定义并验证：

- 重复点击启动是幂等操作，不产生第二个 xrdp 进程；
- 父进程保存明确 PID，不通过模糊进程名批量杀进程；
- 显式停止先发送约定信号，超时后再有限升级，最终 `waitpid` 回收；
- 应用强停、卸载、HAP 升级后不留下孤儿 xrdp；
- xrdp 异常退出时 UI 收到退出码和可诊断原因；
- 配置、证书、PID 文件只写应用/HNP允许的沙箱路径；
- 不用 `sh -c` 拼接命令，不把密码或验证码放入命令行和日志；
- HNP 使用 `private`，首版 `independentSign=false`，避免公共包名冲突和额外签名链。

### 5.3 独立进程不等于后台常驻

HNP 解决的是可执行文件交付和独立 OS 进程启动，不自动授予无限后台运行能力。

本方案首版承诺：应用拥有 XRDP 进程，用户显式启动后在允许的前后台生命周期内运行，用户停止、应用强停或卸载时应退出。

如果产品要求“UI 完全退出后仍长期被控、系统重启后自启动”，必须单独评估后台任务、企业设备策略、系统权限和商店审核；该需求不能由 HNP 打包本身推导出来，也不在本轮默认范围内。

## 6. 权限拆分

### 6.1 目标权限矩阵

| 权限 | tablet Entry | 2in1 Entry | 归属理由 |
|---|---:|---:|---|
| `INTERNET` | 是 | 是 | RDP 客户端和网络服务 |
| `GET_NETWORK_INFO` | 是 | 是 | 连接与网络诊断 |
| `PRINT` | 按当前打印能力保留 | 保留 | RDP 客户端打印重定向 |
| `READ_PASTEBOARD` | 按需声明/请求 | 按需声明/请求 | RDP 客户端剪贴板通道 |
| `MICROPHONE` | 按需声明/请求 | 按需声明/请求 | RDP 客户端 `audin` |
| `CAMERA` | 按需声明/请求 | 按需声明/请求 | RDP 客户端 `rdpecam` |
| `APPROXIMATELY_LOCATION` | 按功能开关 | 按功能开关 | RDP 客户端 location 通道 |
| `LOCATION` | 按功能开关 | 按功能开关 | RDP 客户端 location 通道 |
| `CUSTOM_SCREEN_RECORDING` | 否 | 是 | XRDP 被控画面采集 |
| `CONTROL_DEVICE` | 否 | 是 | XRDP 长期键鼠注入 |

不能把剪贴板、麦克风、摄像头、定位和打印简单视为“PC 权限”；它们当前属于 RDP 客户端通道，tablet 保留对应功能时仍需要声明。权限是否保留应由功能矩阵决定，而不是设备名称决定。

### 6.2 权限行为门禁

- tablet HAP 解包后的 `requestPermissions` 不得出现 `CUSTOM_SCREEN_RECORDING`、`CONTROL_DEVICE`。
- tablet 安装后的系统权限页不得出现这两项。
- tablet 冷启动、连接 RDP、进入设置和恢复旧状态时，这两项请求次数必须为 0。
- 2in1 只在用户启动被控服务或进入明确授权流程时处理录屏/控制权限。
- `CONTROL_DEVICE` 的 profile ACL、应用 APL、签名证书和商店审核材料必须与 2in1 发布流程一起验证。
- 所有 HAP/HSP 使用同一证书；即使统一 profile 中有 ACL 允许项，tablet HAP 仍不得在 module profile 声明对应权限。

## 7. HNP 内容和包边界

2in1 Entry 的最终 HNP 至少包含：

```text
xrdp.hnp
├─ hnp.json
├─ bin/xrdp
├─ config/xrdp.ini
├─ config/rsakeys.ini
├─ config/xrdp_keyboard.toml
├─ config/km-*.toml
├─ share/sans-10.fv1
└─ lib/<xrdp 实际 DT_NEEDED 依赖>
```

修改 `package-xrdp-hnp.ps1` 前先用 `llvm-readelf -d` 对 `bin/xrdp` 和插件逐一审计 `DT_NEEDED`。只有证明不是独立进程运行所需的 `libxrdpserver.so` 或重复运行库才可以删除，不能按文件名猜测。

2026-08-06 对当前本地 `sysroot/sbin/xrdp` 的初步审计结果：直接 `DT_NEEDED` 包含
`libcommon.so.0`、`libipm.so.0`、`libxrdp.so.0`、`libtoml.so.1`、OHOS AVCodec 库、
`libopenh264.so.7` 和 `libc.so`，不直接依赖 `libxrdpserver.so`；仍需继续审计插件和传递依赖后
才能从 HNP 删除该库。当前二进制的 `RUNPATH` 还混入了构建机
`/mnt/c/Users/.../sysroot/lib/xrdp` 绝对路径，发布前必须在
`harmony/scripts/wsl/build-xrdp-ohos.sh`/libtool 链接参数中清除，只允许基于 `$ORIGIN` 的相对
路径。包门禁应拒绝包含 `/mnt/`、Windows 盘符、仓库绝对路径或其他构建机路径的 ELF。

2in1 `module.json5` 目标声明：

```json5
"hnpPackages": [
  {
    "package": "xrdp.hnp",
    "type": "private",
    "independentSign": false
  }
]
```

tablet `module.json5` 中不得存在 `hnpPackages` 字段，tablet HAP 压缩条目中不得存在 `hnp/` 目录。

## 8. 文件级修改方案

### 8.1 工程和模块

| 文件/目录 | 修改内容 | 完成标准 |
|---|---|---|
| `harmony/app/build-profile.json5` | 注册 `common`、`entry`、`entry_tablet` 模块；统一 product、SDK、版本和签名配置 | `assembleApp` 能识别三个模块，同设备仅一个 Entry |
| `harmony/app/common/` | 新建 Shared Library/HSP 模块 | 构建生成 `common.hsp` |
| `harmony/app/entry/` | 保留模块名，设备收窄到 `2in1`，只保留薄系统壳和 2in1 实现 | 2in1 HAP 含 HNP/PC 权限，UI 与现状一致 |
| `harmony/app/entry_tablet/` | 新建 tablet Entry 和不可用远控实现 | tablet HAP 无 HNP/PC 权限，RDP 客户端可用 |
| 两个 Entry 的 `oh-package.json5` | 依赖同一个应用内 `common` HSP | 无复制业务 UI |
| `common/oh-package.json5` | 声明共享模块和 Native 类型依赖 | 两个 Entry 加载同一共享实现 |

### 8.2 ArkTS 和资源迁移

| 当前范围 | 目标范围 | 说明 |
|---|---|---|
| `entry/src/main/ets/pages` | `common/src/main/ets/pages` | 保持同一页面实现 |
| `entry/src/main/ets/components` | `common/src/main/ets/components` | 不复制 tablet/2in1 UI |
| `entry/src/main/ets/adaptive` | `common/src/main/ets/adaptive` | 继续只按窗口断点布局 |
| `entry/src/main/ets/rdp` 客户端部分 | `common/src/main/ets/rdp` | Controller、会话、配置、权限协调共享 |
| `RemoteControlCoordinator`/`XrdpServerController` | `common` 中依赖 `RemoteControlPort` | 不直接 import 2in1 Native 模块 |
| `entry/src/main/resources` 业务资源 | `common/src/main/resources` | 图标、主题、字符串和页面资源共享 |
| `EntryAbility.ets` | 两个 Entry 各保留薄壳 | 注入能力后加载相同 UI 根组件 |
| `MuHubPrintExtension.ets` | 两个 Entry 薄壳 + common 实现 | HSP 不声明 ExtensionAbility |

每迁移一组文件都先保持 import 和路由可编译，再删除原文件。禁止一次性复制后长期维护两份。

### 8.3 Native 拆分

| 当前内容 | 目标 |
|---|---|
| `libentry.so` 中 FreeRDP 客户端、Surface、输入、通道 | `common.hsp` 中 `librdpclient.so` |
| `libentry.so` 中 XRDP runtime loader/server bridge | 2in1 Entry 中 `libxrdpcontrol.so` |
| `dlopen + xrdp_ohos_server_main + std::thread` | `fork + execve(HNP/bin/xrdp)` |
| XRDP `.so/config/share` 同时存在于普通 HAP libs | 仅保留 HNP 真实运行所需内容；普通 tablet HAP 为 0 |
| 当前 `libentry.so` 类型声明 | 分成 `librdpclient.so` 和 `libxrdpcontrol.so` 类型声明 |

为降低迁移风险，Native 分两步：

1. 先把客户端和服务端 CMake source list 分组，保持当前行为并通过现有测试；
2. 再生成两个 Native 模块，切换 XRDP 为 HNP 子进程，删除旧线程发布路径。

### 8.4 构建和封包脚本

| 文件 | 修改内容 |
|---|---|
| `harmony/app/build_hap.bat` | 改为兼容入口或拆成明确的 `build_tablet`、`build_2in1`、`build_app` 模式；不再默认只构建一个通用 HAP |
| `harmony/scripts/windows/package-xrdp-hnp.ps1` | 默认输出改到 2in1 Entry；审计独立进程依赖；保留缓存和路径安全检查 |
| `harmony/scripts/windows/repack-hap-with-hnp.ps1` | 参数化模块名和产物名，只允许重封 2in1 HAP；签名后检查 HNP、module profile 和关键依赖 |
| `harmony/scripts/wsl/build-xrdp-ohos.sh` | 收口 xrdp/HNP ELF 的 RUNPATH，只保留 `$ORIGIN` 相对路径并增加构建机绝对路径门禁 |
| 新增 `harmony/scripts/windows/package-multidevice-app.ps1` | 组合最终 HAP/HSP 为 App Pack；校验版本、bundleName、证书和 pack.info 一致性 |
| 新增 `tools/verify_multidevice_app.ps1` | 自动解包 APP/HAP/HSP，执行本方案第 10 节静态断言 |

脚本最终应提供三个明确产物集合：

```text
tablet-debug/
├─ entry_tablet-signed.hap
└─ common-signed.hsp

2in1-debug/
├─ entry-signed-hnp.hap
└─ common-signed.hsp

release/
└─ com.muhub.desktop-<version>-signed.app
```

`build_hap.bat` 当前命令在新脚本真正实现并执行成功前仍是当前单 HAP 构建入口；README 不得提前把计划命令写成已可用命令。

## 9. 实施顺序和提交边界

### MDP-00：包模型最小验证，阻断性 P0

不迁移业务 UI，先创建最小三个模块验证工具链和真机边界：

1. 空壳 `common.hsp`；
2. 只支持 2in1 的测试 Entry，携带最小 HNP；
3. 只支持 tablet 的测试 Entry，不含 HNP；
4. 生成同一 bundle 的 `.app`；
5. 解包检查模块、deviceTypes、权限、HNP 和签名；
6. 通过应用市场内部测试轨或等价分发验证两类设备实际收到的模块集合；
7. 验证现有版本覆盖升级，尤其是 tablet 从旧 `entry` 切换到 `entry_tablet`。

以下任一失败，停止后续迁移：

- App Pack 不接受两个互斥 deviceTypes 的 Entry；
- tablet 分发仍收到含 HNP 的 2in1 HAP；
- HNP HAP 使整个 App Pack 在 tablet 侧被拒；
- tablet 或 2in1 覆盖升级丢失数据、入口或签名身份；
- HSP 与两个 Entry 不能以同一签名集合安装。

### MDP-01：提取共享 UI/HSP

- 新建 `common.hsp`；
- 迁移 UI、资源、客户端 ArkTS 和公共状态；
- 两个 Entry 加载同一根组件；
- 保持 Native 暂时不变；
- 完成 UI 截图、路由、表单状态、窗口断点和会话连续性回归。

### MDP-02：权限和能力物理隔离

- 拆分两个 `module.json5`；
- tablet 删除录屏/控制权限和 XRDP 组件引用；
- 2in1 保留受限权限及 ACL；
- `RemoteControlPort` 注入替代共享层直接判断或直接调用 XRDP；
- 完成权限清单和实际授权行为验证。

### MDP-03：Native 拆分与 HNP 独立进程

- 拆出 `librdpclient.so` 和 `libxrdpcontrol.so`；
- 接入私有 HNP 可执行文件；
- 实现 PID、waitpid、停止、异常退出和日志；
- 删除发布路径的 `dlopen + std::thread`；
- 完成独立 PID 和 MSTSC 动作级验收。

### MDP-04：构建、分发和发布门禁

- 完成三个构建模式和自动解包门禁；
- 完成 clean install、upgrade、rollback 和应用市场内部测试；
- 更新 README、feature matrix、验证基线和旧专项文档；
- 只有全部 P0/P1 证据齐全后才替换当前 canonical 单 HAP 发布路径。

每个 MDP 工作包独立形成可审阅提交，禁止把模块创建、全部 UI 迁移、Native 进程改造和发布脚本塞进同一个提交。

## 10. 自动化包检查

新门禁脚本必须解析最终 `.app` 及其 HAP/HSP，不能只检查源码 `module.json5`。

### 10.1 App Pack 断言

- bundleName、versionCode、versionName、SDK 版本一致；
- 所有 HAP/HSP 签名证书一致；
- 存在且只有 `common.hsp`、2in1 Entry、tablet Entry 三个目标模块；
- 同一 deviceType 恰好一个 Entry；
- pack.info 的 `deviceType`、`moduleType`、`deliveryWithInstall` 正确；
- release App Pack 可由官方拆包工具完整解析。

### 10.2 tablet HAP 断言

- `deviceTypes == ["tablet"]`；
- 无 `hnpPackages`；
- 压缩条目 `hnp/` 数量为 0；
- 不包含 `CUSTOM_SCREEN_RECORDING`、`CONTROL_DEVICE`；
- 不包含 `libxrdpcontrol.so`、`libxrdpserver.so`、`libxrdpohos.so`、`bin/xrdp` 或 XRDP config/share；
- 包中只有薄 Entry/Extension 壳，业务 UI 和客户端实现来自 `common.hsp`。

### 10.3 2in1 HAP 断言

- `deviceTypes == ["2in1"]`；
- 只声明一个私有 `xrdp.hnp`，`independentSign=false`；
- 压缩条目存在 `hnp/arm64-v8a/xrdp.hnp`；
- 存在 `CUSTOM_SCREEN_RECORDING`、`CONTROL_DEVICE`；
- 存在 `libxrdpcontrol.so`；
- HNP 解包后存在可执行 `bin/xrdp` 和经 `DT_NEEDED` 审计的依赖；
- HNP 内全部 ELF 的 RUNPATH/RPATH 不含构建机绝对路径，只允许审核通过的 `$ORIGIN` 相对路径；
- 普通 HAP libs 不保留无用途的第二份 XRDP runtime。

### 10.4 common HSP 断言

- `type == shared`，deviceTypes 覆盖 tablet/2in1；
- 不声明 Ability/ExtensionAbility、HNP 和 PC 专属权限；
- 包含共享 UI、资源和 `librdpclient.so`；
- 不包含 XRDP 服务端启动代码和运行库。

自动化输出必须为机器可读结果，并在任一断言失败时返回非零退出码。

## 11. 真机验证矩阵

### 11.1 tablet

目标设备：已验证会拒绝 HNP 的 tablet 产品，至少包含现有 MatePad Pro 样机。

| ID | 操作 | 通过标准 |
|---|---|---|
| MDP-TAB-01 | 卸载后安装 tablet 模块集合 | 安装成功，无 HNP 8201/9568407/9568409 |
| MDP-TAB-02 | 从当前单 HAP 版本覆盖升级 | bundle 数据、连接配置和入口保留；若系统不支持换 Entry module，禁止发布并重新决策模块名 |
| MDP-TAB-03 | 冷启动、前后台、旋转、分屏、浮窗 | 无崩溃、白屏或 HSP 加载失败 |
| MDP-TAB-04 | RDP 客户端连接和断开 | 连接、显示、输入、resize 与当前基线一致 |
| MDP-TAB-05 | 剪贴板/音频/摄像头/定位/打印/远程文件 | 启用的客户端通道按需授权并可用 |
| MDP-TAB-06 | 权限页和请求日志 | 无录屏、控制设备权限项；请求次数均为 0 |
| MDP-TAB-07 | 旧远控路由/旧持久化状态/非法调用 | fail-closed，不进入 XRDP UI，不调用 Native XRDP |
| MDP-TAB-08 | UI 截图和 layout dump 对比 | 除既定 XRDP 隐藏外，无布局、文案、导航和状态回退 |

### 11.2 2in1

| ID | 操作 | 通过标准 |
|---|---|---|
| MDP-PC-01 | 卸载后安装 2in1 模块集合 | HNP 安装成功，应用启动成功 |
| MDP-PC-02 | 从当前版本覆盖升级 | bundle、module `entry`、数据和签名身份连续 |
| MDP-PC-03 | 启动 XRDP | 产生独立 xrdp PID；PID 不等于应用 PID，进程映射来自 HNP 路径 |
| MDP-PC-04 | 重复启动 | 不产生第二进程，UI 返回已运行状态 |
| MDP-PC-05 | MSTSC 连接 | 3390/转发路径可连接，画面、键鼠、剪贴板和已支持通道通过动作级回归 |
| MDP-PC-06 | 录屏和输入权限 | 未授权时明确阻断；授权后可工作；不静默退回旧线程实现 |
| MDP-PC-07 | 显式停止 | 子进程在阈值内退出并被回收，无残留监听端口 |
| MDP-PC-08 | 子进程异常退出 | UI 状态、退出码和诊断日志更新，可再次启动 |
| MDP-PC-09 | 应用强停、升级、卸载 | 无孤儿进程、无残留监听、HNP 按系统规则卸载 |
| MDP-PC-10 | UI 对比 | 远控入口、设置布局、验证码和权限流程与拆分前一致 |

### 11.3 分发

本地 `hdc install` 只能验证指定 HAP/HSP 集合，不能证明应用市场选择逻辑。发布前必须使用应用市场内部测试或等价真实分发链路：

| ID | 设备 | 通过标准 |
|---|---|---|
| MDP-DIST-01 | tablet | 实际下载模块只有 tablet Entry + common HSP；安装日志不解析/安装 HNP |
| MDP-DIST-02 | 2in1 | 实际下载模块只有 2in1 Entry + common HSP；HNP 正常安装 |
| MDP-DIST-03 | 商店包审查 | App Pack 设备声明、权限、HNP 和签名通过校验 |
| MDP-DIST-04 | 版本升级 | 两类设备都从上一正式版本成功升级，无数据清除和入口丢失 |

## 12. 测试命令规划

以下是实施后的目标命令名，当前尚不存在的脚本不得当作已验证能力：

```powershell
# 现有窄测试，迁移期间持续执行
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\run_tablet_arkts_tests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\run_tablet_native_tests.ps1

# MDP-04 已落地的三个 debug 构建入口
.\harmony\app\build_hap.bat app
.\harmony\app\build_hap.bat tablet
.\harmony\app\build_hap.bat 2in1

# 最终 App Pack 独立门禁
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_multidevice_app.ps1 `
  -AppPath <release-app-path>
```

本地真机验证按依赖顺序安装同设备需要的 HSP/HAP，并显式指定目标设备。当前 SDK
对同一条命令中的多个包没有保证依赖安装顺序，因此先安装 `common.hsp`，再安装设备 Entry：

```powershell
# 2in1
hdc -t <2in1-serial> install -r .\harmony\app\common\build\default\outputs\default\common-default-signed.hsp
hdc -t <2in1-serial> install -r .\harmony\app\entry\build\default\outputs\default\entry-default-signed.hap
hdc -t <2in1-serial> shell aa start -a EntryAbility -b com.muhub.desktop

# tablet
hdc -t <tablet-serial> install -r .\harmony\app\common\build\default\outputs\default\common-default-signed.hsp
hdc -t <tablet-serial> install -r .\harmony\app\entry_tablet\build\default\outputs\default\entry_tablet-default-signed.hap
hdc -t <tablet-serial> shell aa start -a TabletEntryAbility -b com.muhub.desktop
```

`.app` 是应用市场分发候选，不作为本地 HDC 模块安装替代品。2026-08-06 实测本地 debug
签名的 Hvigor 原生 `.app` 和 HNP 重封 `.app` 均被设备以 `9568329/verify signature failed`
拒绝；对应 HAP/HSP 使用相同签名材料可正常安装。这一结果不等同于应用市场 App Pack 校验失败，
MDP-DIST-01 至 04 仍须使用真实应用市场内部测试验证。

### 12.1 MDP-00 当前结果（2026-08-06）

- 新增 `common` shared HSP、`entry_tablet` tablet-only Entry，原 `entry` 收窄为 2in1-only；
- `entry` 和 `entry_tablet` 均从 `common` 导入同一探针，证明两个 Entry 可以依赖共享 HSP；
- `build_hap.bat` 已串起 HNP 生成、debug `assembleApp`、2in1 HAP HNP 重封和最终 App Pack；
- 最终 App Pack 静态检查通过：`common=[2in1,tablet]`、`entry=[2in1]+1 HNP`、
  `entry_tablet=[tablet]+0 HNP`；tablet Entry 仅有 `INTERNET` 和 `GET_NETWORK_INFO`；
- 2in1 `3QC0124C11000711` 已按 HSP→HAP 顺序安装并启动，日志出现
  `EntryAbility PACKAGING_PROBE common.hsp role=2in1`，`bm dump` 可见 `common`、`entry` 和
  `xrdp.hnp`；
- tablet `5JB0223804000371`（PCE-W30）从旧单模块版本覆盖验证：`common.hsp` 安装成功，
  随后安装 `entry_tablet.hap` 被 BMS 以 `9568267/install entry already exist` 拒绝；失败后
  已安装集合仍为 `entry + common`，`hnpPackages` 为空，旧 `EntryAbility` 可正常启动且未清数据；
- 上述结果证明“保留旧 `entry` 的同时用不同 moduleName 的 `entry_tablet` 替换 Entry”不是可接受
  的覆盖升级路径，当前双 Entry 主方案不能进入发布。下一步必须验证第 13.3 节的“通用 `entry`
  + 2in1-only Feature HAP”备选，或取得平台明确支持的同名设备变体封包方式；
- 用户卸载旧版后，同一 tablet 按 `common.hsp`→`entry_tablet.hap` 全新安装成功，
  `TabletEntryAbility` 启动成功；设备端模块仅为 `common,entry_tablet`，无 `xrdp.hnp`，权限仅为
  `INTERNET`、`GET_NETWORK_INFO`，日志出现 `PACKAGING_PROBE common.hsp role=tablet`；截图证据为
  `artifacts/multidevice-hnp/2026-08-06/muhub-tablet-packaging-probe.jpeg`；
- 真实应用市场按设备拆分仍未执行；全新安装通过不消除旧 `entry`→`entry_tablet` 覆盖升级 P0；
- 本工作包只证明包模型和 HNP 安装，不代表 HNP 独立 XRDP 子进程已实现；`fork/execve`、
  独立 PID 和退出清理仍属于 MDP-03。

### 12.2 MDP-01 当前结果（2026-08-06）

- 正式首页、设置、RDP 客户端协调器、资源和 Native bridge 已迁入 `common` HSP；`entry` 与
  `entry_tablet` 只保留各自 UIAbility、模块扩展和一个挂载 `MuHubApp` 的薄页面，不改原有 UI
  结构及交互。
- Native 运行库同步目标从 `entry/libs` 改为 `common/libs`，`libentry.so` 由共享 HSP 提供；2in1
  `entry.hap` 仅封装 `xrdp.hnp`，tablet `entry_tablet.hap` 不含 HNP 和 Native 库。
- `build_hap.bat` 完整构建、签名和包结构门禁通过；最终 App Pack 为 39,692,737 字节，避免了
  迁移中间态将同一套 Native 运行库重复塞入 HSP/HAP 造成的约 77 MB 产物。
- 2in1 `3QC0124C11000711` 按 `common.hsp`→`entry.hap` 覆盖安装并启动成功，正式首页布局与迁移前
  一致；截图证据为
  `artifacts/multidevice-hnp/2026-08-06/muhub-mdp01-pc-thin-foreground.jpeg`。
- `tools/run_tablet_arkts_tests.ps1` 已改为测试 `common`，ArkTS 单测与
  `tools/run_tablet_native_tests.ps1` 全部通过。
- tablet `5JB0223804000371` 已按 `common.hsp`→`entry_tablet.hap` 安装并启动正式 UI；设备端仅有
  `common,entry_tablet`，`hnpPackages` 为空，权限仅为 `INTERNET`、`GET_NETWORK_INFO`。
  分屏 Expanded 首页和浮窗 Compact 首页均正常，设置页仅显示基础设置、共享目录和项目帮助，
  不显示 2in1 远控服务能力。截图证据为
  `artifacts/multidevice-hnp/2026-08-06/muhub-mdp01-tablet-real-ui.jpeg`、
  `artifacts/multidevice-hnp/2026-08-06/muhub-mdp01-tablet-real-ui-fullscreen.jpeg` 和
  `artifacts/multidevice-hnp/2026-08-06/muhub-mdp01-tablet-settings.jpeg`。
- 当前共享 HSP 仍包含一体化 `libentry.so` 及 XRDP server 相关动态库；tablet 已做到“无 HNP、无
  PC 权限”，尚未做到服务端 Native 字节物理隔离。`librdpclient.so`/`libxrdpcontrol.so` 拆分及
  XRDP 独立进程仍由 MDP-03 完成，不能将 MDP-01 标记为发布完成。

### 12.3 MDP-02/03 当前结果（2026-08-06）

- `common` 只构建并导出 `librdpclient.so`，不再暴露 XRDP 启停接口；2in1 `entry` 单独构建
  `libxrdpcontrol.so`，通过注入的 `RemoteControlPort` 向共享 UI 提供能力，tablet 使用不可用实现，
  因而不会加载或调用服务端控制代码。
- tablet manifest 保留 RDP 客户端实际需要的网络、打印、剪贴板、麦克风、摄像头和定位权限，共
  8 项；`CUSTOM_SCREEN_RECORDING`、`CONTROL_DEVICE` 仍只属于 2in1 Entry。
- XRDP 改为 `fork/execve` 启动私有 HNP 中的 `bin/xrdp`，使用独立 PID；停止路径执行
  `SIGTERM`、限时等待和 `SIGKILL` 兜底，并用 `PR_SET_PDEATHSIG` 处理父进程异常退出。旧
  `dlopen`/进程内 server 线程路径和 `libxrdpserver.so` 打包均已删除。
- HNP ELF 门禁拒绝构建机 RUNPATH 和 `libxrdpserver.so` 依赖；当前 HNP 为 3,696,721 字节。
- tablet `5JB0223804000371` 重新安装启动通过，设备端仅有 `common,entry_tablet`，
  `hnpPackages` 为空、没有 xrdp 进程；正式 UI 截图为
  `artifacts/multidevice-hnp/2026-08-06/muhub-mdp03-tablet.jpeg`。
- 2026-08-06 实施时没有在线 2in1，独立 PID、3389/3390 连接、显式/异常/强停/卸载清理当时未验；
  2026-08-07 已通过 MDP-03A 补齐启动、3390 监听和应用强停清理，MSTSC 会话、显式 UI 停止、异常
  退出和卸载清理仍需继续补验。

#### MDP-03A：修正私有 HNP 沙箱运行路径

- 状态：`Implemented / 2in1 runtime verified`。
- 触发证据：2026-08-07 在 2in1 `3QC0124C11000711` 的木枢进程挂载命名空间中确认，私有
  `xrdp.hnp` 暴露为 `/data/app/xrdp.org/xrdp_0.1.0`，清单生成的稳定可执行文件链接为
  `/data/app/bin/xrdp`；`/data/service/hnp` 是公共 HNP 沙箱挂载点，在该应用内为空。当前 bridge
  错用 `/data/service/hnp/xrdp.org/xrdp_0.1.0`，因此在 `fork()` 前的 runtime 检查即失败。
- 修改范围：仅调整 2in1 `libxrdpcontrol.so` 的 HNP 路径解析，并在独立进程策略门禁中固定私有
  HNP 沙箱约束；不修改 tablet Entry、共享 UI、权限或分发模块边界。
- 目标行为：可执行文件从无版本号链接 `/data/app/bin/xrdp` 启动，并用 `realpath()` 解析链接当前指向的
  版本目录，再从真实的 `.../bin/xrdp` 路径反推 `lib/config/share` 根目录；运行时代码不得硬编码
  `xrdp_0.1.0` 等 HNP 版本号；
  不允许硬编码宿主机 `/data/app/el1/bundle/<userId>/hnp/<bundleName>` 物理路径，也不回退到进程内
  XRDP。
- 验收：静态门禁通过；2in1 HAP/App Pack 构建与覆盖安装通过；启动后 xrdp 为独立 PID，监听
  `3390`，日志不再出现 `HNP xrdp runtime is unavailable`；tablet 包继续无 HNP、无
  `libxrdpcontrol.so`。显式停止后进程和端口均消失。
- 实施结果：`xrdp_server_internal.h` 只保留 `/data/app/bin/xrdp`；`xrdp_runtime_loader.cpp` 用
  `realpath()` 从该链接动态反推 HNP 根目录；`verify_xrdp_process_control.ps1` 拒绝公共 HNP、宿主机
  物理路径和固定版本号。无设计偏差。
- 验证结果：`verify_xrdp_process_control.ps1` 与 `git diff --check` 通过；2in1 Native/ArkTS/HNP
  重封及签名构建成功，HAP 为 5,384,040 字节，覆盖安装成功。木枢 PID `32250` 启动独立 xrdp
  PID `32402` 并监听 `0.0.0.0:3390`；应用强停后进程和端口均消失，重启后木枢 PID `32318`、
  xrdp PID `32704` 再次监听 3390。tablet HAP 构建成功，为 197,649 字节，包内 37 项且无 HNP、
  无 `libxrdpcontrol.so`。尚未执行 MSTSC 会话、显式 UI 停止、异常退出和卸载清理。

### 12.4 MDP-04 当前结果（2026-08-07）

- `build_hap.bat` 支持 `app`、`tablet`、`2in1` 三种显式模式；三种模式均已实际构建通过。
- `tools/verify_multidevice_app.ps1` 校验 App 签名、精确模块/设备类型、权限边界、HNP 唯一归属、
  Native 库边界、HNP 文件结构及 xrdp ELF 依赖/RUNPATH；`tools/verify_xrdp_process_control.ps1`
  阻止旧进程内 server 路径回归。
- 2in1 与 tablet Entry 的 35 项产品资源逐文件一致，均注册 `MuHubPrintExtension`；tablet 真机
  `bm dump` 已确认打印扩展存在。交付包不再包含打包探针、`probe_icon` 或诊断用
  `libfreerdp_ohos_probe.so`。
- 最终 App Pack 为 39,799,360 字节，SHA-256 为
  `1c5a2d542fb794557819fea7672a28077973e89df32ca475725bd7c9fc200bb2`，本地门禁通过。
- MDP-00 已确认旧 `entry` 到 `entry_tablet` 的覆盖升级失败；真实应用市场按设备选包也未执行。
  因此该 App Pack 仍是工程验证产物，不能替换发布基线。发布前必须验证第 13.3 节的通用 Entry +
  2in1-only Feature 方案，或取得平台支持的同名设备变体方案。

### 12.5 MDP-04A release 跨模块导出兼容性修复（2026-09-01）

- 故障范围：`1.0.2`（`versionCode=1000002`）release App Pack 在 2in1 上首次安装启动，以及从
  `1000001` 覆盖安装后启动。三份 HiviewDFX 日志均在 `EntryAbility.abc` 模块实例化阶段报告
  `common/Index` 不提供 `entry` 请求的混淆导出名 `u1`；`libark_jsruntime.so` 回溯是该
  `SyntaxError` 的承载栈，不是 Native 崩溃根因。
- 根因边界：`entry` 在拆分为依赖 `common.hsp` 的薄壳后仍启用了 `-enable-export-obfuscation`。
  release 编译把 HSP 公共 API 的消费侧导入名改写为 `u1`，而独立构建的 `common.hsp` 继续导出
  `SettingsTheme`、`RemoteFilesDirectory`、`ImeHostWindowBinder` 等稳定名称，导致链接契约不一致。
- 目标策略：HSP 边界上的导出名属于模块 ABI，必须保持稳定；`entry` 仍可保留属性、模块顶层内部
  名称和文件名混淆，但不得启用 export obfuscation。Native N-API 属性白名单保持不变。
- 允许修改范围：`harmony/app/entry/obfuscation-rules.txt`、App Pack 静态门禁、本文档和验证记录；
  不修改业务逻辑、持久化数据、bundle/module 身份、签名材料或 HNP 生命周期。
- 验收：静态门禁拒绝任一 Entry 再启用 `-enable-export-obfuscation`；release App Pack 重新构建并
  通过既有包结构/签名门禁；新包在 2in1 上全新安装与 `1000001 -> 1000002` 覆盖安装后均可启动，
  日志不再出现 `common/Index` 缺少导出名的 `SyntaxError`。未完成真机两条启动路径前状态不得标为
  `Verified`。
- 实施结果：删除 `entry` 的 export obfuscation 开关，保留其余混淆与 N-API 属性白名单；
  `verify_multidevice_app.ps1` 同时增加源规则门禁，并用 SDK `ark_disasm` 反汇编 App Pack 内三个模块的
  `ets/modules.abc`，逐项确认两个 Entry 对 `common/Index` 的运行时导入均存在于 `common.hsp`
  导出集合。无业务逻辑、数据格式、签名身份或 HNP 行为变化。
- 验证结果：release `assembleApp`、HNP 重封、App Pack 重组和签名完成；最终包
  `harmony/app/build/outputs/default/app-default-signed.app` 为 `20,033,437` 字节，SHA-256 为
  `a76d9c650c8c4e3bf7dd73628b2a76357768e880af3b79ad50c9e89fc15b37ca`。App Pack 门禁输出
  `arktsCommonAbi=passed`；2in1 Entry 的 8 个公共导入均恢复为稳定名称，不再包含故障导入名 `u1`。
  真机全新安装和 `1000001 -> 1000002` 覆盖安装尚未执行，因此状态为 package verified，非
  `Verified`。

### 12.6 Release 正式签名测试包重建（2026-09-05）

- Change ID：CHG-20260905-003；状态：Implemented / package verified。
- 本次为本地交付操作：临时将 `harmony/app/build-profile.json5` 的证书/profile 切换到现有 `muhub_release.cer` / `muhub_releaseRelease.p7b`，执行 clean 与 release assembleApp；结束后恢复原配置。
- HNP 重封和 App Pack 重签均显式使用相同 Release 材料，保留 xrdp HNP 功能；不修改业务代码和包身份。
- 验收：签名验证、三个模块实际 ABC 公共导入/导出检查通过；检查包内 release profile 与非 debug 构建标志。验证成功后清理本次输出目录中的旧 Debug 包、HNP 重复包、未签名包和打包临时目录，仅交付最终 `app-default-signed.app`。清理限于仓库内生成目录，不删除签名材料和源代码。
- 真机启动回归由用户使用新包进行，不把包检查记为真机启动通过。
- 执行结果：clean + release assembleApp、HNP 正式重签、App Pack 正式重签与 `verify_multidevice_app.ps1` 均通过，`arktsCommonAbi=passed`。三个模块 `debug=false`；App Pack 与三个源模块的签名均验证通过，提取的 profile 与 `muhub_releaseRelease.p7b` 哈希一致，内容 `type=release`。App Pack 内模块由打包工具处理，不以嵌套包可单独验签作为门禁。
- 最终 App Pack：20,010,801 bytes；SHA-256 `d1fb89ed242bf85b07aac32b174dbd7b0e0d952c21888343e272342e6ccc69cc`。本次临时签名配置已原样恢复；日常 `build_hap.bat app` 仍是 Debug 默认入口，不能把本次操作理解为默认脚本已改成 Release。
- 清理结果：自动审批策略阻止删除命令；改用可恢复移动，将 App 输出目录的重复包、未签名包、打包输入、符号和验证目录移至本机忽略目录 `tmp/release-build-intermediates`。主输出目录只保留 `app-default-signed.app`；不宣称中间文件已物理删除。

## 13. 升级、回滚和发布策略

### 13.1 升级

- 2in1 保留 `entry` moduleName，必须验证从当前正式 HAP 覆盖升级。
- tablet 将从当前 `entry` 迁移到 `entry_tablet`，这是本方案最高升级风险，MDP-TAB-02 和 MDP-DIST-04 是阻断项。
- 如果平台不允许该 Entry module 迁移，不能要求用户静默清数据解决。必须在 MDP-00 重新选择 moduleName/target 组合或制定明确版本迁移策略后再继续。
- 公共数据库、首选项和文件继续使用 bundle 沙箱，不按 moduleName 创建第二份业务数据。

### 13.2 回滚

- MDP-00 至 MDP-03 期间，当前无 HNP 的单 HAP 仍是 tablet 可安装基线，不替换 canonical 产物。
- 新 App Pack 未完成应用市场分发验证前，不删除现有构建脚本和安装说明。
- 新版本一旦改变 Entry module 集合，回滚到旧单 HAP 也要真机验证，不能只验证向前升级。
- HNP 启动失败不允许运行时回退到进程内 XRDP；发布回滚应回到上一完整版本。

### 13.3 备选方案

如果 MDP-00 证明两个设备 Entry 的升级或 App Pack 分发不可接受，备选方案才是：

```text
通用 Entry HAP（tablet + 2in1，保留全部 UI）
+ 2in1-only Feature HAP（只承载 HNP/PC 权限）
```

启用备选方案前必须单独证明：Feature 中声明的 `usedScene` 权限可由通用 EntryAbility 正确申请、HNP 子进程获得预期 AccessToken、零 UI Feature HAP 通过工具链和商店校验。未证明前不作为主方案。

## 14. 实施台账

| Change ID | 状态 | 目标 | 允许修改范围 | 验收 |
|---|---|---|---|---|
| MDP-00 | Implemented / P0 failed | 最小多 Entry/HSP/HNP App Pack 和升级验证 | 新测试模块、build-profile、临时封包脚本 | 2in1 与 tablet 全新安装/启动通过，tablet 无 HNP/PC 权限；但旧 `entry`→`entry_tablet` 覆盖升级返回 9568267，双 Entry 主方案停止进入发布 |
| MDP-01 | Implemented / device verified | 正式 UI、资源、RDP 客户端与 Native bridge 迁入共享 HSP，两个 Entry 薄壳挂载同一 `MuHubApp` | `common`、两个 Entry、运行库同步和测试脚本 | 构建/包门禁/ArkTS/Native 通过；2in1 与 tablet UI 真机通过，tablet 无 HNP/PC 权限，Native 物理拆分留 MDP-03 |
| MDP-02 | Implemented / tablet verified | 权限和能力物理隔离 | 两个 manifest、能力注入、签名 profile | tablet 包与设备权限通过；2in1 权限回归待设备在线补验 |
| MDP-03 | Implemented / package verified | HNP 独立 XRDP 进程 | xrdp bridge、CMake、HNP 脚本 | 编译、进程策略和包门禁通过；2in1 独立 PID/连接/清理待补验 |
| MDP-03A | Implemented / 2in1 runtime verified | 修正私有 HNP 沙箱运行路径 | xrdp runtime loader、进程策略门禁、验证基线 | 从 `/data/app/bin/xrdp` 动态解析当前版本根目录；2in1 独立 PID/3390/强停清理通过；tablet 物理隔离不变；MSTSC/显式停止/异常/卸载待验 |
| MDP-04 | Implemented / release blocked | 构建、包门禁和分发收口 | 构建脚本、验证脚本、README/基线 | 三模式与本地门禁通过；覆盖升级 P0 和应用市场真实分发未通过 |
| MDP-04A | Implemented / package verified | 修复 release Entry 与 `common.hsp` 公共导出名不一致导致的启动 JsCrash | Entry 混淆规则、App Pack 门禁、发布验证记录 | 禁止 Entry export obfuscation；release 构建、签名、包结构及实际 ABC 导入/导出门禁通过；2in1 全新安装和 `1000001 -> 1000002` 覆盖安装启动待真机验证 |

状态定义：

- `Planned`：有方向，尚未完成文件/API/验收审阅；
- `DesignReady`：设计、范围、阻断项和验收已明确，可以开始该工作包；
- `Implemented`：代码完成，但真机/分发证据未齐；
- `Verified`：对应自动化、真机、升级和分发证据全部通过。

## 15. 完成定义

只有同时满足以下条件，才能把多设备 HNP 架构标记为 Verified 并替换当前发布基线：

1. 最终 `.app` 包含两个互斥设备 Entry 和一个 common HSP，官方工具解析无错误。
2. tablet 实际分发和安装集合中完全没有 HNP、`hnpPackages`、PC 权限和 XRDP 服务端运行库。
3. 2in1 实际分发和安装集合包含私有 HNP，XRDP 以独立 PID 运行，不使用旧线程发布路径。
4. 两类设备使用同一 UI 实现，截图、布局、路由、状态和 RDP 会话回归通过。
5. tablet/2in1 从上一正式版本覆盖升级以及必要的回滚测试通过。
6. 权限清单、实际授权、ACL、签名证书和商店审核材料一致。
7. 显式启停、异常退出、应用强停和卸载均无孤儿进程和残留端口。
8. App Pack 经真实应用市场内部测试证明按设备选择正确模块，而不只是本地手工安装成功。
9. README、feature matrix、验证基线和旧单 HAP 专项文档已同步当前事实。
10. 所有验证记录使用实际命令、退出码、包哈希、设备信息和日志证据；未执行项明确标记未执行。

## 2026-09-05 本地改动归档（CHG-20260905-001，历史记录）

后续 SCOPE-20260905-001 已删除录屏模式和独立文件传输设计，下列内容仅描述当时恢复的范围，不能作为当前功能入口。现行范围见 [项目范围](project-scope-and-session-controls.md)。

- 状态：Implemented；将原 stash 的打包文件名修正、签名配置与密码解析、跨 HSP 导出混淆门禁、CPU-RECORD-001 录屏配置和 FT-ARCH-001 独立文件传输设计恢复到最新 main。
- 文件范围：本设计对应的打包/签名脚本与 tools/verify_multidevice_app.ps1；CPU 代码范围见 feature matrix 的 CPU-RECORD-001；独立客户端范围见文件传输设计稿。保留原 stash 全部 19 个文件，额外更新仓库修改记录和本次验证记录。
- 本次验证：tools/run_tablet_native_tests.ps1 通过；4 个新增/修改 PowerShell 脚本语法解析通过；decrypt-hvigor-password.js 的 Node --check 通过；git diff --check 通过。
- 验证限制：tools/run_tablet_arkts_tests.ps1 在 Entry 资源一致性门禁失败；检查发现 8 个资源文件仅 CRLF/LF 不同，换行归一化后文本一致，且本次未修改这些资源。本次未重新构建 HAP/App Pack、未进行签名或真机验证，历史验证记录不代表本次验证。
