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
$geometryTestSource = "$cppRoot/tests/remote_content_geometry_test.cpp"
$geometryPolicySource = "$cppRoot/surface/remote_content_geometry.cpp"
$geometryTestBinary = '/tmp/muhub-remote-content-geometry-test'

$napiExportsSource = Join-Path $repoRoot 'harmony/app/entry/src/main/cpp/napi/napi_exports.cpp'
$napiExportsText = Get-Content -Raw -Encoding utf8 $napiExportsSource
$orientationMonitorSource = Join-Path $repoRoot 'harmony/app/entry/src/main/cpp/session/rdp_display_orientation_monitor.cpp'
$orientationMonitorText = Get-Content -Raw -Encoding utf8 $orientationMonitorSource
if ([regex]::Matches($orientationMonitorText, 'Refresh\("native_display_initial"').Count -ne 1) {
  throw 'Native display monitor must refresh the initial orientation exactly once after registration.'
}
foreach ($required in @(
  '{"onPermissionRequest", nullptr, OnPermissionRequest',
  '{"completePermissionRequest", nullptr, CompletePermissionRequest',
  '{"microphone", MicrophonePermissionRequestSink, CompleteMicrophonePermissionRequestFromUi}',
  '{"camera", CameraPermissionRequestSink, CompleteCameraPermissionRequestFromUi}',
  '{"clipboard", ClipboardPermissionRequestSink, CompleteClipboardPermissionRequestFromUi}',
  '{"location", LocationPermissionRequestSink, CompleteLocationPermissionRequestFromUi}',
  'FindPermissionRoute(typeName)'
)) {
  if (-not $napiExportsText.Contains($required)) {
    throw "Unified native permission routing is incomplete: missing $required"
  }
}
foreach ($forbidden in @(
  '"onMicrophonePermissionRequest"',
  '"onCameraPermissionRequest"',
  '"onClipboardPermissionRequest"',
  '"onLocationPermissionRequest"',
  '"completeMicrophonePermissionRequest"',
  '"completeCameraPermissionRequest"',
  '"completeClipboardPermissionRequest"',
  '"completeLocationPermissionRequest"'
)) {
  if ($napiExportsText.Contains($forbidden)) {
    throw "Legacy native permission export remains: found $forbidden"
  }
}

$gestureSource = Join-Path $repoRoot 'harmony/app/entry/src/main/cpp/input/xcomponent_native_gesture.cpp'
$rawTouchSource = Join-Path $repoRoot 'harmony/app/entry/src/main/cpp/input/xcomponent_touch_gesture.cpp'
$sessionPageSource = Join-Path $repoRoot 'harmony/app/entry/src/main/ets/components/session/RdpSessionPage.ets'
$gestureText = Get-Content -Raw -Encoding utf8 $gestureSource
foreach ($required in @(
  'createGroupGesture(PARALLEL_GROUP)',
  'createGroupGesture(EXCLUSIVE_GROUP)',
  'createTapGesture(1, 1)',
  'createTapGesture(2, 1)',
  'createLongPressGesture(1, false',
  'createPanGesture(',
  'OH_ArkUI_SetGestureRecognizerLimitFingerCount'
)) {
  if (-not $gestureText.Contains($required)) {
    throw "Native system gesture topology is incomplete: missing $required"
  }
}
$rawTouchText = Get-Content -Raw -Encoding utf8 $rawTouchSource
foreach ($forbidden in @('NativeTouchGesturePolicy', 'HandleLongPressTimeout')) {
  if ($rawTouchText.Contains($forbidden)) {
    throw "Raw XComponent Touch still owns gesture semantics: found $forbidden"
  }
}
$sessionPageText = Get-Content -Raw -Encoding utf8 $sessionPageSource
foreach ($forbidden in @('XComponent({', 'XComponentController', 'TapGesture', 'PanGesture', 'LongPressGesture')) {
  if ($sessionPageText.Contains($forbidden)) {
    throw "ArkTS session page still owns XComponent/gesture behavior: found $forbidden"
  }
}

$command = "set -e; trap 'rm -f $resizeTestBinary $pointerTestBinary $touchTestBinary $geometryTestBinary' EXIT; " +
  "g++ -std=c++17 -pthread " +
  "-I'$cppRoot' -I'$freeRdpRoot' -I'$freeRdpInclude' " +
  "'$resizeTestSource' '$coordinatorSource' -o '$resizeTestBinary'; '$resizeTestBinary'; " +
  "g++ -std=c++17 -I'$cppRoot' '$pointerTestSource' '$pointerPolicySource' " +
  "-o '$pointerTestBinary'; '$pointerTestBinary'; " +
  "g++ -std=c++17 -I'$cppRoot' '$touchTestSource' '$touchPolicySource' " +
  "-o '$touchTestBinary'; '$touchTestBinary'; " +
  "g++ -std=c++17 -I'$cppRoot' '$geometryTestSource' '$geometryPolicySource' " +
  "-o '$geometryTestBinary'; '$geometryTestBinary'"

& wsl.exe bash -lc $command
if ($LASTEXITCODE -ne 0) {
  throw "Tablet native tests failed with exit code $LASTEXITCODE."
}

Write-Output 'Tablet native resize, remote pointer text, touch gesture, and content geometry tests passed.'
