$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectDirectory = Join-Path $repositoryRoot 'harmony\app'
$studioCandidates = @()

$nativeTypesPath = Join-Path $repositoryRoot 'harmony\app\entry\src\main\cpp\types\libentry\Index.d.ts'
$indexPath = Join-Path $repositoryRoot 'harmony\app\entry\src\main\ets\pages\Index.ets'
$etsRoot = Join-Path $repositoryRoot 'harmony\app\entry\src\main\ets'
$gatewayPath = Join-Path $etsRoot 'rdp\NativeRdpGateway.ets'
$controllerPath = Join-Path $etsRoot 'rdp\RdpClientController.ets'
$modulePath = Join-Path $repositoryRoot 'harmony\app\entry\src\main\module.json5'
$nativeTypesText = Get-Content -Raw -Encoding utf8 $nativeTypesPath
$indexText = Get-Content -Raw -Encoding utf8 $indexPath
$gatewayText = Get-Content -Raw -Encoding utf8 $gatewayPath
$controllerText = Get-Content -Raw -Encoding utf8 $controllerPath
$moduleText = Get-Content -Raw -Encoding utf8 $modulePath
$allEtsText = (Get-ChildItem -Path $etsRoot -Recurse -Filter '*.ets' |
  ForEach-Object { Get-Content -Raw -Encoding utf8 $_.FullName }) -join "`n"
foreach ($required in @(
  "export type NativePermissionType = 'microphone' | 'camera' | 'clipboard' | 'location'",
  'onPermissionRequest(callback: (request: NativePermissionRequest) => void)',
  'completePermissionRequest(result: NativePermissionResult)',
  'attachXComponentContent(nodeContent: NodeContent)',
  'NativeRdpGateway.onPermissionRequest',
  'NativeRdpGateway.completePermissionRequest'
)) {
  if (-not ($nativeTypesText.Contains($required) -or $allEtsText.Contains($required))) {
    throw "Unified native permission contract is incomplete: missing $required"
  }
}

$nativeImports = @(Get-ChildItem -Path $etsRoot -Recurse -Filter '*.ets' |
  Select-String -SimpleMatch "from 'libentry.so'")
if ($nativeImports.Count -ne 1 -or $nativeImports[0].Path -ne $gatewayPath) {
  $locations = ($nativeImports | ForEach-Object { "$($_.Path):$($_.LineNumber)" }) -join ', '
  throw "NativeRdpGateway must be the only ArkTS libentry.so import. Found: $locations"
}
if ($indexText.Contains('rdpNative.') -or $indexText.Contains('NativeRdpGateway') -or
  -not $indexText.Contains('RdpClientController')) {
  throw 'Index must coordinate RDP through dedicated controllers without Native gateway details.'
}
$indexLineCount = (Get-Content -Encoding utf8 $indexPath).Count
if ($indexLineCount -gt 950) {
  throw "Index page coordinator grew beyond the architecture budget: $indexLineCount lines."
}
foreach ($forbiddenIndexDetail in @(
  'new WindowsConnectionStore',
  'new XrdpServerController',
  'ensureScreenRecordingPermission(',
  'requestScreenRecordingPermission(`xrdp',
  'NativePermissionType',
  'xrdpServerStartRequest',
  '/^[0-9.]+$/'
)) {
  if ($indexText.Contains($forbiddenIndexDetail)) {
    throw "Index still contains extracted business detail: $forbiddenIndexDetail"
  }
}
foreach ($requiredCoordinator in @(
  'RdpConnectionValidator',
  'WindowsConnectionProfileCoordinator',
  'RdpPermissionRequestCoordinator',
  'RemoteControlCoordinator',
  'RdpSurfaceContentHost'
)) {
  if (-not $indexText.Contains($requiredCoordinator)) {
    throw "Index is missing required coordinator boundary: $requiredCoordinator"
  }
}
$componentNativeDependencies = @(Get-ChildItem -Path (Join-Path $etsRoot 'components') -Recurse -Filter '*.ets' |
  Select-String -Pattern "libentry\.so|NativeRdpGateway")
if ($componentNativeDependencies.Count -ne 0) {
  $locations = ($componentNativeDependencies | ForEach-Object { "$($_.Path):$($_.LineNumber)" }) -join ', '
  throw "Display components must receive callbacks instead of depending on Native: $locations"
}
foreach ($forbiddenDependency in @('../components/', '../capability/', '../adaptive/', '@kit.ArkUI')) {
  if ($controllerText.Contains($forbiddenDependency)) {
    throw "RdpClientController must remain UI and device-policy independent: found $forbiddenDependency"
  }
}
if (-not $moduleText.Contains('"orientation": "auto_rotation"') -or
  -not ($moduleText -match '"supportWindowMode"\s*:\s*\[[^\]]*"split"')) {
  throw 'EntryAbility must declare auto_rotation and split window support.'
}
foreach ($forbidden in @(
  'onMicrophonePermissionRequest',
  'onCameraPermissionRequest',
  'onClipboardPermissionRequest',
  'onLocationPermissionRequest',
  'completeMicrophonePermissionRequest',
  'completeCameraPermissionRequest',
  'completeClipboardPermissionRequest',
  'completeLocationPermissionRequest',
  'attachXComponentContent(nodeContent: Object)'
)) {
  if ($nativeTypesText.Contains($forbidden) -or $indexText.Contains($forbidden)) {
    throw "Legacy or weakly typed native contract remains: found $forbidden"
  }
}

if ($env:DEVECOSTUDIO_HOME) {
  $studioCandidates += $env:DEVECOSTUDIO_HOME
}
$studioCandidates += 'C:\Program Files\Huawei\DevEco Studio'

$studioDirectory = $studioCandidates |
  Where-Object { Test-Path (Join-Path $_ 'tools\hvigor\bin\hvigorw.bat') } |
  Select-Object -First 1

if (-not $studioDirectory) {
  Write-Error 'DevEco Studio was not found. Set DEVECOSTUDIO_HOME to its installation directory.'
  exit 1
}

$hvigor = Join-Path $studioDirectory 'tools\hvigor\bin\hvigorw.bat'
$studioJava = Join-Path $studioDirectory 'jbr\bin'
if (Test-Path (Join-Path $studioJava 'java.exe')) {
  $env:PATH = "$studioJava;$env:PATH"
}

Push-Location $projectDirectory
try {
  & $hvigor --no-daemon test --mode module -p product=default -p module=entry@default
  $testExitCode = $LASTEXITCODE
} finally {
  Pop-Location
}

if ($testExitCode -ne 0) {
  Write-Error "Tablet ArkTS policy tests failed with exit code $testExitCode."
  exit $testExitCode
}

Write-Output 'Tablet ArkTS policy tests passed.'
