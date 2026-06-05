---
name: third-party-security-compliance
description: Review security, license, privacy, and distribution risks for third-party native dependencies. Use when Codex needs to evaluate licenses, NOTICE obligations, CVEs, security advisories, cryptography, bundled codecs, export-sensitive features, dependency provenance, SBOM data, privacy-sensitive capabilities, or app-store compliance for C/C++ libraries and native runtimes.
---

# Third-Party Security Compliance

## Workflow

Treat third-party native code as a supply-chain and distribution surface.

1. Identify source origin, version, license, transitive dependencies, and bundled artifacts.
2. Check security advisories, CVEs, and upstream release notes for the selected revision.
3. Review license obligations for source, binaries, codecs, plugins, and bundled data files.
4. Identify privacy-sensitive capabilities such as camera, microphone, screen capture, files, location, input, or network exposure.
5. Confirm distribution constraints for app stores, enterprise delivery, and target regions.
6. Produce a concise risk report with required notices, mitigations, and blockers.

## License Review

Track:

- Dependency name and version.
- License expression.
- Static or dynamic linking mode.
- Modified or unmodified source.
- Notice, attribution, source offer, or copyleft obligations.
- Binary redistribution constraints.

When license terms are ambiguous or high impact, flag for legal review instead of guessing.

## Security Review

Check:

- Known CVEs and advisories for the exact version.
- Whether the build enables affected modules.
- Whether local patches change attack surface.
- Whether network listeners, file parsers, codecs, crypto, or decompression paths are exposed.
- Whether dependencies are bundled from trusted sources.

Prefer fixing by upgrading or disabling affected features over adding app-level workarounds.

## Suggested Interfaces

Use a dependency record:

```text
DependencyRiskRecord {
  name
  version
  source
  license
  linkMode
  enabledFeatures
  advisories
  obligations
  riskLevel
  mitigation
}
```

Use a report:

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

## Privacy and App Store Checks

For privacy-sensitive features, verify:

- The app declares the right permissions or entitlements.
- User prompts match the actual capability.
- The feature is disabled until permission is granted.
- Privacy policy or store metadata covers the capability.
- Logs and diagnostics do not leak secrets, tokens, file paths, or personal data.

## Output

Finish with:

- License obligations.
- Security findings and affected feature paths.
- Required notices or source offers.
- Privacy-sensitive capabilities.
- Distribution blockers and recommended mitigations.
