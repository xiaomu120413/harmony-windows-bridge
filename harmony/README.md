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
- M4.3 dynamically loads FreeRDP at runtime and verifies the authentication-only connect path after TCP succeeds.
- M4.4 disables authentication-only mode, runs a persistent FreeRDP event loop, and lets `disconnect()` abort the active native context.
- M5.1 registers the ArkUI `XComponent` surface with the native module and reports native surface lifecycle status.
- M5.2 writes an RGBA test pattern into the `XComponent` NativeWindow buffer from C++.
- M5.3 factors the NativeWindow write path into a reusable RGBA frame renderer for future FreeRDP updates.
- M5.4 registers FreeRDP GDI paint callbacks and routes the GDI primary framebuffer into the native RGBA renderer.
- M5.5 switches to the Session tab before starting native connect and suppresses the automatic debug paint while a connection is active.
- M6.0 exposes native `sendPointer()` and `sendKey()` bridge calls backed by FreeRDP input APIs.
- M6.1 wires Session page single-touch left click/drag and toolbar key strokes into the native input bridge.
- M6.2 adds toolbar latch state for Ctrl, Alt, and Win so follow-up key strokes can be sent as combinations.
- End-to-end verification with a live Windows desktop frame is still pending.

## Native bridge

The entry module builds a native shared library from `entry/src/main/cpp/CMakeLists.txt`.

Current exported calls:

- `probe()` returns bridge version, ABI, FreeRDP, WinPR, and OpenSSL probe status.
- `connect(params)` validates the basic connection fields, starts the native session worker, and returns the initial state.
- `disconnect()` returns a native disconnect result.
- `paintTestPattern()` writes a CPU-generated test frame into the current `XComponent` surface.
- `onState(callback)` receives session states: `Resolving`, `TCP connected`, `Negotiating`, `Authenticating`, `Connected`, `Disconnected`, or `Failed`.
- `onLog(callback)` receives native session log lines.
- `onError(callback)` receives validation or worker errors.

M4.1 verification:

- Clean HAP build through HarmonyOS MCP succeeded.
- HAP installed and launched on device `3QC0124C11000711`.
- The connection form started the native worker, ArkUI switched to the session page, and the page displayed `Connected` from the native state callback.

Remaining issues carried forward:

- M4.4 can hold a connected FreeRDP session loop, but FreeRDP desktop pixels are not rendered yet.
- M5.2 only proves NativeWindow buffer writes with a synthetic frame; FreeRDP frame callbacks are not wired to render yet.
- Callback lifecycle is only smoke-tested for one connect/disconnect path; reconnect, page teardown, and app backgrounding still need stress testing.

M4.2 notes:

- The module declares only `ohos.permission.INTERNET`; no signing-profile privileges are added.
- TCP success/failure is reported through `onState`, `onLog`, and `onError`.

M4.3 notes:

- `libentry.so` dynamically loads `libfreerdp3.so`, `libwinpr3.so`, OpenSSL, zlib, and cJSON from the packaged runtime libraries.
- `OPENSSL_MODULES` is set to the packaged `ossl-modules` directory before FreeRDP starts, so NTLM/NLA can use OpenSSL providers.
- Runtime libraries are kept loaded for the process lifetime because WinPR registers thread-local destructors.
- Tested negative path on device `3QC0124C11000711`: TCP to `35.180.139.74:3389` succeeds, FreeRDP fails cleanly with `ERRCONNECT_SECURITY_NEGO_CONNECT_FAILED`, and the app remains alive.

M4.4 notes:

- The worker now calls `freerdp_connect` with `FreeRDP_AuthenticationOnly=false`.
- After connect succeeds, the worker uses `freerdp_get_event_handles`, WinPR `WaitForMultipleObjects`, and `freerdp_check_event_handles` to keep the RDP session alive.
- `disconnect()` clears the worker running flag and calls `freerdp_abort_connect_context` on the active context so connect/wait can unwind.
- A real Windows RDP success path still needs device-side validation with working credentials; the available automated check is currently a negative negotiation test.

M5.1 notes:

- ArkUI `XComponent` now sets `libraryname: 'entry'` and uses an `XComponentController`.
- The native module reads `OH_NATIVE_XCOMPONENT_OBJ` during N-API init and registers `OH_NativeXComponent_Callback`.
- Native callbacks track surface created/changed/destroyed events, surface id, dimensions, and touch count.
- `probe()` exposes the current surface status so the diagnostics page can confirm whether the native surface is ready.
- The surface is mounted only while the Session tab is visible; switching away destroys it until the next M5 rendering step decides whether to keep it mounted.

M5.2 notes:

- Native code treats the XComponent callback `window` as an `OHNativeWindow*`, configures RGBA8888 geometry, requests a buffer, maps it through `OH_NativeBuffer_FromNativeWindowBuffer` plus `OH_NativeBuffer_Map`, writes a gradient/grid test pattern, unmaps, and flushes the dirty region.
- `probe()` now exposes `surfacePaintCount` and `surfaceLastPaintMessage`; the Session page calls `paintTestPattern()` when the XComponent loads and also provides a temporary Paint button for manual verification.
- The test renderer writes the full surface every time. Dirty rect rendering, double buffering strategy, and FreeRDP-to-RGBA conversion remain future work.
- If the Session tab is not mounted, `paintTestPattern()` returns a non-fatal failure and logs that the XComponent surface is not ready.

M5.3 notes:

- The surface bridge now has a `RenderRgbaFrame` path that accepts source pixels, dimensions, stride, and label, then handles native buffer mapping, row copy, RGBA/BGRA conversion, unmap, and flush.
- `paintTestPattern()` now generates a synthetic RGBA frame and renders it through the same path that FreeRDP desktop frames will use.
- The renderer still copies a full frame and does not scale or letterbox mismatched desktop/surface sizes yet.

M5.4 notes:

- `connect()` now configures FreeRDP software GDI and registers `PostConnect`, `PostDisconnect`, `BeginPaint`, `EndPaint`, and `DesktopResize` callbacks.
- `PostConnect` initializes GDI with `PIXEL_FORMAT_RGBA32`; `EndPaint` wraps `rdpGdi::primary_buffer` as an RGBA frame and renders it through `RenderRgbaFrame`.
- Real frame validation still depends on connecting to a reachable Windows host; current device verification only proves the callbacks build/package/load with the existing runtime libraries.

M5.5 notes:

- The Connect action now moves the UI to the Session tab before invoking `native.connect()`, so the XComponent can be created while TCP negotiation/authentication are still in progress.
- The XComponent `onLoad` debug paint only runs in idle/disconnected/failed states; active connection states leave the surface ready for FreeRDP frames.

M6.0 notes:

- The native module now dynamically loads `freerdp_input_send_mouse_event` and `freerdp_input_send_keyboard_event_ex`.
- `sendPointer({ flags, x, y })` and `sendKey({ scancode, down })` validate that a FreeRDP session is connected before dispatching to `rdpContext::input`.
- ArkUI gesture/key mapping is still pending; this step only proves the N-API and native input dispatch surface.

M6.1 notes:

- The `XComponent` surface now handles single-touch down/move/up and maps it to RDP left-button down/drag/up pointer events.
- The toolbar sends one key press/release for Ctrl, Alt, Win, Esc, and Tab through `sendKey()`.
- Wheel gestures, long-press right click, text input, and coordinate scaling against a mismatched desktop resolution are still pending.

M6.2 notes:

- Ctrl, Alt, and Win are now latch buttons: first tap sends key-down and marks the button active; second tap sends key-up.
- Esc and Tab still send press/release strokes, so active modifiers can combine with them.
- Disconnect and non-connected state callbacks clear modifier UI state; `disconnect()` also releases active modifiers before closing the native session when possible.
- Wheel gestures, long-press right click, text input, and coordinate scaling against a mismatched desktop resolution are still pending.

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
