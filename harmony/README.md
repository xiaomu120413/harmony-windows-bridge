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
- M6.3 maps two-finger vertical drags on the remote surface to RDP mouse wheel events.
- M6.4 adds long-press right click and defers left-click down until click release or drag threshold.
- M6.5 scales touch coordinates from the XComponent surface size into the requested RDP desktop resolution.
- M6.6 adds native Unicode keyboard input and a Session page soft text entry control.
- M6.7 exposes the native FreeRDP desktop size through `probe()` and uses it for pointer coordinate mapping when available.
- M6.8 maps two-finger horizontal drags on the remote surface to RDP horizontal wheel events.
- M6.9 adds the `resize(width, height)` N-API surface and reports that dynamic resize is blocked by the current no-channel FreeRDP build.
- M6.10 queues pointer/key/Unicode input on the FreeRDP worker thread, adds TOFU/strict/ignore certificate policies, and reports the disabled channel/codec feature set through `probe()`.
- M6.11 switches the WSL FreeRDP build to the enhanced channel/codec profile: client channels, cliprdr, rdpdr, drive, printer, smartcard, rdpsnd, audin, disp, rdpgfx, TSMF, uriparser, OpenSLES, FFmpeg, and OpenH264 are compiled and packaged.
- M6.12 adds a FreeRDP OHOS feature matrix check and compiles the WinPR smartcard PCSC backend into the enhanced runtime; CUPS and FUSE remain disabled because the current OHOS sysroot does not provide those backends.
- A live Windows desktop frame has been verified on device; current follow-up validation is focused on reliable remote operation and lifecycle stress.

## Native bridge

The entry module builds a native shared library from `entry/src/main/cpp/CMakeLists.txt`.

Current exported calls:

- `probe()` returns bridge version, ABI, FreeRDP, WinPR, and OpenSSL probe status.
- `connect(params)` validates the basic connection fields, starts the native session worker, and returns the initial state.
- `disconnect()` returns a native disconnect result.
- `resize(input)` validates a target size and still returns an explicit unsupported message until display-control monitor-layout PDUs are wired.
- `paintTestPattern()` writes a CPU-generated test frame into the current `XComponent` surface.
- `sendPointer(input)` queues RDP pointer, button, and wheel events for dispatch on the FreeRDP worker thread.
- `sendKey(input)` queues RDP scancode key events for dispatch on the FreeRDP worker thread.
- `sendUnicode(input)` queues BMP UTF-16 Unicode keyboard events for dispatch on the FreeRDP worker thread.
- `onState(callback)` receives session states: `Resolving`, `TCP connected`, `Negotiating`, `Authenticating`, `Connected`, `Disconnected`, or `Failed`.
- `onLog(callback)` receives native session log lines.
- `onError(callback)` receives validation or worker errors.

M4.1 verification:

- Clean HAP build through HarmonyOS MCP succeeded.
- HAP installed and launched on device `3QC0124C11000711`.
- The connection form started the native worker, ArkUI switched to the session page, and the page displayed `Connected` from the native state callback.

Remaining issues carried forward:

- FreeRDP channels and H.264/FFmpeg/OpenH264 are now compiled into the enhanced WSL build, but several product features still need runtime wiring: clipboard callbacks, drive path selection and permissions, printer backend, smartcard PCSC backend, RD Gateway UI/settings, and display-control resize PDUs.
- IME composition is still limited to the explicit Session text box sending BMP UTF-16 code units; inline composition and non-BMP input remain future work.
- Callback lifecycle is only smoke-tested for basic connect/disconnect paths; reconnect, page teardown, app backgrounding, and network jitter still need stress testing.

M4.2 notes:

- The module declares only `ohos.permission.INTERNET`; no signing-profile privileges are added.
- TCP success/failure is reported through `onState`, `onLog`, and `onError`.

M4.3 notes:

- `libentry.so` dynamically loads `libfreerdp3.so`, `libfreerdp-client3.so`, `libwinpr3.so`, OpenSSL, zlib, cJSON, and the packaged codec/channel runtime dependencies.
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
- Long-press right click, text input, and coordinate scaling against a mismatched desktop resolution are still pending.

M6.2 notes:

- Ctrl, Alt, and Win are now latch buttons: first tap sends key-down and marks the button active; second tap sends key-up.
- Esc and Tab still send press/release strokes, so active modifiers can combine with them.
- Disconnect and non-connected state callbacks clear modifier UI state; `disconnect()` also releases active modifiers before closing the native session when possible.
- Wheel gestures, long-press right click, text input, and coordinate scaling against a mismatched desktop resolution are still pending.

M6.3 notes:

- The session `XComponent` now consumes two-finger vertical touch moves and sends `PTR_FLAGS_WHEEL` pointer events through the existing native `sendPointer()` bridge.
- The first version uses a 24 px movement threshold and one wheel event per threshold crossing; downward finger movement is mapped to negative wheel direction.
- If a second finger appears while a left-button drag is active, ArkTS releases the left button before starting wheel handling.
- Horizontal wheel, text input, and finer gesture conflict handling are still pending.

M6.4 notes:

- Single-touch down now starts a pending touch instead of immediately sending left-button down.
- Releasing before the drag threshold sends a left click; moving at least 8 px starts a left-button drag from the original touch point.
- Holding for 550 ms sends a right click and suppresses the later left click.
- Horizontal wheel, text input, coordinate scaling, and finer gesture conflict handling are still pending.

M6.5 notes:

- `probe()` surface dimensions are now stored in ArkTS and used to map touch coordinates into the connection form resolution, for example surface `2432x1077` to requested desktop `1280x720`.
- The mapping clamps coordinates to the requested desktop bounds before calling native `sendPointer()`.
- This is a first-pass client-side mapping; the native side still does not expose the actual negotiated desktop size after server-side adjustment.
- Horizontal wheel, actual negotiated desktop-size mapping, and finer gesture conflict handling are still pending.

M6.6 notes:

- The native bridge now dynamically loads `freerdp_input_send_unicode_keyboard_event` and exposes `sendUnicode({ code, down })`.
- The Session page has a compact text entry row; tapping Send emits each BMP UTF-16 code unit as Unicode key down/up.
- Non-BMP characters are skipped and logged because FreeRDP's Unicode keyboard event takes a `UINT16` code.
- IME composition handling and finer gesture conflict handling are still pending.

M6.7 notes:

- Native code records the FreeRDP desktop size after `PostConnect` and `DesktopResize`, clears it on disconnect, and exposes `sessionConnected`, `desktopWidth`, and `desktopHeight` through `probe()`.
- ArkTS refreshes `probe()` when the state reaches `Connected` and prefers the native desktop size over the requested form resolution for pointer coordinate mapping.
- If the session has not connected yet, coordinate mapping still falls back to the requested resolution.
- Horizontal wheel, IME composition handling, and finer gesture conflict handling are still pending.

M6.8 notes:

- Two-finger touch movement now tracks both axes; vertical-dominant movement sends `PTR_FLAGS_WHEEL`, horizontal-dominant movement sends `PTR_FLAGS_HWHEEL`.
- The same 24 px threshold is used for vertical and horizontal wheel events.
- IME composition handling and finer gesture conflict tuning are still pending.

M6.9 notes:

- The bridge now exports `resize({ width, height })` and the Session page has a Resize action that uses the connection form resolution.
- The native method validates size bounds and active-session state, but returns unsupported for live resize because the current FreeRDP build has display-control channels disabled.
- Real dynamic resolution requires enabling the FreeRDP display-control channel and wiring monitor layout PDUs in a later build step.
- IME composition handling and finer gesture conflict tuning are still pending.

M6.10 notes:

- `sendPointer()`, `sendKey()`, and `sendUnicode()` now enqueue input from the ArkTS/N-API thread and dispatch it from the FreeRDP session loop, avoiding direct cross-thread calls into `rdpContext::input`.
- Pointer move events are coalesced at the tail of the queue to reduce backlog; non-move input is bounded and reports dropped events through `probe()`.
- Certificate policy is now explicit: `tofu` accepts first untrusted certificates through the FreeRDP certificate callback and asks FreeRDP to store them, `strict` rejects untrusted certificates, and `ignore` accepts for the current session only.
- The connect page uses fixed TOFU/Strict/Ignore controls instead of a free-text certificate policy field.
- The diagnostics probe reports worker-thread input mode, input queued/sent/dropped counters, and the current minimal FreeRDP feature set.
- Touch/mouse conflict handling now suppresses duplicate mouse events shortly after touch input, and two-finger scroll suppresses the remaining single-finger gesture until fingers are lifted.
- `aboutToDisappear()` releases active pointer buttons and latched Ctrl/Alt/Win modifiers to reduce stuck input during page teardown.

M6.11 notes:

- `harmony/scripts/wsl/build-freerdp-ohos.sh` now builds uriparser, OpenH264, and FFmpeg before FreeRDP, then configures FreeRDP with `WITH_CHANNELS=ON`, `WITH_CLIENT_CHANNELS=ON`, `WITH_FFMPEG=ON`, `WITH_OPENH264=ON`, `WITH_URIPARSER=ON`, and OpenSLES when the OHOS NDK provides it.
- FreeRDP source adaptations are tracked in the `harmony/third_party/FreeRDP` submodule on the `ohos-port` branch; the current submodule commit disables WinPR `pthread_cancel` on `__OHOS__`.
- The enabled client channels are `cliprdr`, `drdynvc`, `disp`, `rdpgfx`, `rdpsnd`, `audin`, `rdpdr`, `drive`, `printer`, `smartcard`, and `tsmf`.
- `libentry.so` loads `libfreerdp-client3.so`, registers the static client addin provider, and requests cliprdr, rdpdr, rdpgfx/H.264, display-control, and rdpsnd at session start.
- Runtime library sync now copies the whole `runtime-libs` directory, including FFmpeg/OpenH264/uriparser/OpenSLES/`libc++_shared.so` shared libraries, instead of a fixed minimal list. Use a clean HAP build after changing the native library set so hvigor does not reuse stale package metadata.
- Remaining risk: CUPS, PCSC, and FUSE are intentionally optional build flags because HarmonyOS does not provide those Linux services as normal app APIs. Printer, physical smartcard, and clipboard file-copy need OHOS-specific backends or explicit third-party ports before they are truly usable.

M6.12 notes:

- `harmony/scripts/wsl/check-freerdp-ohos-feature-matrix.sh` records compile/configure status for CUPS, PCSC, FUSE, and the current enhanced runtime without modifying the main build output.
- `WITH_SMARTCARD_PCSC=ON` is now part of the default enhanced FreeRDP build. The WinPR PCSC code compiles into `libwinpr3.so` and loads `libpcsclite.so.1`/`libpcsclite.so` at runtime when smartcard is used.
- `WITH_CUPS=ON` still fails at configure time because the OHOS sysroot does not provide CUPS headers/libraries.
- `WITH_FUSE=ON` still fails at configure time because `fuse3` is not available in the OHOS cross sysroot.
- The detailed matrix and true-device checklist are in `docs/freerdp-ohos-feature-matrix.md`.

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
