$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectDirectory = Join-Path $repositoryRoot 'harmony\app'
$studioCandidates = @()

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
