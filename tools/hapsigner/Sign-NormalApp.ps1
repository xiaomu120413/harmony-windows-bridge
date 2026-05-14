param(
    [Parameter(Mandatory = $true)]
    [string]$InputHap,

    [string]$OutputName = "securitytool-normal-signed.hap"
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$Java = "C:\Program Files\Huawei\DevEco Studio\jbr\bin\java.exe"
$StageDir = Join-Path $ScriptRoot "stage-normal-app"
$OutputDir = Join-Path $ScriptRoot "output"
$UnsignedHap = Join-Path $OutputDir "securitytool-normal-unsigned.hap"
$ProfileJson = Join-Path $OutputDir "UnsignedNormalProfileTemplate.json"
$ProfileP7b = Join-Path $OutputDir "securitytool-normal-profile.p7b"
$SignedHap = Join-Path $OutputDir $OutputName
$VerifyProfileJson = Join-Path $OutputDir "securitytool-normal-verify-profile.json"
$CertChain = Join-Path $OutputDir "securitytool-normal-cert-chain.cer"
$ExtractedProfile = Join-Path $OutputDir "securitytool-normal-extracted-profile.p7b"

if (-not (Test-Path $Java)) {
    throw "Java runtime not found: $Java"
}

if (-not (Test-Path $InputHap)) {
    throw "Input HAP not found: $InputHap"
}

foreach ($required in @(
    "hap-sign-tool.jar",
    "OpenHarmony.p12",
    "OpenHarmonyApplication.pem",
    "OpenHarmonyProfileDebug.pem",
    "UnsgnedDebugProfileTemplate.json"
)) {
    $path = Join-Path $ScriptRoot $required
    if (-not (Test-Path $path)) {
        throw "Required signing file not found: $path"
    }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

foreach ($path in @($StageDir)) {
    if (Test-Path $path) {
        $resolved = (Resolve-Path $path).Path
        if (-not $resolved.StartsWith($ScriptRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean unexpected path: $resolved"
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

foreach ($path in @($UnsignedHap, $ProfileJson, $ProfileP7b, $SignedHap, $VerifyProfileJson, $CertChain, $ExtractedProfile)) {
    if (Test-Path $path) {
        Remove-Item -LiteralPath $path -Force
    }
}

Add-Type -AssemblyName System.IO.Compression.FileSystem

function New-HapArchive {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourceDirectory,

        [Parameter(Mandatory = $true)]
        [string]$DestinationPath
    )

    if (Test-Path $DestinationPath) {
        Remove-Item -LiteralPath $DestinationPath -Force
    }

    $sourceRoot = (Resolve-Path $SourceDirectory).Path.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
    $destinationStream = [IO.File]::Open($DestinationPath, [IO.FileMode]::CreateNew)
    $archive = [IO.Compression.ZipArchive]::new($destinationStream, [IO.Compression.ZipArchiveMode]::Create)
    try {
        Get-ChildItem -LiteralPath $sourceRoot -Recurse -File | ForEach-Object {
            $relativePath = $_.FullName.Substring($sourceRoot.Length + 1).Replace('\', '/')
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive,
                $_.FullName,
                $relativePath,
                [IO.Compression.CompressionLevel]::Optimal
            ) | Out-Null
        }
    }
    finally {
        $archive.Dispose()
        $destinationStream.Dispose()
    }
}

[IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path $InputHap), $StageDir)

$ModulePath = Join-Path $StageDir "module.json"
$ModuleJson = Get-Content -Raw -Encoding UTF8 $ModulePath | ConvertFrom-Json

$ModuleJson.module.PSObject.Properties.Remove("requestPermissions")
if ($ModuleJson.module.PSObject.Properties.Name -contains "extensionAbilities" -and $ModuleJson.module.extensionAbilities) {
    $ModuleJson.module.extensionAbilities = @(
        $ModuleJson.module.extensionAbilities | Where-Object {
            $_.type -ne "enterpriseAdmin" -and $_.name -ne "EnterpriseAdminAbility"
        }
    )
}

$ModuleJson | ConvertTo-Json -Depth 100 -Compress | Set-Content -Path $ModulePath -Encoding UTF8
New-HapArchive -SourceDirectory $StageDir -DestinationPath $UnsignedHap

$TemplatePath = Join-Path $ScriptRoot "UnsgnedDebugProfileTemplate.json"
$Template = Get-Content -Raw -Encoding UTF8 $TemplatePath | ConvertFrom-Json
if ($Template.PSObject.Properties.Name -contains "app-distribution-type") {
    $Template.PSObject.Properties.Remove("app-distribution-type")
}
$Template.uuid = [guid]::NewGuid().ToString()
$Template."bundle-info"."bundle-name" = $ModuleJson.app.bundleName
$Template."bundle-info".apl = "normal"
$Template."bundle-info"."app-feature" = "hos_normal_app"
$Template.acls."allowed-acls" = @()
$Template.permissions."restricted-permissions" = @()
$Template | ConvertTo-Json -Depth 100 | Set-Content -Path $ProfileJson -Encoding UTF8

Push-Location $ScriptRoot
try {
    & $Java -jar "hap-sign-tool.jar" sign-profile -mode "localSign" -keyAlias "OpenHarmony Application Profile Debug" -keyPwd "123456" -inFile $ProfileJson -outFile $ProfileP7b -keystoreFile "OpenHarmony.p12" -keystorePwd "123456" -signAlg "SHA256withECDSA" -profileCertFile "OpenHarmonyProfileDebug.pem"
    if ($LASTEXITCODE -ne 0) { throw "sign-profile failed with exit code $LASTEXITCODE" }

    & $Java -jar "hap-sign-tool.jar" sign-app -keyAlias "openharmony application release" -signAlg "SHA256withECDSA" -mode "localSign" -appCertFile "OpenHarmonyApplication.pem" -profileFile $ProfileP7b -inFile $UnsignedHap -keystoreFile "OpenHarmony.p12" -outFile $SignedHap -keyPwd "123456" -keystorePwd "123456"
    if ($LASTEXITCODE -ne 0) { throw "sign-app failed with exit code $LASTEXITCODE" }

    & $Java -jar "hap-sign-tool.jar" verify-profile -inFile $ProfileP7b -outFile $VerifyProfileJson
    if ($LASTEXITCODE -ne 0) { throw "verify-profile failed with exit code $LASTEXITCODE" }

    & $Java -jar "hap-sign-tool.jar" verify-app -inFile $SignedHap -outCertChain $CertChain -outProfile $ExtractedProfile
    if ($LASTEXITCODE -ne 0) { throw "verify-app failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

foreach ($path in @($StageDir, $UnsignedHap, $ProfileJson, $ProfileP7b, $VerifyProfileJson, $CertChain, $ExtractedProfile)) {
    if (Test-Path $path) {
        $resolved = (Resolve-Path $path).Path
        if (-not $resolved.StartsWith($ScriptRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean unexpected path: $resolved"
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

Write-Host "Signed HAP: $SignedHap"
