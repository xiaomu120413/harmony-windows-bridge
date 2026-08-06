param(
  [string]$ProjectRoot = "harmony/app",
  [string]$CompatibleVersion = "22",
  [string]$JavaPath = "C:\Program Files\Huawei\DevEco Studio\jbr\bin\java.exe",
  [string]$AppPackingToolJar = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\lib\app_packing_tool.jar",
  [string]$HapSignToolJar = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\lib\hap-sign-tool.jar",
  [string]$SigningRoot = "tools/hapsigner",
  [string]$KeyAlias = "openharmony application release",
  [string]$AppCertFileName = "OpenHarmonyApplication.pem",
  [string]$ProfileFileName = "ohos_provision_debug.p7b",
  [string]$KeystoreFileName = "OpenHarmony.p12",
  [string]$SigningPassword = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$project = (Resolve-Path (Join-Path $repoRoot $ProjectRoot)).Path
$outputs = Join-Path $project "build/outputs/default"
$entryHap = Join-Path $project "entry/build/default/outputs/default/entry-default-signed.hap"
$tabletHap = Join-Path $project "entry_tablet/build/default/outputs/default/entry_tablet-default-signed.hap"
$commonHsp = Join-Path $project "common/build/default/outputs/default/common-default-signed.hsp"
$packInfo = Join-Path $outputs "pack.info"
$pacJson = Join-Path $outputs "pac.json"
$unsignedApp = Join-Path $outputs "app-default-hnp-unsigned.app"
$signedApp = Join-Path $outputs "app-default-hnp-signed.app"
$canonicalApp = Join-Path $outputs "app-default-signed.app"
$signingFiles = Join-Path $repoRoot $SigningRoot
$appCert = Join-Path $signingFiles $AppCertFileName
$profile = Join-Path $signingFiles $ProfileFileName
$keystore = Join-Path $signingFiles $KeystoreFileName

foreach ($path in @($JavaPath, $AppPackingToolJar, $HapSignToolJar, $entryHap, $tabletHap, $commonHsp,
    $packInfo, $pacJson, $appCert, $profile, $keystore)) {
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing multi-device App Pack input: $path"
  }
}

if ([string]::IsNullOrWhiteSpace($SigningPassword)) {
  if (-not [string]::IsNullOrWhiteSpace($env:HAP_SIGN_PASSWORD)) {
    $SigningPassword = $env:HAP_SIGN_PASSWORD
  } else {
    throw "Missing signing password. Set HAP_SIGN_PASSWORD or pass -SigningPassword."
  }
}

& $JavaPath '-Dfile.encoding=UTF-8' -jar $AppPackingToolJar `
  --mode app `
  --pack-info-path $packInfo `
  --hap-path "$entryHap,$tabletHap" `
  --hsp-path $commonHsp `
  --force true `
  --out-path $unsignedApp `
  --main-module-limit 2 `
  --normal-module-limit 2 `
  --pac-json-path $pacJson
if ($LASTEXITCODE -ne 0) {
  throw "app_packing_tool failed with exit code $LASTEXITCODE"
}

$signOutput = & $JavaPath -jar $HapSignToolJar sign-app `
  -mode localSign `
  -keyAlias $KeyAlias `
  -keyPwd $SigningPassword `
  -appCertFile $appCert `
  -profileFile $profile `
  -inFile $unsignedApp `
  -signAlg SHA256withECDSA `
  -keystoreFile $keystore `
  -keystorePwd $SigningPassword `
  -outFile $signedApp `
  -compatibleVersion $CompatibleVersion `
  -signCode 1 2>&1
if ($LASTEXITCODE -ne 0) {
  $signOutput | ForEach-Object { Write-Error $_ }
  throw "hap-sign-tool failed with exit code $LASTEXITCODE"
}

$verificationDir = Join-Path $outputs "verification"
New-Item -ItemType Directory -Force -Path $verificationDir | Out-Null
$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$verifyOutput = & $JavaPath -jar $HapSignToolJar verify-app `
  -inFile $signedApp `
  -outCertChain (Join-Path $verificationDir "app-default-hnp-cert-chain.cer") `
  -outProfile (Join-Path $verificationDir "app-default-hnp-profile.p7b") 2>&1
$verifyExitCode = $LASTEXITCODE
$ErrorActionPreference = $previousErrorActionPreference
if ($verifyExitCode -ne 0) {
  $verifyOutput | ForEach-Object { Write-Error $_ }
  throw "App Pack signature verification failed with exit code $verifyExitCode"
}

Add-Type -AssemblyName System.IO.Compression
$appStream = [System.IO.File]::OpenRead($signedApp)
$appZip = [System.IO.Compression.ZipArchive]::new($appStream, [System.IO.Compression.ZipArchiveMode]::Read)
try {
  $expected = @{
    "common" = @{ Type = "shared"; Devices = @("2in1", "tablet"); Hnp = 0; LibEntry = 1 }
    "entry" = @{ Type = "entry"; Devices = @("2in1"); Hnp = 1; LibEntry = 0 }
    "entry_tablet" = @{ Type = "entry"; Devices = @("tablet"); Hnp = 0; LibEntry = 0 }
  }
  $seen = @{}

  foreach ($entry in $appZip.Entries | Where-Object { $_.FullName -match '\.(hap|hsp)$' }) {
    $memory = [System.IO.MemoryStream]::new()
    $source = $entry.Open()
    try { $source.CopyTo($memory) } finally { $source.Dispose() }
    $memory.Position = 0
    $packageZip = [System.IO.Compression.ZipArchive]::new($memory, [System.IO.Compression.ZipArchiveMode]::Read)
    try {
      $reader = [System.IO.StreamReader]::new($packageZip.GetEntry("module.json").Open())
      try { $manifest = $reader.ReadToEnd() | ConvertFrom-Json } finally { $reader.Dispose() }
      $name = $manifest.module.name
      if (-not $expected.ContainsKey($name)) {
        throw "Unexpected module in App Pack: $name"
      }
      $rule = $expected[$name]
      $devices = @($manifest.module.deviceTypes)
      $hnpCount = @($packageZip.Entries | Where-Object { $_.FullName -like "hnp/*" }).Count
      $libEntryCount = @($packageZip.Entries | Where-Object {
        $_.FullName -eq "libs/arm64-v8a/libentry.so"
      }).Count
      if ($manifest.module.type -ne $rule.Type -or
          (Compare-Object $devices $rule.Devices) -or
          $hnpCount -ne $rule.Hnp -or
          $libEntryCount -ne $rule.LibEntry) {
        throw "Module boundary check failed: $name type=$($manifest.module.type) devices=$($devices -join ',') hnp=$hnpCount libentry=$libEntryCount"
      }
      if ($name -eq "entry_tablet") {
        $tabletPermissions = @($manifest.module.requestPermissions | ForEach-Object { $_.name })
        $allowed = @("ohos.permission.INTERNET", "ohos.permission.GET_NETWORK_INFO")
        if (Compare-Object $tabletPermissions $allowed) {
          throw "Tablet permission boundary check failed: $($tabletPermissions -join ',')"
        }
      }
      $seen[$name] = $true
    } finally {
      $packageZip.Dispose()
      $memory.Dispose()
    }
  }

  foreach ($name in $expected.Keys) {
    if (-not $seen.ContainsKey($name)) {
      throw "Missing module in App Pack: $name"
    }
  }
} finally {
  $appZip.Dispose()
  $appStream.Dispose()
}

Copy-Item -LiteralPath $signedApp -Destination $canonicalApp -Force
$item = Get-Item -LiteralPath $canonicalApp
Write-Host ("multi-device App Pack: {0} bytes={1}" -f $item.FullName, $item.Length)
