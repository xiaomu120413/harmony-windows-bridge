# Native Bridge Layout

This directory is the Harmony app-side native bridge. It owns N-API entry points,
XComponent/native-window lifetime, app callbacks, and glue to the FreeRDP OHOS
client helpers.

## Directories

- `common/`: small bridge-local helpers and shared app/native data types.
- `napi/`: N-API exports, event sinks, native context, and app permission bridges.
- `session/`: app session worker, retry/fallback orchestration, and queued input/channel control.
- `freerdp/`: app-side FreeRDP runtime loading plus thin adapters that call exported
  `client/OHOS` helpers. This is not the long-term home for protocol semantics.
- `channels/`: app wiring for FreeRDP virtual channels and RDPGFX diagnostics callbacks.
- `input/`: XComponent input conversion and OHOS keyboard adapter glue.
- `surface/`: XComponent rendering, GDI/RGBA presentation, render ownership, and
  app-owned GPU compositor code.
- `types/`: Harmony N-API package metadata and TypeScript declarations.

## FreeRDP Boundary

Move logic into `harmony/third_party/FreeRDP/client/OHOS` when it describes RDP
protocol semantics, FreeRDP settings, capability negotiation, channel behavior,
or RDPGFX update/dirty/EndFrame ownership. Keep code here when it depends on
N-API, ArkTS callbacks, XComponent/native-window lifetime, EGL/GLES objects, or
app session state.

Good candidates for future FreeRDP-side cleanup:

- graphics mode parsing, fallback policy, and capability diagnostics currently
  wrapped by `freerdp/graphics_config.*`;
- more of the certificate policy callback return-code handling currently wrapped
  by `freerdp/certificate_policy.*`;
- any RDPGFX AVC444 suppression/present policy that is still app-side and does
  not require XComponent or GL objects.

Keep app-side:

- `freerdp/freerdp_runtime.*`, because it owns dynamic loading from the packaged
  shared libraries;
- N-API exports and event sinks;
- XComponent/native-window target selection and render ownership;
- OH_AVCodec and GLES compositor object lifetime.

`surface/avc444_gpu_compositor.cpp` is intentionally still a single file after
the directory cleanup. Split it separately so file movement and compositor
behavior changes do not land in the same patch.
