[CmdletBinding()]
param(
    [string]$VcpkgRoot = (Join-Path $PSScriptRoot '..\tools\vcpkg'),
    [string]$Triplet = 'x64-windows'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$bridgeSource = Join-Path $repoRoot 'native\freerdp-bridge'
$bridgeBuild = Join-Path $bridgeSource 'build'
$vcpkgExe = Join-Path $VcpkgRoot 'vcpkg.exe'
$bootstrap = Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat'
$cmakeExe = (Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -First 1).Source

if (-not $cmakeExe) {
    $defaultCmake = 'C:\Program Files\CMake\bin\cmake.exe'
    if (Test-Path -LiteralPath $defaultCmake) {
        $cmakeExe = $defaultCmake
    } else {
        throw 'CMake was not found. Install Kitware.CMake first.'
    }
}

$vcvars = Get-ChildItem 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat' -ErrorAction SilentlyContinue |
    Select-Object -First 1

if (-not $vcvars) {
    throw 'MSVC vcvars64.bat was not found. Install Visual Studio Build Tools with the C++ workload first.'
}

if (-not (Test-Path -LiteralPath $vcpkgExe)) {
    if (-not (Test-Path -LiteralPath $bootstrap)) {
        throw "vcpkg was not found at $VcpkgRoot. Clone https://github.com/microsoft/vcpkg there first."
    }
    & cmd /c "`"$bootstrap`" -disableMetrics"
}

& cmd /c "`"$($vcvars.FullName)`" >nul && `"$vcpkgExe`" install freerdp:$Triplet"

$freeRdpRoot = Join-Path $VcpkgRoot "installed\$Triplet"
& cmd /c "`"$($vcvars.FullName)`" >nul && set FREERDP_ROOT=$freeRdpRoot&& `"$cmakeExe`" -S `"$bridgeSource`" -B `"$bridgeBuild`" -G `"NMake Makefiles`""
& cmd /c "`"$($vcvars.FullName)`" >nul && `"$cmakeExe`" --build `"$bridgeBuild`""

Copy-Item (Join-Path $freeRdpRoot 'bin\*.dll') $bridgeBuild -Force

$bridgeExe = Join-Path $bridgeBuild 'freerdp_bridge.exe'
& $bridgeExe --probe

Write-Host "Native bridge built: $bridgeExe"
