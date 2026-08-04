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
$resizeTestSource = "$cppRoot/tests/rdp_display_resize_coordinator_test.cpp"
$coordinatorSource = "$cppRoot/session/rdp_display_resize_coordinator.cpp"
$resizeTestBinary = '/tmp/muhub-rdp-display-resize-test'
$pointerTestSource = "$cppRoot/tests/remote_pointer_text_policy_test.cpp"
$pointerPolicySource = "$cppRoot/input/remote_pointer_text_policy.cpp"
$pointerTestBinary = '/tmp/muhub-remote-pointer-text-test'
$touchTestSource = "$cppRoot/tests/xcomponent_touch_policy_test.cpp"
$touchPolicySource = "$cppRoot/input/xcomponent_touch_policy.cpp"
$touchTestBinary = '/tmp/muhub-xcomponent-touch-policy-test'

$command = "set -e; trap 'rm -f $resizeTestBinary $pointerTestBinary $touchTestBinary' EXIT; " +
  "g++ -std=c++17 -pthread " +
  "-I'$cppRoot' -I'$freeRdpRoot' -I'$freeRdpInclude' " +
  "'$resizeTestSource' '$coordinatorSource' -o '$resizeTestBinary'; '$resizeTestBinary'; " +
  "g++ -std=c++17 -I'$cppRoot' '$pointerTestSource' '$pointerPolicySource' " +
  "-o '$pointerTestBinary'; '$pointerTestBinary'; " +
  "g++ -std=c++17 -I'$cppRoot' '$touchTestSource' '$touchPolicySource' " +
  "-o '$touchTestBinary'; '$touchTestBinary'"

& wsl.exe bash -lc $command
if ($LASTEXITCODE -ne 0) {
  throw "Tablet native tests failed with exit code $LASTEXITCODE."
}

Write-Output 'Tablet native resize, remote pointer text, and touch gesture policy tests passed.'
