# 木枢

基于 FreeRDP 的 HarmonyOS 远程桌面客户端 Demo。仓库同时保留了本地 Web/Node 验证工具，以及 HarmonyOS App 的 native bridge 接入代码。

GitHub: https://github.com/xiaomu120413/harmony-windows-bridge

## 功能概览

- HarmonyOS HAP 客户端：填写 Windows host、端口、用户名、密码后发起 RDP 连接。
- FreeRDP native bridge：ArkTS 通过 NAPI 调用 native 层，远程画面通过 `XComponent` surface 显示。
- xrdp/MSTSC 路径：仅 2in1 HAP 携带 xrdp HNP，并通过独立子进程监听本机 `3390`；tablet HAP 不含 HNP、PC 权限或 XRDP 服务端库。
- 证书策略：支持 `TOFU` 和 `Strict`，用于测试和更严格的证书校验。
- 权限回调：远程会话请求剪贴板、麦克风、摄像头或地理位置时，由应用侧通用权限桥触发系统权限处理。
- 打印重定向：默认向 Windows 暴露虚拟打印机，Windows 实际提交打印作业时才启动 HarmonyOS PrintKit。
- 设置页：包含深色模式、浅色模式、跟随系统、使用说明、本机 IP、关于项目和第三方开源组件信息。
- 本地 Web Demo：用于在桌面侧先验证 RDP 网络、账号和 FreeRDP 可用性。

## 目录结构

- `harmony/app/`: HarmonyOS 应用工程，HAP 构建入口。
- `harmony/third_party/FreeRDP/`: HarmonyOS 侧使用的 FreeRDP 三方源码和许可证文件。
- `harmony/third_party/xrdp/`: HarmonyOS 侧 xrdp 服务端源码和 OHOS backend。
- `app/native/freerdp-bridge/`: 桌面侧 FreeRDP library/native bridge 骨架。
- `app/`: 本地浏览器界面和 Node 后端 Demo。
- `docs/README.md`: 当前文档索引，区分活文档和历史归档。
- `docs/freerdp-ohos-feature-matrix.md`: 当前 OHOS FreeRDP 功能边界。
- `docs/freerdp-ohos-validation-baseline.md`: 构建、同步、打包和真机验收基线。
- `docs/windows-rdp-environment-setup.md`: Windows RDP 服务端和网络排查说明。
- `docs/release-third-party-notices.md`: 第三方组件 NOTICE 和许可证履约材料。
- `docs/archive/`: 调试复盘和旧真机记录，不作为当前 source of truth。
- `config.example.json`: 桌面 Demo 连接配置模板，不提交真实密码。

## HarmonyOS 构建

准备：

- DevEco Studio / HarmonyOS SDK。
- 可用的 `hdc` 设备连接。
- 目标 Windows 机器已开启远程桌面，并且当前设备能访问目标机 TCP `3389` 端口。
- 目标账号允许远程登录，且不能使用空密码。

构建完整 App Pack，或显式构建 tablet/2in1 设备包：

```powershell
cd harmony\app
.\build_hap.bat app
.\build_hap.bat tablet
.\build_hap.bat 2in1
```

工程启用 Hvigor 自动签名时，`tools/hapsigner/material` 是加密签名密码的解密材料，必须与 `harmony/app/build-profile.json5` 中的加密密码配置配套保留。若 `SignHap` 报 `ENOENT ... tools/hapsigner/material`，应先恢复该目录中的受版本管理文件；这类错误不是证书、P12 或包名不一致。

FreeRDP runtime 变更后应先按 `docs/freerdp-ohos-validation-baseline.md` 重建并同步 `harmony/out/ohos-arm64/runtime-libs`。

xrdp runtime 变更后，先构建 `harmony/out/xrdp-ohos-arm64`，再运行 `app` 或 `2in1` 模式；脚本会打包 `xrdp.hnp` 并只注入 2in1 HAP。`tablet` 模式不会生成或封装 HNP。

构建产物默认位于：

```text
harmony\app\build\outputs\default\app-default-signed.app
harmony\app\common\build\default\outputs\default\common-default-signed.hsp
harmony\app\entry\build\default\outputs\default\entry-default-signed.hap
harmony\app\entry_tablet\build\default\outputs\default\entry_tablet-default-signed.hap
```

安装到设备：

```powershell
hdc list targets
hdc install -r harmony\app\common\build\default\outputs\default\common-default-signed.hsp
# 按设备二选一安装对应 Entry
hdc install -r harmony\app\entry\build\default\outputs\default\entry-default-signed.hap
hdc install -r harmony\app\entry_tablet\build\default\outputs\default\entry_tablet-default-signed.hap
```

## HarmonyOS 使用说明

1. 在主界面填写 `Windows host`、`Port`、`Username` 和 `Password`。
2. 选择证书策略。内网测试可用 `TOFU`，更严格环境使用 `Strict`。
3. 点击 `Connect` 后，应用会调用 native FreeRDP 会话并打开远程桌面 surface。
4. 远程文件：应用启动后会通过系统下载控件准备 `Download/com.muhub.desktop`。连接 Windows 后，在远程桌面中打开 `\\tsclient\Downloads`，可与该目录互传小文件；当前只共享这个固定目录。
5. 进入 `设置` 可以查看使用说明、本机 IP、关于信息，或切换深色/浅色/跟随系统。

## xrdp / MSTSC 反向控制

应用启动后会尝试启动 xrdp 服务端，默认监听设备侧 `3390`。在 Windows 侧通过 HDC 转发后，用 MSTSC 连接本地转发端口：

```powershell
hdc fport tcp:13390 tcp:3390
mstsc /v:127.0.0.1:13390
```

主界面可以开启 xrdp 验证码门禁；开启后，远程登录需要输入界面显示的 6 位验证码。

常见排查：

- 连接失败时优先检查目标 IP、同一网络、Windows 防火墙、远程桌面开关、账号权限和端口 `3389`。
- Windows Home 通常不能作为标准远程桌面主机。
- 生产环境不要忽略证书风险，也不要把 RDP 密码写入脚本、配置文件或命令行。

## 本地 Web Demo

安装依赖后启动：

```powershell
npm start
```

然后访问：

```text
http://127.0.0.1:5173
```

页面里可以填写目标 Windows 机器 IP、用户名、端口等信息，先点击测试端口确认 `3389` 可访问，再尝试连接。

连接引擎有两种：

- `FreeRDP library / native bridge`: 推荐方向，适合后续迁到 HarmonyOS。
- `wfreerdp executable / 兼容模式`: 调用现成 `wfreerdp.exe`，适合先验证 Windows RDP 网络和账号。

如果没有构建 native bridge，可以先切到兼容模式。仓库的 `tools/freerdp/wfreerdp.exe` 会被应用自动识别；如果你换成自己的 FreeRDP 构建，在页面高级选项里填写 `wfreerdp.exe` 完整路径。

## Windows 目标机准备

目标 Windows 机器需要在系统设置中开启远程桌面，允许目标账号远程登录，并确保当前设备可以访问 TCP `3389`。本地 Web Demo 内置“测试端口”能力，可以直接检查目标地址的 TCP/RDP 握手。

## 第三方开源组件

完整 NOTICE 以 `docs/release-third-party-notices.md` 为准。当前关于页展示的主要组件包括：

- FreeRDP / WinPR: Apache-2.0
- OpenSSL: Apache-2.0
- FFmpeg: LGPL 组件，最终以构建配置为准
- OpenH264: BSD-2-Clause
- zlib: zlib License
- cJSON: MIT
- uriparser: BSD-3-Clause
- LLVM libc++ runtime: Apache-2.0 WITH LLVM-exception

## 许可证状态

项目代码采用 MIT License，见仓库根目录 `LICENSE`。

第三方组件按各自许可证履约，发布包需要同步归档对应 LICENSE、NOTICE 和构建配置说明。

## 安全边界

- 不要把 RDP `3389` 端口直接暴露到公网。
- 跨网络控制建议使用 VPN、受控隧道或 RD Gateway。
- 生产环境不要使用忽略证书校验的策略。
- 不要提交真实账号、密码、证书、签名材料或设备私有配置。
