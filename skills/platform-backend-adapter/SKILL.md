---
name: platform-backend-adapter
description: Adapt a third-party library's platform backend to native OS APIs while preserving the portable core. Use when Codex needs to connect library features to platform APIs such as camera, audio, rendering, file access, networking, input, sensors, windowing, permissions, or lifecycle systems across OHOS, Android, iOS, Linux, or embedded targets.
---

# Platform Backend Adapter

## Workflow

Use the library's existing extension point before inventing a new integration.

1. Identify the feature that needs platform access: camera, audio, display, input, file system, network, device, or lifecycle.
2. Find the closest existing backend: Android, iOS, Linux, SDL, Wayland, X11, ALSA, PulseAudio, V4L2, AVFoundation, MediaCodec, or similar.
3. Preserve the portable core and add a target backend or capability adapter.
4. Map library callbacks and ownership rules to platform lifecycle rules.
5. Gate sensitive operations behind app-level permission and state checks.
6. Add logs around backend initialization, capability selection, and fallback paths.

## Adapter Shape

Prefer this layering:

```text
upstream core
  -> library backend interface
  -> platform adapter
  -> OS API
  -> app lifecycle and permission gate
```

Keep OS objects out of portable core code. Pass handles, callbacks, or small adapter structs through existing extension points.

## Suggested Interface

Use a backend contract like this, adapted to the library's language and style:

```text
PlatformBackend {
  name
  enumerateCapabilities() -> CapabilityList
  open(options, callbacks) -> Handle
  close(handle) -> Result
  getDiagnostics(handle) -> Diagnostics
}
```

If the backend pushes data, callbacks should include `onData`, `onStateChanged`, and `onError`. If it pulls data, expose a small read or acquire/release contract and document ownership.

## Community Compatibility

Follow the style used by mainstream libraries:

- Put new backend files beside existing platform backends.
- Reuse existing naming, logging, thread, and error conventions.
- Add build flags such as `WITH_OHOS_BACKEND` or `ENABLE_PLATFORM_CAMERA` only when they match local conventions.
- Keep Android/Linux/iOS paths working after the new backend is added.
- Avoid changing public APIs unless the library already supports backend capability registration.

## Lifecycle Checks

Before calling platform APIs, verify:

- The app or process has the required permission.
- The platform object is initialized on the expected thread or event loop.
- Buffers remain valid until the library has consumed them.
- Stop, pause, disconnect, and teardown paths release native resources.

## Validation

Validate the backend separately from the whole app:

- Build with the backend enabled and disabled.
- Exercise initialization failure, permission denial, runtime stop, and restart.
- Confirm fallback behavior is explicit rather than silent.
- Capture logs showing selected backend, negotiated capability, and shutdown result.
