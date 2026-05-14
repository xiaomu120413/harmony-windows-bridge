# FreeRDP remote-control demo

这个仓库是统一仓库：包含 FreeRDP/RDP 控制端 demo、FreeRDP library/native bridge 骨架、Windows RDP 环境排查文档，以及可复用的 Codex troubleshooting skill。

## 目录

- `app/`: 本地应用，浏览器界面 + Node 后端。
- `native/freerdp-bridge/`: 链接 FreeRDP 三方库的 native bridge 骨架。
- `docs/harmonyos-porting.md`: 后续迁移到鸿蒙应用的结构说明。
- `docs/windows-rdp-environment-setup.md`: Windows 远程桌面服务端、用户、网络、VPN/Guest Wi-Fi 排查文档。
- `skills/windows-rdp-troubleshooting/`: 可复用的 Codex skill 草稿，用于以后排查 RDP/FreeRDP 连接问题。
- `config.example.json`: 连接配置模板，不放密码。
- `scripts/Test-FreeRdpDemo.ps1`: 检查 FreeRDP 是否可用、目标机 3389 端口是否通。
- `scripts/Connect-FreeRdpDemo.ps1`: 调用 `wfreerdp`/`xfreerdp` 发起远程桌面连接。
- `scripts/Enable-WindowsRdpTarget.ps1`: 在目标 Windows 机器上开启远程桌面和防火墙规则。

## 应用方式运行

启动本地应用：

```powershell
npm start
```

然后打开：

```text
http://127.0.0.1:5173
```

在页面里填写目标 Windows 机器 IP、用户名、端口等信息，点击“测试端口”确认 `3389` 可访问，再点击“连接”。

连接引擎有两种：

- `FreeRDP library / native bridge`: 推荐方向，适合后续迁到鸿蒙。需要先构建 `native/freerdp-bridge`。
- `wfreerdp executable / 兼容模式`: 调用现成 `wfreerdp.exe`，适合先验证 Windows RDP 网络和账号。

如果没有构建 native bridge，可以先切到兼容模式。仓库的 `tools/freerdp/wfreerdp.exe` 会被应用自动识别；如果你换成自己的 FreeRDP 构建，在页面的“高级选项”里填写 `wfreerdp.exe` 完整路径。

## 构建 FreeRDP library bridge

如果已经有 vcpkg 和 VS C++ Build Tools，可以直接运行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\Build-NativeBridge.ps1
```

或者手动准备 FreeRDP 开发库，然后设置 `FREERDP_ROOT` 指向安装目录：

```powershell
$env:FREERDP_ROOT = "C:\path\to\freerdp-install"
cmake -S native\freerdp-bridge -B native\freerdp-bridge\build
cmake --build native\freerdp-bridge\build --config Release
```

构建完成后，在应用高级选项里填写生成的 `freerdp_bridge.exe` 路径，或把它放到：

```text
native\freerdp-bridge\build\Release\freerdp_bridge.exe
```

当前 bridge 已经链接并调用 FreeRDP 库做探测；真正的应用内画面渲染和输入回调是下一阶段 native 工作。

## 你需要准备什么

1. 控制端安装 FreeRDP。

   这台机器当前没有检测到 `wfreerdp` 或 `xfreerdp`。Windows 上可以从 FreeRDP 官方预构建页面下载安装包或静态构建版本，然后把 `wfreerdp.exe` 所在目录加入 `PATH`，也可以运行脚本时用 `-FreeRdpPath` 指定完整路径。

   官方入口：
   - [FreeRDP 项目](https://github.com/FreeRDP/FreeRDP)
   - [FreeRDP Prebuilds](https://github.com/FreeRDP/FreeRDP/wiki/Prebuilds)

2. 目标机器开启 RDP。

   如果目标机器是 Windows，建议用管理员 PowerShell 在目标机器上执行：

   ```powershell
   Set-ExecutionPolicy -Scope Process Bypass
   .\scripts\Enable-WindowsRdpTarget.ps1
   ```

   目标 Windows 机器需要支持作为 RDP 主机。Windows Home 通常不能作为标准远程桌面主机。

3. 两台机器网络互通。

   控制端需要能访问目标机的 TCP `3389` 端口。目标机 IP 可以在目标机上运行 `ipconfig` 查看。

4. 目标账号必须有密码。

   Windows RDP 默认不接受空密码登录。目标账号也需要被允许远程登录，管理员账号通常可以，普通账号需要加入 `Remote Desktop Users` 组。

## 快速运行

复制配置模板：

```powershell
Copy-Item .\config.example.json .\config.local.json
notepad .\config.local.json
```

把 `host` 和 `user` 改成你的目标机器信息，例如：

```json
{
  "host": "192.168.1.20",
  "port": 3389,
  "user": "TARGET-PC\\demo",
  "domain": "",
  "certMode": "tofu",
  "size": "1400x900",
  "fullscreen": false,
  "clipboard": true,
  "sharePath": ""
}
```

先检查环境和连通性：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\Test-FreeRdpDemo.ps1
```

发起连接：

```powershell
.\scripts\Connect-FreeRdpDemo.ps1
```

也可以不写配置，直接传参：

```powershell
.\scripts\Connect-FreeRdpDemo.ps1 -TargetHost 192.168.1.20 -User "TARGET-PC\demo" -CertMode tofu
```

如果 `wfreerdp.exe` 没有加入 `PATH`：

```powershell
.\scripts\Connect-FreeRdpDemo.ps1 -TargetHost 192.168.1.20 -User "TARGET-PC\demo" -FreeRdpPath "C:\tools\freerdp\wfreerdp.exe"
```

脚本不会把密码写进配置文件，也不会默认把密码拼进命令行。FreeRDP 会在连接时提示你输入密码。

## 常见问题

- `FreeRDP executable was not found`: 安装 FreeRDP，或用 `-FreeRdpPath` 指向 `wfreerdp.exe`。
- `TcpTestSucceeded: False`: 目标机没有开 RDP、防火墙未放行、IP 写错、两台机器不在同一网络，或端口不是 `3389`。
- 登录后提示凭据错误：确认用户名格式。常用格式是 `目标机器名\用户名`、`域名\用户名` 或 `用户名@域名`。
- 证书提示：demo 可以用 `certMode: "tofu"` 首次信任；只在临时测试时使用 `ignore`。

## 安全边界

不要把 RDP `3389` 端口直接暴露到公网。跨网络控制建议走 VPN、内网穿透的受控隧道或 RD Gateway。生产环境不要用 `certMode: "ignore"`，也不要把密码放到命令行、脚本或配置文件里。
