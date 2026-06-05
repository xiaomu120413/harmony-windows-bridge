---
name: cross-platform-native-porting
description: Port C/C++ third-party libraries to a new platform target while preserving upstream compatibility. Use when Codex needs to adapt libraries that use CMake, Autotools, Meson, Makefile, pkg-config, sysroots, cross-compilers, feature flags, or native dependency trees for OHOS, Android, iOS, Linux, embedded, or similar targets.
---

# Cross-Platform Native Porting

## Workflow

Treat the target as a normal community platform port, not a one-off fork.

1. Identify the upstream build system and supported platform abstractions.
2. Locate existing target patterns such as Android, iOS, Linux, Unix, BSD, or embedded builds.
3. Prefer toolchain files, configure cache variables, pkg-config paths, and feature flags over source edits.
4. Keep upstream source changes minimal and isolate platform changes behind target guards.
5. Produce a repeatable build script that sets the compiler, sysroot, install prefix, dependency prefix, and feature profile.
6. Record disabled features with the reason and expected fallback.

## Build Systems

Use the library's native community path first:

- CMake: provide a toolchain file or explicit `CMAKE_SYSTEM_NAME`, compiler, sysroot, prefix path, and install prefix.
- Autotools: use `--host`, `--build`, `PKG_CONFIG_PATH`, `CPPFLAGS`, `LDFLAGS`, cache variables, and staged install directories.
- Meson: use a cross file and keep feature options explicit.
- Makefile-only projects: wrap compiler, archiver, sysroot, include, and library paths in a reproducible script.

Do not hard-code local machine paths into upstream files. Put environment-specific values in scripts or documented variables.

## Dependency Strategy

Build or locate dependencies before the target library.

- Prefer a target sysroot or dependency prefix with headers, libraries, and `.pc` files.
- Validate that pkg-config resolves target artifacts, not host artifacts.
- Keep optional dependencies explicit so feature changes are reviewable.
- Avoid vendoring new dependencies unless the project already follows that pattern.

## Patch Strategy

Keep patches easy to upstream or rebase:

- Put platform code in new backend files when the project has backend boundaries.
- Use existing feature macros and platform detection conventions.
- Avoid broad rewrites of portable core code.
- Keep local compatibility shims small and named by platform or capability.
- Record why a patch is target-specific when the reason is not obvious.

## Suggested Architecture and Interfaces

Use a `BuildProfile` boundary for each target:

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

Keep the build script responsible for translating this profile into CMake, Autotools, Meson, or Makefile arguments. Do not scatter target paths through source files.

Recommended build flow:

```text
source checkout
  -> dependency sysroot
  -> target build profile
  -> staged install prefix
  -> runtime bundling
  -> feature validation
```

## Deliverables

Finish with:

- A repeatable build command or script.
- Installed headers and libraries in a target prefix.
- A concise feature profile covering enabled, disabled, and fallback features.
- Notes for upstream upgrade risks, especially patched files and changed build options.
