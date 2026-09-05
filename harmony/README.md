# HarmonyOS App

This directory contains the HarmonyOS HAP project for the FreeRDP-based remote
desktop client.

## Current Runtime

- Shared ArkTS calls the RDP client N-API bridge in `librdpclient.so` from the
  `common` HSP. The 2in1-only Entry injects `libxrdpcontrol.so`; tablet never
  packages or loads that control library.
- The native bridge loads FreeRDP/WinPR runtime libraries synced from
  `harmony/out/ohos-arm64/runtime-libs`.
- `probe()`, `connect(params)`, `disconnect()`, `resize(input)`,
  `sendPointer(input)`, `sendKey(input)` and `sendUnicode(input)` are available.
- The remote desktop is rendered through ArkUI `XComponent` and native
  `NativeWindow` handling.
- FreeRDP source adaptations live in `harmony/third_party/FreeRDP`; HAP code
  should stay a UI, permission, N-API and surface relay.

Enabled delivery capabilities include:

- RDP/TLS/NLA and app-local storage paths. Native supports TOFU/Strict; the current app UI pins TOFU and has no policy selector.
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

Both device entries declare the client permissions below. The two PC-only
permissions marked below exist only in `entry/src/main/module.json5`, never in
`entry_tablet`:

- `ohos.permission.INTERNET`
- `ohos.permission.GET_NETWORK_INFO`
- `ohos.permission.CUSTOM_SCREEN_RECORDING` (2in1 only)
- `ohos.permission.CONTROL_DEVICE` (2in1 only)
- `ohos.permission.PRINT`
- `ohos.permission.READ_PASTEBOARD`
- `ohos.permission.MICROPHONE`
- `ohos.permission.CAMERA`
- `ohos.permission.APPROXIMATELY_LOCATION`
- `ohos.permission.LOCATION`

Declaring a permission is not the same as prompting at connection start. On API 26,
`CONTROL_DEVICE` is opened from the remote-control settings page and also requires an
approved signing-profile ACL; older injection-dialog authorization remains the native fallback.
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
wsl.exe bash -lc "cd /mnt/c/path/to/your-checkout && export OHOS_NDK_HOME=/opt/ohos/sdk-6.1.0.830/command-line-tools/sdk/default/openharmony/native && ./harmony/scripts/wsl/build-freerdp-ohos.sh"
```

Sync the runtime libraries into the HAP project:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\harmony\scripts\windows\sync-freerdp-runtime.ps1
```

Build the complete App Pack, tablet-only HAP set, or 2in1-only HAP set:

```powershell
.\harmony\app\build_hap.bat app
.\harmony\app\build_hap.bat tablet
.\harmony\app\build_hap.bat 2in1
```

The principal output paths are:

```text
harmony/app/build/outputs/default/app-default-signed.app
harmony/app/common/build/default/outputs/default/common-default-signed.hsp
harmony/app/entry/build/default/outputs/default/entry-default-signed.hap
harmony/app/entry_tablet/build/default/outputs/default/entry_tablet-default-signed.hap
```

Install to a connected device:

```powershell
hdc install -r harmony\app\common\build\default\outputs\default\common-default-signed.hsp
hdc install -r harmony\app\entry_tablet\build\default\outputs\default\entry_tablet-default-signed.hap
```

Install `common.hsp` before the device-matching Entry HAP. Local build outputs
and synced runtime libraries under `common/libs/` are ignored
by git.

## Current Docs

- `../docs/README.md`: documentation index.
- `../docs/freerdp-ohos-feature-matrix.md`: current feature and fallback matrix.
- `../docs/freerdp-ohos-validation-baseline.md`: repeatable build and
  true-device validation checklist.
- `../docs/freerdp-ohos-sdk-quickstart.md`: SDK integration guide.
- `../docs/ohos-native-cpp-module-guidelines.md`: native module and ownership
  rules.
