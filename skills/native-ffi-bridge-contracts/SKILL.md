---
name: native-ffi-bridge-contracts
description: Design stable cross-language bridge contracts between app code and native third-party libraries. Use when Codex needs to expose C/C++ functionality through NAPI, JNI, Swift/Objective-C, C ABI, Rust FFI, Dart FFI, Python bindings, callbacks, async events, thread handoff, handles, buffers, or structured diagnostics.
---

# Native FFI Bridge Contracts

## Workflow

Keep the bridge narrow, typed, and lifecycle-aware.

1. Identify the app-facing use cases, not every upstream native function.
2. Define a stable C ABI or platform-native bridge boundary.
3. Convert app objects into native options, handles, and callbacks at the boundary.
4. Keep threading, ownership, and async delivery explicit.
5. Return structured results and diagnostics instead of raw native errors.
6. Version the bridge contract when the app and native runtime may ship independently.

## Recommended Shape

Use this layer boundary:

```text
App language API
  -> bridge facade
  -> stable native ABI
  -> native controller
  -> third-party library
```

The bridge facade should be ergonomic for app code. The native ABI should be boring, stable, and easy to test.

## Suggested Interfaces

Use opaque handles for native resources:

```text
NativeHandle {
  id
  type
  generation
}
```

Use structured calls:

```text
BridgeApi {
  initialize(runtimeBundle, callbacks) -> Result
  createSession(options) -> NativeHandle
  start(handle) -> Result
  stop(handle, reason) -> Result
  destroy(handle) -> Result
  getStatus(handle) -> Status
  getDiagnostics(handle) -> Diagnostics
}
```

Use event callbacks for async state:

```text
BridgeCallbacks {
  onStateChanged(handle, state)
  onError(handle, error)
  onData(handle, payload)
  onLog(event)
}
```

## Ownership Rules

Define:

- Who allocates and frees buffers.
- Whether callbacks may outlive the original call.
- Which thread invokes callbacks.
- Whether handles are valid after stop, error, surface destruction, or app backgrounding.
- Whether native code may retain app-provided memory.

Avoid passing app-language object references into long-lived native code unless the platform bridge requires and safely pins them.

## Error Model

Return:

```text
Result {
  ok
  error
}

Error {
  category
  code
  message
  recoverable
  detail
}
```

Prefer categories such as `argument`, `lifecycle`, `permission`, `load`, `backend`, `thread`, `native`, and `unsupported`.

## Validation

Test invalid handles, double start, double stop, callback after destroy, app thread handoff, large buffers, permission denial, native crash-prone error paths, and version mismatch.
