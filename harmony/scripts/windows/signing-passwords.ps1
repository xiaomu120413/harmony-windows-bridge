function Resolve-HvigorSigningPasswords {
  param(
    [string]$RepoRoot,
    [string]$SigningRoot,
    [string]$SigningPassword,
    [string]$StorePassword,
    [string]$KeyPassword
  )

  if ([string]::IsNullOrWhiteSpace($StorePassword)) {
    $StorePassword = $env:HAP_STORE_PASSWORD
  }
  if ([string]::IsNullOrWhiteSpace($KeyPassword)) {
    $KeyPassword = $env:HAP_KEY_PASSWORD
  }
  if (-not [string]::IsNullOrWhiteSpace($SigningPassword)) {
    if ([string]::IsNullOrWhiteSpace($StorePassword)) { $StorePassword = $SigningPassword }
    if ([string]::IsNullOrWhiteSpace($KeyPassword)) { $KeyPassword = $SigningPassword }
  } elseif (-not [string]::IsNullOrWhiteSpace($env:HAP_SIGN_PASSWORD)) {
    if ([string]::IsNullOrWhiteSpace($StorePassword)) { $StorePassword = $env:HAP_SIGN_PASSWORD }
    if ([string]::IsNullOrWhiteSpace($KeyPassword)) { $KeyPassword = $env:HAP_SIGN_PASSWORD }
  }

  if ([string]::IsNullOrWhiteSpace($StorePassword) -or [string]::IsNullOrWhiteSpace($KeyPassword)) {
    $profilePath = Join-Path $RepoRoot 'harmony/app/build-profile.json5'
    $profileText = Get-Content -LiteralPath $profilePath -Raw
    $storeMatch = [regex]::Match($profileText, '"storePassword"\s*:\s*"([0-9A-Fa-f]+)"')
    $keyMatch = [regex]::Match($profileText, '"keyPassword"\s*:\s*"([0-9A-Fa-f]+)"')
    if (-not $storeMatch.Success -or -not $keyMatch.Success) {
      throw 'Unable to locate encrypted signing passwords in build-profile.json5'
    }
    $nodePath = 'C:\Program Files\Huawei\DevEco Studio\tools\node\node.exe'
    $decryptScript = Join-Path $PSScriptRoot 'decrypt-hvigor-password.js'
    $signingPath = (Resolve-Path (Join-Path $RepoRoot $SigningRoot)).Path
    if ([string]::IsNullOrWhiteSpace($StorePassword)) {
      $StorePassword = & $nodePath $decryptScript $signingPath $storeMatch.Groups[1].Value
      if ($LASTEXITCODE -ne 0) { throw 'Unable to decrypt the signing store password' }
    }
    if ([string]::IsNullOrWhiteSpace($KeyPassword)) {
      $KeyPassword = & $nodePath $decryptScript $signingPath $keyMatch.Groups[1].Value
      if ($LASTEXITCODE -ne 0) { throw 'Unable to decrypt the signing key password' }
    }
  }

  return @{ StorePassword = $StorePassword; KeyPassword = $KeyPassword }
}
