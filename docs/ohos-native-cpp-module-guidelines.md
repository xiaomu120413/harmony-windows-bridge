# OHOS Native C++ Module Guidelines

This document defines the module boundaries for the HarmonyOS native RDP bridge.
New native code should be added to the owning module, not to the bootstrap or
N-API entry files.

## Goals

- Keep each C++ source file under 1000 lines. Target 500 lines when practical.
- Keep `module_init.cpp` / `napi_exports.cpp` as wiring only.
- Keep platform, FreeRDP, rendering, channels, and N-API concerns separated.
- Make each module independently reviewable and testable by build.
- Add a new module when a feature has a clear ownership boundary.

## Delivery Boundary

`harmony/app` is the validation and product shell. Long-term RDP protocol
semantics and platform backends belong in `harmony/third_party/FreeRDP`.

FreeRDP OHOS source owns:

- keyboard, IME, pointer, wheel and modifier mapping;
- session settings, storage paths, certificate policy and standard channels;
- clipboard protocol handling and Harmony Pasteboard access;
- fixed Download-directory `rdpdr/drive` registration and path mapping;
- `rdpsnd` playback, `audin` capture and their diagnostics;
- location sampling and RDP location PDU handling;
- printer channel handling and OHOS PrintKit job submission;
- display-control layout, RDPGFX capability policy and OHOS AVCodec routing.

The HAP may keep:

- ArkUI pages, connection forms, settings and product diagnostics;
- runtime permission prompts for Pasteboard, microphone and location;
- Download directory picker authorization and startup directory preparation;
- `PRINT` permission declaration for PrintKit submission;
- N-API transport, user options, app sandbox paths and certificate storage root;
- `XComponent` / `NativeWindow` lifecycle and surface handle forwarding.

If a new feature needs RDP protocol state or OS data conversion, put the
protocol/platform implementation in FreeRDP source first, then keep HAP code as
a thin UI, permission or handle relay.

## Target Build Layout

The native target should evolve toward this shape:

```cmake
add_library(entry SHARED
    module_init.cpp
    napi_exports.cpp
    napi_utils.cpp
    bridge_log.cpp
    net_utils.cpp
    string_utils.cpp
    freerdp_runtime.cpp
    freerdp_settings.cpp
    certificate_policy.cpp
    rdp_session_core.cpp
    rdp_session_input.cpp
    rdp_session_channels.cpp
    surface/surface_bridge.cpp
    surface/latest_frame_renderer.cpp
    surface/gpu_rgba_renderer.cpp
    surface/avc444_surface_pool.cpp
    channels/clipboard_bridge.cpp
    channels/clipboard_format.cpp
    channels/rdpgfx_diagnostics.cpp
    channels/rdpgfx_pipeline.cpp
    channels/audio_diagnostics.cpp
)
```

The current tree may temporarily keep some legacy filenames while migration is
in progress. New code should still follow the target ownership rules below.

## Module Ownership

`module_init.cpp`

- Owns `napi_module` registration only.
- Calls export registration helpers.
- Must not contain FreeRDP, rendering, input, clipboard, or channel logic.

`napi_exports.cpp`

- Owns exported N-API function wrappers.
- Converts N-API arguments and return values.
- Delegates real work to session, surface, channel, or utility modules.
- Must not contain protocol logic or rendering implementation.

`napi_utils.*`

- Owns generic N-API value creation and argument parsing helpers.
- Must not know about RDP session state or rendering state.

`bridge_log.*`

- Owns native logging helpers.
- Must not depend on N-API callback sinks.

`net_utils.*`, `string_utils.*`

- Own small reusable helpers only.
- Must stay dependency-light and platform-safe.

`freerdp_runtime.*`

- Owns dynamic library loading and FreeRDP/WinPR symbol binding.
- Must not configure a session or send input events.

`freerdp_settings.*`

- Owns FreeRDP setting assignment and connection parameter mapping.
- Must not start threads, run the event loop, or touch surfaces.

`certificate_policy.*`

- Owns certificate policy parsing, storage paths, and FreeRDP certificate
  callbacks.
- May emit diagnostics through an injected log sink.

`rdp_session_core.*`

- Owns session lifetime, worker thread, connect/disconnect state, active context
  registration, and the FreeRDP event loop.
- Must delegate input, channel attach, display-control, graphics, and clipboard
  behavior to focused modules.

`rdp_session_input.*`

- Owns pointer, keyboard, unicode, wheel, drag, queueing, dispatch counters, and
  input diagnostics.
- Must be the only place that calls FreeRDP input send functions.

`rdp_session_channels.*`

- Owns FreeRDP channel connected/disconnected routing for the session.
- Must delegate clipboard, rdpgfx, display-control, and audio specifics.

`surface/surface_bridge.*`

- Owns XComponent surface lifecycle, viewport calculation, native window buffer
  rendering fallback, dirty history, and surface snapshots.
- Must not own FreeRDP session state or input queueing.

`surface/latest_frame_renderer.*`

- Owns latest-frame queueing, dirty merge, render pacing, render worker, and
  render statistics.
- Uses injected callbacks for actual surface painting and logging.

`surface/gpu_rgba_renderer.*`

- Owns GLES RGBA texture upload/rendering only.
- Must not know about FreeRDP, channels, or session state.

`surface/avc444_surface_pool.*`

- Owns AVC444 decode surface allocation and release only.
- Must not configure rdpgfx policy.

`channels/clipboard_bridge.*`

- Owns FreeRDP cliprdr integration and HarmonyOS pasteboard interaction.
- Uses `channels/clipboard_format.*` for text conversion.

`channels/clipboard_format.*`

- Owns UTF-8 / UTF-16LE clipboard conversion only.
- Must not call pasteboard or FreeRDP APIs.

`channels/rdpgfx_diagnostics.*`

- Owns rdpgfx counters, codec names, and diagnostics strings.
- Must not install hooks or change negotiation policy.

`channels/rdpgfx_pipeline.*`

- Owns rdpgfx hook installation, graphics pipeline bridge, AVC420/AVC444 route
  decisions, and fallback behavior.
- Uses surface callbacks for decode surface registration.

`channels/audio_diagnostics.*`

- Owns rdpsnd diagnostics and audio stats formatting.
- Must not configure the whole session.

## File Size Rules

- Hard limit: no `.cpp` or `.h` should exceed 1000 lines.
- Preferred limit: keep files under 500 lines.
- If a file exceeds 500 lines, split when the next related change is made.
- If a file would exceed 1000 lines, split before landing the change.
- Do not add unrelated refactors while splitting a module.

## Dependency Rules

- Dependencies should point inward:
  - utility modules have no dependency on session/surface/channel modules;
  - channel modules may depend on FreeRDP runtime/types and utilities;
  - session modules may compose channel/surface modules;
  - N-API modules only call public module APIs.
- Avoid global mutable state outside the owning module.
- Shared structs belong in a small `*_types.h` or `bridge_types.h`.
- Cross-module callbacks should use small function objects or interfaces.
- Do not include HarmonyOS UI/N-API headers in modules that do not need them.

## N-API Boundary Rules

- N-API wrappers parse inputs, call module APIs, and build result objects.
- N-API wrappers must not:
  - call FreeRDP input APIs directly;
  - run the session event loop;
  - manipulate EGL/GLES state;
  - read/write pasteboard data directly;
  - store protocol counters.
- If a new exported method needs more than argument parsing and result creation,
  add the implementation to the owning module first.

## Adding New Native Features

Before writing code:

1. Pick the owning module from this document.
2. If no owner fits, create a new focused module and add it to CMake.
3. Put shared public API in a small header.
4. Keep implementation details in the `.cpp` file or private class.
5. Add diagnostics in the owning module, not in the N-API wrapper.
6. Build the HAP after the module change.
7. Commit each completed split or feature step separately.

## Migration Plan

Remaining high-value splits from the current native bridge:

1. Move `SurfaceBridge` into `surface/surface_bridge.*`.
2. Move `RdpSession` into `rdp_session_core.*`.
3. Move session input queue and dispatch into `rdp_session_input.*`.
4. Move display-control and rdpgfx hook routing into `rdp_session_channels.*`
   and `channels/rdpgfx_pipeline.*`.
5. Move audio diagnostics into `channels/audio_diagnostics.*`.
6. Split `napi_init.cpp` into `module_init.cpp` and `napi_exports.cpp`.

After migration, `napi_init.cpp` should either be removed or reduced to a thin
compatibility wrapper.
