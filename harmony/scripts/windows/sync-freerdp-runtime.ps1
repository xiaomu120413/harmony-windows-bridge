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
Get-ChildItem -LiteralPath $runtimeSource -Force |
  Where-Object { $_.Name -notin @("libohaudio.so", "libOpenSLES.so", "libhilog_ndk.z.so") } |
  ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $target -Recurse -Force
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

Get-ChildItem -Recurse -File $target | Select-Object FullName, Length
