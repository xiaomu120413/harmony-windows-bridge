param(
  [string]$SourceRoot = "harmony/out/ohos-arm64",
  [string]$TargetRoot = "harmony/app/entry/libs/arm64-v8a"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$source = Resolve-Path (Join-Path $repoRoot $SourceRoot)
$target = Join-Path $repoRoot $TargetRoot

$required = @(
  "runtime-libs/libcjson.so.1",
  "runtime-libs/libcrypto.so.3",
  "runtime-libs/libfreerdp-client3.so",
  "runtime-libs/libfreerdp3.so",
  "runtime-libs/libssl.so.3",
  "runtime-libs/libwinpr3.so",
  "runtime-libs/libz.so.1",
  "probe/libfreerdp_ohos_probe.so"
)

New-Item -ItemType Directory -Force -Path $target | Out-Null

foreach ($relative in $required) {
  $from = Join-Path $source $relative
  if (-not (Test-Path -LiteralPath $from)) {
    throw "Missing runtime library: $from"
  }
  Copy-Item -LiteralPath $from -Destination $target -Force
}

$osslSource = Join-Path $source "runtime-libs/ossl-modules"
if (Test-Path -LiteralPath $osslSource) {
  $osslTarget = Join-Path $target "ossl-modules"
  New-Item -ItemType Directory -Force -Path $osslTarget | Out-Null
  Copy-Item -LiteralPath (Join-Path $osslSource "legacy.so") -Destination $osslTarget -Force
}

Get-ChildItem -Recurse -File $target | Select-Object FullName, Length
