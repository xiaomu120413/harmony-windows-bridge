---
name: native-runtime-bundling
description: Package, load, and verify native shared-library runtimes inside application bundles or deployment artifacts. Use when Codex needs to copy .so/.dll/.dylib files, resolve transitive native dependencies, configure rpath or $ORIGIN lookup, generate runtime material, add native probes, or debug app-time loading failures for third-party libraries.
---

# Native Runtime Bundling

## Workflow

Separate build success from runtime load success.

1. Inventory produced native artifacts and transitive dependencies.
2. Decide the app bundle layout for libraries, config files, data files, plugins, and probes.
3. Preserve relative lookup with `$ORIGIN`, rpath, loader path, or platform equivalent.
4. Copy only target-runtime artifacts, not host build leftovers.
5. Add a small native or app-level probe that reports version, feature flags, and load status.
6. Verify the bundled app loads the same artifacts that the build produced.

## Artifact Inventory

List:

- Primary shared libraries.
- Plugin modules and dynamically loaded backends.
- Required config, certificates, codecs, fonts, models, or data files.
- License files if distribution requires them.
- Debug symbols or stripped variants if the release process distinguishes them.

Use platform dependency tools where available, but confirm manually when libraries use `dlopen` or plugin discovery.

## Bundle Layout

Prefer a predictable layout:

```text
app runtime root
  lib/
  plugins/
  config/
  data/
  probes/
```

Keep config and generated runtime material outside immutable library directories when the platform requires writable paths.

## Suggested Interface

Expose runtime layout as a single object:

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

Native initialization should receive this bundle or equivalent paths once. Avoid hard-coded library and config locations inside the native core.

## Loader Rules

Check these before debugging application code:

- The app can see the library directory.
- rpath or loader path points to sibling dependency directories.
- Architecture and ABI match the target.
- Optional plugins do not fail the whole app unless they are required.
- Library names match what the code loads at runtime.

## Runtime Probe

When possible, add a lightweight probe that returns:

- Library name and version.
- Build commit or source revision.
- Enabled feature flags.
- Important dependency versions.
- Whether required symbols or plugin entry points are present.

Use the probe in smoke tests and support diagnostics.

Suggested probe contract:

```text
RuntimeProbe {
  getVersions() -> VersionReport
  getFeatureProfile() -> FeatureProfile
  checkDependencies() -> DependencyReport
  checkSymbols() -> SymbolReport
}
```
