# Repository notes

This repository intentionally excludes generated and downloaded artifacts:

- `tools/vcpkg/`
- `tools/freerdp/*.exe`
- `harmony/app/.hvigor/`
- `harmony/app/oh_modules/`
- `harmony/app/**/build/`
- `harmony/app/**/*.hap`
- `harmony/app/tmp/`
- `app/native/freerdp-bridge/build/`
- `app/native/freerdp-bridge/out/`
- `config.local.json`
- `app-server*.log`

To rebuild local dependencies, follow:

```powershell
$env:FREERDP_ROOT = "C:\path\to\freerdp-install"
cmake -S app\native\freerdp-bridge -B app\native\freerdp-bridge\build
cmake --build app\native\freerdp-bridge\build --config Release
```

To use process-mode FreeRDP, place `wfreerdp.exe` under:

```text
tools\freerdp\wfreerdp.exe
```

or provide a custom path in the app's advanced options.
