param(
  [string]$AppPath = "harmony/app/build/outputs/default/app-default-signed.app",
  [string]$JavaPath = "C:\Program Files\Huawei\DevEco Studio\jbr\bin\java.exe",
  [string]$HapSignToolJar = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\lib\hap-sign-tool.jar",
  [string]$ReadElfToolPath = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\native\llvm\bin\llvm-readelf.exe",
  [string]$ArkDisasmToolPath = "C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\toolchains\ark_disasm.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$app = (Resolve-Path (Join-Path $repoRoot $AppPath)).Path
$verificationRoot = Join-Path $repoRoot "harmony/out/multidevice-verification"
$workDir = Join-Path $verificationRoot ([Guid]::NewGuid().ToString("N"))

function Assert-EqualSet([string[]]$Actual, [string[]]$Expected, [string]$Label) {
  if (Compare-Object @($Actual) @($Expected)) {
    throw "$Label mismatch: actual=[$($Actual -join ',')] expected=[$($Expected -join ',')]"
  }
}

function Read-ZipText($Zip, [string]$Name) {
  $entry = $Zip.GetEntry($Name)
  if ($null -eq $entry) { throw "Missing archive entry: $Name" }
  $reader = [System.IO.StreamReader]::new($entry.Open())
  try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
}

function Open-NestedZip($Entry, [ref]$BackingStream) {
  $memory = [System.IO.MemoryStream]::new()
  $source = $Entry.Open()
  try { $source.CopyTo($memory) } finally { $source.Dispose() }
  $memory.Position = 0
  $BackingStream.Value = $memory
  return [System.IO.Compression.ZipArchive]::new($memory, [System.IO.Compression.ZipArchiveMode]::Read)
}

function Assert-NoCrossModuleExportObfuscation([string]$ModuleName) {
  $rulePath = Join-Path $repoRoot "harmony/app/$ModuleName/obfuscation-rules.txt"
  if (-not (Test-Path -LiteralPath $rulePath)) { return }

  $enabled = Get-Content -LiteralPath $rulePath | Where-Object {
    $line = $_.Trim()
    $line.Length -gt 0 -and -not $line.StartsWith("#") -and
      $line -eq "-enable-export-obfuscation"
  }
  if ($enabled) {
    throw "$ModuleName enables export obfuscation, which breaks the independently compiled common.hsp API contract"
  }
}

foreach ($moduleName in @("entry", "entry_tablet")) {
  Assert-NoCrossModuleExportObfuscation $moduleName
}

foreach ($path in @($JavaPath, $HapSignToolJar, $ReadElfToolPath, $ArkDisasmToolPath, $app)) {
  if (-not (Test-Path -LiteralPath $path)) { throw "Missing verifier input: $path" }
}

New-Item -ItemType Directory -Force -Path $workDir | Out-Null
try {
  $previousErrorActionPreference = $ErrorActionPreference
  $ErrorActionPreference = "Continue"
  $verifyOutput = & $JavaPath -jar $HapSignToolJar verify-app `
    -inFile $app `
    -outCertChain (Join-Path $workDir "cert-chain.cer") `
    -outProfile (Join-Path $workDir "profile.p7b") 2>&1
  $verifyExitCode = $LASTEXITCODE
  $ErrorActionPreference = $previousErrorActionPreference
  if ($verifyExitCode -ne 0) {
    throw "App signature verification failed: $($verifyOutput -join [Environment]::NewLine)"
  }

  Add-Type -AssemblyName System.IO.Compression
  $appStream = [System.IO.File]::OpenRead($app)
  $appZip = [System.IO.Compression.ZipArchive]::new($appStream, [System.IO.Compression.ZipArchiveMode]::Read)
  try {
    $expectedModules = @{
      "common" = @{ Type = "shared"; Devices = @("2in1", "tablet"); Hnp = 0; Client = 1; Control = 0 }
      "entry" = @{ Type = "entry"; Devices = @("2in1"); Hnp = 1; Client = 0; Control = 1 }
      "entry_tablet" = @{ Type = "entry"; Devices = @("tablet"); Hnp = 0; Client = 0; Control = 0 }
    }
    $tabletPermissions = @(
      "ohos.permission.APPROXIMATELY_LOCATION", "ohos.permission.CAMERA",
      "ohos.permission.GET_NETWORK_INFO", "ohos.permission.INTERNET",
      "ohos.permission.LOCATION", "ohos.permission.MICROPHONE",
      "ohos.permission.PRINT", "ohos.permission.READ_PASTEBOARD"
    )
    $pcOnlyPermissions = @("ohos.permission.CONTROL_DEVICE", "ohos.permission.CUSTOM_SCREEN_RECORDING")
    $seen = @{}
    $moduleAssembly = @{}

    foreach ($packageEntry in $appZip.Entries | Where-Object { $_.FullName -match '\.(hap|hsp)$' }) {
      $packageMemory = $null
      $packageZip = Open-NestedZip $packageEntry ([ref]$packageMemory)
      try {
        $manifest = (Read-ZipText $packageZip "module.json") | ConvertFrom-Json
        $name = $manifest.module.name
        if (-not $expectedModules.ContainsKey($name)) { throw "Unexpected module: $name" }
        $rule = $expectedModules[$name]
        if ($manifest.module.type -ne $rule.Type) { throw "$name module type mismatch" }
        Assert-EqualSet @($manifest.module.deviceTypes) @($rule.Devices) "$name deviceTypes"

        $names = @($packageZip.Entries | ForEach-Object { $_.FullName })
        $probeArtifacts = @($names | Where-Object { $_ -match '(?i)probe' })
        if ($probeArtifacts.Count -gt 0) {
          throw "$name contains probe artifact: $($probeArtifacts -join ',')"
        }
        $hnpEntries = @($names | Where-Object { $_ -like "hnp/*" })
        $clientCount = @($names | Where-Object { $_ -eq "libs/arm64-v8a/librdpclient.so" }).Count
        $controlCount = @($names | Where-Object { $_ -eq "libs/arm64-v8a/libxrdpcontrol.so" }).Count
        $hnpPresent = if ($hnpEntries.Count -gt 0) { 1 } else { 0 }
        if ($hnpPresent -ne $rule.Hnp -or
            $clientCount -ne $rule.Client -or $controlCount -ne $rule.Control) {
          throw "$name native boundary mismatch: hnp=$($hnpEntries.Count) client=$clientCount control=$controlCount"
        }
        $forbiddenServerLibs = @($names | Where-Object {
          $_ -match '(^|/)lib(xrdpserver|xrdpohos|xrdp|common|ipm|toml)\.so'
        })
        if ($forbiddenServerLibs.Count -gt 0) {
          throw "$name contains XRDP server libraries outside HNP: $($forbiddenServerLibs -join ',')"
        }

        if ($name -eq "entry_tablet") {
          $actualPermissions = @($manifest.module.requestPermissions | ForEach-Object { $_.name })
          Assert-EqualSet $actualPermissions $tabletPermissions "tablet permissions"
          foreach ($permission in $pcOnlyPermissions) {
            if ($actualPermissions -contains $permission) { throw "Tablet contains PC-only permission: $permission" }
          }
          if ($null -ne $manifest.module.hnpPackages) { throw "Tablet manifest contains hnpPackages" }
        }

        if ($name -eq "entry" -or $name -eq "entry_tablet") {
          $printExtensions = @($manifest.module.extensionAbilities | Where-Object {
            $_.name -eq "MuHubPrintExtension" -and $_.type -eq "print"
          })
          if ($printExtensions.Count -ne 1) {
            throw "$name must contain exactly one MuHub print extension"
          }
        }

        if ($name -eq "entry") {
          $actualPermissions = @($manifest.module.requestPermissions | ForEach-Object { $_.name })
          foreach ($permission in $pcOnlyPermissions) {
            if ($actualPermissions -notcontains $permission) { throw "PC entry misses permission: $permission" }
          }
          Assert-EqualSet @($manifest.module.hnpPackages | ForEach-Object { $_.package }) @("xrdp.hnp") "PC HNP declaration"
          $hnpEntry = $packageZip.GetEntry("hnp/arm64-v8a/xrdp.hnp")
          if ($null -eq $hnpEntry) { throw "PC entry misses private xrdp.hnp" }
          $hnpMemory = $null
          $hnpZip = Open-NestedZip $hnpEntry ([ref]$hnpMemory)
          try {
            $hnpNames = @($hnpZip.Entries | ForEach-Object { $_.FullName })
            foreach ($required in @("xrdp/bin/xrdp", "xrdp/config/xrdp.ini", "xrdp/lib/libxrdpohos.so", "xrdp/hnp.json")) {
              if ($hnpNames -notcontains $required) { throw "HNP misses $required" }
            }
            if ($hnpNames | Where-Object { $_ -match 'libxrdpserver\.so' }) {
              throw "HNP contains removed embedded server library"
            }
            $binaryPath = Join-Path $workDir "xrdp"
            $binaryStream = [System.IO.File]::Create($binaryPath)
            $entryStream = $hnpZip.GetEntry("xrdp/bin/xrdp").Open()
            try { $entryStream.CopyTo($binaryStream) } finally { $entryStream.Dispose(); $binaryStream.Dispose() }
            $dynamic = (& $ReadElfToolPath -d $binaryPath 2>&1) -join "`n"
            if ($LASTEXITCODE -ne 0 -or $dynamic -match '/mnt/|[A-Za-z]:\\|/Users/|/home/' -or
                $dynamic -match 'Shared library: \[libxrdpserver\.so\]') {
              throw "HNP xrdp ELF dependency/RUNPATH check failed"
            }
          } finally { $hnpZip.Dispose(); $hnpMemory.Dispose() }
        }

        $abcEntry = $packageZip.GetEntry("ets/modules.abc")
        if ($null -eq $abcEntry) { throw "$name misses ets/modules.abc" }
        $abcPath = Join-Path $workDir "$name-modules.abc"
        $assemblyPath = Join-Path $workDir "$name-modules.pa"
        $abcOutput = [System.IO.File]::Create($abcPath)
        $abcInput = $abcEntry.Open()
        try { $abcInput.CopyTo($abcOutput) } finally { $abcInput.Dispose(); $abcOutput.Dispose() }
        & $ArkDisasmToolPath $abcPath $assemblyPath
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $assemblyPath)) {
          throw "Failed to disassemble $name ArkTS bytecode"
        }
        $moduleAssembly[$name] = $assemblyPath
        $seen[$name] = $true
      } finally { $packageZip.Dispose(); $packageMemory.Dispose() }
    }
    Assert-EqualSet @($seen.Keys) @($expectedModules.Keys) "App modules"

    $commonAssembly = Get-Content -LiteralPath $moduleAssembly["common"] -Raw
    $exportPattern = 'ModuleTag: (?:INDIRECT|LOCAL)_EXPORT,.*?export_name: ([^,;]+)'
    $commonExports = @([regex]::Matches($commonAssembly, $exportPattern) | ForEach-Object {
      $_.Groups[1].Value
    } | Sort-Object -Unique)
    $commonImportPattern =
      'ModuleTag: REGULAR_IMPORT, local_name: [^,]+, import_name: ([^,]+), module_request: @normalized:N&common&&common/Index&;'
    foreach ($entryModule in @("entry", "entry_tablet")) {
      $entryAssembly = Get-Content -LiteralPath $moduleAssembly[$entryModule] -Raw
      $commonImports = @([regex]::Matches($entryAssembly, $commonImportPattern) | ForEach-Object {
        $_.Groups[1].Value
      } | Sort-Object -Unique)
      if ($commonImports.Count -eq 0) {
        throw "$entryModule has no runtime imports from common/Index"
      }
      $missingExports = @($commonImports | Where-Object { $_ -notin $commonExports })
      if ($missingExports.Count -gt 0) {
        throw "$entryModule imports names not exported by common/Index: $($missingExports -join ',')"
      }
    }
  } finally { $appZip.Dispose(); $appStream.Dispose() }

  $hash = (Get-FileHash -LiteralPath $app -Algorithm SHA256).Hash.ToLowerInvariant()
  $item = Get-Item -LiteralPath $app
  Write-Host "Multi-device App verification passed: $($item.FullName)"
  Write-Host "bytes=$($item.Length) sha256=$hash modules=common,entry,entry_tablet arktsCommonAbi=passed"
} finally {
  $resolvedWork = [System.IO.Path]::GetFullPath($workDir)
  $resolvedRoot = [System.IO.Path]::GetFullPath($verificationRoot) + [System.IO.Path]::DirectorySeparatorChar
  if ($resolvedWork.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
      (Test-Path -LiteralPath $resolvedWork)) {
    Remove-Item -LiteralPath $resolvedWork -Recurse -Force
  }
}
