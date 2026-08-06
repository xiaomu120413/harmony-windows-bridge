# Documentation Index

Keep current operating docs in this directory. Retained retrospectives and dated
validation notes live under `docs/archive/`.

## Current Docs

- [CHANGELOG.md](CHANGELOG.md): repository-wide change ledger. Each Git commit
  has one concise record of what changed, its impact and where to find
  validation evidence.

- [harmonyos-tablet-adaptation-architecture-and-acceptance.md](harmonyos-tablet-adaptation-architecture-and-acceptance.md):
  existing tablet/2in1 UI, session, input, XComponent and acceptance ledger.
  Its former single-HAP packaging constraint is retained as history and is
  superseded by the multi-device packaging plan below.

- [harmonyos-multidevice-hnp-packaging-plan.md](harmonyos-multidevice-hnp-packaging-plan.md):
  DesignReady target architecture and staged validation plan for one App Pack,
  device-specific Entry HAPs, shared HSP, 2in1-only HNP/permissions, independent
  XRDP process, upgrade safety and AppGallery distribution checks.

- `freerdp-ohos-feature-matrix.md`: current OHOS FreeRDP feature status,
  default channel policy, permissions and fallback behavior.
- `freerdp-ohos-pen-and-multimon-design.md`: DesignReady ownership, ABI,
  fallback and acceptance contract for native stylus and multi-monitor support.
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
