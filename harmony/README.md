# HarmonyOS App

This directory contains the HarmonyOS HAP project for the FreeRDP-based remote
desktop client.

## Current Runtime

- ArkTS calls the native N-API bridge in `libentry.so`.
- The native bridge loads FreeRDP/WinPR runtime libraries synced from
  `harmony/out/ohos-arm64/runtime-libs`.
- `probe()`, `connect(params)`, `disconnect()`, `resize(input)`,
  `sendPointer(input)`, `sendKey(input)` and `sendUnicode(input)` are available.
- The remote desktop is rendered through ArkUI `XComponent` and native
  `NativeWindow` handling.
- FreeRDP source adaptations live in `harmony/third_party/FreeRDP`; HAP code
  should stay a UI, permission, N-API and surface relay.

Enabled delivery capabilities include:

- RDP/TLS/NLA, TOFU/Strict certificate policy and app-local storage paths.
- `cliprdr` text clipboard with Pasteboard permission requested on demand.
- `disp` display-control resize support.
- `geometry` dynamic virtual channel registration. The current HAP does not
  consume region data or alter rendering/layout from this channel.
- `rdpecam` dynamic virtual channel registration with OHOS CameraKit camera
  enumeration/media capability discovery and ImageReceiver-based frame capture.
- `rdpgfx-h264` with OHOS AVCodec-backed AVC444 GPU compositor and native GDI
  fallback.
- `rdpsnd` playback and `audin` capture through OHOS audio backends.
- `location` redirection through OHOS LocationKit. The backend is built in, but
  the default session config keeps the channel disabled; when enabled, a
  permission prompt appears only if the remote side sends `LocationStart`.
- `rdpdr/drive` file redirection for the fixed public Download subdirectory
  `com.muhub.desktop`. The HAP obtains Download-directory authorization through
  the system download picker during startup; FreeRDP maps the directory to
  `\\tsclient\Downloads` without receiving an ETS-provided path.
- `printer` redirection through the OHOS PrintKit backend. The session exposes a
  virtual printer to Windows, but PrintKit initialization, printer lookup and job
  submission are deferred until Windows sends a print job.

Smartcard source/channel/PCSC, TSMF, CUPS and FUSE are not part of the delivery
profile.

## Permissions

`entry/src/main/module.json5` declares:

- `ohos.permission.INTERNET`
- `ohos.permission.GET_NETWORK_INFO`
- `ohos.permission.CUSTOM_SCREEN_RECORDING`
- `ohos.permission.PRINT`
- `ohos.permission.READ_PASTEBOARD`
- `ohos.permission.MICROPHONE`
- `ohos.permission.CAMERA`
- `ohos.permission.APPROXIMATELY_LOCATION`
- `ohos.permission.LOCATION`

Declaring a permission is not the same as prompting at connection start.
Pasteboard, microphone, camera and enabled location prompts are driven by a
shared native permission-request bridge when the matching RDP feature is used.
Screen recording is requested before starting the local xrdp-controlled desktop
stream. The Download directory picker is invoked at app startup to authorize and
prepare the fixed drive-redirection directory. `PRINT` is used by the lazy
PrintKit job submission path.

## Build

From the repository root, rebuild FreeRDP after changing the submodule or native
FreeRDP-facing code:

```powershell
wsl.exe bash -lc "cd /mnt/c/path/to/demo-prelaunch && export OHOS_NDK_HOME=/opt/ohos/sdk-6.1.0.830/command-line-tools/sdk/default/openharmony/native && ./harmony/scripts/wsl/build-freerdp-ohos.sh"
```

Sync the runtime libraries into the HAP project:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\harmony\scripts\windows\sync-freerdp-runtime.ps1
```

Build the signed HAP:

```powershell
.\harmony\app\build_hap.bat
```

The output path is:

```text
harmony/app/entry/build/default/outputs/default/entry-default-signed.hap
```

Install to a connected device:

```powershell
hdc install -r harmony\app\entry\build\default\outputs\default\entry-default-signed.hap
```

Local build outputs and synced runtime libraries under `entry/libs/` are ignored
by git.

## Current Docs

- `../docs/README.md`: documentation index.
- `../docs/freerdp-ohos-feature-matrix.md`: current feature and fallback matrix.
- `../docs/freerdp-ohos-validation-baseline.md`: repeatable build and
  true-device validation checklist.
- `../docs/freerdp-ohos-sdk-quickstart.md`: SDK integration guide.
- `../docs/ohos-native-cpp-module-guidelines.md`: native module and ownership
  rules.
