$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if ($repoRoot.Length -lt 3 -or $repoRoot[1] -ne ':') {
  throw "Unsupported Windows workspace path: $repoRoot"
}
$drive = $repoRoot.Substring(0, 1).ToLowerInvariant()
$pathTail = $repoRoot.Substring(2).Replace('\', '/')
$wslRoot = "/mnt/$drive$pathTail"

$cppRoot = "$wslRoot/harmony/app/entry/src/main/cpp"
$freeRdpRoot = "$wslRoot/harmony/third_party/FreeRDP"
$freeRdpInclude = "$wslRoot/harmony/out/ohos-arm64/sysroot/include/freerdp3"
$testSource = "$cppRoot/tests/rdp_display_resize_coordinator_test.cpp"
$coordinatorSource = "$cppRoot/session/rdp_display_resize_coordinator.cpp"
$testBinary = '/tmp/muhub-rdp-display-resize-test'

$command = "set -e; trap 'rm -f $testBinary' EXIT; " +
  "g++ -std=c++17 -pthread " +
  "-I'$cppRoot' -I'$freeRdpRoot' -I'$freeRdpInclude' " +
  "'$testSource' '$coordinatorSource' -o '$testBinary'; '$testBinary'"

& wsl.exe bash -lc $command
if ($LASTEXITCODE -ne 0) {
  throw "Tablet native tests failed with exit code $LASTEXITCODE."
}

Write-Output 'Tablet native resize tests passed.'
