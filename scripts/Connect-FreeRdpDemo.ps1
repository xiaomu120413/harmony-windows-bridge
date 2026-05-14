[CmdletBinding()]
param(
    [string]$ConfigPath,
    [string]$TargetHost,
    [int]$Port,
    [string]$User,
    [string]$Domain,
    [ValidateSet('ask', 'ignore', 'tofu')]
    [string]$CertMode,
    [string]$Size,
    [switch]$Fullscreen,
    [switch]$NoClipboard,
    [string]$SharePath,
    [string]$FreeRdpPath,
    [switch]$WhatIfCommand
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

    throw 'FreeRDP executable was not found. Install FreeRDP or pass -FreeRdpPath C:\path\to\wfreerdp.exe.'
}

function Get-TargetAddress {
    param(
        [string]$ComputerName,
        [int]$ComputerPort
    )

    if ($ComputerPort -and $ComputerPort -ne 3389) {
        return "$ComputerName`:$ComputerPort"
    }

    $ComputerName
}

$config = Read-DemoConfig -Path $ConfigPath

if (-not $PSBoundParameters.ContainsKey('TargetHost')) {
    $TargetHost = Get-ConfigValue -Config $config -Name 'host'
}
if (-not $PSBoundParameters.ContainsKey('Port')) {
    $Port = [int](Get-ConfigValue -Config $config -Name 'port' -DefaultValue 3389)
}
if (-not $PSBoundParameters.ContainsKey('User')) {
    $User = Get-ConfigValue -Config $config -Name 'user'
}
if (-not $PSBoundParameters.ContainsKey('Domain')) {
    $Domain = Get-ConfigValue -Config $config -Name 'domain'
}
if (-not $PSBoundParameters.ContainsKey('CertMode')) {
    $CertMode = Get-ConfigValue -Config $config -Name 'certMode' -DefaultValue 'tofu'
}
if (-not $PSBoundParameters.ContainsKey('Size')) {
    $Size = Get-ConfigValue -Config $config -Name 'size' -DefaultValue '1400x900'
}
if ($PSBoundParameters.ContainsKey('Fullscreen')) {
    $useFullscreen = [bool]$Fullscreen
} else {
    $useFullscreen = [bool](Get-ConfigValue -Config $config -Name 'fullscreen' -DefaultValue $false)
}
if ($PSBoundParameters.ContainsKey('NoClipboard')) {
    $disableClipboard = [bool]$NoClipboard
} else {
    $clipboard = [bool](Get-ConfigValue -Config $config -Name 'clipboard' -DefaultValue $true)
    $disableClipboard = -not $clipboard
}
if (-not $PSBoundParameters.ContainsKey('SharePath')) {
    $SharePath = Get-ConfigValue -Config $config -Name 'sharePath'
}

if (-not $TargetHost) {
    throw 'Target host is required. Set config.local.json host or pass -TargetHost.'
}
if (-not $User) {
    throw 'Target user is required. Set config.local.json user or pass -User.'
}

$freeRdp = Resolve-FreeRdpExecutable -PreferredPath $FreeRdpPath
$target = Get-TargetAddress -ComputerName $TargetHost -ComputerPort $Port

$arguments = @(
    "/v:$target",
    "/u:$User",
    "/cert:$CertMode",
    '/dynamic-resolution'
)

if ($Domain) {
    $arguments += "/d:$Domain"
}
if (-not $disableClipboard) {
    $arguments += '+clipboard'
}
if ($useFullscreen) {
    $arguments += '/f'
} elseif ($Size) {
    $arguments += "/size:$Size"
}
if ($SharePath) {
    if (-not (Test-Path -LiteralPath $SharePath)) {
        throw "SharePath does not exist: $SharePath"
    }
    $resolvedSharePath = (Resolve-Path -LiteralPath $SharePath).Path
    $arguments += "/drive:demo,$resolvedSharePath"
}

Write-Host "FreeRDP: $freeRdp"
Write-Host "Target : $target"
Write-Host "User   : $User"
Write-Host 'Password is not passed on the command line. Enter it in the FreeRDP prompt/window.'
Write-Host ''
Write-Host ('Command: {0} {1}' -f $freeRdp, ($arguments -join ' '))

if ($WhatIfCommand) {
    return
}

& $freeRdp @arguments
