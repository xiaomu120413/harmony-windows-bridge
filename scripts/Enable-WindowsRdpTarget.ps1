[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$UserToAllow
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    throw 'Run this script in an elevated PowerShell window on the target Windows machine.'
}

if ($PSCmdlet.ShouldProcess('Target Windows machine', 'Enable Remote Desktop')) {
    Set-ItemProperty -Path 'HKLM:\System\CurrentControlSet\Control\Terminal Server' -Name 'fDenyTSConnections' -Value 0
    Enable-NetFirewallRule -DisplayGroup 'Remote Desktop'
    Set-ItemProperty -Path 'HKLM:\System\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'UserAuthentication' -Value 1
}

if ($UserToAllow) {
    if ($PSCmdlet.ShouldProcess($UserToAllow, 'Add to Remote Desktop Users group')) {
        Add-LocalGroupMember -Group 'Remote Desktop Users' -Member $UserToAllow
    }
}

$rdpState = Get-ItemProperty -Path 'HKLM:\System\CurrentControlSet\Control\Terminal Server' -Name 'fDenyTSConnections'
$nlaState = Get-ItemProperty -Path 'HKLM:\System\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'UserAuthentication'
$ipAddresses = Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -notlike '169.254.*' -and $_.IPAddress -ne '127.0.0.1' } |
    Select-Object -ExpandProperty IPAddress

[pscustomobject]@{
    RemoteDesktopEnabled = ($rdpState.fDenyTSConnections -eq 0)
    NetworkLevelAuthentication = ($nlaState.UserAuthentication -eq 1)
    TargetIPv4 = ($ipAddresses -join ', ')
    Port = 3389
}
