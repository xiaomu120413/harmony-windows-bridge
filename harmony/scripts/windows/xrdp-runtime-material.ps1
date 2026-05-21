function Get-XrdpCArrayBytes {
  param(
    [Parameter(Mandatory = $true)][string]$SourceText,
    [Parameter(Mandatory = $true)][string]$Name
  )

  $pattern = "static\s+tui8\s+$([regex]::Escape($Name))\[[^\]]+\].*?\{(?<body>.*?)\};"
  $match = [regex]::Match($SourceText, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
  if (-not $match.Success) {
    throw "Unable to find xrdp keygen array: $Name"
  }

  $values = [regex]::Matches($match.Groups["body"].Value, "0x([0-9a-fA-F]{1,2})")
  $bytes = New-Object byte[] $values.Count
  for ($i = 0; $i -lt $values.Count; $i++) {
    $bytes[$i] = [Convert]::ToByte($values[$i].Groups[1].Value, 16)
  }
  return $bytes
}

function ConvertTo-LittleEndianBytes {
  param(
    [Parameter(Mandatory = $true)][byte[]]$BigEndianBytes,
    [Parameter(Mandatory = $true)][int]$Length
  )

  $result = New-Object byte[] $Length
  $source = [byte[]]$BigEndianBytes.Clone()
  [Array]::Reverse($source)
  $copy = [Math]::Min($source.Length, $Length)
  [Array]::Copy($source, 0, $result, 0, $copy)
  return $result
}

function ConvertTo-PositiveBigInteger {
  param([Parameter(Mandatory = $true)][byte[]]$LittleEndianBytes)

  $positive = New-Object byte[] ($LittleEndianBytes.Length + 1)
  [Array]::Copy($LittleEndianBytes, 0, $positive, 0, $LittleEndianBytes.Length)
  return [System.Numerics.BigInteger]::new($positive)
}

function ConvertFrom-PositiveBigInteger {
  param(
    [Parameter(Mandatory = $true)][System.Numerics.BigInteger]$Value,
    [Parameter(Mandatory = $true)][int]$Length
  )

  $raw = $Value.ToByteArray()
  $result = New-Object byte[] $Length
  $copy = [Math]::Min($raw.Length, $Length)
  [Array]::Copy($raw, 0, $result, 0, $copy)
  return $result
}

function Format-XrdpHexLine {
  param(
    [Parameter(Mandatory = $true)][string]$Name,
    [Parameter(Mandatory = $true)][byte[]]$Data
  )

  return $Name + "=" + (($Data | ForEach-Object { "0x{0:x2}" -f $_ }) -join ",")
}

function New-XrdpRsaKeysIni {
  param(
    [Parameter(Mandatory = $true)][string]$RepoRoot,
    [Parameter(Mandatory = $true)][string]$DestinationPath
  )

  Add-Type -AssemblyName System.Numerics

  $keygenPath = Join-Path $RepoRoot "harmony/third_party/xrdp/keygen/keygen.c"
  $sourceText = Get-Content -LiteralPath $keygenPath -Raw
  $ppkN = (Get-XrdpCArrayBytes $sourceText "g_ppk_n")[0..63]
  $ppkD = (Get-XrdpCArrayBytes $sourceText "g_ppk_d")[0..63]
  $testKey = [byte[]](Get-XrdpCArrayBytes $sourceText "g_testkey2048")

  $rsa = [System.Security.Cryptography.RSACryptoServiceProvider]::new(2048)
  $parameters = $rsa.ExportParameters($true)

  $publicExponent = ConvertTo-LittleEndianBytes $parameters.Exponent 4
  $publicModulus = ConvertTo-LittleEndianBytes $parameters.Modulus 256
  $privateExponent = ConvertTo-LittleEndianBytes $parameters.D 256

  [Array]::Copy($publicExponent, 0, $testKey, 32, 4)
  [Array]::Copy($publicModulus, 0, $testKey, 36, 256)

  $md5 = [System.Security.Cryptography.MD5]::Create()
  $digest = $md5.ComputeHash($testKey[0..299])
  $md5Final = New-Object byte[] 64
  for ($i = 0; $i -lt $md5Final.Length; $i++) {
    $md5Final[$i] = 0xff
  }
  [Array]::Copy($digest, 0, $md5Final, 0, $digest.Length)
  $md5Final[16] = 0
  $md5Final[62] = 1
  $md5Final[63] = 0

  $signature = [System.Numerics.BigInteger]::ModPow(
    (ConvertTo-PositiveBigInteger $md5Final),
    (ConvertTo-PositiveBigInteger $ppkD),
    (ConvertTo-PositiveBigInteger $ppkN))
  $signatureBytes = ConvertFrom-PositiveBigInteger $signature 64

  $lines = @(
    "[keys]",
    (Format-XrdpHexLine "pub_exp" $publicExponent),
    (Format-XrdpHexLine "pub_mod" $publicModulus),
    (Format-XrdpHexLine "pub_sig" $signatureBytes),
    (Format-XrdpHexLine "pri_exp" $privateExponent)
  )
  Set-Content -LiteralPath $DestinationPath -Value $lines -Encoding ASCII
}

function Copy-XrdpRuntimeConfigExtras {
  param(
    [Parameter(Mandatory = $true)][string]$RepoRoot,
    [Parameter(Mandatory = $true)][string]$DestinationDir
  )

  $keymapSource = Join-Path $RepoRoot "harmony/third_party/xrdp/instfiles"
  Get-ChildItem -LiteralPath $keymapSource -Filter "km-*.toml" -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $DestinationDir -Force
  }

  $keymapNames = Join-Path $keymapSource "keymap-names.txt"
  if (Test-Path -LiteralPath $keymapNames) {
    Copy-Item -LiteralPath $keymapNames -Destination $DestinationDir -Force
  }

  New-XrdpRsaKeysIni -RepoRoot $RepoRoot -DestinationPath (Join-Path $DestinationDir "rsakeys.ini")
}
