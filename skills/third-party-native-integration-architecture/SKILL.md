---
name: third-party-native-integration-architecture
description: Design reusable architecture and interface contracts for integrating third-party native libraries into app platforms. Use when Codex needs to propose or review the core approach, layer boundaries, APIs, controller contracts, permission gates, runtime packaging, platform backends, diagnostics, or validation architecture for C/C++ libraries on OHOS, Android, iOS, Linux, desktop, or embedded targets.
---

# Third-Party Native Integration Architecture

## Core Idea

Treat every platform integration as a productized port, not a local patch set.

Preserve the upstream core. Put target-specific work behind explicit build profiles, runtime packaging, platform backends, permission gates, lifecycle controllers, and validation probes. The app owns user-facing state and permissions; native code owns portable protocol, media, codec, compute, or service logic.

## Recommended Architecture

Use this layer model unless the repository already has a stronger local pattern:

```text
Product UI or workflow
  -> App integration state
  -> Capability and permission gate
  -> Native bridge or FFI boundary
  -> Runtime loader and service controller
  -> Platform backend adapters
  -> Upstream third-party core
  -> Target sysroot and bundled dependencies
  -> Feature matrix, probes, and diagnostics
  -> Upstream patch management and compliance records
```

Keep dependencies flowing downward. The upstream core must not know about UI state, app routing, or permission prompts.

## Boundary Rules

- Put platform API calls in backend adapters, not in portable core code.
- Put app lifecycle and permission decisions above the native bridge.
- Put daemon process control behind a controller, not in UI components.
- Put runtime paths, dependency layout, and probes in packaging code.
- Put feature truth in a feature matrix backed by build flags and runtime probes.
- Put cross-language calls behind a small bridge facade with stable handles and structured callbacks.
- Put upstream divergence into patch records and upgrade plans.
- Put security, license, and privacy obligations into a compliance report.
- Avoid exposing platform object types through public library APIs unless the ecosystem already expects handles.

## Suggested Interfaces

Define only the contracts the integration needs. Prefer versioned structs and stable C ABI at cross-language boundaries.

### Build Profile

```text
BuildProfile {
  targetName
  targetTriple
  toolchainPath
  sysrootPath
  dependencyPrefix
  installPrefix
  featureFlags
  sourceRevision
}
```

Use this to make cross-compilation reproducible and reviewable.

### Runtime Bundle

```text
RuntimeBundle {
  libDir
  pluginDir
  configDir
  dataDir
  writableRunDir
  probePath
}
```

Use this to keep library loading, config, generated files, and writable state separate.

### Capability Gate

```text
CapabilityGate {
  check(feature) -> CapabilityState
  request(feature, reason) -> CapabilityState
  canStart(feature) -> boolean
  subscribe(listener)
}
```

States should include `unknown`, `checking`, `granted`, `denied`, `unavailable`, `busy`, and `failed`.

### Native Controller

```text
NativeController {
  initialize(RuntimeBundle, callbacks) -> Result
  start(options) -> Result
  stop(reason) -> Result
  getStatus() -> Status
  getDiagnostics() -> Diagnostics
  shutdown() -> Result
}
```

Use this for app-facing native components, including client libraries, media engines, and embedded services.

### Native Bridge

```text
BridgeApi {
  initialize(RuntimeBundle, callbacks) -> Result
  createSession(options) -> NativeHandle
  start(handle) -> Result
  stop(handle, reason) -> Result
  destroy(handle) -> Result
  getStatus(handle) -> Status
  getDiagnostics(handle) -> Diagnostics
}
```

Use this at NAPI, JNI, Swift/Objective-C, Rust FFI, Dart FFI, Python binding, or C ABI boundaries.

### Platform Backend

```text
PlatformBackend {
  name
  enumerateCapabilities() -> CapabilityList
  open(options, callbacks) -> Handle
  close(handle) -> Result
  getDiagnostics(handle) -> Diagnostics
}
```

Backends adapt camera, audio, rendering, files, devices, input, network, sensors, or windowing APIs.

### Service Controller

```text
ServiceController {
  ensureStarted(reason) -> ServiceStatus
  stop(reason) -> ServiceStatus
  restart(reason) -> ServiceStatus
  refreshDiagnostics() -> Diagnostics
}
```

Use this for native daemons and long-running local services.

### Frame Pipeline

```text
FrameSink {
  attachSurface(surfaceHandle, size) -> Result
  detachSurface(reason) -> Result
  submitFrame(frame) -> Result
  resize(size) -> Result
  setFlowControl(policy) -> Result
}
```

Use this for video, remote display, GPU compositor, decoder, or capture output.

### Runtime Probe

```text
RuntimeProbe {
  getVersions() -> VersionReport
  getFeatureProfile() -> FeatureProfile
  checkDependencies() -> DependencyReport
  checkSymbols() -> SymbolReport
}
```

Use this to prove the packaged app can load and use the expected target artifacts.

### Patch Record

```text
PatchRecord {
  id
  category
  upstreamBase
  files
  reason
  upstreamable
  validation
}
```

Use this to keep local source changes understandable across upstream upgrades.

### Compliance Report

```text
ComplianceReport {
  dependencies
  noticesRequired
  sourceOfferRequired
  securityFindings
  privacyCapabilities
  distributionBlockers
}
```

Use this before shipping bundled native code or enabling sensitive features.

## Error and Diagnostics Model

Return structured errors:

```text
Error {
  category
  code
  message
  recoverable
  detail
}
```

Use categories such as `build`, `bundle`, `load`, `permission`, `backend`, `service`, `render`, `network`, and `validation`. Redact secrets in diagnostics.

## Delivery Checklist

Finish an integration design with:

- Layer diagram or written boundary map.
- Interface contracts that match the chosen layers.
- Build profile and runtime bundle layout.
- Permission and lifecycle state model.
- Backend selection and fallback rules.
- Feature matrix and probe plan.
- Bridge contract and callback ownership rules.
- Patch records for local divergence.
- Security, license, privacy, and distribution review.
