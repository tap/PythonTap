# Copyright 2022-2026 Timothy Place. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.
#
# Install the embedded Python runtime for tap.python~ into <package>\support.
#
# Downloads a relocatable CPython from python-build-standalone
# (https://github.com/astral-sh/python-build-standalone), verifies it against
# the release's SHA256SUMS, unpacks it into support\, and installs the Python
# packages used by the examples (attrs, numpy).
#
# Usage (from the package root):
#   powershell -ExecutionPolicy Bypass -File scripts\install-runtime.ps1
#
# Pinned via the parameters below; override to upgrade:
#   scripts\install-runtime.ps1 -PbsRelease 20260623 -PythonSeries 3.13

param(
    [string]$PbsRelease = "20260623",
    [string]$PythonSeries = "3.13"
)

$ErrorActionPreference = "Stop"
$PipPackages = @("attrs", "numpy")

$PackageRoot = Split-Path -Parent $PSScriptRoot
$Support = Join-Path $PackageRoot "support"
$UrlPrefix = "https://github.com/astral-sh/python-build-standalone/releases/download/$PbsRelease"
$Triple = "x86_64-pc-windows-msvc"

$Work = Join-Path ([System.IO.Path]::GetTempPath()) ("tap-python-runtime-" + [System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $Work | Out-Null

try {
    Write-Host "==> Fetching asset manifest for python-build-standalone $PbsRelease"
    $sums = Join-Path $Work "SHA256SUMS"
    Invoke-WebRequest -Uri "$UrlPrefix/SHA256SUMS" -OutFile $sums

    $seriesPattern = [regex]::Escape($PythonSeries)
    $pattern = "cpython-$seriesPattern\.[0-9]+\+$PbsRelease-$Triple-install_only\.tar\.gz"
    $found = Select-String -Path $sums -Pattern $pattern -AllMatches |
        ForEach-Object { $_.Matches } | ForEach-Object { $_.Value } | Sort-Object -Unique
    if (-not $found) {
        throw "no cpython $PythonSeries install_only asset for $Triple in release $PbsRelease"
    }
    $asset = $found | Select-Object -Last 1

    Write-Host "==> Downloading $asset"
    $archive = Join-Path $Work "python.tar.gz"
    Invoke-WebRequest -Uri "$UrlPrefix/$asset" -OutFile $archive

    $expected = (Select-String -Path $sums -Pattern ([regex]::Escape($asset)) |
        Select-Object -First 1).Line.Split(" ")[0]
    $actual = (Get-FileHash -Algorithm SHA256 $archive).Hash.ToLower()
    if ($expected -ne $actual) {
        throw "SHA256 mismatch for $asset"
    }

    if (Test-Path $Support) {
        Write-Host "==> Removing existing runtime at $Support"
        Remove-Item -Recurse -Force $Support
    }

    Write-Host "==> Installing runtime into $Support"
    $extract = Join-Path $Work "extracted"
    New-Item -ItemType Directory -Path $extract | Out-Null
    tar -xzf $archive -C $extract        # bsdtar ships with Windows 10+
    Move-Item (Join-Path $extract "python") $Support

    Write-Host "==> Installing Python packages: $($PipPackages -join ', ')"
    $python = Join-Path $Support "python.exe"
    & $python -m pip install --quiet --no-warn-script-location --upgrade pip
    & $python -m pip install --quiet --no-warn-script-location @PipPackages

    Write-Host "==> Done. $(& $python --version) installed at $Support"
}
finally {
    Remove-Item -Recurse -Force $Work -ErrorAction SilentlyContinue
}
