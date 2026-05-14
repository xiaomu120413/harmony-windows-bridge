# HarmonyOS App

This directory contains the HarmonyOS HAP project copied into this repo so it can be modified and built without touching the source project under `C:\Users\mu\Desktop\code\security_tool`.

Current milestone:

- Build a normal-permission HAP first.
- Keep `mstsc` / `wfreerdp.exe` out of the HarmonyOS runtime path.
- Add the RDP ArkUI and N-API work after the HAP build path is stable.

## Build unsigned HAP

```powershell
.\harmony\app\build_hap.bat
```

Expected local output:

```text
harmony\app\entry\build\default\outputs\default\entry-default-unsigned.hap
```

## Sign as a normal app

```powershell
.\tools\hapsigner\Sign-NormalApp.ps1 -InputHap .\harmony\app\entry\build\default\outputs\default\entry-default-unsigned.hap -OutputName freerdp-normal-signed.hap
```

Signed HAP output is local only and ignored by git.
