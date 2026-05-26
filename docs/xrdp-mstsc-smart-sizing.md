# xrdp MSTSC sizing notes

This document is only for the xrdp server path. It covers the case where the
HarmonyOS display is larger than the local Windows MSTSC window.

## Current decision

- Do not rely on a custom Windows launcher script.
- Users can open the MSTSC UI directly and connect to `127.0.0.1:13390` after
  `hdc fport` is ready.
- The xrdp OHOS backend defaults to a server-side desktop limit of
  `1920x1280`.
- The backend keeps the HarmonyOS display aspect ratio when it caps the RDP
  desktop size.
- MSTSC smart sizing can still be enabled manually in a saved `.rdp` file, but
  it is not required for the default path.
- Keep dynamic resolution disabled by default for now. Dynamic resolution can
  be evaluated later, but it asks the server to resize the desktop when the
  local MSTSC window changes and may restart the capture path.

## Start MSTSC

Forward the local Windows port to the HarmonyOS xrdp port:

```powershell
hdc fport tcp:13390 tcp:3390
```

Then start MSTSC from the UI and connect to:

```text
127.0.0.1:13390
```

Command-line startup is also fine:

```powershell
mstsc /v:127.0.0.1:13390
```

To request a smaller initial RDP desktop explicitly:

```powershell
mstsc /v:127.0.0.1:13390 /w:1920 /h:1280
```

## Server config

The OHOS backend reads these values from the xrdp config:

```ini
[OHOS]
max_desktop_width=1920
max_desktop_height=1280
```

If MSTSC requests a larger desktop, the backend caps it to the configured
maximum while preserving the device aspect ratio.

## Expected server logs

For the default path, device logs should show the xrdp desktop and capture size
as `1920x1280`, while input mapping can still report the real HarmonyOS display
size, for example `target=3120x2080`.

Typical lines:

```text
xrdp.ohos.resize ... requested=1920x1280 target=1920x1280 display=3120x2080 max=1920x1280 limited_by_max=0
xrdp.ohos.input ... desktop=1920x1280
xrdp screen capture frame queued: ... size=1920x1280 target=1920x1280
```

When a user connects directly from the MSTSC UI with a larger requested
desktop, the resize log should include `limited_by_max=1`.
