param(
  [string]$SourceRoot = "harmony/out/xrdp-ohos-arm64",
  [string]$DepsRoot = "harmony/out/ohos-arm64",
  [string]$TargetHnpRoot = "harmony/app/entry/hnp",
  [string]$HnpName = "xrdp",
  [string]$HnpVersion = "0.1.0",
  [string]$HnpCliPath = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\hnpcli.exe"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$repoResolved = (Resolve-Path -LiteralPath $repoRoot).Path
. (Join-Path $PSScriptRoot "xrdp-runtime-material.ps1")
$source = Resolve-Path (Join-Path $repoRoot $SourceRoot)
$deps = Resolve-Path (Join-Path $repoRoot $DepsRoot)
$targetHnpRoot = Join-Path $repoRoot $TargetHnpRoot
$targetAbiDir = Join-Path $targetHnpRoot "arm64-v8a"
$stageRoot = Join-Path $source "hnp-stage"
$stage = Join-Path $stageRoot $HnpName

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

function Copy-LibraryAliases([string]$SourceDir, [string]$RealName, [string[]]$Aliases, [string]$DestinationDir) {
  $sourcePath = Join-Path $SourceDir $RealName
  if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw "Missing xrdp runtime library: $sourcePath"
  }

  $names = @($RealName) + $Aliases
  foreach ($name in ($names | Select-Object -Unique)) {
    Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $DestinationDir $name) -Force
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

Reset-Directory $stage
New-Item -ItemType Directory -Force -Path $targetAbiDir | Out-Null

$stageBin = Join-Path $stage "bin"
$stageConfig = Join-Path $stage "config"
$stageLib = Join-Path $stage "lib"
$stageShare = Join-Path $stage "share"
New-Item -ItemType Directory -Force -Path $stageBin, $stageConfig, $stageLib, $stageShare | Out-Null

Copy-Item -LiteralPath $xrdpExecutable -Destination (Join-Path $stageBin "xrdp") -Force
Copy-Item -LiteralPath $embeddedServer -Destination (Join-Path $stageLib "libxrdpserver.so") -Force

Copy-LibraryAliases $xrdpLibSource "libcommon.so.0.0.0" @("libcommon.so.0", "libcommon.so") $stageLib
Copy-LibraryAliases $xrdpLibSource "libipm.so.0.0.0" @("libipm.so.0", "libipm.so") $stageLib
Copy-LibraryAliases $xrdpLibSource "libtoml.so.1.0.0" @("libtoml.so.1", "libtoml.so") $stageLib
Copy-LibraryAliases $xrdpLibSource "libxrdp.so.0.0.0" @("libxrdp.so.0", "libxrdp.so") $stageLib
Copy-LibraryAliases $xrdpLibSource "libxrdpohos.so" @() $stageLib

Copy-LibraryAliases $depsLibSource "libssl.so.3" @("libssl.so") $stageLib
Copy-LibraryAliases $depsLibSource "libcrypto.so.3" @("libcrypto.so") $stageLib
Copy-LibraryAliases $depsLibSource "libz.so.1.3.1" @("libz.so.1", "libz.so") $stageLib

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

$targetHnp = Join-Path $targetAbiDir "$HnpName.hnp"
if (Test-Path -LiteralPath $targetHnp) {
  Remove-Item -LiteralPath $targetHnp -Force
}

& $HnpCliPath pack -i $stage -o $targetAbiDir
if ($LASTEXITCODE -ne 0) {
  throw "hnpcli pack failed with exit code $LASTEXITCODE"
}
if (-not (Test-Path -LiteralPath $targetHnp)) {
  throw "HNP output was not created: $targetHnp"
}

$requiredHnpStageFiles = @(
  (Join-Path $stageBin "xrdp"),
  (Join-Path $stageLib "libxrdpserver.so"),
  (Join-Path $stageLib "libxrdpohos.so"),
  (Join-Path $stageLib "libxrdp.so.0"),
  (Join-Path $stageLib "libcommon.so.0"),
  (Join-Path $stageLib "libssl.so.3"),
  (Join-Path $stageConfig "xrdp.ini"),
  (Join-Path $stageConfig "rsakeys.ini"),
  (Join-Path $stageConfig "km-00000409.toml"),
  (Join-Path $stageConfig "xrdp_keyboard.toml"),
  (Join-Path $stageShare "sans-10.fv1")
)

foreach ($path in $requiredHnpStageFiles) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing HNP staged file: $path"
  }
}

Get-Item -LiteralPath $targetHnp | Select-Object FullName, Length
