function Get-BuildCacheRelativePath {
  param(
    [Parameter(Mandatory = $true)][string]$Root,
    [Parameter(Mandatory = $true)][string]$Path
  )

  $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
  $full = [System.IO.Path]::GetFullPath($Path)
  if ($full.StartsWith($rootFull, [System.StringComparison]::OrdinalIgnoreCase)) {
    return $full.Substring($rootFull.Length).TrimStart('\', '/')
  }
  return $full
}

function Get-BuildCacheInputFiles {
  param([string[]]$Paths)

  $files = New-Object System.Collections.Generic.List[System.IO.FileInfo]
  foreach ($path in $Paths) {
    if ([string]::IsNullOrWhiteSpace($path)) {
      continue
    }
    if (-not (Test-Path -LiteralPath $path)) {
      throw "Missing cache input: $path"
    }
    $item = Get-Item -LiteralPath $path
    if ($item.PSIsContainer) {
      Get-ChildItem -LiteralPath $item.FullName -Recurse -File | ForEach-Object {
        $files.Add($_)
      }
    } else {
      $files.Add($item)
    }
  }

  return $files | Sort-Object -Property FullName -Unique
}

function Get-BuildCacheFingerprint {
  param(
    [Parameter(Mandatory = $true)][string]$Root,
    [string[]]$Paths = @(),
    [string[]]$Extra = @()
  )

  $sha = [System.Security.Cryptography.SHA256]::Create()

  function Add-FingerprintLine {
    param([string]$Line)
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Line + "`n")
    [void]$sha.TransformBlock($bytes, 0, $bytes.Length, $bytes, 0)
  }

  foreach ($item in ($Extra | Sort-Object)) {
    Add-FingerprintLine ("extra`t{0}" -f $item)
  }

  foreach ($file in (Get-BuildCacheInputFiles -Paths $Paths)) {
    $relative = Get-BuildCacheRelativePath -Root $Root -Path $file.FullName
    Add-FingerprintLine ("file`t{0}`t{1}`t{2}" -f $relative, $file.Length, $file.LastWriteTimeUtc.Ticks)
  }

  $empty = New-Object byte[] 0
  [void]$sha.TransformFinalBlock($empty, 0, 0)
  return [BitConverter]::ToString($sha.Hash).Replace("-", "").ToLowerInvariant()
}

function Test-BuildCacheStamp {
  param(
    [Parameter(Mandatory = $true)][string]$StampFile,
    [Parameter(Mandatory = $true)][string]$Fingerprint,
    [string[]]$Outputs = @()
  )

  if (-not (Test-Path -LiteralPath $StampFile)) {
    return $false
  }
  foreach ($path in $Outputs) {
    if (-not (Test-Path -LiteralPath $path)) {
      return $false
    }
  }
  $stored = (Get-Content -LiteralPath $StampFile -Raw).Trim()
  return $stored -eq $Fingerprint
}

function Write-BuildCacheStamp {
  param(
    [Parameter(Mandatory = $true)][string]$StampFile,
    [Parameter(Mandatory = $true)][string]$Fingerprint
  )

  $stampDir = Split-Path -Parent $StampFile
  New-Item -ItemType Directory -Force -Path $stampDir | Out-Null
  Set-Content -LiteralPath $StampFile -Value $Fingerprint -Encoding ASCII
}

function Get-BuildCacheFileStats {
  param([Parameter(Mandatory = $true)][string]$Path)

  if (-not (Test-Path -LiteralPath $Path)) {
    return [PSCustomObject]@{ Count = 0; Bytes = 0 }
  }
  $files = Get-ChildItem -LiteralPath $Path -Recurse -File
  $bytes = ($files | Measure-Object -Property Length -Sum).Sum
  if ($null -eq $bytes) {
    $bytes = 0
  }
  return [PSCustomObject]@{ Count = $files.Count; Bytes = [int64]$bytes }
}
