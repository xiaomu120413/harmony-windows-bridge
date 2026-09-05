param(
  [string]$ProjectRoot = "harmony/app",
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
  [string]$KeyPassword = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
. (Join-Path $PSScriptRoot "signing-passwords.ps1")
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
$packageInputDir = Join-Path $outputs "app-package-inputs"
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

$passwords = Resolve-HvigorSigningPasswords -RepoRoot $repoRoot -SigningRoot $SigningRoot `
  -SigningPassword $SigningPassword -StorePassword $StorePassword -KeyPassword $KeyPassword

# AppGallery resolves package files from pack.info package names. DevEco stages
# signed module outputs without the "-signed" suffix before building the App Pack.
# Keep the same canonical names when rebuilding the App after HNP injection.
if (Test-Path -LiteralPath $packageInputDir) {
  Remove-Item -LiteralPath $packageInputDir -Recurse -Force
}
New-Item -ItemType Directory -Path $packageInputDir | Out-Null
$stagedEntryHap = Join-Path $packageInputDir "entry-default.hap"
$stagedTabletHap = Join-Path $packageInputDir "entry_tablet-default.hap"
$stagedCommonHsp = Join-Path $packageInputDir "common-default.hsp"
Copy-Item -LiteralPath $entryHap -Destination $stagedEntryHap
Copy-Item -LiteralPath $tabletHap -Destination $stagedTabletHap
Copy-Item -LiteralPath $commonHsp -Destination $stagedCommonHsp

& $JavaPath '-Dfile.encoding=UTF-8' -jar $AppPackingToolJar `
  --mode app `
  --pack-info-path $packInfo `
  --hap-path "$stagedEntryHap,$stagedTabletHap" `
  --hsp-path $stagedCommonHsp `
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
  -keyPwd $passwords.KeyPassword `
  -appCertFile $appCert `
  -profileFile $profile `
  -inFile $unsignedApp `
  -signAlg SHA256withECDSA `
  -keystoreFile $keystore `
  -keystorePwd $passwords.StorePassword `
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
    "common" = @{ File = "common-default.hsp"; Type = "shared"; Devices = @("2in1", "tablet"); Hnp = 0; RdpClient = 1; XrdpControl = 0 }
    "entry" = @{ File = "entry-default.hap"; Type = "entry"; Devices = @("2in1"); Hnp = 1; RdpClient = 0; XrdpControl = 1 }
    "entry_tablet" = @{ File = "entry_tablet-default.hap"; Type = "entry"; Devices = @("tablet"); Hnp = 0; RdpClient = 0; XrdpControl = 0 }
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
      if ($entry.FullName -ne $rule.File) {
        throw "Package filename does not match pack.info name: module=$name expected=$($rule.File) actual=$($entry.FullName)"
      }
      $devices = @($manifest.module.deviceTypes)
      $hnpCount = @($packageZip.Entries | Where-Object { $_.FullName -like "hnp/*" }).Count
      $rdpClientCount = @($packageZip.Entries | Where-Object {
        $_.FullName -eq "libs/arm64-v8a/librdpclient.so"
      }).Count
      $xrdpControlCount = @($packageZip.Entries | Where-Object {
        $_.FullName -eq "libs/arm64-v8a/libxrdpcontrol.so"
      }).Count
      if ($manifest.module.type -ne $rule.Type -or
          (Compare-Object $devices $rule.Devices) -or
          $hnpCount -ne $rule.Hnp -or
          $rdpClientCount -ne $rule.RdpClient -or
          $xrdpControlCount -ne $rule.XrdpControl) {
        throw "Module boundary check failed: $name type=$($manifest.module.type) devices=$($devices -join ',') hnp=$hnpCount rdpclient=$rdpClientCount xrdpcontrol=$xrdpControlCount"
      }
      if ($name -eq "entry_tablet") {
        $tabletPermissions = @($manifest.module.requestPermissions | ForEach-Object { $_.name })
        $allowed = @(
          "ohos.permission.INTERNET",
          "ohos.permission.GET_NETWORK_INFO",
          "ohos.permission.PRINT",
          "ohos.permission.READ_PASTEBOARD",
          "ohos.permission.MICROPHONE",
          "ohos.permission.CAMERA",
          "ohos.permission.APPROXIMATELY_LOCATION",
          "ohos.permission.LOCATION"
        )
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
