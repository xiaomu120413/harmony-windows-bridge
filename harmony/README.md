# HarmonyOS App

This directory contains the HarmonyOS HAP project copied into this repo so it can be modified and built without touching the source project under `C:\Users\mu\Desktop\code\security_tool`.

Current milestone:

- Build a normal-permission HAP first.
- Keep `mstsc` / `wfreerdp.exe` out of the HarmonyOS runtime path.
- ArkTS calls into the C++ N-API bridge through `libentry.so`.
- Native `probe()`, `connect(params)`, and `disconnect()` are available.
- `probe()` dynamically loads the local FreeRDP probe library when runtime `.so` files have been synced from `harmony/out/`.
- M4.1 has a native session worker and cross-thread state/log/error callbacks.
- M4.2 performs a real TCP reachability check from the native worker.
- M4.3 dynamically loads FreeRDP at runtime and runs `freerdp_connect` in authentication-only mode after TCP succeeds.
- A persistent connected session loop is still pending before M5 rendering.

## Native bridge

The entry module builds a native shared library from `entry/src/main/cpp/CMakeLists.txt`.

Current exported calls:

- `probe()` returns bridge version, ABI, FreeRDP, WinPR, and OpenSSL probe status.
- `connect(params)` validates the basic connection fields, starts the native session worker, and returns the initial state.
- `disconnect()` returns a native disconnect result.
- `onState(callback)` receives session states: `Resolving`, `TCP connected`, `Negotiating`, `Authenticating`, `Connected`, `Disconnected`, or `Failed`.
- `onLog(callback)` receives native session log lines.
- `onError(callback)` receives validation or worker errors.

M4.1 verification:

- Clean HAP build through HarmonyOS MCP succeeded.
- HAP installed and launched on device `3QC0124C11000711`.
- The connection form started the native worker, ArkUI switched to the session page, and the page displayed `Connected` from the native state callback.

Remaining issues carried forward:

- M4.3 verifies the FreeRDP connect/auth path in authentication-only mode; it does not keep a desktop session alive yet.
- Callback lifecycle is only smoke-tested for one connect/disconnect path; reconnect, page teardown, and app backgrounding still need stress testing.
- The XComponent is only a placeholder until M5 rendering.

M4.2 notes:

- The module declares only `ohos.permission.INTERNET`; no signing-profile privileges are added.
- TCP success/failure is reported through `onState`, `onLog`, and `onError`.

M4.3 notes:

- `libentry.so` dynamically loads `libfreerdp3.so`, `libwinpr3.so`, OpenSSL, zlib, and cJSON from the packaged runtime libraries.
- `OPENSSL_MODULES` is set to the packaged `ossl-modules` directory before FreeRDP starts, so NTLM/NLA can use OpenSSL providers.
- Runtime libraries are kept loaded for the process lifetime because WinPR registers thread-local destructors.
- Tested negative path on device `3QC0124C11000711`: TCP to `35.180.139.74:3389` succeeds, FreeRDP fails cleanly with `ERRCONNECT_SECURITY_NEGO_CONNECT_FAILED`, and the app remains alive.

The signed HAP currently packages `libentry.so` for `arm64-v8a` and `x86_64`; FreeRDP runtime libraries are synced for `arm64-v8a`.

Sync local FreeRDP runtime libraries before building the HAP with FreeRDP probe support:

```powershell
.\harmony\scripts\windows\sync-freerdp-runtime.ps1
```

The synced runtime libraries under `entry/libs/` are local build outputs and are ignored by git.

## Build and install with MCP

Use the HarmonyOS MCP service for normal development:

- `build_app` with project path `harmony/app`
- `install_app` with the generated HAP path

The project-level `build-profile.json5` already contains the normal-app signing configuration. Do not run a separate signing script.

## Local build fallback

```powershell
.\harmony\app\build_hap.bat
```

Local HAP outputs are ignored by git.
