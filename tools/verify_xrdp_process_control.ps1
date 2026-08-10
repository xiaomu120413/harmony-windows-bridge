$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$bridge = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/entry/src/main/cpp/xrdp/xrdp_server_bridge.cpp") -Raw
$loader = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/entry/src/main/cpp/xrdp/xrdp_runtime_loader.cpp") -Raw
$internal = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/entry/src/main/cpp/xrdp/xrdp_server_internal.h") -Raw
$coordinator = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/common/src/main/ets/rdp/RemoteControlCoordinator.ets") -Raw
$page = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/common/src/main/ets/pages/Index.ets") -Raw
$cards = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/common/src/main/ets/components/settings/RemoteControlCards.ets") -Raw

foreach ($required in @("fork()", "execve(", "waitpid(", "kill(", "PR_SET_PDEATHSIG")) {
  if (-not $bridge.Contains($required)) { throw "Missing XRDP process lifecycle primitive: $required" }
}
foreach ($forbidden in @("dlopen(", "xrdp_ohos_server_main", "xrdp_ohos_server_stop", "std::thread")) {
  if ($bridge.Contains($forbidden) -or $loader.Contains($forbidden)) {
    throw "Embedded XRDP server path is still present: $forbidden"
  }
}
if ($bridge -match 'accessCode.*argv|argv.*accessCode') {
  throw "Access code must not be passed through the process command line"
}
if (-not $internal.Contains('/data/app/bin/xrdp') -or -not $loader.Contains('realpath(')) {
  throw "Private XRDP HNP must be resolved from its version-neutral /data/app link"
}
foreach ($forbiddenPath in @('/data/service/hnp', '/data/app/el1/bundle/', 'xrdp_0.1.0')) {
  if ($internal.Contains($forbiddenPath) -or $loader.Contains($forbiddenPath)) {
    throw "XRDP runtime uses a public-HNP or host-only path: $forbiddenPath"
  }
}
foreach ($requiredLifecycle in @('stopFromSettings()', 'this.remoteControlPort.stop(', 'setInterval(',
  'clearInterval(', 'onStopXrdpServer')) {
  if (-not (($coordinator + $page + $cards).Contains($requiredLifecycle))) {
    throw "Missing XRDP ArkTS lifecycle control: $requiredLifecycle"
  }
}
Write-Host "XRDP independent-process policy tests passed."
