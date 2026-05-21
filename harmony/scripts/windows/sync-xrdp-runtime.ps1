param(
  [string]$SourceRoot = "harmony/out/xrdp-ohos-arm64",
  [string]$DepsRoot = "harmony/out/ohos-arm64",
  [string]$TargetLibRoot = "harmony/app/entry/libs/arm64-v8a",
  [string]$TargetRawRoot = "harmony/app/entry/src/main/resources/rawfile/xrdp"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$repoResolved = (Resolve-Path -LiteralPath $repoRoot).Path
. (Join-Path $PSScriptRoot "xrdp-runtime-material.ps1")
$source = Resolve-Path (Join-Path $repoRoot $SourceRoot)
$deps = Resolve-Path (Join-Path $repoRoot $DepsRoot)
$targetLib = Join-Path $repoRoot $TargetLibRoot
$targetRaw = Join-Path $repoRoot $TargetRawRoot
$targetNativeRuntime = Join-Path $targetLib "xrdp"

$sysroot = Join-Path $source "sysroot"
$xrdpLibSource = Join-Path $sysroot "lib/xrdp"
$depsLibSource = Join-Path $deps "sysroot/lib"
$xrdpExecutable = Join-Path $sysroot "sbin/xrdp"
$embeddedServer = Join-Path $sysroot "lib/libxrdpserver.so"
$configSource = Join-Path $source "config/xrdp"
$shareSource = Join-Path $sysroot "share/xrdp"

function Assert-PathInsideRepo([string]$Path) {
  $full = [System.IO.Path]::GetFullPath($Path)
  if (-not $full.StartsWith($repoResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to write outside repository: $full"
  }
}

function Copy-LibraryAliases([string]$SourceDir, [string]$RealName, [string[]]$Aliases) {
  $sourcePath = Join-Path $SourceDir $RealName
  if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw "Missing xrdp runtime library: $sourcePath"
  }

  $names = @($RealName) + $Aliases
  foreach ($name in ($names | Select-Object -Unique)) {
    Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $targetLib $name) -Force
  }
}

function Get-OpenH264RealName([string]$SourceDir) {
  $candidate = Get-ChildItem -LiteralPath $SourceDir -File -Filter "libopenh264.so*" |
    Where-Object { $_.Length -gt 0 } |
    Sort-Object Name -Descending |
    Select-Object -First 1
  if ($null -eq $candidate) {
    throw "Missing OpenH264 runtime library in dependency sysroot: $SourceDir"
  }
  $candidate.Name
}

if (-not (Test-Path -LiteralPath $xrdpExecutable)) {
  throw "Missing xrdp executable: $xrdpExecutable"
}
if (-not (Test-Path -LiteralPath $embeddedServer)) {
  throw "Missing embedded xrdp server library: $embeddedServer"
}
if (-not (Test-Path -LiteralPath $configSource)) {
  throw "Missing xrdp config directory: $configSource"
}
if (-not (Test-Path -LiteralPath $shareSource)) {
  throw "Missing xrdp share directory: $shareSource"
}

Assert-PathInsideRepo $targetLib
Assert-PathInsideRepo $targetRaw
Assert-PathInsideRepo $targetNativeRuntime

New-Item -ItemType Directory -Force -Path $targetLib | Out-Null
if (Test-Path -LiteralPath $targetNativeRuntime) {
  Remove-Item -LiteralPath $targetNativeRuntime -Recurse -Force
}
if (Test-Path -LiteralPath $targetRaw) {
  $targetRawResolved = (Resolve-Path -LiteralPath $targetRaw).Path
  if (-not $targetRawResolved.StartsWith($repoResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean rawfile directory outside repository: $targetRawResolved"
  }
  Remove-Item -LiteralPath $targetRaw -Recurse -Force
}

$targetBin = Join-Path $targetRaw "bin"
$targetConfig = Join-Path $targetRaw "config"
$targetRawLib = Join-Path $targetRaw "lib"
$targetShare = Join-Path $targetRaw "share"
New-Item -ItemType Directory -Force -Path $targetBin, $targetConfig, $targetRawLib, $targetShare | Out-Null

$targetNativeBin = Join-Path $targetNativeRuntime "bin"
$targetNativeConfig = Join-Path $targetNativeRuntime "config"
$targetNativeShare = Join-Path $targetNativeRuntime "share"
New-Item -ItemType Directory -Force -Path $targetNativeBin, $targetNativeConfig, $targetNativeShare | Out-Null

function Copy-RawLibraryAliases([string]$SourceDir, [string]$RealName, [string[]]$Aliases) {
  $sourcePath = Join-Path $SourceDir $RealName
  if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw "Missing xrdp raw runtime library: $sourcePath"
  }

  $names = @($RealName) + $Aliases
  foreach ($name in ($names | Select-Object -Unique)) {
    Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $targetRawLib $name) -Force
  }
}

$openH264RealName = Get-OpenH264RealName $depsLibSource

Copy-LibraryAliases $xrdpLibSource "libcommon.so.0.0.0" @("libcommon.so.0", "libcommon.so")
Copy-LibraryAliases $xrdpLibSource "libipm.so.0.0.0" @("libipm.so.0", "libipm.so")
Copy-LibraryAliases $xrdpLibSource "libtoml.so.1.0.0" @("libtoml.so.1", "libtoml.so")
Copy-LibraryAliases $xrdpLibSource "libxrdp.so.0.0.0" @("libxrdp.so.0", "libxrdp.so")
Copy-LibraryAliases $xrdpLibSource "libxrdpohos.so" @()
Copy-Item -LiteralPath $embeddedServer -Destination (Join-Path $targetLib "libxrdpserver.so") -Force

Copy-LibraryAliases $depsLibSource "libssl.so.3" @("libssl.so")
Copy-LibraryAliases $depsLibSource "libcrypto.so.3" @("libcrypto.so")
Copy-LibraryAliases $depsLibSource "libz.so.1.3.1" @("libz.so.1", "libz.so")
Copy-LibraryAliases $depsLibSource $openH264RealName @("libopenh264.so.7", "libopenh264.so")

Copy-RawLibraryAliases $xrdpLibSource "libcommon.so.0.0.0" @("libcommon.so.0", "libcommon.so")
Copy-RawLibraryAliases $xrdpLibSource "libipm.so.0.0.0" @("libipm.so.0", "libipm.so")
Copy-RawLibraryAliases $xrdpLibSource "libtoml.so.1.0.0" @("libtoml.so.1", "libtoml.so")
Copy-RawLibraryAliases $xrdpLibSource "libxrdp.so.0.0.0" @("libxrdp.so.0", "libxrdp.so")
Copy-RawLibraryAliases $xrdpLibSource "libxrdpohos.so" @()
Copy-Item -LiteralPath $embeddedServer -Destination (Join-Path $targetRawLib "libxrdpserver.so") -Force
Copy-RawLibraryAliases $depsLibSource "libssl.so.3" @("libssl.so")
Copy-RawLibraryAliases $depsLibSource "libcrypto.so.3" @("libcrypto.so")
Copy-RawLibraryAliases $depsLibSource "libz.so.1.3.1" @("libz.so.1", "libz.so")
Copy-RawLibraryAliases $depsLibSource $openH264RealName @("libopenh264.so.7", "libopenh264.so")

Copy-Item -LiteralPath $xrdpExecutable -Destination (Join-Path $targetBin "xrdp") -Force
Copy-Item -LiteralPath $xrdpExecutable -Destination (Join-Path $targetNativeBin "xrdp") -Force
Get-ChildItem -LiteralPath $configSource -File | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $targetConfig -Force
  Copy-Item -LiteralPath $_.FullName -Destination $targetNativeConfig -Force
}
Copy-XrdpRuntimeConfigExtras -RepoRoot $repoRoot -DestinationDir $targetConfig
Copy-XrdpRuntimeConfigExtras -RepoRoot $repoRoot -DestinationDir $targetNativeConfig
Get-ChildItem -LiteralPath $shareSource -File | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $targetShare -Force
  Copy-Item -LiteralPath $_.FullName -Destination $targetNativeShare -Force
}

$requiredLibs = @(
  "libcommon.so.0",
  "libipm.so.0",
  "libtoml.so.1",
  "libxrdp.so.0",
  "libxrdpohos.so",
  "libxrdpserver.so",
  "libssl.so.3",
  "libcrypto.so.3",
  "libopenh264.so.7"
)

foreach ($name in $requiredLibs) {
  $path = Join-Path $targetLib $name
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing synced xrdp runtime library: $path"
  }
}

$requiredRawFiles = @(
  (Join-Path $targetBin "xrdp"),
  (Join-Path $targetRawLib "libxrdpserver.so"),
  (Join-Path $targetRawLib "libxrdpohos.so"),
  (Join-Path $targetRawLib "libxrdp.so.0"),
  (Join-Path $targetRawLib "libcommon.so.0"),
  (Join-Path $targetRawLib "libssl.so.3"),
  (Join-Path $targetRawLib "libopenh264.so.7"),
  (Join-Path $targetConfig "xrdp.ini"),
  (Join-Path $targetConfig "rsakeys.ini"),
  (Join-Path $targetConfig "km-00000409.toml"),
  (Join-Path $targetConfig "km-00000804.toml"),
  (Join-Path $targetConfig "xrdp_keyboard.toml"),
  (Join-Path $targetShare "sans-10.fv1")
)

$requiredNativeRuntimeFiles = @(
  (Join-Path $targetNativeBin "xrdp"),
  (Join-Path $targetNativeConfig "xrdp.ini"),
  (Join-Path $targetNativeConfig "rsakeys.ini"),
  (Join-Path $targetNativeConfig "km-00000409.toml"),
  (Join-Path $targetNativeConfig "km-00000804.toml"),
  (Join-Path $targetNativeConfig "xrdp_keyboard.toml"),
  (Join-Path $targetNativeShare "sans-10.fv1")
)

foreach ($path in $requiredRawFiles) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing synced xrdp rawfile: $path"
  }
}

foreach ($path in $requiredNativeRuntimeFiles) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing synced xrdp native runtime file: $path"
  }
}

Get-ChildItem -File $targetLib | Where-Object { $_.Name -like "libxrdp*" -or $_.Name -like "libcommon*" -or $_.Name -like "libipm*" -or $_.Name -like "libtoml*" } |
  Select-Object FullName, Length
Get-ChildItem -Recurse -File $targetNativeRuntime | Select-Object FullName, Length
Get-ChildItem -Recurse -File $targetRaw | Select-Object FullName, Length
