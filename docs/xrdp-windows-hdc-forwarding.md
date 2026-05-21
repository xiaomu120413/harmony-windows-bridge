# xrdp Windows HDC 端口转发连接说明

本文只描述 xrdp 服务端线路：Windows 自带 `mstsc` 通过 HDC 端口转发连接到 HarmonyOS 设备上的 xrdp server。FreeRDP 客户端线路不适用本文。

## 当前链路

```text
Windows mstsc
  -> Windows 本机 127.0.0.1:13390
  -> hdc fport
  -> HarmonyOS 设备 127.0.0.1:3390 / 0.0.0.0:3390
  -> HAP 内嵌 xrdp server
  -> xrdp OHOS backend
```

当前建议先用这条 HDC 转发链路验证能力。这样不依赖 HarmonyOS 设备和 Windows 是否处在同一可直连网段，也避免设备侧端口暴露到局域网。

## 前置条件

- Windows 上可以执行 `hdc`，并且能看到 HarmonyOS 设备。
- HAP 已安装并启动。
- HAP 启动后会拉起内嵌 xrdp server，设备侧监听端口为 `3390`。
- 当前连接阶段至少需要 `ohos.permission.INTERNET`，用于监听 TCP 端口并接受 RDP 连接。
- 如果后续启用真实屏幕采集，还需要屏幕录制授权能力；这和 TCP 转发本身是两件事。

## Windows 侧操作

确认设备在线：

```powershell
hdc list targets
```

建立端口转发：

```powershell
hdc fport tcp:13390 tcp:3390
```

查看转发是否存在：

```powershell
hdc fport ls
```

验证 Windows 本地端口是否可连：

```powershell
Test-NetConnection 127.0.0.1 -Port 13390
```

启动 Windows 自带远程桌面：

```powershell
mstsc /v:127.0.0.1:13390
```

也可以在 mstsc UI 的“计算机”里填：

```text
127.0.0.1:13390
```

注意：使用 HDC 转发时，mstsc 连接的是 Windows 本机端口 `127.0.0.1:13390`，不是 HarmonyOS 设备的 Wi-Fi IP。如果填 `172.x.x.x` 这类地址，就会绕过 HDC 转发，变成局域网直连验证。

## HarmonyOS 侧检查

查看设备是否能看到 xrdp 监听：

```powershell
hdc shell "netstat -an | grep 3390"
```

如果设备镜像没有 `grep`，可以先直接看端口列表：

```powershell
hdc shell "netstat -an"
```

查看 HAP/xrdp 日志：

```powershell
hdc hilog -x | findstr /i "xrdp rdp mstsc bridge ohos"
```

期望看到的信息包括：

- HAP native bridge 初始化 xrdp runtime 路径。
- xrdp server 监听 `3390`。
- mstsc 连接进入后有 client/session/backend 相关日志。
- 真实画面线路启用后，持续有帧输出或显示几何信息。

## 直连模式

如果后续要验证不经过 HDC 的局域网直连，需要满足：

- HarmonyOS 设备和 Windows 在同一个可互通网络。
- 设备侧 `3390` 对 Windows 可达。
- 网络没有 AP isolation、VPN 路由抢占或防火墙拦截。
- 当前 xrdp bring-up 配置的安全策略只适合内网调试，不应暴露到不可信网络。

直连时 mstsc 填：

```text
<HarmonyOS设备IP>:3390
```

直连失败时，在 Windows 上先验证：

```powershell
Test-NetConnection <HarmonyOS设备IP> -Port 3390
```

## 常见问题

### mstsc 提示无法连接远程计算机

优先按顺序确认：

1. `hdc list targets` 能看到设备。
2. HAP 已经启动，并且没有崩溃退出。
3. `hdc fport ls` 里有 `tcp:13390 -> tcp:3390`。
4. `Test-NetConnection 127.0.0.1 -Port 13390` 成功。
5. 设备日志里 xrdp 已监听 `3390`。
6. mstsc 填的是 `127.0.0.1:13390`，不是设备 IP。

如果本地端口冲突，换一个 Windows 本机端口：

```powershell
hdc fport tcp:13391 tcp:3390
mstsc /v:127.0.0.1:13391
```

### 已连接但黑屏或只有首帧

这通常说明 TCP/RDP/xrdp 会话已经通了，问题转到显示采集或帧发送线路：

- 检查屏幕录制授权是否完成。
- 检查 xrdp backend 是否收到帧并调用图像更新。
- 检查 HAP 进程是否仍在运行。
- 检查日志里是否持续出现 frame/update/geometry 相关记录。

### 鼠标或键盘无效

这通常说明 RDP 输入已经进入 xrdp，但 OHOS 输入注入或坐标映射还有问题：

- xrdp 侧鼠标滚轮按 `WM_BUTTON4/5DOWN/UP` 语义进入 backend。
- OHOS 侧需要把这些事件转换成鼠标轴事件。
- 坐标映射要以 xrdp desktop 内容区域和真实屏幕显示区域为准，不能简单按整屏比例硬缩放。

## 当前结论

HDC 转发链路是当前 bring-up 阶段最稳的验证方式：

- Windows 只需要使用系统自带 `mstsc`。
- HarmonyOS 设备不需要先解决局域网入站可达问题。
- xrdp server 仍然运行在 HAP/native/HNP 线路里，验证目标没有变。
- 等画面、输入、权限和安全策略稳定后，再切到 `<HarmonyOS设备IP>:3390` 的直连模式验证。
