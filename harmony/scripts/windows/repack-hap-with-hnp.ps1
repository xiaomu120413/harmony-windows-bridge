param(
  [string]$ModuleRoot = "harmony/app/entry",
  [string]$HnpSourceRoot = "harmony/app/entry/hnp",
  [string]$HnpPackage = "arm64-v8a/xrdp.hnp",
  [string]$CompatibleVersion = "22",
  [string]$JavaPath = "C:\Program Files\Huawei\DevEco Studio\jbr\bin\java.exe",
  [string]$AppPackingToolJar = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\lib\app_packing_tool.jar",
  [string]$HapSignToolJar = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\lib\hap-sign-tool.jar",
  [string]$SigningPassword = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$repoResolved = (Resolve-Path -LiteralPath $repoRoot).Path
$moduleRootPath = Resolve-Path (Join-Path $repoRoot $ModuleRoot)
$hnpSourceRootPath = Resolve-Path (Join-Path $repoRoot $HnpSourceRoot)
$outputs = Join-Path $moduleRootPath "build/default/outputs/default"
$intermediates = Join-Path $moduleRootPath "build/default/intermediates"
$nativeOut = Join-Path $outputs "native"
$nativeRuntimeSource = Join-Path $moduleRootPath "libs/arm64-v8a/xrdp"
$nativeRuntimeTarget = Join-Path $intermediates "libs/default/arm64-v8a/xrdp"

$unsignedHnp = Join-Path $outputs "entry-default-unsigned-hnp.hap"
$signedHnp = Join-Path $outputs "entry-default-signed-hnp.hap"
$unsignedDefault = Join-Path $outputs "entry-default-unsigned.hap"
$signedDefault = Join-Path $outputs "entry-default-signed.hap"

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

if (-not (Test-Path -LiteralPath $JavaPath)) {
  throw "Java was not found: $JavaPath"
}
if (-not (Test-Path -LiteralPath $AppPackingToolJar)) {
  throw "app_packing_tool.jar was not found: $AppPackingToolJar"
}
if (-not (Test-Path -LiteralPath $HapSignToolJar)) {
  throw "hap-sign-tool.jar was not found: $HapSignToolJar"
}

$hnpSource = Join-Path $hnpSourceRootPath $HnpPackage
if (-not (Test-Path -LiteralPath $hnpSource)) {
  throw "HNP package was not found: $hnpSource"
}

$requiredInputs = @(
  (Join-Path $intermediates "package/default/module.json"),
  (Join-Path $intermediates "res/default/resources"),
  (Join-Path $intermediates "loader_out/default/ets"),
  (Join-Path $intermediates "libs/default"),
  (Join-Path $intermediates "res/default/resources.index"),
  (Join-Path $intermediates "loader/default/pkgContextInfo.json"),
  (Join-Path $outputs "pack.info")
)

foreach ($path in $requiredInputs) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing HAP packaging input: $path"
  }
}

Reset-Directory $nativeOut
Get-ChildItem -LiteralPath $hnpSourceRootPath | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $nativeOut -Recurse -Force
}
if (Test-Path -LiteralPath $nativeRuntimeSource) {
  Assert-PathInsideRepo $nativeRuntimeTarget
  if (Test-Path -LiteralPath $nativeRuntimeTarget) {
    Remove-Item -LiteralPath $nativeRuntimeTarget -Recurse -Force
  }
  Copy-Item -LiteralPath $nativeRuntimeSource -Destination $nativeRuntimeTarget -Recurse -Force
}

& $JavaPath -jar $AppPackingToolJar `
  --mode hap `
  --json-path (Join-Path $intermediates "package/default/module.json") `
  --resources-path (Join-Path $intermediates "res/default/resources") `
  --ets-path (Join-Path $intermediates "loader_out/default/ets") `
  --out-path $unsignedHnp `
  --hnp-path $nativeOut `
  --lib-path (Join-Path $intermediates "libs/default") `
  --index-path (Join-Path $intermediates "res/default/resources.index") `
  --pack-info-path (Join-Path $outputs "pack.info") `
  --pkg-context-path (Join-Path $intermediates "loader/default/pkgContextInfo.json") `
  --force true
if ($LASTEXITCODE -ne 0) {
  throw "app_packing_tool failed with exit code $LASTEXITCODE"
}

$repoTools = Join-Path $repoRoot "tools/hapsigner"
$appCertFile = Join-Path $repoTools "OpenHarmonyApplication.pem"
$profileFile = Join-Path $repoTools "ohos_provision_debug.p7b"
$keystoreFile = Join-Path $repoTools "OpenHarmony.p12"

foreach ($path in @($appCertFile, $profileFile, $keystoreFile)) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing signing input: $path"
  }
}

if ([string]::IsNullOrWhiteSpace($SigningPassword)) {
  if (-not [string]::IsNullOrWhiteSpace($env:HAP_SIGN_PASSWORD)) {
    $SigningPassword = $env:HAP_SIGN_PASSWORD
  } else {
    $SigningPassword = "123456"
  }
}

& $JavaPath -jar $HapSignToolJar sign-app `
  -mode localSign `
  -keyAlias "openharmony application release" `
  -keyPwd $SigningPassword `
  -appCertFile $appCertFile `
  -profileFile $profileFile `
  -inFile $unsignedHnp `
  -signAlg SHA256withECDSA `
  -keystoreFile $keystoreFile `
  -keystorePwd $SigningPassword `
  -outFile $signedHnp `
  -compatibleVersion $CompatibleVersion `
  -signCode 1
if ($LASTEXITCODE -ne 0) {
  throw "hap-sign-tool failed with exit code $LASTEXITCODE"
}

$hnpEntry = "hnp/$HnpPackage"
$tarList = & tar -tf $signedHnp
if ($LASTEXITCODE -ne 0) {
  throw "Unable to inspect signed HAP: $signedHnp"
}
if (-not ($tarList | Where-Object { $_ -eq $hnpEntry })) {
  throw "Signed HAP does not contain expected HNP entry: $hnpEntry"
}
if (-not ($tarList | Where-Object { $_ -eq "libs/arm64-v8a/libentry.so" })) {
  throw "Signed HAP native library layout is invalid"
}
if (Test-Path -LiteralPath $nativeRuntimeSource) {
  if (-not ($tarList | Where-Object { $_ -eq "libs/arm64-v8a/xrdp/config/xrdp.ini" })) {
    throw "Signed HAP does not contain xrdp native runtime config"
  }
}

Copy-Item -LiteralPath $unsignedHnp -Destination $unsignedDefault -Force
Copy-Item -LiteralPath $signedHnp -Destination $signedDefault -Force

Get-Item -LiteralPath $signedDefault | Select-Object FullName, Length
