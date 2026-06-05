---
name: media-render-pipeline-adapter
description: Adapt native media, video, graphics, or remote-display frame pipelines to platform rendering surfaces. Use when Codex needs to bridge decoded frames, compositor output, retained buffers, H.264 or GPU paths, resize events, surface ownership, frame pacing, or CPU/GPU copy paths between a third-party library and an app UI.
---

# Media Render Pipeline Adapter

## Workflow

Make ownership and timing explicit before changing rendering code.

1. Identify frame producers, consumers, surfaces, queues, and ownership boundaries.
2. Map pixel formats, color spaces, stride, crop, rotation, and scaling rules.
3. Decide whether frames are copied, retained, imported, or presented by reference.
4. Handle resize and surface recreation as normal runtime events.
5. Add backpressure or flow control where producers can outpace consumers.
6. Validate blank frame, stale frame, teardown, and reconnect scenarios.

## Pipeline Map

Document the path:

```text
decoder or capture
  -> frame buffer
  -> optional compositor
  -> platform surface or texture
  -> UI layer
```

Mark which layer owns each buffer and which layer releases it.

## Suggested Interfaces

Use a surface-facing sink contract:

```text
FrameSink {
  attachSurface(surfaceHandle, size) -> Result
  detachSurface(reason) -> Result
  submitFrame(frame) -> Result
  resize(size) -> Result
  setFlowControl(policy) -> Result
}
```

Use a producer contract when the library emits frames:

```text
FrameSource {
  start(callbacks) -> Result
  pause(reason) -> Result
  resume(reason) -> Result
  stop(reason) -> Result
}
```

Every frame contract must define ownership, format, timestamp, stride, visible rectangle, and release semantics.

## Adapter Rules

Follow these rules:

- Keep rendering adapters separate from protocol or media core logic.
- Prefer existing library hooks for frame callbacks, surface updates, and resize events.
- Avoid assuming a surface lives for the lifetime of the connection.
- Treat retained output as an explicit cache with invalidation rules.
- Log format negotiation and surface recreation.

## Common Failure Modes

Check for:

- White, black, or stale frames after reconnect.
- Frame size mismatch after rotation or smart sizing.
- Producer writes into a released surface.
- GPU path works but CPU fallback does not, or the reverse.
- H.264 flow control missing under slow consumers.
- First frame arrives before the UI surface is ready.

## Validation

Use visual and log validation:

- First frame appears.
- Resize updates without tearing or stale borders.
- Disconnect and reconnect do not reuse invalid buffers.
- CPU and GPU fallback paths behave consistently.
- Frame pacing remains stable under load.
