# Copyright 2022-2026 Timothy Place. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.
#
# Install the embedded Python runtime for tap.python~ into <package>\support (Windows).
#
# Downloads a relocatable CPython from python-build-standalone
# (https://github.com/astral-sh/python-build-standalone), verifies it against the SHA256
# committed in scripts\runtime.lock, unpacks it into support\, and installs the Python packages
# used by the examples (attrs, numpy) from scripts\requirements.lock, hashes enforced. What gets
# installed is exactly what those files name; to upgrade, regenerate them with
# scripts\update-locks.py and commit the result.
#
# Re-running replaces the runtime. The previous one is kept until the new one is fully
# installed and restored if anything fails. Packages you added yourself with pip are not
# carried over; reinstall them afterwards.
#
# Usage (from the package root):
#   powershell -ExecutionPolicy Bypass -File scripts\install-runtime.ps1

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$PackageRoot      = Split-Path -Parent $PSScriptRoot
$Support          = Join-Path $PackageRoot "support"
$Previous         = Join-Path $PackageRoot "support.previous"
$RuntimeLock      = Join-Path $PSScriptRoot "runtime.lock"
$RequirementsLock = Join-Path $PSScriptRoot "requirements.lock"
$Triple           = "x86_64-pc-windows-msvc"

# $ErrorActionPreference does not apply to native programs: check their exit codes.
function Invoke-Native {
    param([string]$What, [scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$What failed (exit code $LASTEXITCODE)"
    }
}

# The pinned release, asset name and SHA256 for our triple, from runtime.lock.
$lock = Get-Content $RuntimeLock | Where-Object { $_ -and -not $_.StartsWith("#") } |
    ForEach-Object { , ($_ -split '\s+') }
$PbsRelease = ($lock | Where-Object { $_[0] -eq "release" } | Select-Object -First 1)[1]
$entry = $lock | Where-Object { $_[0] -eq $Triple } | Select-Object -First 1
if (-not $PbsRelease -or -not $entry) {
    throw "scripts\runtime.lock has no release or no entry for $Triple"
}
$asset    = $entry[1]
$expected = $entry[2]
$UrlPrefix = "https://github.com/astral-sh/python-build-standalone/releases/download/$PbsRelease"

$Work = Join-Path ([System.IO.Path]::GetTempPath()) ("tap-python-runtime-" + [System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $Work | Out-Null
$swapped   = $false   # this run moved the old runtime aside
$installed = $false

try {
    # Download, verify and unpack in the scratch folder before touching support\.
    Write-Host "==> Downloading $asset"
    $archive = Join-Path $Work "python.tar.gz"
    Invoke-WebRequest -Uri "$UrlPrefix/$asset" -OutFile $archive

    $actual = (Get-FileHash -Algorithm SHA256 $archive).Hash.ToLower()
    if ($expected -ne $actual) {
        throw "SHA256 mismatch for $asset`n  expected $expected (scripts\runtime.lock)`n  got      $actual"
    }

    $extract = Join-Path $Work "extracted"
    New-Item -ItemType Directory -Path $extract | Out-Null
    Invoke-Native "Unpacking $asset" { tar -xzf $archive -C $extract }   # bsdtar ships with Windows 10+

    # Swap: keep the previous runtime until the new one is complete (restored below on failure).
    if (Test-Path $Previous) {
        Remove-Item -Recurse -Force $Previous
    }
    if (Test-Path $Support) {
        Write-Host "==> Moving the existing runtime aside"
        Move-Item $Support $Previous
        $swapped = $true
    }
    Write-Host "==> Installing runtime into $Support"
    Move-Item (Join-Path $extract "python") $Support

    Write-Host "==> Installing Python packages from scripts\requirements.lock"
    $python = Join-Path $Support "python.exe"
    Invoke-Native "Installing Python packages" {
        & $python -m pip install --quiet --no-warn-script-location --disable-pip-version-check `
            --require-hashes --only-binary :all: -r $RequirementsLock
    }

    $installed = $true
    if (Test-Path $Previous) {
        Remove-Item -Recurse -Force $Previous
    }
    Write-Host "==> Done. $(& $python --version) installed at $Support"
}
finally {
    if ($swapped -and -not $installed -and (Test-Path $Previous)) {
        Write-Host "==> Install failed; restoring the previous runtime"
        if (Test-Path $Support) {
            Remove-Item -Recurse -Force $Support
        }
        Move-Item $Previous $Support
    }
    Remove-Item -Recurse -Force $Work -ErrorAction SilentlyContinue
}
