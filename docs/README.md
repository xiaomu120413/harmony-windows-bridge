# Documentation Index

Keep current operating docs in this directory. Retained retrospectives and dated
validation notes live under `docs/archive/`.

## Current Docs

- [harmonyos-tablet-adaptation-architecture-and-acceptance.md](harmonyos-tablet-adaptation-architecture-and-acceptance.md):
  proposed architecture and acceptance baseline (not yet implemented) for the
  single-HAP tablet/2in1 UI and capability isolation, file-level change list
  and XComponent adaptation. Current runtime capability remains defined by the
  code and the FreeRDP feature matrix.

- `freerdp-ohos-feature-matrix.md`: current OHOS FreeRDP feature status,
  default channel policy, permissions and fallback behavior.
- `freerdp-ohos-validation-baseline.md`: repeatable build, runtime sync, HAP
  packaging and true-device validation checklist.
- `freerdp-ohos-sdk-quickstart.md`: public OHOS FreeRDP SDK integration guide.
- `ohos-native-cpp-module-guidelines.md`: HAP/native module ownership,
  file-size rules and FreeRDP-vs-HAP delivery boundary.
- `release-third-party-notices.md`: third-party component, license and release
  material checklist.
- `windows-rdp-environment-setup.md`: Windows target-machine setup and network
  troubleshooting notes.

## App Docs

- `../README.md`: repository overview and normal user/developer entry point.
- `../harmony/README.md`: HarmonyOS HAP build, runtime sync and current runtime
  notes.

## Archive

`docs/archive/` contains retained debug retrospectives and dated validation
logs. They are useful when investigating old regressions, but they should not be
treated as the current source of truth.
