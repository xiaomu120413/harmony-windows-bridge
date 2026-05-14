# HAP signer

This directory keeps the local HAP signing toolchain used by this repo.

The signed HAP output is generated locally under `output/` and is intentionally ignored by git.
Intermediate files are removed after verification.

## Sign as a normal app

```powershell
.\tools\hapsigner\Sign-NormalApp.ps1 -InputHap C:\path\to\entry-default-unsigned.hap
```

The script rewrites the input HAP metadata before signing:

- removes `requestPermissions`
- removes `EnterpriseAdminAbility`
- signs with `apl: normal`
- signs with `app-feature: hos_normal_app`

Expected local output:

```text
tools\hapsigner\output\securitytool-normal-signed.hap
```
