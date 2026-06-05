---
name: capability-permission-gating
description: Map third-party library capabilities to platform permissions, entitlements, privacy prompts, and app state gates. Use when Codex needs to prevent native features from starting before camera, microphone, screen capture, file access, network, input control, device, or remote-control permissions are granted across OHOS, Android, iOS, Linux, or desktop platforms.
---

# Capability Permission Gating

## Workflow

Make app state the source of truth for sensitive capability startup.

1. Identify the library feature and the platform permission or entitlement it requires.
2. Split capability discovery, permission request, and feature startup into separate steps.
3. Request permission at the user-facing moment, not during passive library initialization.
4. Expose a gate result to native code: allowed, denied, unavailable, busy, or unknown.
5. Reflect native state back to UI so the user sees the real capability status.
6. Handle revocation, denial, timeout, and restart paths.

## Gate Model

Use a simple state model:

```text
unknown -> checking -> granted -> starting -> active
                    -> denied
                    -> unavailable
                    -> failed
```

Do not let native code directly trigger privacy prompts unless the app's platform model requires it. Prefer app-managed permission flows and explicit native callbacks.

## Suggested Interface

Use one gate for all sensitive capabilities:

```text
CapabilityGate {
  check(feature) -> CapabilityState
  request(feature, reason) -> CapabilityState
  canStart(feature) -> boolean
  subscribe(listener)
}
```

Native code should receive the gate result, not own the user prompt. The app should translate gate state into UI and startup decisions.

## Capability Mapping

Create a local mapping table:

- Library feature.
- Platform permission or entitlement.
- Runtime check API.
- Prompt API or settings page.
- Native startup function.
- UI state and fallback behavior.

Keep mappings close to the app integration layer so upstream library code stays portable.

## Failure Handling

Treat these as first-class outcomes:

- Permission denied.
- Permission granted but OS service unavailable.
- Capability already in use.
- App backgrounded or surface destroyed.
- Native library reports feature unsupported.
- User disables the gate after startup.

Always stop or skip native startup cleanly when the gate is not granted.

## Validation

Test:

- First-run grant.
- First-run denial.
- Permission revoked from settings.
- Feature restart after grant.
- App relaunch with persisted permission state.
- Native failure after permission grant.
