# Windows RDP environment setup and troubleshooting

This document captures the Windows-side setup and network checks used for the HarmonyOS FreeRDP client.

## Roles

- A computer: the client/control computer.
- B computer: the target Windows computer being controlled.
- RDP credentials must be a B-computer account, not an A-computer account.

Example:

- B computer name: `DESKTOP-UH7T7O1`
- B computer user: `aoqiduan`
- A computer name/user: irrelevant for B login
- Login from A to B with: `DESKTOP-UH7T7O1\aoqiduan`

## Enable RDP on B

Run in an elevated PowerShell on B:

```powershell
Set-ItemProperty 'HKLM:\System\CurrentControlSet\Control\Terminal Server' -Name fDenyTSConnections -Value 0
Enable-NetFirewallRule -DisplayGroup 'Remote Desktop'
Set-NetFirewallRule -DisplayGroup 'Remote Desktop' -Profile Any -Enabled True
net localgroup "Remote Desktop Users" aoqiduan /add
Restart-Service TermService -Force
```

If the English group name is not recognized, try:

```powershell
net localgroup "远程桌面用户" aoqiduan /add
```

## Verify B

Run on B:

```powershell
ipconfig
Get-Service TermService
Get-NetTCPConnection -LocalPort 3389 -State Listen
qwinsta
net localgroup "Remote Desktop Users"
net localgroup Administrators
Get-NetConnectionProfile
Get-NetFirewallProfile
```

Expected:

- `TermService` is `Running`.
- `3389` listens on `0.0.0.0` and/or `::`.
- `rdp-tcp` is listed by `qwinsta`.
- The target user is in `Remote Desktop Users` or `Administrators`.
- The target user has a password.

Windows Home usually cannot act as a standard RDP host.

## Verify A to B

Run on A with B's current IP:

```powershell
ping <B-IP>
Test-NetConnection -ComputerName <B-IP> -Port 3389
mstsc /v:<B-IP>
```

The demo app also performs a lightweight RDP negotiation check. The important distinctions are:

- `timeout`: A cannot reach B's TCP 3389.
- `TCP connected, but no RDP negotiation response`: something accepts/closes TCP but does not behave like a normal RDP service.
- `RDP negotiation response received`: network and RDP service are usable; remaining failures are usually credentials, NLA, or account policy.

## Network findings from this setup

The successful setup used the phone-hotspot network:

- A: `172.20.10.2`
- B: `172.20.10.3`
- Gateway: `172.20.10.1`

Earlier failures came from mixed or isolated networks:

- Guest Wi-Fi (`Huawei-Guest`) likely isolated clients.
- A-side VPN/tunnel (`0dcloud`, `198.18.0.1`) stole the route to B and made diagnostics misleading.
- B's WSL/Hyper-V address `172.23.208.1` was not the correct LAN RDP address.

Use this on A to check route selection:

```powershell
Find-NetRoute -RemoteIPAddress <B-IP>
route print <B-IP>
Get-NetTCPConnection -RemoteAddress <B-IP>
```

If traffic goes through VPN/tunnel instead of the Wi-Fi interface, disable the VPN or add a correct host route temporarily.

## Common fixes

Use the same non-Guest network on both computers, or use the same phone hotspot.

Avoid guest Wi-Fi unless AP isolation/client isolation is disabled.

Temporarily disable Windows Firewall on B only for diagnosis:

```powershell
Set-NetFirewallProfile -Profile Domain,Private,Public -Enabled False
```

Restore it after testing:

```powershell
Set-NetFirewallProfile -Profile Domain,Private,Public -Enabled True
```

If the network is correct and `mstsc` reaches the login prompt, use:

```text
DESKTOP-UH7T7O1\aoqiduan
```

with the B computer user's password.
