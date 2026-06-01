param(
  [string]$SourceRoot = "harmony/out/xrdp-ohos-arm64",
  [string]$DepsRoot = "harmony/out/ohos-arm64",
  [string]$TargetHnpRoot = "harmony/app/entry/hnp",
  [string]$HnpName = "xrdp",
  [string]$HnpVersion = "0.1.0",
  [string]$HnpCliPath = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\hnpcli.exe",
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
$targetHnpRoot = Join-Path $repoRoot $TargetHnpRoot
$targetAbiDir = Join-Path $targetHnpRoot "arm64-v8a"
$stageRoot = Join-Path $source "hnp-stage"
$stage = Join-Path $stageRoot $HnpName
$cacheDir = Join-Path $repoRoot "harmony/out/.build-cache"
$stampFile = Join-Path $cacheDir "package-xrdp-hnp.sha256"
$targetHnp = Join-Path $targetAbiDir "$HnpName.hnp"

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

function Reset-Directory([string]$Path) {
  Assert-PathInsideRepo $Path
  if (Test-Path -LiteralPath $Path) {
    Remove-Item -LiteralPath $Path -Recurse -Force
  }
  New-Item -ItemType Directory -Force -Path $Path | Out-Null
}

function Copy-LibraryAs([string]$SourceDir, [string]$RealName, [string]$TargetName, [string]$DestinationDir) {
  $sourcePath = Join-Path $SourceDir $RealName
  if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw "Missing xrdp runtime library: $sourcePath"
  }

  Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $DestinationDir $TargetName) -Force
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

if (-not (Test-Path -LiteralPath $HnpCliPath)) {
  throw "hnpcli was not found: $HnpCliPath"
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

Assert-PathInsideRepo $stageRoot
Assert-PathInsideRepo $targetHnpRoot

$openH264RealName = Get-OpenH264RealName $depsLibSource
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
  -Extra @("package-xrdp-hnp:v1", "name=$HnpName", "version=$HnpVersion", "target=$TargetHnpRoot", "strip=$StripToolPath")

if (-not $Force -and (Test-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint -Outputs @($targetHnp))) {
  $targetHnpItem = Get-Item -LiteralPath $targetHnp
  Write-Host ("packaged HNP: cached {0} bytes={1}" -f $targetHnpItem.FullName, $targetHnpItem.Length)
  return
}

Reset-Directory $stage
New-Item -ItemType Directory -Force -Path $targetAbiDir | Out-Null

$stageBin = Join-Path $stage "bin"
$stageConfig = Join-Path $stage "config"
$stageLib = Join-Path $stage "lib"
$stageShare = Join-Path $stage "share"
New-Item -ItemType Directory -Force -Path $stageBin, $stageConfig, $stageLib, $stageShare | Out-Null

Copy-Item -LiteralPath $xrdpExecutable -Destination (Join-Path $stageBin "xrdp") -Force
Copy-Item -LiteralPath $embeddedServer -Destination (Join-Path $stageLib "libxrdpserver.so") -Force

Copy-LibraryAs $xrdpLibSource "libcommon.so.0.0.0" "libcommon.so.0" $stageLib
Copy-LibraryAs $xrdpLibSource "libipm.so.0.0.0" "libipm.so.0" $stageLib
Copy-LibraryAs $xrdpLibSource "libtoml.so.1.0.0" "libtoml.so.1" $stageLib
Copy-LibraryAs $xrdpLibSource "libxrdp.so.0.0.0" "libxrdp.so.0" $stageLib
Copy-LibraryAs $xrdpLibSource "libxrdpohos.so" "libxrdpohos.so" $stageLib

Copy-LibraryAs $depsLibSource "libssl.so.3" "libssl.so.3" $stageLib
Copy-LibraryAs $depsLibSource "libcrypto.so.3" "libcrypto.so.3" $stageLib
Copy-LibraryAs $depsLibSource "libz.so.1.3.1" "libz.so.1" $stageLib
Copy-LibraryAs $depsLibSource $openH264RealName "libopenh264.so.7" $stageLib

Get-ChildItem -LiteralPath $configSource -File | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $stageConfig -Force
}
Copy-XrdpRuntimeConfigExtras -RepoRoot $repoRoot -DestinationDir $stageConfig
Get-ChildItem -LiteralPath $shareSource -File | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $stageShare -Force
}

$hnpJson = @"
{
  "type": "hnp-config",
  "name": "$HnpName",
  "version": "$HnpVersion",
  "install": {
    "links": [
      {
        "source": "/bin/xrdp",
        "target": "xrdp"
      }
    ]
  }
}
"@
Set-Content -LiteralPath (Join-Path $stage "hnp.json") -Value $hnpJson -Encoding ASCII

$requiredHnpStageFiles = @(
  (Join-Path $stageBin "xrdp"),
  (Join-Path $stageLib "libxrdpserver.so"),
  (Join-Path $stageLib "libxrdpohos.so"),
  (Join-Path $stageLib "libxrdp.so.0"),
  (Join-Path $stageLib "libcommon.so.0"),
  (Join-Path $stageLib "libssl.so.3"),
  (Join-Path $stageLib "libcrypto.so.3"),
  (Join-Path $stageLib "libz.so.1"),
  (Join-Path $stageLib "libopenh264.so.7"),
  (Join-Path $stageConfig "xrdp.ini"),
  (Join-Path $stageConfig "rsakeys.ini"),
  (Join-Path $stageConfig "km-00000409.toml"),
  (Join-Path $stageConfig "km-00000804.toml"),
  (Join-Path $stageConfig "xrdp_keyboard.toml"),
  (Join-Path $stageShare "sans-10.fv1")
)

foreach ($path in $requiredHnpStageFiles) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing HNP staged file: $path"
  }
}

Strip-FileIfPossible (Join-Path $stageBin "xrdp")
Get-ChildItem -LiteralPath $stageLib -File | ForEach-Object {
  Strip-FileIfPossible $_.FullName
}

if (Test-Path -LiteralPath $targetHnp) {
  Remove-Item -LiteralPath $targetHnp -Force
}

$hnpOutput = & $HnpCliPath pack -i $stage -o $targetAbiDir 2>&1
if ($LASTEXITCODE -ne 0) {
  $hnpOutput | ForEach-Object { Write-Error $_ }
  throw "hnpcli pack failed with exit code $LASTEXITCODE"
}
if (-not (Test-Path -LiteralPath $targetHnp)) {
  throw "HNP output was not created: $targetHnp"
}

$targetHnpItem = Get-Item -LiteralPath $targetHnp
Write-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint
Write-Host ("packaged HNP: {0} bytes={1}" -f $targetHnpItem.FullName, $targetHnpItem.Length)
