---
name: upstream-patch-upgrade-management
description: Maintain local platform patches, forks, submodules, and upgrade workflows for third-party libraries while staying aligned with upstream community projects. Use when Codex needs to review or manage vendored source, git submodules, patch queues, rebases, upstream version bumps, local target-specific changes, changelog impact, or long-term fork hygiene.
---

# Upstream Patch Upgrade Management

## Workflow

Make local divergence intentional and easy to rebase.

1. Identify the upstream source, revision, branch, release tag, and local import method.
2. Separate local patches into build, platform backend, bug fix, packaging, and product-behavior categories.
3. Keep target-specific patches small and guarded by feature flags or platform macros.
4. Record why each patch exists and whether it should be upstreamed, dropped, or kept local.
5. During upgrades, compare upstream changelog, security advisories, API changes, and local patch conflicts.
6. Re-run build, runtime bundle, feature matrix, and smoke validation before accepting the upgrade.

## Patch Categories

Use these buckets:

- `build`: toolchain, sysroot, feature flags, configure scripts.
- `platform-backend`: OS APIs for camera, audio, rendering, files, input, or services.
- `runtime`: loading, plugin discovery, rpath, config paths.
- `bug-fix`: correctness fixes that may be upstreamable.
- `product`: behavior intentionally different from upstream defaults.
- `diagnostics`: logging, probes, or support hooks.

Avoid mixing categories in one large patch.

## Suggested Interfaces

Track local divergence with:

```text
PatchRecord {
  id
  category
  upstreamBase
  files
  reason
  upstreamable
  validation
  owner
}
```

Track upgrades with:

```text
UpgradePlan {
  currentRevision
  targetRevision
  changelogSummary
  patchConflictRisk
  apiCompatibilityRisk
  securityChanges
  validationPlan
}
```

## Upgrade Checks

Before changing the source revision:

- Check upstream release notes and breaking changes.
- Check security advisories and fixed CVEs.
- Check whether local patches overlap with upstream changes.
- Check generated build files if the project uses Autotools or vendored generated artifacts.
- Check ABI or public header changes if the app links directly.

After upgrading:

- Rebuild from a clean target sysroot.
- Repackage runtime artifacts.
- Run probes and smoke tests.
- Update the feature matrix and patch records.

## Git Hygiene

Prefer readable history:

- Keep third-party source changes separate from app integration changes.
- Use submodule commits, vendor import commits, or patch files consistently.
- Avoid editing generated files unless the upstream workflow requires it.
- Do not hide target changes inside unrelated vendor updates.
