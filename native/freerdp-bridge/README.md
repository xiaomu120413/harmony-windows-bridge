# FreeRDP native bridge

This bridge is the desktop stand-in for the native layer the HarmonyOS app will need.

It links against FreeRDP as a library and exposes a small process boundary for the current Node-based demo:

- `freerdp_bridge --probe`: verifies that the bridge can load and call FreeRDP.
- `freerdp_bridge --connect ...`: receives app connection parameters. The rendering/input loop is intentionally left as the next native milestone.

## Build

Install or build FreeRDP development files first, then point `FREERDP_ROOT` at the install prefix:

```powershell
$env:FREERDP_ROOT = "C:\path\to\freerdp-install"
cmake -S native\freerdp-bridge -B native\freerdp-bridge\build
cmake --build native\freerdp-bridge\build --config Release
```

Expected output path on Windows:

```text
native\freerdp-bridge\build\Release\freerdp_bridge.exe
```

If your generator writes directly under `build`, put the executable path into the app's "native bridge path" field.

## Why this exists

Starting `wfreerdp.exe` is useful for checking RDP credentials and network access, but it is not the final product architecture. A HarmonyOS implementation should call FreeRDP from a native module and render frames into an app surface.
