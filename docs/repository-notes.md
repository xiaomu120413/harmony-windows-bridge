# Repository notes

当前产品范围为 HarmonyOS FreeRDP 客户端和 2in1 xrdp 被控端。根目录桌面验证程序已移除。HarmonyOS 构建仍依赖 DevEco/Hvigor 的 Node 环境。

生成目录以根 `.gitignore` 为准，包括 `harmony/out/`、各模块的 `build/`、`.cxx/`、`.test/`、`oh_modules/` 和同步后的 Native 库。忽略规则中的旧工具路径用于隔离本机残留，不表示产品仍提供对应入口。

从仓库根目录构建：

```powershell
.\harmony\app\build_hap.bat app
```

当前构建依赖 DevEco、Windows PowerShell 和 WSL/Linux 交叉编译环境；运行库变更后先重建并同步。详见 [验证基线](freerdp-ohos-validation-baseline.md)。

Windows 连接 2in1 被控端：

```powershell
hdc fport tcp:13390 tcp:3390
mstsc /v:127.0.0.1:13390
```

端口转发只处理网络路径，不能替代屏幕录制与输入权限。详见 [HDC 连接说明](xrdp-windows-hdc-forwarding.md)。