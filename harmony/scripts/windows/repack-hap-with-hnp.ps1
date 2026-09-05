param(
  [string]$ModuleRoot = "harmony/app/entry",
  [string]$HnpSourceRoot = "harmony/app/entry/hnp",
  [string]$HnpPackage = "arm64-v8a/xrdp.hnp",
  [string]$CompatibleVersion = "22",
  [string]$JavaPath = "C:\Program Files\Huawei\DevEco Studio\jbr\bin\java.exe",
  [string]$AppPackingToolJar = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\lib\app_packing_tool.jar",
  [string]$HapSignToolJar = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\lib\hap-sign-tool.jar",
  [string]$SigningRoot = "tools/app",
  [string]$KeyAlias = "muhub",
  [string]$AppCertFileName = "muhub_debug.cer",
  [string]$ProfileFileName = "muhub_debugDebug.p7b",
  [string]$KeystoreFileName = "muhub.p12",
  [string]$SigningPassword = "",
  [string]$StorePassword = "",
  [string]$KeyPassword = "",
  [switch]$Force
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$repoResolved = (Resolve-Path -LiteralPath $repoRoot).Path
. (Join-Path $PSScriptRoot "build-cache.ps1")
. (Join-Path $PSScriptRoot "signing-passwords.ps1")
$moduleRootPath = Resolve-Path (Join-Path $repoRoot $ModuleRoot)
$hnpSourceRootPath = Resolve-Path (Join-Path $repoRoot $HnpSourceRoot)
$outputs = Join-Path $moduleRootPath "build/default/outputs/default"
$intermediates = Join-Path $moduleRootPath "build/default/intermediates"
$nativeOut = Join-Path $outputs "native"
$defaultLibPath = Join-Path $intermediates "libs/default"
$strippedLibPath = Join-Path $intermediates "stripped_native_libs/default"
$libPath = if (Test-Path -LiteralPath $strippedLibPath) {
  $strippedLibPath
} elseif (Test-Path -LiteralPath $defaultLibPath) {
  $defaultLibPath
} else {
  $null
}

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
  (Join-Path $intermediates "res/default/resources.index"),
  (Join-Path $intermediates "loader/default/pkgContextInfo.json"),
  (Join-Path $outputs "pack.info")
)

foreach ($path in $requiredInputs) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing HAP packaging input: $path"
  }
}

$repoTools = Join-Path $repoRoot $SigningRoot
$appCertFile = Join-Path $repoTools $AppCertFileName
$profileFile = Join-Path $repoTools $ProfileFileName
$keystoreFile = Join-Path $repoTools $KeystoreFileName

foreach ($path in @($appCertFile, $profileFile, $keystoreFile)) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing signing input: $path"
  }
}

$fingerprintInputs = New-Object System.Collections.Generic.List[string]
$fingerprintInputs.Add($PSCommandPath)
$fingerprintInputs.Add((Join-Path $PSScriptRoot "build-cache.ps1"))
$fingerprintInputs.Add($hnpSource)
foreach ($path in $requiredInputs) {
  if ($path -ne $libPath) {
    $fingerprintInputs.Add($path)
  }
}
if ($null -ne $libPath) {
  $fingerprintInputs.Add($libPath)
}
foreach ($path in @($appCertFile, $profileFile, $keystoreFile)) {
  $fingerprintInputs.Add($path)
}

$stampFile = Join-Path $outputs ".entry-default-signed-hnp.input.sha256"
$fingerprint = Get-BuildCacheFingerprint `
  -Root $repoRoot `
  -Paths $fingerprintInputs.ToArray() `
  -Extra @("repack-hap-with-hnp:v3", "hnp=$HnpPackage", "compatible=$CompatibleVersion", "libPath=$libPath",
    "signingRoot=$SigningRoot", "keyAlias=$KeyAlias", "cert=$AppCertFileName", "profile=$ProfileFileName",
    "keystore=$KeystoreFileName")

if (-not $Force -and (Test-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint -Outputs @($unsignedHnp, $signedHnp))) {
  Copy-Item -LiteralPath $unsignedHnp -Destination $unsignedDefault -Force
  Copy-Item -LiteralPath $signedHnp -Destination $signedDefault -Force
  $signedDefaultItem = Get-Item -LiteralPath $signedDefault
  Write-Host ("signed HAP: cached {0} bytes={1}" -f $signedDefaultItem.FullName, $signedDefaultItem.Length)
  return
}

Reset-Directory $nativeOut
Get-ChildItem -LiteralPath $hnpSourceRootPath | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination $nativeOut -Recurse -Force
}
$packArgs = @(
  "-jar", $AppPackingToolJar,
  "--mode", "hap",
  "--json-path", (Join-Path $intermediates "package/default/module.json"),
  "--resources-path", (Join-Path $intermediates "res/default/resources"),
  "--ets-path", (Join-Path $intermediates "loader_out/default/ets"),
  "--out-path", $unsignedHnp,
  "--hnp-path", $nativeOut,
  "--index-path", (Join-Path $intermediates "res/default/resources.index"),
  "--pack-info-path", (Join-Path $outputs "pack.info"),
  "--pkg-context-path", (Join-Path $intermediates "loader/default/pkgContextInfo.json"),
  "--force", "true"
)
if ($null -ne $libPath) {
  $packArgs += @("--lib-path", $libPath)
}

& $JavaPath @packArgs
if ($LASTEXITCODE -ne 0) {
  throw "app_packing_tool failed with exit code $LASTEXITCODE"
}

$passwords = Resolve-HvigorSigningPasswords -RepoRoot $repoRoot -SigningRoot $SigningRoot `
  -SigningPassword $SigningPassword -StorePassword $StorePassword -KeyPassword $KeyPassword

$signOutput = & $JavaPath -jar $HapSignToolJar sign-app `
  -mode localSign `
  -keyAlias $KeyAlias `
  -keyPwd $passwords.KeyPassword `
  -appCertFile $appCertFile `
  -profileFile $profileFile `
  -inFile $unsignedHnp `
  -signAlg SHA256withECDSA `
  -keystoreFile $keystoreFile `
  -keystorePwd $passwords.StorePassword `
  -outFile $signedHnp `
  -compatibleVersion $CompatibleVersion `
  -signCode 1 2>&1
if ($LASTEXITCODE -ne 0) {
  $signOutput | ForEach-Object { Write-Error $_ }
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
Copy-Item -LiteralPath $unsignedHnp -Destination $unsignedDefault -Force
Copy-Item -LiteralPath $signedHnp -Destination $signedDefault -Force

$signedDefaultItem = Get-Item -LiteralPath $signedDefault
Write-BuildCacheStamp -StampFile $stampFile -Fingerprint $fingerprint
Write-Host ("signed HAP: {0} bytes={1}" -f $signedDefaultItem.FullName, $signedDefaultItem.Length)
