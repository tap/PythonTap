# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
#
# Authenticode-sign every unsigned binary in an assembled package (plan 4.5).
#
# usage: scripts\release\sign-windows.ps1 -Package <package-dir> -Certificate <file.pfx> -Password <password>
#
# Signs the externals (.mxe64) and every .dll, .pyd and .exe that does not already carry a valid
# signature (the runtime's own may), with SHA-256 digests and an RFC 3161 timestamp, then verifies.

param(
    [Parameter(Mandatory = $true)][string]$Package,
    [Parameter(Mandatory = $true)][string]$Certificate,
    [Parameter(Mandatory = $true)][string]$Password,
    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

$ErrorActionPreference = "Stop"

$signtool = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe" |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $signtool) { throw "signtool.exe not found (Windows SDK)" }

$files = Get-ChildItem -Path $Package -Recurse -File -Include *.mxe64, *.dll, *.pyd, *.exe |
    Where-Object { $_.Extension -eq ".mxe64" -or (Get-AuthenticodeSignature $_.FullName).Status -ne "Valid" }
if (-not $files) { throw "nothing to sign in $Package" }

# signtool takes many files per call; keep each command line well under the length limit
$batch = 50
for ($i = 0; $i -lt $files.Count; $i += $batch) {
    $chunk = $files[$i..([Math]::Min($i + $batch, $files.Count) - 1)].FullName
    & $signtool.FullName sign /f $Certificate /p $Password /fd SHA256 /tr $TimestampUrl /td SHA256 $chunk
    if ($LASTEXITCODE -ne 0) { throw "signtool sign failed ($LASTEXITCODE)" }
    & $signtool.FullName verify /pa $chunk
    if ($LASTEXITCODE -ne 0) { throw "signtool verify failed ($LASTEXITCODE)" }
}
Write-Output "signed $($files.Count) files in $Package"
