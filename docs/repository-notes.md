# Repository notes

This repository intentionally excludes generated and downloaded artifacts:

- `tools/vcpkg/`
- `tools/freerdp/*.exe`
- `harmony/app/.hvigor/`
- `harmony/app/oh_modules/`
- `harmony/app/**/build/`
- `harmony/app/**/*.hap`
- `harmony/app/tmp/`
- `native/freerdp-bridge/build/`
- `config.local.json`
- `app-server*.log`

To build and package the HarmonyOS app, use:

```powershell
cmd /c harmony\app\build_hap.bat
```

To connect to the xrdp path, use Windows MSTSC directly after HDC port
forwarding. Do not use repository PowerShell launcher scripts for connection
startup.

```powershell
hdc fport tcp:13390 tcp:3390
mstsc /v:127.0.0.1:13390
```
