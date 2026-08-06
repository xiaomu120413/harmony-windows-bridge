param(
  [string]$SourceRoot = "harmony/out/xrdp-ohos-arm64",
  [string]$DepsRoot = "harmony/out/ohos-arm64",
  [string]$TargetLibRoot = "harmony/app/common/libs/arm64-v8a",
  [string]$LegacyRawRoot = "harmony/app/entry/src/main/resources/rawfile/xrdp",
  [string]$StripToolPath = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\native\llvm\bin\llvm-strip.exe",
  [switch]$Force
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$repoResolved = (Resolve-Path -LiteralPath $repoRoot).Path
. (Join-Path $PSScriptRoot "build-cache.ps1")
. (Join-Path $PSScriptRoot "xrdp-runtime-material.ps1")
$source = Resolve-Path (Join-Path $repoRoot $SourceRoot)
$deps = Resolve-Path (Join-Path $repoRoot $DepsRoot)
$targetLib = Join-Path $repoRoot $TargetLibRoot
$legacyRaw = Join-Path $repoRoot $LegacyRawRoot
$targetNativeRuntime = Join-Path $targetLib "xrdp"
$cacheDir = Join-Path $repoRoot "harmony/out/.build-cache"
$stampFile = Join-Path $cacheDir "sync-xrdp-runtime.sha256"

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

function Copy-LibraryAs([string]$SourceDir, [string]$RealName, [string]$TargetName) {
  $sourcePath = Join-Path $SourceDir $RealName
  if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw "Missing xrdp runtime library: $sourcePath"
  }

  Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $targetLib $TargetName) -Force
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

function Strip-FileIfPossible([string]$Path) {
  if (-not (Test-Path -LiteralPath $StripToolPath)) {
    return
  }
  if (-not (Test-Path -LiteralPath $Path)) {
    return
  }

  & $StripToolPath --strip-unneeded $Path
  if ($LASTEXITCODE -ne 0) {
    throw "llvm-strip failed for $Path with exit code $LASTEXITCODE"
  }
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
Assert-PathInsideRepo $legacyRaw
Assert-PathInsideRepo $targetNativeRuntime

$targetNativeBin = Join-Path $targetNativeRuntime "bin"
$targetNativeConfig = Join-Path $targetNativeRuntime "config"
$targetNativeShare = Join-Path $targetNativeRuntime "share"
$openH264RealName = Get-OpenH264RealName $depsLibSource

$requiredLibs = @(
  "libcommon.so.0",
  "libipm.so.0",
  "libtoml.so.1",
  "libxrdp.so.0",
  "libxrdpohos.so",
  "libxrdpserver.so",
  "libssl.so.3",
  "libcrypto.so.3",
  "libz.so.1",
  "libopenh264.so.7"
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

$keymapSource = Join-Path $repoRoot "harmony/third_party/xrdp/instfiles"
$keygenSource = Join-Path $repoRoot "harmony/third_party/xrdp/keygen/keygen.c"
$sourceInputs = @(
  $PSCommandPath,
  (Join-Path $PSScriptRoot "build-cache.ps1"),
  (Join-Path $PSScriptRoot "xrdp-runtime-material.ps1"),
  $xrdpExecutable,
  $embeddedServer,
  (Join-Path $xrdpLibSource "libcommon.so.0.0.0"),
  (Join-Path $xrdpLibSource "libipm.so.0.0.0"),
  (Join-Path $xrdpLibSource "libtoml.so.1.0.0"),
  (Join-Path $xrdpLibSource "libxrdp.so.0.0.0"),
  (Join-Path $xrdpLibSource "libxrdpohos.so"),
  (Join-Path $depsLibSource "libssl.so.3"),
  (Join-Path $depsLibSource "libcrypto.so.3"),
  (Join-Path $depsLibSource "libz.so.1.3.1"),
  (Join-Path $depsLibSource $openH264RealName),
  $configSource,
  $shareSource,
  $keymapSource,
  $keygenSource
)
$fingerprint = Get-BuildCacheFingerprint `
  -Root $repoRoot `
  -Paths $sourceInputs `
  -Extra @("sync-xrdp-runtime:v1", "target=$TargetLibRoot", "strip=$StripToolPath")

$requiredOutputs = @($requiredLibs | ForEach-Object { Join-Path $targetLib $_ }) + $requiredNativeRuntimeFiles
if (-not $Force -and (Test-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint -Outputs $requiredOutputs)) {
  $syncedLibs = Get-ChildItem -File $targetLib | Where-Object {
    $_.Name -like "libxrdp*" -or $_.Name -like "libcommon*" -or $_.Name -like "libipm*" -or $_.Name -like "libtoml*"
  }
  $syncedLibBytes = ($syncedLibs | Measure-Object -Property Length -Sum).Sum
  $runtimeStats = Get-BuildCacheFileStats -Path $targetNativeRuntime
  if ($null -eq $syncedLibBytes) {
    $syncedLibBytes = 0
  }
  Write-Host ("synced xrdp runtime: cached libs={0} libBytes={1} files={2} fileBytes={3}" -f `
    $syncedLibs.Count, [int64]$syncedLibBytes, $runtimeStats.Count, $runtimeStats.Bytes)
  return
}

New-Item -ItemType Directory -Force -Path $targetLib | Out-Null
if (Test-Path -LiteralPath $targetNativeRuntime) {
  Remove-Item -LiteralPath $targetNativeRuntime -Recurse -Force
}
if (Test-Path -LiteralPath $legacyRaw) {
  $legacyRawResolved = (Resolve-Path -LiteralPath $legacyRaw).Path
  if (-not $legacyRawResolved.StartsWith($repoResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean rawfile directory outside repository: $legacyRawResolved"
  }
  Remove-Item -LiteralPath $legacyRaw -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $targetNativeBin, $targetNativeConfig, $targetNativeShare | Out-Null

$managedLibNames = @(
  "libcommon.so.0.0.0", "libcommon.so.0", "libcommon.so",
  "libipm.so.0.0.0", "libipm.so.0", "libipm.so",
  "libtoml.so.1.0.0", "libtoml.so.1", "libtoml.so",
  "libxrdp.so.0.0.0", "libxrdp.so.0", "libxrdp.so",
  "libxrdpohos.so", "libxrdpserver.so",
  "libssl.so.3", "libssl.so",
  "libcrypto.so.3", "libcrypto.so",
  "libz.so.1.3.1", "libz.so.1", "libz.so",
  "libopenh264.so.2.4.1", "libopenh264.so.7", "libopenh264.so"
)
foreach ($name in $managedLibNames) {
  $path = Join-Path $targetLib $name
  if (Test-Path -LiteralPath $path) {
    Remove-Item -LiteralPath $path -Force
  }
}

Copy-LibraryAs $xrdpLibSource "libcommon.so.0.0.0" "libcommon.so.0"
Copy-LibraryAs $xrdpLibSource "libipm.so.0.0.0" "libipm.so.0"
Copy-LibraryAs $xrdpLibSource "libtoml.so.1.0.0" "libtoml.so.1"
Copy-LibraryAs $xrdpLibSource "libxrdp.so.0.0.0" "libxrdp.so.0"
Copy-LibraryAs $xrdpLibSource "libxrdpohos.so" "libxrdpohos.so"
Copy-Item -LiteralPath $embeddedServer -Destination (Join-Path $targetLib "libxrdpserver.so") -Force

Copy-LibraryAs $depsLibSource "libssl.so.3" "libssl.so.3"
Copy-LibraryAs $depsLibSource "libcrypto.so.3" "libcrypto.so.3"
Copy-LibraryAs $depsLibSource "libz.so.1.3.1" "libz.so.1"
Copy-LibraryAs $depsLibSource $openH264RealName "libopenh264.so.7"

Copy-Item -LiteralPath $xrdpExecutable -Destination (Join-Path $targetNativeBin "xrdp") -Force
Get-ChildItem -LiteralPath $configSource -File | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $targetNativeConfig -Force
}
Copy-XrdpRuntimeConfigExtras -RepoRoot $repoRoot -DestinationDir $targetNativeConfig
Get-ChildItem -LiteralPath $shareSource -File | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $targetNativeShare -Force
}

foreach ($name in $requiredLibs) {
  $path = Join-Path $targetLib $name
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing synced xrdp runtime library: $path"
  }
  if ($name -like "libxrdp*" -or $name -like "libcommon*" -or $name -like "libipm*" -or $name -like "libtoml*") {
    Strip-FileIfPossible $path
  }
}

foreach ($path in $requiredNativeRuntimeFiles) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing synced xrdp native runtime file: $path"
  }
}
Strip-FileIfPossible (Join-Path $targetNativeBin "xrdp")

$syncedLibs = Get-ChildItem -File $targetLib | Where-Object {
  $_.Name -like "libxrdp*" -or $_.Name -like "libcommon*" -or $_.Name -like "libipm*" -or $_.Name -like "libtoml*"
}
$syncedLibBytes = ($syncedLibs | Measure-Object -Property Length -Sum).Sum
$syncedRuntimeFiles = Get-ChildItem -Recurse -File $targetNativeRuntime
$syncedRuntimeBytes = ($syncedRuntimeFiles | Measure-Object -Property Length -Sum).Sum
if ($null -eq $syncedLibBytes) {
  $syncedLibBytes = 0
}
if ($null -eq $syncedRuntimeBytes) {
  $syncedRuntimeBytes = 0
}
Write-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint
Write-Host ("synced xrdp runtime: libs={0} libBytes={1} files={2} fileBytes={3}" -f `
  $syncedLibs.Count, [int64]$syncedLibBytes, $syncedRuntimeFiles.Count, [int64]$syncedRuntimeBytes)
