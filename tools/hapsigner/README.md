# Legacy OpenHarmony signing material

This directory retains the earlier OpenHarmony signing profile and its historical debug material. It is not the current application signing default.

Current builds use `tools/app` as configured in `harmony/app/build-profile.json5`. Build through `harmony/app/build_hap.bat`; this flow includes HNP repacking and signing when required. See the [current signing baseline](../../docs/freerdp-ohos-validation-baseline.md).

The files here are retained for historical reproducibility. Do not substitute them for the current application's certificate/profile or infer current ACL approval from them. Generated output under `output/` remains ignored by Git.