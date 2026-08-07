param(
  [string]$SourceRoot = "harmony/out/ohos-arm64",
  [string]$TargetRoot = "harmony/app/common/libs/arm64-v8a",
  [switch]$Force
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
. (Join-Path $PSScriptRoot "build-cache.ps1")
$source = Resolve-Path (Join-Path $repoRoot $SourceRoot)
$target = Join-Path $repoRoot $TargetRoot
$cacheDir = Join-Path $repoRoot "harmony/out/.build-cache"
$stampFile = Join-Path $cacheDir "sync-freerdp-runtime.sha256"

$runtimeSource = Join-Path $source "runtime-libs"

if (-not (Test-Path -LiteralPath $runtimeSource)) {
  throw "Missing runtime library directory: $runtimeSource"
}

$runtimeLibraryNames = @(
  "libc++_shared.so",
  "libcjson.so.1",
  "libcrypto.so.3",
  "libfreerdp-client3.so",
  "libfreerdp3.so",
  "libssl.so.3",
  "libwinpr3.so",
  "libz.so.1",
  "liburiparser.so.1",
  "libopenh264.so.7",
  "libavcodec.so.60",
  "libavdevice.so.60",
  "libavfilter.so.9",
  "libavformat.so.60",
  "libavutil.so.58",
  "libswresample.so.4",
  "libswscale.so.7"
)

$sourceInputs = New-Object System.Collections.Generic.List[string]
foreach ($name in $runtimeLibraryNames) {
  $sourcePath = Join-Path $runtimeSource $name
  if (Test-Path -LiteralPath $sourcePath) {
    $sourceInputs.Add($sourcePath)
  }
}

$osslModuleSource = Join-Path $runtimeSource "ossl-modules/legacy.so"
if (Test-Path -LiteralPath $osslModuleSource) {
  $sourceInputs.Add($osslModuleSource)
}

$requiredNames = @(
  "libcjson.so.1",
  "libcrypto.so.3",
  "libfreerdp-client3.so",
  "libfreerdp3.so",
  "libssl.so.3",
  "libwinpr3.so",
  "libz.so.1"
)

$requiredOutputs = New-Object System.Collections.Generic.List[string]
foreach ($name in $requiredNames) {
  $requiredOutputs.Add((Join-Path $target $name))
}
if (Test-Path -LiteralPath $osslModuleSource) {
  $requiredOutputs.Add((Join-Path $target "ossl-modules/legacy.so"))
}

# XRDP server runtime belongs exclusively to the 2in1 HNP. Remove legacy copies
# before evaluating the FreeRDP cache so they cannot leak into common.hsp.
$obsoleteServerRuntime = @(
  "libcommon.so.0",
  "libipm.so.0",
  "libtoml.so.1",
  "libxrdp.so.0",
  "libxrdpohos.so",
  "libxrdpserver.so",
  "libfreerdp_ohos_probe.so"
)
foreach ($name in $obsoleteServerRuntime) {
  $obsoletePath = Join-Path $target $name
  if (Test-Path -LiteralPath $obsoletePath) {
    Remove-Item -LiteralPath $obsoletePath -Force
  }
}
$obsoleteRuntimeTree = Join-Path $target "xrdp"
if (Test-Path -LiteralPath $obsoleteRuntimeTree) {
  Remove-Item -LiteralPath $obsoleteRuntimeTree -Recurse -Force
}

$fingerprint = Get-BuildCacheFingerprint `
  -Root $repoRoot `
  -Paths $sourceInputs.ToArray() `
  -Extra @("sync-freerdp-runtime:v3-production-client-only", "target=$TargetRoot")

if (-not $Force -and (Test-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint -Outputs $requiredOutputs.ToArray())) {
  $stats = Get-BuildCacheFileStats -Path $target
  Write-Host ("synced FreeRDP runtime: cached files={0} bytes={1}" -f $stats.Count, $stats.Bytes)
  return
}

New-Item -ItemType Directory -Force -Path $target | Out-Null

$targetResolved = (Resolve-Path -LiteralPath $target).Path
$repoResolved = (Resolve-Path -LiteralPath $repoRoot).Path
if (-not $targetResolved.StartsWith($repoResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing to write target outside repository: $targetResolved"
}

foreach ($name in $runtimeLibraryNames) {
  $sourcePath = Join-Path $runtimeSource $name
  $targetPath = Join-Path $target $name
  if (Test-Path -LiteralPath $sourcePath) {
    Copy-Item -LiteralPath $sourcePath -Destination $targetPath -Force
  } elseif (Test-Path -LiteralPath $targetPath) {
    Remove-Item -LiteralPath $targetPath -Force
  }
}

if (Test-Path -LiteralPath $osslModuleSource) {
  $osslModuleTarget = Join-Path $target "ossl-modules"
  New-Item -ItemType Directory -Force -Path $osslModuleTarget | Out-Null
  Copy-Item -LiteralPath $osslModuleSource -Destination (Join-Path $osslModuleTarget "legacy.so") -Force
} else {
  $osslModuleTarget = Join-Path $target "ossl-modules/legacy.so"
  if (Test-Path -LiteralPath $osslModuleTarget) {
    Remove-Item -LiteralPath $osslModuleTarget -Force
  }
}
foreach ($name in $requiredNames) {
  $path = Join-Path $target $name
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing synced runtime library: $path"
  }
}

$syncedFiles = Get-ChildItem -Recurse -File $target
$syncedBytes = ($syncedFiles | Measure-Object -Property Length -Sum).Sum
if ($null -eq $syncedBytes) {
  $syncedBytes = 0
}
Write-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint
Write-Host ("synced FreeRDP runtime: files={0} bytes={1}" -f $syncedFiles.Count, [int64]$syncedBytes)
