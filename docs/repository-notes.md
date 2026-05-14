# Repository notes

This repository intentionally excludes generated and downloaded artifacts:

- `tools/vcpkg/`
- `tools/freerdp/*.exe`
- `native/freerdp-bridge/build/`
- `config.local.json`
- `app-server*.log`

To rebuild local dependencies, follow:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\Build-NativeBridge.ps1
```

To use process-mode FreeRDP, place `wfreerdp.exe` under:

```text
tools\freerdp\wfreerdp.exe
```

or provide a custom path in the app's advanced options.
