# HAP signing material

This directory keeps the signing material consumed by the HarmonyOS project build.

Signing is configured in `harmony/app/build-profile.json5` and should run through the HarmonyOS MCP `build_app` flow. Do not use a standalone signing script.

Required files:

- `OpenHarmony.p12`
- `OpenHarmonyApplication.pem`
- `OpenHarmonyProfileDebug.pem`
- `ohos_provision_debug.p7b`
- `material/`

Generated HAPs and verification output remain local under `output/` and are ignored by git.
