---
name: third-party-feature-validation
description: Build and maintain feature matrices, probes, and validation baselines for third-party native libraries. Use when Codex needs to prove which features are enabled, disabled, fallback-backed, packaged, or runtime-usable after porting, upgrading, cross-compiling, or integrating complex dependencies.
---

# Third-Party Feature Validation

## Workflow

Verify features, not just builds.

1. List expected capabilities from product requirements and upstream documentation.
2. Map each capability to build flags, dependencies, runtime artifacts, permissions, and validation steps.
3. Generate or update a feature matrix with enabled, disabled, fallback, and unknown states.
4. Add probes or smoke tests that can run in the target runtime.
5. Compare the current matrix against the previous known-good baseline.
6. Record upgrade risks and unresolved gaps.

## Feature Matrix

Track:

- Capability name.
- Upstream feature or module.
- Build option.
- Required dependency.
- Runtime artifact or plugin.
- Permission or platform capability.
- Validation method.
- Status: enabled, disabled, fallback, blocked, unknown.
- Notes and owner when follow-up is needed.

Prefer Markdown for human review and machine-readable JSON when automation will consume the matrix.

## Suggested Interfaces

Use a feature record that can be rendered as Markdown or JSON:

```text
FeatureRecord {
  capability
  upstreamModule
  buildOption
  dependency
  runtimeArtifact
  permission
  validationMethod
  status
  notes
}
```

Use a probe report for runtime facts:

```text
ProbeReport {
  versions
  featureProfile
  dependencies
  symbols
  backendSelection
  smokeResults
}
```

## Probe Strategy

Use probes for facts that logs and build output cannot prove:

- Linked library versions.
- Compiled feature flags.
- Runtime-loaded plugins.
- Required symbols.
- Platform backend selection.
- Basic open/start/stop behavior.

Keep probes small and deterministic so they can run during packaging or smoke validation.

## Baseline Comparison

When upgrading a third-party library:

- Compare source revision, dependency versions, and build options.
- Detect newly disabled features.
- Detect features that compile but no longer load at runtime.
- Check whether local patches still apply cleanly.
- Re-run target smoke tests before declaring the upgrade complete.

## Validation Output

Finish with:

- Updated feature matrix.
- Probe or smoke-test output summary.
- Known gaps and fallback behavior.
- Exact build or package revision used for validation.
