$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$bridge = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/entry/src/main/cpp/xrdp/xrdp_server_bridge.cpp") -Raw
$loader = Get-Content -LiteralPath (Join-Path $repoRoot "harmony/app/entry/src/main/cpp/xrdp/xrdp_runtime_loader.cpp") -Raw

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
Write-Host "XRDP independent-process policy tests passed."
