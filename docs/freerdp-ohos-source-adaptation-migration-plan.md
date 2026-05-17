# FreeRDP OHOS Source Adaptation Migration Plan

Date: 2026-05-17

This plan corrects the delivery boundary: the final deliverable is the
HarmonyOS adaptation in the FreeRDP source branch `ohos-port`. The HAP is only a
validation shell for UI, permissions, surface handles and user configuration.

## Delivery Boundary

Target ownership:

```text
harmony/third_party/FreeRDP
  client/OHOS/                 platform client helpers
  channels/rdpsnd/client/ohos/ OHAudio backend
  channels/audin/client/ohos/  future microphone backend
  libfreerdp/codec/            OHOS AVCodec backend
  channels/cliprdr/client/     cliprdr protocol remains FreeRDP-owned

harmony/app/entry
  ArkTS UI
  N-API transport
  permission prompts
  NativeWindow/XComponent handles
  validation-only diagnostics
```

The HAP must not own long-term RDP protocol semantics such as keyboard mapping,
RDPGFX codec policy, rdpsnd buffering rules or cliprdr message handling.

## Current Gap

Already in FreeRDP source:

- `libfreerdp/codec/h264_ohos_avcodec.c`
- `channels/rdpsnd/client/ohos/rdpsnd_ohos.c`
- WinPR OHOS compatibility patches

Still too much in HAP native:

- keyboard mapping and future pressed-key/repeat/modifier state
- Pasteboard plus `cliprdr` bridge
- `rdpgfx` surface/codec diagnostics and AVC route glue
- display-control resize helper
- final ownership of IME Unicode dispatch

## Migration Phases

### P0: Create FreeRDP `client/OHOS` Landing Zone

Status: started.

Modification scope:

- Add `harmony/third_party/FreeRDP/client/OHOS/`.
- Move OHOS keycode to Windows VK mapping into `client/OHOS/ohos_keyboard.*`.
- Keep HAP `input/ohos_keyboard_adapter.*` as a thin wrapper only.

Acceptance:

- HAP clean debug build passes.
- HAP install to device passes.
- No behavior change expected yet.

Risk:

- This does not fix Ctrl combinations or long-press repeat by itself; it only
  moves the mapping ownership to the FreeRDP source branch.

### P1: Keyboard State Backend in FreeRDP Source

Status: implemented in source; pending final remote manual validation.

Modification scope:

- Extend `client/OHOS/ohos_keyboard.*` with:
  - pressed-key table
  - repeat normalization
  - modifier state tracking
  - release-all-keys helper
  - resolved VK plus extended flag payload
- HAP N-API forwards ArkUI key events unchanged.
- `rdp_session_input.*` consumes resolved FreeRDP OHOS key events and dispatches
  through WinPR scancode mapping and `freerdp_input_send_keyboard_event_ex()`.

Acceptance:

- Remote Notepad accepts normal letters and digits.
- Long-press digits repeats.
- Long-press Backspace/Delete repeats.
- Ctrl+A/C/V/X/Z work remotely.
- Function keys, arrows, Home/End/PageUp/PageDown/Insert/Delete work.
- Focus loss, disconnect and page destruction release all pressed keys.

Risk:

- HarmonyOS may not deliver explicit modifier down/up for every keyboard path.
  If that happens, modifier synthesis must live inside the FreeRDP OHOS keyboard
  backend, not ArkTS.
- The current implementation keeps a single active native repeat key, matching
  common desktop behavior. Multi-key repeat is not supported.

### P2: IME Boundary Cleanup

Modification scope:

- Add `client/OHOS/ohos_ime.*` or keep a FreeRDP-facing helper under the same
  OHOS client layer.
- ArkTS only submits committed text.
- Native sends BMP UTF-16 via FreeRDP Unicode keyboard events.

Acceptance:

- Chinese IME committed text reaches the remote session.
- English soft keyboard input works.
- Soft-keyboard Backspace does not interfere with hardware Backspace/Delete.

Risk:

- Non-BMP characters may need surrogate handling and should be logged until
  full support is added.

### P3: Clipboard Backend Migration

Modification scope:

- Move text clipboard ownership toward:
  - `client/OHOS/ohos_clipboard_backend.*`
  - `client/OHOS/ohos_pasteboard.*`
  - `client/OHOS/ohos_cliprdr.*`
- HAP keeps permission prompts only.
- FreeRDP OHOS backend owns `CliprdrClientContext` callbacks, format list,
  format-data request/response and echo suppression.

Acceptance:

- Windows to HarmonyOS text copy works.
- HarmonyOS to Windows text paste works.
- Ctrl+C/V remains keyboard input, not clipboard special casing in ArkTS.

Risk:

- Pasteboard permissions and background behavior remain OS-policy dependent.
- First source migration should stay text-only.

### P4: RDPGFX and Codec Policy Migration

Modification scope:

- HAP passes only `graphicsMode`.
- FreeRDP OHOS gfx/codec backend owns:
  - RDPGFX capability policy
  - AVC420/AVC444 route choice
  - surface availability checks
  - AVCodec fallback logging
  - codec diagnostics

Acceptance:

- Logs identify actual server codec: CLEARCODEC, AVC420, AVC444 or fallback.
- AVC420 uses OHOS AVCodec surface path where available.
- AVC444 uses the planned multi-surface path or falls back cleanly.
- No black screen on fallback.

Risk:

- AVC444 GPU composition is high-risk because it needs two decoded streams and
  shader composition into the final desktop surface.

### P5: Display Resize Helper Migration

Note: the HAP-side resize behavior has already been adjusted. The remaining
work is ownership cleanup.

Modification scope:

- Keep ETS listening to real XComponent size.
- Move display-control helper into FreeRDP OHOS client source:
  - build one-monitor layout
  - call `DispClientContext.SendMonitorLayout`
  - log accepted/rejected resize requests
- HAP only forwards width and height.

Acceptance:

- No hardcoded `1280x700`.
- Remote resolution changes through `disp`, not local stretch only.
- Resize failure is logged with reason.

Risk:

- Some Windows servers may reject dynamic resolution changes; the app must keep
  rendering the current remote size when resize is rejected.

### P6: Audio Backend Consolidation

Modification scope:

- Keep `rdpsnd/client/ohos` as the only playback protocol owner.
- Add `audin/client/ohos` later for microphone.
- HAP only passes permission/toggle and shows diagnostics.

Acceptance:

- Audio plays without stutter under normal desktop load.
- Queue depth, underrun and latency metrics are logged.
- Disconnect and background lifecycle stop the renderer cleanly.

Risk:

- Audio stutter can be caused by RDP packet timing, renderer callback size,
  queue latency or UI/render thread contention. Metrics must stay in FreeRDP
  backend logs.

## Commit Rule

Each phase is committed separately. Commit messages must include:

- completion percentage
- validation result
- remaining risks
- whether WSL FreeRDP rebuild/sync is required for runtime testing

## Current Phase Result

P0 source migration has started by adding `client/OHOS/ohos_keyboard.*` and
using it from the HAP wrapper. Runtime behavior is intentionally unchanged until
P1 implements the actual state backend.

P1 adds the first FreeRDP-owned keyboard state backend:

- native pressed-key table
- modifier synthesis for Ctrl/Shift/Alt/Win shortcuts
- long-press repeat generation in the FreeRDP input pump
- release-all-keys N-API hook used by the HAP validation shell on focus/lifecycle cleanup
