# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
#
# Run the plan 9.0 spike in Max on Windows and collect what the console said (THROWAWAY).
#
#   cmake -S . -B build-spike -DTAP_PYTHON_SPIKE=ON
#   cmake --build build-spike --config Release
#   powershell -ExecutionPolicy Bypass -File runtime-tests\spike\run_spike_windows.ps1
#   powershell -ExecutionPolicy Bypass -File runtime-tests\spike\run_spike_windows.ps1 -Only q2,q3
#
# The Windows counterpart of run_spike.py, which needs the macOS-only harness: each patcher runs in
# a fresh Max of its own, started as Explorer starts it (Max.exe "<patcher>") together with a small
# patcher that sends Max `; max clean; max quit` once the patcher under test has had its time. Max
# must be quit first, and the package (and runtime-tests\spike, for the patchers) must be in a
# Packages folder — C:\ProgramData\Max 9\Packages\PythonTap and ...\PythonTap-spike as junctions
# to this checkout and to runtime-tests\spike, say. What Max said comes from its own log
# (%APPDATA%\Cycling '74\Max 9\Logs\Max.log); the lines of interest go to results\windows.txt, each
# whole log to runtime-tests\logs\spike-windows-<session>.maxlog.
#
# Sessions: q1-control (init\tap.python.txt moved aside), q1-mapping, q1-spike-first,
# q1-tilde-first, then q2 to q6 each on its own, and q7 (q7\'s files copied into docs\ and extras\
# until Max's file database reports its update complete; the database is then asked what it made of
# them). For the run, "Restore Windows on Launch" is off and Max's crash-recovery workspaces are
# moved aside to runtime-tests\logs; Max's preference files are put back as they were afterwards.

[CmdletBinding()]
param(
    [string[]]$Only = @(),
    [string]$Max = "C:\Program Files\Cycling '74\Max 9\Max.exe"
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
# with -File, "-Only q2,q3" arrives as one string
$Only = @($Only | ForEach-Object { $_ -split ',' } | Where-Object { $_ })

$Spike    = $PSScriptRoot
$Root     = (Resolve-Path (Join-Path $Spike '..\..')).Path
$Patchers = Join-Path $Spike 'patchers'
$Q7       = Join-Path $Spike 'q7'
$Results  = Join-Path $Spike 'results'
$Logs     = Join-Path $Root 'runtime-tests\logs'
$MaxData  = Join-Path $env:APPDATA "Cycling '74\Max 9"
$MaxLog   = Join-Path $MaxData 'Logs\Max.log'
$Settings = Join-Path $MaxData 'Settings'
$Init     = Join-Path $Root 'init\tap.python.txt'
$InitAside = "$Init.spike-aside"
$Python   = Join-Path $Root 'support\python.exe'

# (session, patchers, seconds to give them before Max is told to quit)
$Sessions = @(
    @('q1-control',     @('q1-mapping'),     6),
    @('q1-mapping',     @('q1-mapping'),     6),
    @('q1-spike-first', @('q1-spike-first'), 6),
    @('q1-tilde-first', @('q1-tilde-first'), 6),
    @('q2-outlets',     @('q2-outlets'),     6),
    @('q3-dumpout',     @('q3-dumpout'),     6),
    @('q4-dispatch',    @('q4-dispatch'),    6),
    @('q5-threads',     @('q5-threads'),     12),
    @('q6-strings',     @('q6-strings'),     6),
    @('q7',             @(),                 900)
)

$Interesting = "spike|recorder|\bq[1-7]\b|tap\.python|No such object|python:|Loaded |doesn't understand|" +
               "missing arguments|obex|error\]|crash|maxdb update complete"
$Noise = "RNBO|rnbo|j\.loader|bach|Could not load package|Error 126 loading external"

function Write-QuitPatcher([int]$Seconds) {
    $path = Join-Path $Logs "spike-quit-$Seconds.maxpat"
    $ms = $Seconds * 1000
    $json = @"
{ "patcher": { "fileversion": 1, "appversion": { "major": 9, "minor": 1, "revision": 5, "architecture": "x64", "modernui": 1 },
  "classnamespace": "box", "rect": [ 900.0, 600.0, 320.0, 160.0 ],
  "boxes": [
    { "box": { "id": "obj-1", "maxclass": "comment", "text": "spike: quits Max after $Seconds s (run_spike_windows.ps1)", "numinlets": 1, "numoutlets": 0, "patching_rect": [ 20.0, 10.0, 280.0, 20.0 ] } },
    { "box": { "id": "obj-2", "maxclass": "newobj", "text": "loadbang", "numinlets": 1, "numoutlets": 1, "outlettype": [ "bang" ], "patching_rect": [ 20.0, 40.0, 60.0, 22.0 ] } },
    { "box": { "id": "obj-3", "maxclass": "newobj", "text": "delay $ms", "numinlets": 2, "numoutlets": 1, "outlettype": [ "bang" ], "patching_rect": [ 20.0, 70.0, 90.0, 22.0 ] } },
    { "box": { "id": "obj-4", "maxclass": "message", "text": "; max clean; max quit", "numinlets": 2, "numoutlets": 1, "outlettype": [ "" ], "patching_rect": [ 20.0, 100.0, 150.0, 22.0 ] } }
  ],
  "lines": [ { "patchline": { "source": [ "obj-2", 0 ], "destination": [ "obj-3", 0 ] } },
             { "patchline": { "source": [ "obj-3", 0 ], "destination": [ "obj-4", 0 ] } } ] } }
"@
    Set-Content -Path $path -Value $json -Encoding ASCII
    return $path
}

function Clear-CrashRecovery {
    $aside = Join-Path $Logs 'spike-windows-crash-recovery'
    Get-ChildItem (Join-Path $MaxData 'Crash Recovery') -Filter 'maxworkspace-*.txt' -ErrorAction SilentlyContinue |
        ForEach-Object {
            New-Item -ItemType Directory -Force $aside | Out-Null
            Move-Item -Force $_.FullName (Join-Path $aside $_.Name)
        }
}

function Install-Q7 {
    $copied = @()
    Get-ChildItem $Q7 -Recurse -File | ForEach-Object {
        $relative = $_.FullName.Substring($Q7.Length + 1)
        $destination = Join-Path $Root $relative
        if (Test-Path $destination) { throw "$destination already exists" }
        New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
        Copy-Item $_.FullName $destination
        $copied += $relative
    }
    return , $copied
}

function Remove-Q7([string[]]$Copied) {
    foreach ($relative in $Copied) { Remove-Item -Force (Join-Path $Root $relative) -ErrorAction SilentlyContinue }
    foreach ($folder in @('docs\tutorials\pythontap-tut', 'docs\tutorials', 'docs\vignettes', 'docs\topics', 'extras')) {
        $path = Join-Path $Root $folder
        if ((Test-Path $path) -and -not (Get-ChildItem $path)) { Remove-Item $path }
    }
}

function Get-Q7Database([string[]]$Copied) {
    $database = Get-ChildItem (Join-Path $MaxData 'Database') -Filter '*.maxdb' | Where-Object { $_.Name -notmatch 'lock|userdata' } |
        Sort-Object Length -Descending | Select-Object -First 1
    if (-not $database) { return @('(no file database found)') }
    $copy = Join-Path $Logs 'spike-windows-q7.maxdb'
    Copy-Item $database.FullName $copy -Force
    $names = ($Copied | ForEach-Object { Split-Path $_ -Leaf }) -join '|'
    $query = @"
import sqlite3, sys
names = sys.argv[2].split('|')
db = sqlite3.connect(sys.argv[1])
for name in names:
    stem = name
    for suffix in ('.maxvig.xml', '.maxtut.xml', '.maxpat'):
        if stem.endswith(suffix): stem = stem[: -len(suffix)]
    rows = db.execute('SELECT _name, _kind, _typedesc, _path FROM _things WHERE _name = ? OR _filename = ?', (stem, name)).fetchall()
    for row in rows: print(f"  {name}: name '{row[0]}', kind {row[1]} ({row[2]}), folder {row[3]}")
    if not rows: print(f"  {name}: NOT in the database")
"@
    $script = Join-Path $Logs 'spike-windows-q7-query.py'
    [System.IO.File]::WriteAllText($script, $query)
    $lines = @('', "Max's file database ($($database.Name)), for the files q7 put in the package:")
    $lines += & $Python $script $copy $names
    return $lines
}

function Invoke-Session([string]$Name, [string[]]$Which, [int]$Seconds) {
    Write-Host "== $Name"
    $copied = @()
    if ($Name -eq 'q1-control') { Move-Item $Init $InitAside }
    if ($Name -eq 'q7') { $copied = Install-Q7 }
    try {
        Clear-CrashRecovery
        $arguments = @()
        foreach ($patcher in $Which) {
            $path = Join-Path $Patchers "tap.python.spike-$patcher.maxpat"
            Write-Host "  opening $(Split-Path $path -Leaf)"
            $arguments += "`"$path`""
        }
        $arguments += "`"$(Write-QuitPatcher $Seconds)`""
        $started = Get-Date
        $process = Start-Process $Max -ArgumentList $arguments -PassThru
        $deadline = $started.AddSeconds($Seconds + 180)
        while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
            Start-Sleep -Seconds 2
            if ($Name -eq 'q7' -and (Test-Path $MaxLog) -and
                (Select-String -Path $MaxLog -Pattern 'maxdb update complete' -Quiet)) {
                Start-Sleep -Seconds 5
                Write-Host "  Max's file database is up to date; stopping Max"
                Stop-Process -Id $process.Id -Force
                break
            }
        }
        if (-not $process.HasExited) {
            Write-Host "  Max did not quit; stopping it"
            Stop-Process -Id $process.Id -Force
        }
        $process.WaitForExit()
        Start-Sleep -Seconds 1
    }
    finally {
        if (Test-Path $InitAside) { Move-Item $InitAside $Init }
    }
    $log = Join-Path $Logs "spike-windows-$Name.maxlog"
    Copy-Item $MaxLog $log -Force
    $lines = Get-Content $log | Where-Object {
        $_ -match $Interesting -and $_ -notmatch '\] (->|<-) ' -and $_ -notmatch $Noise
    }
    $report = @("## session $Name", '') + $lines
    if ($Name -eq 'q7') {
        $report += Get-Q7Database $copied
        Remove-Q7 $copied
    }
    $lines | ForEach-Object { Write-Host "    $_" }
    return , ($report + '')
}

if (Get-Process Max -ErrorAction SilentlyContinue) { throw 'Max is running: quit it first' }
if (-not (Test-Path (Join-Path $Root 'externals\tap.python~.mxe64'))) { throw 'the external is not built' }
$binary = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes((Join-Path $Root 'externals\tap.python~.mxe64')))
if (-not $binary.Contains('plan 9.0 spike build')) { throw 'the external has no spike in it: build with -DTAP_PYTHON_SPIKE=ON' }
if (Test-Path $InitAside) { Move-Item $InitAside $Init }
New-Item -ItemType Directory -Force $Logs, $Results | Out-Null

# Max's preference files, put back afterwards; Restore Windows on Launch off for the run
$backup = Join-Path $Logs 'spike-windows-preferences-before'
Remove-Item -Recurse -Force $backup -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $backup | Out-Null
Get-ChildItem $Settings -File | Where-Object { $_.Name -match 'maxpreferences|audioprefs|@|recentitems' } |
    ForEach-Object { Copy-Item $_.FullName $backup }
$preferencesFile = Join-Path $Settings 'maxpreferences.maxpref'
if (Test-Path $preferencesFile) {
    $text = Get-Content $preferencesFile -Raw
    $edited = $text -replace '"restorewindows"\s*:\s*1', '"restorewindows" : 0'
    if ($edited -ne $text) { [System.IO.File]::WriteAllText($preferencesFile, $edited) }  # UTF-8, no BOM
}

$report = @()
try {
    foreach ($session in $Sessions) {
        $name = $session[0]
        if ($Only.Count -and -not ($Only | Where-Object { $name -like "*$_*" })) { continue }
        $report += Invoke-Session $name $session[1] $session[2]
    }
}
finally {
    Get-ChildItem $backup -File | ForEach-Object { Copy-Item $_.FullName $Settings -Force }
    if (Test-Path $InitAside) { Move-Item $InitAside $Init }
}

$version = (Get-Item $Max).VersionInfo.ProductVersion
$os = (Get-CimInstance Win32_OperatingSystem)
$header = @("# The plan 9.0 spike in Max $version, $($os.Caption) $($os.Version) $env:PROCESSOR_ARCHITECTURE",
            '# (written by runtime-tests/spike/run_spike_windows.ps1: the lines of interest from Max''s log)', '')
$output = Join-Path $Results 'windows.txt'
[System.IO.File]::WriteAllLines($output, [string[]]($header + $report))  # UTF-8, no BOM
Write-Host "wrote $output"
