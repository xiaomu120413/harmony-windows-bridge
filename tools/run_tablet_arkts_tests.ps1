$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectDirectory = Join-Path $repositoryRoot 'harmony\app'
$studioCandidates = @()

$nativeTypesPath = Join-Path $repositoryRoot 'harmony\app\common\src\main\cpp\types\librdpclient\Index.d.ts'
$indexPath = Join-Path $repositoryRoot 'harmony\app\common\src\main\ets\pages\Index.ets'
$etsRoot = Join-Path $repositoryRoot 'harmony\app\common\src\main\ets'
$gatewayPath = Join-Path $etsRoot 'rdp\NativeRdpGateway.ets'
$controllerPath = Join-Path $etsRoot 'rdp\RdpClientController.ets'
$connectionPageCoordinatorPath = Join-Path $etsRoot 'rdp\WindowsConnectionPageCoordinator.ets'
$endpointProbePath = Join-Path $etsRoot 'rdp\RdpEndpointProbe.ets'
$connectionAttemptPath = Join-Path $etsRoot 'rdp\RdpConnectionAttemptCoordinator.ets'
$diagnosticsExporterPath = Join-Path $etsRoot 'rdp\RdpDiagnosticsExporter.ets'
$connectionDetailsPath = Join-Path $etsRoot 'components\home\HomeConnectionDetails.ets'
$deviceListPath = Join-Path $etsRoot 'components\home\HomeDeviceList.ets'
$homeTextPath = Join-Path $etsRoot 'components\home\HomeText.ets'
$homeHeaderPath = Join-Path $etsRoot 'components\home\HomeHeader.ets'
$settingsPagePath = Join-Path $etsRoot 'components\SettingsPage.ets'
$settingsOverviewPath = Join-Path $etsRoot 'components\settings\SettingsOverviewPage.ets'
$basicSettingsPath = Join-Path $etsRoot 'components\settings\BasicSettingsPage.ets'
$projectHelpPath = Join-Path $etsRoot 'components\settings\ProjectHelpPage.ets'
$remoteSettingsPath = Join-Path $etsRoot 'components\settings\RemoteControlSettingsPage.ets'
$remoteCardsPath = Join-Path $etsRoot 'components\settings\RemoteControlCards.ets'
$remoteFilesCardPath = Join-Path $etsRoot 'components\settings\RemoteFilesCard.ets'
$diagnosticsCardPath = Join-Path $etsRoot 'components\settings\DiagnosticsCard.ets'
$settingsConstantsPath = Join-Path $etsRoot 'components\settings\SettingsConstants.ets'
$modulePath = Join-Path $repositoryRoot 'harmony\app\entry\src\main\module.json5'
$tabletModulePath = Join-Path $repositoryRoot 'harmony\app\entry_tablet\src\main\module.json5'
$nativeTypesText = Get-Content -Raw -Encoding utf8 $nativeTypesPath
$indexText = Get-Content -Raw -Encoding utf8 $indexPath
$gatewayText = Get-Content -Raw -Encoding utf8 $gatewayPath
$controllerText = Get-Content -Raw -Encoding utf8 $controllerPath
$connectionPageCoordinatorText = Get-Content -Raw -Encoding utf8 $connectionPageCoordinatorPath
$endpointProbeText = Get-Content -Raw -Encoding utf8 $endpointProbePath
$connectionAttemptText = Get-Content -Raw -Encoding utf8 $connectionAttemptPath
$diagnosticsExporterText = Get-Content -Raw -Encoding utf8 $diagnosticsExporterPath
$connectionDetailsText = Get-Content -Raw -Encoding utf8 $connectionDetailsPath
$deviceListText = Get-Content -Raw -Encoding utf8 $deviceListPath
$homeTextText = Get-Content -Raw -Encoding utf8 $homeTextPath
$homeHeaderText = Get-Content -Raw -Encoding utf8 $homeHeaderPath
$settingsPageText = Get-Content -Raw -Encoding utf8 $settingsPagePath
$settingsOverviewText = Get-Content -Raw -Encoding utf8 $settingsOverviewPath
$basicSettingsText = Get-Content -Raw -Encoding utf8 $basicSettingsPath
$projectHelpText = Get-Content -Raw -Encoding utf8 $projectHelpPath
$remoteSettingsText = Get-Content -Raw -Encoding utf8 $remoteSettingsPath
$remoteCardsText = Get-Content -Raw -Encoding utf8 $remoteCardsPath
$remoteFilesCardText = Get-Content -Raw -Encoding utf8 $remoteFilesCardPath
$diagnosticsCardText = Get-Content -Raw -Encoding utf8 $diagnosticsCardPath
$settingsConstantsText = Get-Content -Raw -Encoding utf8 $settingsConstantsPath
$moduleText = Get-Content -Raw -Encoding utf8 $modulePath
$tabletModuleText = Get-Content -Raw -Encoding utf8 $tabletModulePath
$entryResourceRoot = Join-Path $repositoryRoot 'harmony\app\entry\src\main\resources'
$tabletResourceRoot = Join-Path $repositoryRoot 'harmony\app\entry_tablet\src\main\resources'
$allEtsText = (Get-ChildItem -Path $etsRoot -Recurse -Filter '*.ets' |
  ForEach-Object { Get-Content -Raw -Encoding utf8 $_.FullName }) -join "`n"
if ($tabletModuleText.Contains('probe_icon') -or $tabletModuleText.Contains('packaging probe') -or
    -not $tabletModuleText.Contains('MuHubPrintExtension') -or
    -not $tabletModuleText.Contains('$media:layered_image')) {
  throw 'Tablet entry must use production icon, description, and print extension resources.'
}
$entryResources = @(Get-ChildItem $entryResourceRoot -Recurse -File | ForEach-Object {
  "$($_.FullName.Substring($entryResourceRoot.Length + 1))|$((Get-FileHash $_.FullName).Hash)"
})
$tabletResources = @(Get-ChildItem $tabletResourceRoot -Recurse -File | ForEach-Object {
  "$($_.FullName.Substring($tabletResourceRoot.Length + 1))|$((Get-FileHash $_.FullName).Hash)"
})
if (Compare-Object $entryResources $tabletResources) {
  throw 'PC and tablet Entry resource files must remain identical.'
}
foreach ($required in @(
  "export type NativePermissionType = 'microphone' | 'camera' | 'clipboard' | 'location'",
  'onPermissionRequest(callback: (request: NativePermissionRequest) => void)',
  'completePermissionRequest(result: NativePermissionResult)',
  'attachXComponentContent(nodeContent: NodeContent)',
  'NativeRdpGateway.onPermissionRequest',
  'NativeRdpGateway.completePermissionRequest',
  'RemoteControlPort'
)) {
  if (-not ($nativeTypesText.Contains($required) -or $allEtsText.Contains($required))) {
    throw "Unified native permission contract is incomplete: missing $required"
  }
}

$nativeImports = @(Get-ChildItem -Path $etsRoot -Recurse -Filter '*.ets' |
  Select-String -SimpleMatch "from 'librdpclient.so'")
if ($nativeImports.Count -ne 1 -or $nativeImports[0].Path -ne $gatewayPath) {
  $locations = ($nativeImports | ForEach-Object { "$($_.Path):$($_.LineNumber)" }) -join ', '
  throw "NativeRdpGateway must be the only ArkTS librdpclient.so import. Found: $locations"
}
if ($nativeTypesText.Contains('ensureXrdpServerStarted') -or
  $nativeTypesText.Contains('getXrdpServerDiagnostics') -or
  $gatewayText.Contains('NativeXrdpServer')) {
  throw 'Shared RDP client bridge must not expose XRDP server control.'
}
if ($indexText.Contains('rdpNative.') -or $indexText.Contains('NativeRdpGateway') -or
  -not $indexText.Contains('RdpClientController')) {
  throw 'Index must coordinate RDP through dedicated controllers without Native gateway details.'
}
$oversizedEtsFiles = @()
foreach ($moduleName in @('common', 'entry', 'entry_tablet')) {
  $sourceRoot = Join-Path $projectDirectory "$moduleName\src"
  if (-not (Test-Path $sourceRoot)) {
    continue
  }
  Get-ChildItem -Path $sourceRoot -Recurse -File -Filter '*.ets' | ForEach-Object {
    $lineCount = (Get-Content -Encoding utf8 $_.FullName).Count
    if ($lineCount -gt 600) {
      $relativePath = [System.IO.Path]::GetRelativePath($repositoryRoot, $_.FullName)
      $oversizedEtsFiles += "$relativePath ($lineCount lines)"
    }
  }
}
if ($oversizedEtsFiles.Count -gt 0) {
  throw "ArkTS files exceed the 600-line architecture limit: $($oversizedEtsFiles -join ', ')"
}
if (-not $indexText.Contains('remoteControlPageCoordinator: RemoteControlPageCoordinator | null = null') -or
  -not $indexText.Contains('private remoteControlCoordinator(): RemoteControlPageCoordinator') -or
  $indexText.Contains('readonly remoteControlPageCoordinator: RemoteControlPageCoordinator = new')) {
  throw 'RemoteControlPageCoordinator must be created lazily after the Entry remoteControlPort property is injected.'
}
foreach ($requiredProbeRule in @('constructTCPSocketInstance', 'timeout: timeoutMs', 'await tcpSocket.close()',
  'private generation', 'generation !== this.generation')) {
  if (-not ($endpointProbeText + $connectionAttemptText).Contains($requiredProbeRule)) {
    throw "RDP endpoint preflight is missing required cancellation or cleanup behavior: $requiredProbeRule"
  }
}
foreach ($requiredDiagnosticRule in @('schemaVersion: 2', 'hostLength', 'usernameLength',
  'NativeRdpGateway.getDiagnostics()', 'fileIo.OpenMode.TRUNC', 'cameraPacketCounters',
  'lastErrorCategory', 'endpointProbeElapsedMs', 'clipboardRuntimeCounters',
  'passiveScreenCaptureCounters')) {
  if (-not $diagnosticsExporterText.Contains($requiredDiagnosticRule)) {
    throw "RDP diagnostics export is missing required versioning or redaction behavior: $requiredDiagnosticRule"
  }
}
foreach ($sensitiveDiagnosticField in @('password:', 'accessCode:', 'host: string', 'username: string')) {
  if ($diagnosticsExporterText.Contains($sensitiveDiagnosticField)) {
    throw "RDP diagnostics export must not serialize sensitive connection data: $sensitiveDiagnosticField"
  }
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
  'WindowsConnectionPageCoordinator',
  'RemoteControlPageCoordinator',
  'RdpSessionLifecycleCoordinator',
  'RdpSurfaceContentHost'
)) {
  if (-not $indexText.Contains($requiredCoordinator)) {
    throw "Index is missing required coordinator boundary: $requiredCoordinator"
  }
}
foreach ($requiredCredentialSwitchRule in @(
  'selectionGeneration',
  'this.delegate.applyProfile(profile, password === null',
  'passwordLoading: this.connectionProfilePasswordLoading'
)) {
  if (-not ($indexText + $connectionPageCoordinatorText).Contains($requiredCredentialSwitchRule)) {
    throw "Connection profile switching is missing atomic credential loading: $requiredCredentialSwitchRule"
  }
}
if (-not $connectionDetailsText.Contains('placeholder: HomeText.WINDOWS_USERNAME_PLACEHOLDER') -or
  ([regex]::Matches($connectionDetailsText, '\.enabled\(!this\.passwordLoading\)').Count -lt 2)) {
  throw 'Connection details must use the Windows account hint and disable password/connect while credentials load.'
}
if (-not $homeTextText.Contains('DESKTOP-ABC\\zhangsan') -or
  $deviceListText.Contains("profile.username.substring(0, slashIndex)")) {
  throw 'Windows account guidance or full saved username rendering regressed.'
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
if ($basicSettingsText.Contains('@kit.NetworkKit') -or
  $basicSettingsText.Contains('BASIC_NETWORK_SECTION')) {
  throw 'Basic settings must not own remote-control connection network information.'
}
if (-not $settingsOverviewText.Contains('currentStateCardHovered') -or
  -not $settingsOverviewText.Contains('setCurrentStateCardHovered')) {
  throw 'The overview current-state card must keep the shared animated hover treatment.'
}
if (-not $settingsPageText.Contains('remoteControlServerAvailable: this.remoteControlServerAvailable') -or
  -not $projectHelpText.Contains('if (this.remoteControlServerAvailable)') -or
  -not $projectHelpText.Contains('this.controlledGuide()') -or
  -not $projectHelpText.Contains('this.showControllerGuide = !this.remoteControlServerAvailable') -or
  -not $projectHelpText.Contains('selected: this.showControllerGuide') -or
  -not $projectHelpText.Contains('if (this.showControllerGuide)') -or
  -not $settingsOverviewText.Contains('SettingsShellText.HELP_CLIENT_DESC')) {
  throw 'Project help must gate controlled-device guidance behind the 2in1 capability snapshot.'
}
if (-not $remoteSettingsText.Contains('RemoteAccessCard') -or
  -not $remoteSettingsText.Contains('if (this.passiveControlAvailable)') -or
  -not $settingsPageText.Contains('passiveControlAvailable: this.remoteControlServerAvailable') -or
  $settingsPageText.Contains('this.pageName === SettingsRoute.REMOTE_CONTROL && this.remoteControlServerAvailable') -or
  -not $settingsOverviewText.Contains('SettingsShellText.REMOTE_CLIENT_DESC') -or
  -not $remoteSettingsText.Contains('SettingsRemoteText.PASSIVE_SECTION') -or
  -not $remoteSettingsText.Contains('SettingsRemoteText.ACTIVE_SECTION') -or
  -not $remoteSettingsText.Contains('localIpAddress')) {
  throw 'Remote settings must separate passive and active control, keep verification, and expose the address.'
}
if ($remoteSettingsText.LastIndexOf('RemoteFilesCard') -gt $remoteSettingsText.LastIndexOf('RemoteAccessCard')) {
  throw 'Active-control verification must appear below the Harmony shared-directory card.'
}
if (-not $remoteSettingsText.Contains('DiagnosticsCard') -or
  -not $remoteSettingsText.Contains('SettingsRemoteText.DIAGNOSTICS_SECTION') -or
  $remoteFilesCardText.Contains('onExportDiagnostics') -or
  -not $diagnosticsCardText.Contains('onExportDiagnostics')) {
  throw 'Diagnostics export must live in its own remote-settings section and card.'
}
if ($settingsPageText.Contains('copyConnectionAddress') -or
  $remoteSettingsText.Contains('onCopyConnectionAddress') -or
  $remoteCardsText.Contains('copyButton()') -or
  $settingsConstantsText.Contains('CONNECTION_COPY_ACTION') -or
  $settingsConstantsText.Contains('CONNECTION_COPIED')) {
  throw 'Remote connection address must be display-only without copy action or copied state.'
}
if ($remoteCardsText.Contains('.layoutWeight(this.layoutMode === LayoutMode.EXPANDED ? 1 : 0)')) {
  throw 'The standalone connection-address block must not expand the server card to fill the scroll viewport.'
}
if (-not $allEtsText.Contains('SECONDARY_ACTION_HEIGHT: number = 36') -or
  -not $homeHeaderText.Contains('.height(SettingsTheme.SECONDARY_ACTION_HEIGHT)') -or
  -not $deviceListText.Contains('minWidth: 136, minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT') -or
  ([regex]::Matches($connectionDetailsText,
    'constraintSize\(\{ minHeight: SettingsTheme\.SECONDARY_ACTION_HEIGHT \}\)').Count -lt 2) -or
  -not $connectionDetailsText.Contains('this.buildActionLabel(HomeText.DEVICE_ACTIONS)') -or
  -not $connectionDetailsText.Contains('.height(SettingsTheme.SECONDARY_ACTION_HEIGHT)') -or
  -not $projectHelpText.Contains('.constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })') -or
  ([regex]::Matches($remoteCardsText,
    'constraintSize\(\{ minHeight: SettingsTheme\.SECONDARY_ACTION_HEIGHT \}\)').Count -lt 4)) {
  throw 'Settings and remote-control secondary actions must share the 36vp height token.'
}
if ($projectHelpText.Contains('localIpAddress') -or
  $projectHelpText.Contains('onCopyConnectionAddress') -or
  $projectHelpText.Contains('this.controlledReadyCard()') -or
  -not $projectHelpText.Contains('ControlledHelpStatusCard({') -or
  -not $projectHelpText.Contains('ControlledHelpStepCard({') -or
  -not $projectHelpText.Contains('.onHover((isHover: boolean) =>')) {
  throw 'Project help must use animated status and step cards without rendering the old address card.'
}
if (-not ($settingsConstantsText -match '\u4E3B\u63A7\u7AEF\uFF1A') -or
  -not ($settingsConstantsText -match '\u88AB\u63A7\u7AEF\uFF1A')) {
  throw 'Controlled-device troubleshooting must separate controller and controlled-device checks.'
}
$settingsTextDomains = @(
  'SettingsShellText',
  'SettingsAppearanceText',
  'SettingsRemoteText',
  'SettingsHelpText',
  'SettingsUsageText',
  'SettingsNetworkText',
  'SettingsAboutText'
)
foreach ($domain in $settingsTextDomains) {
  if (-not $settingsConstantsText.Contains("export class $domain")) {
    throw "Settings copy hierarchy is missing domain: $domain"
  }
}
if ($allEtsText.Contains('SettingsText.')) {
  throw 'Settings copy must use domain-specific text classes instead of the legacy flat SettingsText class.'
}
$staticTextDefinitions = [regex]::Matches(
  $settingsConstantsText,
  "static readonly [A-Z0-9_]+: string =\s*'(?<value>[^']*)'"
)
$duplicateStaticText = @($staticTextDefinitions | ForEach-Object { $_.Groups['value'].Value } |
  Group-Object | Where-Object Count -gt 1)
if ($duplicateStaticText.Count -gt 0) {
  $values = ($duplicateStaticText | ForEach-Object { $_.Name }) -join ', '
  throw "Settings copy contains duplicate static values: $values"
}
if (([regex]::Matches($projectHelpText, 'LayoutMode\.EXPANDED').Count -lt 1) -or
  -not $remoteCardsText.Contains('layoutMode === LayoutMode.EXPANDED')) {
  throw 'Controlled-device help steps and remote address cards must define explicit Compact and Expanded layouts.'
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
  & $hvigor --no-daemon test --mode module -p product=default -p module=common@default
  $testExitCode = $LASTEXITCODE
} finally {
  Pop-Location
}

if ($testExitCode -ne 0) {
  Write-Error "Tablet ArkTS policy tests failed with exit code $testExitCode."
  exit $testExitCode
}

Write-Output 'Tablet ArkTS policy tests passed.'
