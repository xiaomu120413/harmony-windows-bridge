[CmdletBinding()]
param(
    [string]$ConfigPath,
    [string]$TargetHost,
    [int]$Port,
    [string]$FreeRdpPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $ConfigPath) {
    $scriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
    $ConfigPath = Join-Path $scriptRoot '..\config.local.json'
}

function Read-DemoConfig {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
}

function Get-ConfigValue {
    param(
        [object]$Config,
        [string]$Name,
        [object]$DefaultValue = $null
    )

    if ($null -eq $Config) {
        return $DefaultValue
    }

    $property = $Config.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value -or $property.Value -eq '') {
        return $DefaultValue
    }

    $property.Value
}

function Resolve-FreeRdpExecutable {
    param([string]$PreferredPath)

    if ($PreferredPath) {
        if (Test-Path -LiteralPath $PreferredPath) {
            return (Resolve-Path -LiteralPath $PreferredPath).Path
        }
        throw "FreeRDP executable was not found at '$PreferredPath'."
    }

    foreach ($name in @('wfreerdp.exe', 'wfreerdp', 'xfreerdp.exe', 'xfreerdp')) {
        $command = Get-Command $name -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($command) {
            return $command.Source
        }
    }

    return $null
}

$config = Read-DemoConfig -Path $ConfigPath

if (-not $PSBoundParameters.ContainsKey('TargetHost')) {
    $TargetHost = Get-ConfigValue -Config $config -Name 'host'
}
if (-not $PSBoundParameters.ContainsKey('Port')) {
    $Port = [int](Get-ConfigValue -Config $config -Name 'port' -DefaultValue 3389)
}

$freeRdp = Resolve-FreeRdpExecutable -PreferredPath $FreeRdpPath

[pscustomobject]@{
    Check = 'Config file'
    Ok = [bool]$config
    Detail = if ($config) { (Resolve-Path -LiteralPath $ConfigPath).Path } else { "Not found: $ConfigPath" }
}

[pscustomobject]@{
    Check = 'FreeRDP client'
    Ok = [bool]$freeRdp
    Detail = if ($freeRdp) { $freeRdp } else { 'Install FreeRDP or pass -FreeRdpPath C:\path\to\wfreerdp.exe' }
}

if ($TargetHost) {
    $tcp = Test-NetConnection -ComputerName $TargetHost -Port $Port -InformationLevel Detailed
    [pscustomobject]@{
        Check = 'Target TCP port'
        Ok = [bool]$tcp.TcpTestSucceeded
        Detail = "$TargetHost`:$Port"
    }
} else {
    [pscustomobject]@{
        Check = 'Target TCP port'
        Ok = $false
        Detail = 'No host configured. Set config.local.json host or pass -TargetHost.'
    }
}
