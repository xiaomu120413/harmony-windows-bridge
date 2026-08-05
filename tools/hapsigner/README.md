# HAP signing material

This directory keeps the signing material consumed by the HarmonyOS project build.

Signing is configured in `harmony/app/build-profile.json5` and should run through the HarmonyOS MCP `build_app` flow. Do not use a standalone signing script.

Required files:

- `OpenHarmony.p12`
- `OpenHarmonyApplication.pem`
- `OpenHarmonyProfileDebug.pem`
- `UnsgnedDebugProfileTemplate.json`
- `ohos_provision_debug.p7b`
- `material/`

Generated HAPs and verification output remain local under `output/` and are ignored by git.

When a restricted permission changes, update both ACL arrays in
`UnsgnedDebugProfileTemplate.json`, regenerate `ohos_provision_debug.p7b` with the bundled
HarmonyOS `hap-sign-tool.jar`, and then rebuild through the normal project build flow. The local
debug ACL is only for device validation and does not replace AGC review or a production profile.
