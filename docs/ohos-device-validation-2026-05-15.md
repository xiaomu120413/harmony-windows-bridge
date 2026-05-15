# HarmonyOS FreeRDP Device Validation - 2026-05-15

## Scope

- Device: `3QC0124C11000711`, HUAWEI MateBook Pro, OpenHarmony `6.1.0.105`, API 23.
- Target: `172.20.10.2:3389`.
- Account format validated: `DESKTOP-UH7T7O1\aoqiduan`.
- Password was entered from prior local context and is not recorded here.
- Signed HAP rebuilt locally through HarmonyOS MCP after refreshing WSL native outputs.

## Build Artifact

- WSL native build refreshed at `2026-05-15T01:43:03Z`.
- Signed HAP: `harmony/app/entry/build/default/outputs/default/entry-default-signed.hap`.
- HAP timestamp: `2026-05-15 09:43:51`.
- HAP size: `74323629` bytes.
- HAP artifact is intentionally not tracked by git.

## Runtime Feature Probe

The installed app reported:

- `bridge 0.6.6`
- `arm64-v8a`
- `FreeRDP 3.26.1-dev0`
- `WinPR 3.26.1-dev0`
- `OpenSSL 3.3.2`
- Client channel loader registered through `libfreerdp-client` static addin provider.
- Compiled/requested features: `cliprdr`, `rdpdr`, `drive`, `printer`, `smartcard`, `rdpsnd`, `audin`, `rdpgfx`, `disp`, H.264, FFmpeg, OpenH264, RD Gateway core.

WSL build manifest still reports:

- `with_opensles=ON`
- `with_smartcard_pcsc=ON`
- `with_pcsc=OFF`
- `with_cups=OFF`
- `with_fuse=OFF`

## Validation Result

### TOFU Certificate Policy

Result: failed before desktop session.

Observed flow:

- `state=Resolving`
- `state=TCP connected`
- `state=Negotiating`
- `state=Authenticating`
- Certificate accepted by TOFU policy for `172.20.10.2:3389`, CN `DESKTOP-UH7T7O1`
- `freerdp_connect returned false`
- Error: `ERRCONNECT_TLS_CONNECT_FAILED [0x00020008] The connection failed at TLS connect.`

Impact:

- TOFU storage/acceptance is not enough for this target yet.
- Strict certificate mode is expected to be stricter and was not useful for this target validation.
- The certificate path needs more work before it can be the default production path.

### Ignore Certificate Policy

Result: connected, but no visible desktop frame yet.

Observed flow:

- `state=Resolving`
- `state=TCP connected`
- `state=Negotiating`
- `state=Authenticating`
- `freerdp_connect returned true`
- `state=Connected`
- `FreeRDP persistent session loop is active`
- `FreeRDP input bridge is using worker-thread dispatch`
- `FreeRDP event loop started`
- Desktop size reported: `1280x720`

Impact:

- Network, account format, password, RDP negotiation, TLS/NLA/auth, and the persistent event loop are proven on device when certificate validation is ignored.
- The current blocker has moved from connection/auth to rendering or frame delivery.
- The session page remains black even while the app reports `Connected`, so frame update callback, GDI invalidation, NativeWindow buffer write, or XComponent lifetime handling needs focused debugging.

## Remaining Issues

- TOFU certificate mode fails at TLS connect on this target; Ignore works.
- Connected session does not show remote desktop pixels.
- Input was not validated against a visible desktop because no frame was rendered.
- `with_pcsc=OFF` means physical PCSC backend is still not present in the default WSL runtime, although `with_smartcard_pcsc=ON` is recorded.
- `with_cups=OFF` and `with_fuse=OFF`; printer backend and FUSE drive backend are not available from the OHOS sysroot.
- Device ICMP ping to `172.20.10.2` had 100% loss earlier, but TCP 3389 succeeds from the app, so ICMP cannot be used as the acceptance gate for this target.

## Next Debug Step

Focus on M5 rendering after a confirmed Ignore-policy connection:

1. Add native counters for GDI update callbacks and paint calls.
2. Log the first dirty rect and source pixel format.
3. Verify `OH_NativeWindow` buffer geometry after `XComponent` creation.
4. Keep the session tab active while connected to avoid destroying the XComponent during diagnostics.
5. If update callbacks do not fire, reduce negotiated graphics options temporarily and test software GDI only without `rdpgfx/h264`.
