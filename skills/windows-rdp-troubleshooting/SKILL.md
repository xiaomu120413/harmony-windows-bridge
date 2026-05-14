---
name: windows-rdp-troubleshooting
description: Diagnose Windows Remote Desktop and FreeRDP connection failures. Use when Codex needs to debug RDP/mstsc/FreeRDP issues involving target Windows setup, allowed users, firewall rules, RDP service state, local network reachability, guest Wi-Fi isolation, VPN route hijacking, or RDP protocol negotiation failures.
---

# Windows RDP Troubleshooting

## Workflow

Start by identifying roles:

- A computer: client/control computer.
- B computer: target Windows computer.
- Credentials must be a B-computer account, not an A-computer account.

Use the detailed command reference in `references/windows-rdp-environment-setup.md` when the task requires exact PowerShell commands or interpreting command output.

## B Computer Checks

Verify the target before debugging the client:

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

Require:

- `TermService` is `Running`.
- TCP `3389` listens on `0.0.0.0` or `::`.
- `qwinsta` lists `rdp-tcp`.
- The login user is in `Remote Desktop Users` or `Administrators`.
- The login user has a non-empty password.

If needed, enable RDP and add the B user:

```powershell
Set-ItemProperty 'HKLM:\System\CurrentControlSet\Control\Terminal Server' -Name fDenyTSConnections -Value 0
Enable-NetFirewallRule -DisplayGroup 'Remote Desktop'
Set-NetFirewallRule -DisplayGroup 'Remote Desktop' -Profile Any -Enabled True
net localgroup "Remote Desktop Users" <B-USER> /add
Restart-Service TermService -Force
```

## A Computer Checks

Check whether A and B are on a mutually reachable network:

```powershell
ipconfig
netsh wlan show interfaces
ping <B-IP>
Test-NetConnection -ComputerName <B-IP> -Port 3389
Find-NetRoute -RemoteIPAddress <B-IP>
route print <B-IP>
```

Interpretation:

- Ping fails and no ARP entry: A and B are not in the same reachable L2 network, or AP/client isolation is active.
- TCP `3389` times out: firewall, service exposure, routing, or AP isolation blocks RDP.
- TCP connects but RDP negotiation gets no response: port is not behaving like a valid RDP service or middleware/VPN is intercepting.
- RDP negotiation response is received: network and target service are usable; continue with credentials/NLA/account policy.

## Common Root Causes

Prioritize network path before user permissions when A cannot reach B:

- A and B are on different subnets or different Wi-Fi networks.
- Guest Wi-Fi has AP isolation/client isolation enabled.
- VPN/tunnel/proxy hijacks the route to B, e.g. traffic leaves through a tunnel interface instead of Wi-Fi.
- The user is adding an A-computer account to B instead of using a B-computer account.
- B is Windows Home, which usually cannot host standard Remote Desktop.

## Login Format

Use B computer credentials:

```text
<B-COMPUTER-NAME>\<B-USER>
```

Example:

```text
DESKTOP-UH7T7O1\aoqiduan
```

Do not use the A computer name or A user when logging into B.
