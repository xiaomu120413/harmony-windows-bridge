param(
  [string]$SourceRoot = "harmony/out/ohos-arm64",
  [string]$TargetRoot = "harmony/app/entry/libs/arm64-v8a"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$source = Resolve-Path (Join-Path $repoRoot $SourceRoot)
$target = Join-Path $repoRoot $TargetRoot

$runtimeSource = Join-Path $source "runtime-libs"
$probeSource = Join-Path $source "probe/libfreerdp_ohos_probe.so"

if (-not (Test-Path -LiteralPath $runtimeSource)) {
  throw "Missing runtime library directory: $runtimeSource"
}

if (-not (Test-Path -LiteralPath $probeSource)) {
  throw "Missing probe library: $probeSource"
}

New-Item -ItemType Directory -Force -Path $target | Out-Null

$targetResolved = (Resolve-Path -LiteralPath $target).Path
$repoResolved = (Resolve-Path -LiteralPath $repoRoot).Path
if (-not $targetResolved.StartsWith($repoResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing to clean target outside repository: $targetResolved"
}

Get-ChildItem -LiteralPath $target -Force | Remove-Item -Recurse -Force

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

foreach ($name in $runtimeLibraryNames) {
  $sourcePath = Join-Path $runtimeSource $name
  if (Test-Path -LiteralPath $sourcePath) {
    Copy-Item -LiteralPath $sourcePath -Destination $target -Force
  }
}

$osslModuleSource = Join-Path $runtimeSource "ossl-modules/legacy.so"
if (Test-Path -LiteralPath $osslModuleSource) {
  $osslModuleTarget = Join-Path $target "ossl-modules"
  New-Item -ItemType Directory -Force -Path $osslModuleTarget | Out-Null
  Copy-Item -LiteralPath $osslModuleSource -Destination $osslModuleTarget -Force
}
Copy-Item -LiteralPath $probeSource -Destination $target -Force

$requiredNames = @(
  "libcjson.so.1",
  "libcrypto.so.3",
  "libfreerdp-client3.so",
  "libfreerdp3.so",
  "libssl.so.3",
  "libwinpr3.so",
  "libz.so.1",
  "libfreerdp_ohos_probe.so"
)

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
Write-Host ("synced FreeRDP runtime: files={0} bytes={1}" -f $syncedFiles.Count, [int64]$syncedBytes)
