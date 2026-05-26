# MuHub

MuHub is a HarmonyOS and Windows interconnection tool. The current HarmonyOS
application embeds FreeRDP-side work and an xrdp-based MSTSC server path.

## Layout

- `harmony/app/`: HarmonyOS HAP project.
- `harmony/third_party/xrdp/`: xrdp OHOS port.
- `harmony/third_party/FreeRDP/`: FreeRDP OHOS work.
- `harmony/scripts/windows/`: build-time runtime sync and HNP packaging helpers.
- `docs/`: design notes, porting plans, and validation notes.

## Build

Build the HAP from Windows:

```powershell
cmd /c harmony\app\build_hap.bat
```

The build script copies the FreeRDP and xrdp runtime files, packages the xrdp
HNP payload, builds the HAP, and signs it.

## Install

```powershell
hdc install -r harmony\app\entry\build\default\outputs\default\entry-default-signed.hap
hdc shell aa start -a EntryAbility -b com.huawei.freerdp
```

## Connect With MSTSC

Do not use repository PowerShell launcher scripts to start the remote session.
Use Windows built-in MSTSC directly.

```powershell
hdc fport tcp:13390 tcp:3390
mstsc /v:127.0.0.1:13390
```

The default xrdp OHOS backend caps the remote desktop to `1920x1280` while
preserving the HarmonyOS display aspect ratio.

## Notes

Generated artifacts remain untracked, including HAP output, build directories,
downloaded dependencies, and local runtime staging files.
