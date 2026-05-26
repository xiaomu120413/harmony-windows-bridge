# FreeRDP OHOS Source Adaptation Migration Plan

Date: 2026-05-17

This plan corrects the delivery boundary again: the final deliverable is the
HarmonyOS adaptation in the FreeRDP source branch `ohos-port`. The HAP is only a
validation shell. It may keep UI, permissions, N-API transport, surface handles
and diagnostics, but RDP protocol behavior and platform device backends must move
into FreeRDP source.

## Delivery Boundary

Target ownership:

```text
harmony/third_party/FreeRDP
  client/OHOS/                         OHOS platform client helpers
  channels/rdpsnd/client/ohos/         OHAudio playback backend
  channels/audin/client/ohos/          OHAudio microphone capture backend
  channels/cliprdr/client/ohos/        OHOS clipboard backend
  client/OHOS/ohos_location.*          LocationKit-backed location provider
  channels/disp/client/ohos helpers    display-control resize helper
  channels/rdpgfx/client/ohos helpers  graphics capability and surface policy
  libfreerdp/codec/                    OHOS AVCodec decoder backend

harmony/app/entry
  ArkTS UI
  permission prompts and privacy UI
  N-API relay
  XComponent and NativeWindow handles
  user configuration
  validation-only diagnostics
```

The HAP must not own long-term semantics for keyboard mapping, pressed-key
state, RDPGFX capability policy, AVC420/AVC444 route selection, rdpsnd/audin
protocol buffering, cliprdr message handling or display-control layout building.

Allowed HAP logic:

- connection form, session view, diagnostic pages and validation toggles
- runtime permission prompts such as microphone, pasteboard, location and storage access
- forwarding native surface/window handles and user-selected options
- showing FreeRDP backend logs and counters

Logic that must migrate to FreeRDP source:

- keyboard keycode/scancode/VK mapping, modifier synthesis and repeat state
- IME committed text to FreeRDP Unicode events
- clipboard format negotiation, text transfer and loop suppression
- rdpsnd playback queue and OHAudio renderer backend
- audin microphone capture and OHAudio capturer backend
- location channel callbacks, LocationKit sampling and RDP location PDU sending
- RDPGFX capability advertisement and codec fallback policy
- AVCodec hardware decoder integration and diagnostics
- dynamic resolution layout construction and `disp` channel calls

## Reference Platform Mapping

Use existing FreeRDP platform backends as the template, then replace only the
platform API calls with OHOS APIs:

```text
Keyboard/input
  client/X11, client/SDL, client/Windows
  -> move OHOS key mapping and state to client/OHOS

Audio playback
  channels/rdpsnd/client/alsa
  channels/rdpsnd/client/pulse
  channels/rdpsnd/client/opensles
  channels/rdpsnd/client/ios
  channels/rdpsnd/client/mac
  -> OHOS backend is channels/rdpsnd/client/ohos

Microphone capture
  channels/audin/client/alsa
  channels/audin/client/pulse
  channels/audin/client/oss
  channels/audin/client/opensles
  channels/audin/client/ios
  channels/audin/client/mac
  -> new OHOS backend must be channels/audin/client/ohos

Clipboard
  client/X11 clipboard integration plus cliprdr client channel callbacks
  -> OHOS Pasteboard wrapper under a FreeRDP OHOS clipboard backend

Location
  channels/location/client plus platform provider callbacks
  -> OHOS provider is client/OHOS/ohos_location.* and uses native LocationKit

Display resize
  channels/disp/client
  -> OHOS helper builds monitor layout, HAP forwards only width/height

Graphics and codecs
  channels/rdpgfx/client
  libfreerdp/codec/h264_*.c
  -> OHOS helper owns AVC420/AVC444 policy and AVCodec route
```

The remaining pattern is explicit: if a FreeRDP channel needs OS data, the
protocol and platform API calls live in FreeRDP source, while the HAP only
bridges runtime permission prompts and UI-owned handles.

## Current State

Already in FreeRDP source:

- `client/OHOS/ohos_keyboard.*`
- `libfreerdp/codec/h264_ohos_avcodec.c`
- `channels/rdpsnd/client/ohos/rdpsnd_ohos.c`
- `channels/audin/client/ohos/`
- `client/OHOS/ohos_clipboard.*`
- `client/OHOS/ohos_location.*`
- WinPR OHOS compatibility patches

Still incomplete or too much in the HAP layer:

- final remote validation for Ctrl combinations and long-press repeat
- IME committed text ownership
- Pasteboard, `audin` and `location` have source backends, but still need broader true-device regression for permission refusal, reconnect and service-side behavior
- audio playback stutter diagnostics and queue tuning
- `drive` shared-directory UI/sandbox policy and product enablement
- `printer` runtime backend through Harmony Print or CUPS-compatible path
- RD Gateway UI/settings exposure and server-side validation
- smartcard/PCSC, FUSE and TSMF remain out of the delivery build
- lifecycle stress testing for disconnect, reconnect, page destroy, background
  and network jitter

## Executable Migration Plan

Each phase below must be implemented and committed separately. Source migration
comes first; HAP changes are allowed only as a validation shell or thin relay.

### P1: Keyboard Backend Finalization

Status: implemented in source; remote manual validation still required.

Modification scope:

- `harmony/third_party/FreeRDP/client/OHOS/ohos_keyboard.*`
- HAP `input/ohos_keyboard_adapter.*` remains a thin bridge only
- HAP session page forwards raw OHOS key events to native

Pseudo code:

```c
OHOS_KEY_EVENT ohos_keyboard_resolve(keyCode, action, modifiers)
{
    vk = ohos_keycode_to_windows_vk(keyCode);
    flags = ohos_vk_to_freerdp_flags(vk);
    update_pressed_table(vk, action);
    synthesize_missing_modifiers(modifiers);
    return { vk, flags, action, repeatable };
}

input_pump()
{
    while (event = dequeue_ohos_key_event())
        freerdp_input_send_keyboard_event_ex(input, event.down, event.vk);

    if (repeat_key_due())
        freerdp_input_send_keyboard_event_ex(input, TRUE, repeated_vk);
}
```

Acceptance:

- Remote letters and digits work.
- Long-press digits repeats.
- Long-press Backspace/Delete repeats.
- Ctrl+A/C/V/X/Z work remotely.
- Function keys, arrows, Home/End/PageUp/PageDown/Insert/Delete work.
- Focus loss, disconnect and page destruction release all pressed keys.

Validation command:

```powershell
mcp harmonyos build_app project=harmony/app module=entry mode=debug clean=true
mcp harmonyos install_app device=3QC0124C11000711 hap=entry-default-signed.hap
```

Risk and impact:

- If HarmonyOS does not deliver modifier down/up consistently, synthesis remains
  in `client/OHOS`, not ArkTS.
- Toolbar helper keys may still use legacy scancode constants until fully moved.

### P2: IME Boundary Migration

Status: not started.

Modification scope:

- Add `harmony/third_party/FreeRDP/client/OHOS/ohos_ime.*`.
- HAP sends only committed text from ArkTS input methods.
- Native converts committed text into FreeRDP Unicode keyboard events.

Pseudo code:

```c
void ohos_ime_send_committed_text(rdpInput* input, const uint16_t* utf16, size_t units)
{
    for each code_unit in utf16:
        if (is_high_surrogate(code_unit))
            log_non_bmp_pending();
        else
            freerdp_input_send_unicode_keyboard_event(input, KBD_FLAGS_DOWN, code_unit);
            freerdp_input_send_unicode_keyboard_event(input, KBD_FLAGS_RELEASE, code_unit);
}
```

Acceptance:

- Chinese IME committed text reaches the remote session.
- English soft keyboard input still works.
- Hardware Backspace/Delete and soft-keyboard deletion do not swallow each other.

Risk and impact:

- Non-BMP characters need surrogate handling.
- IME composition is OS controlled; only committed text should be sent to RDP.

### P3: Clipboard Backend Migration

Status: partially split; needs FreeRDP backend cleanup.

Modification scope:

- Add `client/OHOS/ohos_pasteboard.*` or `channels/cliprdr/client/ohos/*`.
- FreeRDP owns `CliprdrClientContext` callbacks.
- HAP owns only permission prompts and diagnostics.

Pseudo code:

```c
cliprdr_ohos_init(context)
{
    context->ClientFormatList = ohos_cliprdr_format_list;
    context->ClientFormatDataRequest = ohos_cliprdr_data_request;
    context->ClientFormatDataResponse = ohos_cliprdr_data_response;
}

ohos_cliprdr_data_request(format)
{
    text = ohos_pasteboard_read_text();
    cliprdr_send_response(format, utf8_to_utf16le(text));
}

ohos_cliprdr_data_response(data)
{
    if (!is_local_echo(data))
        ohos_pasteboard_write_text(utf16le_to_utf8(data));
}
```

Acceptance:

- Windows to HarmonyOS text copy works.
- HarmonyOS to Windows text paste works.
- Ctrl+C/V remains keyboard input and is not special-cased in ArkTS.
- Loop suppression prevents repeated clipboard echo.

Risk and impact:

- Pasteboard access may be limited by foreground/background policy.
- First migration remains text-only; file clipboard is a separate feature.

### P4: RDPGFX and Codec Policy Migration

Status: partially implemented; policy still too close to HAP/native config.

Modification scope:

- FreeRDP OHOS gfx helper owns capability advertisement and codec policy.
- HAP passes only `graphicsMode` and the target surface handle.
- `libfreerdp/codec/h264_ohos_avcodec.c` owns OHOS AVCodec diagnostics.

Pseudo code:

```c
ohos_gfx_configure(settings, graphicsMode, surfaceCaps)
{
    if (graphicsMode == AVC420_SURFACE && surfaceCaps.avc420)
        advertise_rdpgfx_avc420();
    else if (graphicsMode == AVC444_MULTI_SURFACE && surfaceCaps.avc444)
        advertise_rdpgfx_avc444();
    else
        advertise_bitmap_or_clearcodec_fallback();
}

ohos_h264_decode(frame)
{
    if (frame.is_avc420 && decoder_surface_ready())
        decode_to_surface(frame);
    else if (frame.is_avc444)
        decode_two_planes_then_compose(frame);
    else
        decode_software_and_render_rgba(frame);
}
```

Acceptance:

- Logs identify the actual server route: bitmap, ClearCodec, AVC420, AVC444 or
  fallback.
- AVC420 uses OHOS AVCodec surface path when available.
- AVC444 either uses the planned multi-surface compositor or falls back cleanly.
- No black screen on fallback.

Risk and impact:

- AVC444 GPU composition is high risk because it needs two decoded streams and
  shader composition into the final desktop surface.
- A direct decoder surface is not enough for full desktop composition if cursor,
  dirty rects or separate planes still need composition.

### P5: Display Resize Helper Migration

Status: HAP resize behavior adjusted; source ownership cleanup remains.

Modification scope:

- Add a FreeRDP OHOS helper for display-control.
- ETS listens to real XComponent size.
- HAP forwards only width and height.

Pseudo code:

```c
BOOL ohos_disp_resize(rdpContext* context, UINT32 width, UINT32 height)
{
    DISPLAY_CONTROL_MONITOR_LAYOUT layout = { 0 };
    layout.Width = width;
    layout.Height = height;
    layout.PhysicalWidth = width;
    layout.PhysicalHeight = height;
    layout.Orientation = ORIENTATION_LANDSCAPE;
    return context->disp->SendMonitorLayout(context->disp, 1, &layout);
}
```

Acceptance:

- No hardcoded `1280x700`.
- Remote resolution changes through `disp`, not only local stretch.
- Rejected resize requests are logged with the server reason where available.

Risk and impact:

- Some Windows servers reject dynamic resolution changes; rendering must keep
  the current remote size when resize fails.

### P6: Audio I/O Backend Consolidation

Status: playback backend exists but stutters; microphone backend is missing.

#### P6.1 Playback Stabilization: `rdpsnd/client/ohos`

Modification scope:

- Keep playback protocol ownership in `channels/rdpsnd/client/ohos/`.
- Tune OHAudio renderer callback size, queue depth and underrun recovery.
- Keep metrics in FreeRDP logs and expose read-only diagnostics to HAP.

Pseudo code:

```c
rdpsnd_ohos_play(data, size)
{
    lock(queue);
    append_pcm_to_ring_buffer(data, size);
    update_queue_peak_and_latency();
    unlock(queue);
}

renderer_callback(buffer, requested)
{
    copied = read_pcm_from_ring_buffer(buffer, requested);
    if (copied < requested)
        fill_silence(buffer + copied, requested - copied);
    update_underrun_stats();
}
```

Acceptance:

- Windows system sound plays without obvious stutter under normal desktop load.
- Logs show queue depth, callback size, underrun count and latency.
- Disconnect/background stops and releases renderer cleanly.

Risk and impact:

- Stutter may come from RDP packet timing, renderer callback size, queue latency
  or render thread contention. Metrics must stay in the FreeRDP backend.

#### P6.2 Microphone Skeleton: `audin/client/ohos`

Modification scope:

- Add `harmony/third_party/FreeRDP/channels/audin/client/ohos/`.
- Add `audin_ohos.c` and `CMakeLists.txt`.
- Update `channels/audin/client/CMakeLists.txt` to include `ohos` when
  `WITH_OHAUDIO` or a new OHOS audio option is enabled.
- HAP adds only microphone permission/toggle and logs whether `audin sys:ohos`
  was registered.

Pseudo code:

```c
typedef struct
{
    IAudinDevice iface;
    AUDIO_FORMAT format;
    UINT32 frames_per_packet;
    AudinReceive receive;
    void* user_data;
    OH_AudioCapturer* capturer;
    HANDLE capture_thread;
    BOOL running;
    wLog* log;
} AudinOhosDevice;

FREERDP_ENTRY_POINT(UINT VCAPITYPE ohos_freerdp_audin_client_subsystem_entry(
    PFREERDP_AUDIN_DEVICE_ENTRY_POINTS ep))
{
    device = calloc(1, sizeof(AudinOhosDevice));
    device->iface.Open = audin_ohos_open;
    device->iface.FormatSupported = audin_ohos_format_supported;
    device->iface.SetFormat = audin_ohos_set_format;
    device->iface.Close = audin_ohos_close;
    device->iface.Free = audin_ohos_free;
    return ep->pRegisterAudinDevice(ep->plugin, (IAudinDevice*)device);
}
```

Acceptance:

- WSL FreeRDP cross-build passes.
- HAP clean debug build and install pass after runtime sync.
- Logs show `audin` OHOS backend registration.
- Mic remains disabled unless user permission and setting enable it.

Risk and impact:

- Registration alone does not capture audio.
- Option naming must not break existing OpenSLES or desktop backends.

#### P6.3 Microphone Capture: OHOS AudioCapturer

Modification scope:

- Implement OHOS AudioCapturer open/start/read/stop/release in
  `channels/audin/client/ohos/audin_ohos.c`.
- Support PCM first: 16-bit, mono/stereo, 16000/44100/48000 Hz.
- Convert or reject unsupported server formats with clear logs.

Pseudo code:

```c
static BOOL audin_ohos_format_supported(IAudinDevice* device, const AUDIO_FORMAT* format)
{
    return format->wFormatTag == WAVE_FORMAT_PCM &&
           format->wBitsPerSample == 16 &&
           (format->nChannels == 1 || format->nChannels == 2) &&
           (format->nSamplesPerSec == 16000 ||
            format->nSamplesPerSec == 44100 ||
            format->nSamplesPerSec == 48000);
}

static UINT audin_ohos_open(IAudinDevice* device, AudinReceive receive, void* user_data)
{
    ohos->receive = receive;
    ohos->user_data = user_data;
    ohos->capturer = create_oh_audio_capturer(ohos->format);
    ohos->running = TRUE;
    start_capturer(ohos->capturer);
    ohos->capture_thread = CreateThread(NULL, 0, audin_ohos_capture_loop, ohos, 0, NULL);
    return CHANNEL_RC_OK;
}

static DWORD audin_ohos_capture_loop(void* arg)
{
    while (ohos->running)
    {
        bytes = oh_audio_capturer_read(ohos->capturer, buffer, packet_bytes);
        if (bytes > 0)
            ohos->receive(&ohos->format, buffer, bytes, ohos->user_data);
        update_capture_stats(bytes);
    }
    return 0;
}
```

Acceptance:

- Windows remote sound recorder or meeting app receives local microphone audio.
- Diagnostics show captured frames, bytes, read errors and dropped packets.
- Disconnect, page destroy and app background stop the capturer.
- Permission denial produces a clear error and does not crash the RDP session.

Risk and impact:

- Microphone capture needs user permission and privacy indicator behavior.
- Audio focus, device route changes and background capture may vary by device
  policy.
- Resampling and echo cancellation are out of scope for the first pass.

#### P6.4 Audio Lifecycle and Diagnostics

Modification scope:

- Add shared OHOS audio diagnostics for playback and capture.
- HAP reads diagnostics but does not implement protocol behavior.
- Add log markers for open, format, start, underrun, read error, stop and free.

Acceptance:

- One device validation report covers playback and microphone.
- Audio resources are released on disconnect and background.
- Reconnect does not leave stale renderer/capturer instances.

Risk and impact:

- Long-running sessions can expose slow queue drift or capturer thread leaks.

### P7: HAP Cleanup as Validation Shell

Status: ongoing.

Modification scope:

- Remove feature logic that duplicates FreeRDP source behavior.
- Keep diagnostics that help validate source backends.
- Keep user options that FreeRDP backends need, such as `graphicsMode` and
  requested resolution. Clipboard and microphone permissions remain callback-
  driven and are requested only when the backend actually needs access.

Pseudo code:

```ts
native.connect({
  host,
  username,
  password,
  graphicsMode,
  surfaceHandle
})
```

Acceptance:

- HAP code is a thin UI/N-API/surface/permission shell.
- Source backend logs provide the authoritative behavior report.
- Removing validation buttons does not remove runtime backend functionality.

Risk and impact:

- During migration, temporary diagnostics may remain in HAP. They must be marked
  validation-only and not become the protocol implementation.

### P8: WSL Rebuild, Runtime Sync and Device Validation

Status: required after any FreeRDP source change that affects runtime `.so`.

Modification scope:

- WSL builds FreeRDP and dependencies.
- Windows sync script updates HAP runtime libraries.
- MCP builds and installs the signed debug HAP.

Pseudo code:

```bash
# WSL
harmony/scripts/wsl/build-freerdp-ohos.sh
```

```powershell
# Windows
powershell -NoProfile -ExecutionPolicy Bypass -File .\harmony\scripts\windows\sync-freerdp-runtime.ps1
```

Acceptance:

- FreeRDP OHOS cross-build passes in WSL.
- HAP clean debug build passes.
- HAP installs to device `3QC0124C11000711`.
- Runtime logs show the expected OHOS backend registration.
- Manual Windows RDP validation covers input, clipboard, graphics, playback and
  microphone for the phase being tested.

Risk and impact:

- A source commit without runtime sync may build in HAP but not reflect the
  latest WSL FreeRDP shared libraries on device.

## Regenerated TODO

| Priority | Item | Target source location | Validation |
| --- | --- | --- | --- |
| P0 | Verify keyboard backend on device | `client/OHOS/ohos_keyboard.*` | remote Ctrl combos, function keys, long-press digits/Delete |
| P1 | IME helper source migration | `client/OHOS/ohos_ime.*` | completed in source; still needs Chinese committed text and soft keyboard deletion validation |
| P1 | Clipboard backend source migration | `client/OHOS/ohos_clipboard.*` | completed for text bridge; still needs bidirectional device validation |
| P1 | Stabilize playback | `channels/rdpsnd/client/ohos/` | OHAudio backend is source-owned; stutter/underrun tuning remains |
| P1 | Add microphone backend | `channels/audin/client/ohos/` | source backend and permission callback exist; Windows receive path still needs device validation |
| P2 | Move display-control helper | `client/OHOS/ohos_display.*` | completed in source; remote resolution changes through `disp` need device validation |
| P2 | Move RDPGFX/codec policy | `client/OHOS/ohos_graphics.*`, `libfreerdp/codec/` | completed for policy and AVCodec entry points; AVC420/AVC444 route needs device validation |
| P2 | Move standard session settings/channels | `client/OHOS/ohos_session_config.*` | completed in source; HAP consumes exported helper |
| P2 | HAP cleanup | `harmony/app/entry` | HAP is now thinner; rdpgfx diagnostic hooks and XComponent surface orchestration remain validation-shell code |
| P3 | Lifecycle pressure test | FreeRDP OHOS backends plus HAP shell | disconnect/reconnect/background/network jitter |

## Commit Rule

Each phase is committed separately. Commit messages must include:

- completion percentage
- validation result
- remaining risks
- whether WSL FreeRDP rebuild/sync is required for runtime testing

## Open Issues and Impact

| Issue | Current impact | Owner |
| --- | --- | --- |
| Microphone receive path not device-proven after source migration | Remote Windows may still fail to receive local mic audio | FreeRDP `channels/audin/client/ohos` plus HAP permission relay |
| Audio playback stutters | Remote sound is usable but not product quality | FreeRDP `channels/rdpsnd/client/ohos` |
| Ctrl combos and long-press need final device proof | Remote keyboard reliability not fully closed | FreeRDP `client/OHOS` plus HAP thin relay |
| Clipboard needs bidirectional device proof | Copy/paste behavior can still fail because runtime Pasteboard availability and focus rules are device-dependent | FreeRDP `client/OHOS/ohos_clipboard.*` |
| RDPGFX/AVC route needs real server proof | Build can prove helpers exist, but not which caps Windows selects | FreeRDP OHOS gfx/codec backend |
| AVC444 GPU composition unresolved | High-resolution desktop can remain CPU-heavy or fallback-only | FreeRDP OHOS codec/compositor |
| WSL runtime sync required after source changes | Device may run stale libraries if sync is skipped | Build/release process |

## Current Phase Result

2026-05-17 source migration status:

- FreeRDP `client/OHOS` now contains keyboard, IME, clipboard, display-control,
  graphics policy and session settings/channel helpers.
- FreeRDP channel backends now contain OHOS rdpsnd playback and audin capture
  paths; HAP only relays microphone permission and displays diagnostics.
- HAP no longer compiles `client/OHOS/*.c` helper implementations into
  `libentry.so`; it consumes exported symbols from `libfreerdp-client3.so`.
- WSL FreeRDP build, runtime sync and MCP clean HAP build passed after the
  migration.

Remaining device validation focus:

- Ctrl combinations, function keys, long-press digits and Delete/Backspace.
- Bidirectional text clipboard.
- Audio stutter metrics and microphone receive path.
- Display-control resize after XComponent size changes.
- AVC420/AVC444 negotiation and whether the server actually drives the OHOS
  AVCodec surface route.
