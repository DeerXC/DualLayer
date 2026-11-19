# check.ps1 - one command for everything that a machine can check, so that only the looks are left for the eye.
#   .\tools\check.ps1                                  run it all, change nothing
#   .\tools\check.ps1 -Commit "feat(scene): ..."       ... and, only if everything passed: git add, git audit, git commit
#   .\tools\check.ps1 -Commit "feat(scene): ..." -Push ... and git push
# Run it from any PowerShell in any folder: it finds the project and the Visual Studio build tools by itself.
# Steps:  1 build tools  2 logic tests (tests\run_tests.ps1)  3 Debug and Release build  4 the game starts, walks, jumps and answers
#         the keys F / Tab / N and closes cleanly  5 repository safety (nothing that must not be uploaded)  6 commit / push (only on request)
# Step 4 presses keys in the game window for about 35 seconds: do not touch the keyboard or the mouse until it says it is done.
# Output:  a table at the end; the exit code is 0 only when nothing failed.
param(
    [switch]$SkipBuild,
    [switch]$SkipSmoke,
    [string]$Commit = "",
    [switch]$Push
)

$ErrorActionPreference = "Continue"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$results = New-Object System.Collections.ArrayList

function Add-Result([string]$name, [string]$status, [string]$detail)
{
    [void]$results.Add([pscustomobject]@{ Step = $name; Status = $status; Detail = $detail })
    $color = "Green"
    if ($status -eq "FAIL") { $color = "Red" }
    if ($status -eq "SKIP" -or $status -eq "WARN") { $color = "Yellow" }
    Write-Host ("[{0}] {1}  {2}" -f $status, $name, $detail) -ForegroundColor $color
}

function Failed()
{
    return @($results | Where-Object { $_.Status -eq "FAIL" }).Count -gt 0
}

# ---- 1. the Visual Studio build tools (cl, msbuild)
Write-Host "== 1. build tools" -ForegroundColor Cyan
if (-not (Get-Command cl -ErrorAction SilentlyContinue))
{
    $vs = $null
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere)
    {
        $vs = (& $vswhere -latest -prerelease -products * -property installationPath | Select-Object -First 1)
    }
    if (-not $vs -and (Test-Path "C:\Program Files\Microsoft Visual Studio\18\Community"))
    {
        $vs = "C:\Program Files\Microsoft Visual Studio\18\Community"
    }
    if ($vs)
    {
        $devShell = Join-Path $vs "Common7\Tools\Launch-VsDevShell.ps1"
        if (Test-Path $devShell)
        {
            & $devShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
            Set-Location $root
        }
    }
}
$haveCl = [bool](Get-Command cl -ErrorAction SilentlyContinue)
$haveMsbuild = [bool](Get-Command msbuild -ErrorAction SilentlyContinue)
if ($haveCl -and $haveMsbuild) { Add-Result "build tools" "OK" "cl and msbuild found" }
else { Add-Result "build tools" "FAIL" ("cl: {0}, msbuild: {1}. Open a Developer PowerShell for VS and run this again." -f $haveCl, $haveMsbuild) }

# ---- 2. the logic tests
Write-Host "== 2. logic tests" -ForegroundColor Cyan
if ($haveCl)
{
    $testOut = & .\tests\run_tests.ps1 2>&1 | ForEach-Object { "$_" }
    $testExit = $LASTEXITCODE
    $summary = @($testOut | Where-Object { $_ -match "checks passed|FAIL|error C|warning C" })
    $summary | ForEach-Object { Write-Host "   $_" }
    if ($testExit -eq 0) { Add-Result "logic tests" "OK" "run_tests.ps1 exit code 0" }
    else { Add-Result "logic tests" "FAIL" ("run_tests.ps1 exit code {0}; read the lines above" -f $testExit) }
}
else
{
    Add-Result "logic tests" "SKIP" "no build tools"
}

# ---- 3. the two builds
function Build-Config([string]$config)
{
    $out = & msbuild DualLayer.vcxproj /m /nologo /v:minimal "/p:Configuration=$config" /p:Platform=x64 2>&1 | ForEach-Object { "$_" }
    $code = $LASTEXITCODE
    $errors = @($out | Where-Object { $_ -match ": error " })
    $warnings = @($out | Where-Object { $_ -match ": warning " })
    if ($code -ne 0)
    {
        $errors | Select-Object -First 8 | ForEach-Object { Write-Host "   $_" }
        Add-Result "build $config" "FAIL" ("msbuild exit code {0}, {1} error(s), {2} warning(s)" -f $code, $errors.Count, $warnings.Count)
    }
    elseif ($warnings.Count -gt 0)
    {
        $warnings | Select-Object -First 8 | ForEach-Object { Write-Host "   $_" }
        Add-Result "build $config" "WARN" ("builds, but {0} warning(s): fix them (Release treats them as errors)" -f $warnings.Count)
    }
    else
    {
        Add-Result "build $config" "OK" "0 errors, 0 warnings"
    }
}

Write-Host "== 3. build" -ForegroundColor Cyan
if ($SkipBuild) { Add-Result "build" "SKIP" "-SkipBuild" }
elseif (-not $haveMsbuild) { Add-Result "build" "SKIP" "no build tools" }
else
{
    Build-Config "Debug"
    Build-Config "Release"
}

# ---- 4. the game starts, walks, jumps, answers F, Tab and N, and closes cleanly
if (-not ([System.Management.Automation.PSTypeName]'DlWin32').Type)
{
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class DlWin32
{
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int w, int h, bool repaint);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
}
"@
}

function Press-Key([byte]$vk)
{
    [DlWin32]::keybd_event($vk, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 120
    [DlWin32]::keybd_event($vk, 0, 2, [UIntPtr]::Zero)
}

function Hold-Key([byte]$vk, [int]$milliseconds)
{
    [DlWin32]::keybd_event($vk, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds $milliseconds
    [DlWin32]::keybd_event($vk, 0, 2, [UIntPtr]::Zero)
}

function Get-Title($proc)
{
    $proc.Refresh()
    return $proc.MainWindowTitle
}

# the camera position from the title bar ("camera x y z"), or $null
function Get-CameraPos($proc)
{
    $m = [regex]::Match((Get-Title $proc), 'camera (-?\d+\.\d) (-?\d+\.\d) (-?\d+\.\d)')
    if (-not $m.Success) { return $null }
    $inv = [System.Globalization.CultureInfo]::InvariantCulture
    return @([double]::Parse($m.Groups[1].Value, $inv), [double]::Parse($m.Groups[2].Value, $inv), [double]::Parse($m.Groups[3].Value, $inv))
}

function Wait-Title($proc, [string]$text, [double]$seconds)
{
    $end = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $end)
    {
        if ($proc.HasExited) { return $false }
        if ((Get-Title $proc) -like "*$text*") { return $true }
        Start-Sleep -Milliseconds 250
    }
    return $false
}

Write-Host "== 4. the game (do not touch the keyboard or the mouse)" -ForegroundColor Cyan
$exe = Join-Path $root "build\x64\Debug\DualLayer.exe"
$jumpTried = $false
if ($SkipSmoke) { Add-Result "game" "SKIP" "-SkipSmoke" }
elseif (Failed) { Add-Result "game" "SKIP" "an earlier step failed" }
elseif (-not (Test-Path $exe)) { Add-Result "game" "SKIP" "no Debug build found" }
else
{
    $proc = Start-Process -FilePath $exe -WorkingDirectory $root -PassThru
    $gotWindow = $false
    for ($i = 0; $i -lt 40; $i++)
    {
        Start-Sleep -Milliseconds 250
        if ($proc.HasExited) { break }
        if ((Get-Title $proc) -like "*layer A*") { $gotWindow = $true; break }
    }
    if (-not $gotWindow)
    {
        $state = "still running without a title"
        if ($proc.HasExited) { $state = "exited with code " + $proc.ExitCode }
        Add-Result "game starts" "FAIL" ("no window with 'layer A' in 10 s ({0}); read logs\dual.log" -f $state)
        if (-not $proc.HasExited) { $proc.Kill() }
    }
    else
    {
        $title = Get-Title $proc
        if ($title -like "*NVIDIA*") { Add-Result "game starts (Q-1)" "OK" "NVIDIA adapter, layer A, collide" }
        else { Add-Result "game starts (Q-1)" "WARN" ("not the NVIDIA adapter? title: {0}" -f $title) }

        # the keys only go to the game while its window is in front; otherwise they would land in another program
        $hwnd = $proc.MainWindowHandle
        [void][DlWin32]::SetForegroundWindow($hwnd)
        Start-Sleep -Milliseconds 400
        if ([DlWin32]::GetForegroundWindow() -ne $hwnd)
        {
            Add-Result "keys (Q-2 .. Q-5, Q-14 .. Q-18)" "SKIP" "the game window is not in front; run it again and do not click elsewhere"
        }
        else
        {
            # ---- walking (Q-15 .. Q-18): the body starts in layer A, at x = 0, z = -3, on the floor
            $ok = (Get-Title $proc) -like "*walk*"
            Add-Result "walking is the default (Q-15)" $(if ($ok) { "OK" } else { "FAIL" }) "the title bar says walk"
            Start-Sleep -Milliseconds 700
            $p0 = Get-CameraPos $proc
            Hold-Key 0x57 1200   # W held for 1.2 s: about 4.8 m ahead, along x = 0 (free of objects in layer A)
            Start-Sleep -Milliseconds 900
            $p1 = Get-CameraPos $proc
            if ($p0 -and $p1)
            {
                $ahead = $p1[2] - $p0[2]
                $ok = ($ahead -gt 2.0) -and ($p1[1] -gt 0.3) -and ($p1[1] -lt 0.8)
                Add-Result "W walks on the floor (Q-16)" $(if ($ok) { "OK" } else { "FAIL" }) ("{0:N1} m ahead, eye height {1:N1} m (about 0.5 is standing)" -f $ahead, $p1[1])
            }
            else { Add-Result "W walks on the floor (Q-16)" "FAIL" "no camera position in the title bar" }
            Press-Key 0x20       # Space: a jump (the height is checked in the log below)
            $jumpTried = $true
            Start-Sleep -Milliseconds 1500
            Press-Key 0x46       # F: flying
            $ok = Wait-Title $proc "fly" 2
            Add-Result "F -> fly (Q-18)" $(if ($ok) { "OK" } else { "FAIL" }) "the title bar says fly"
            Press-Key 0x46       # F: walking again
            $ok = Wait-Title $proc "walk" 2
            Add-Result "F -> walk (Q-18)" $(if ($ok) { "OK" } else { "FAIL" }) "the title bar says walk"
            Start-Sleep -Milliseconds 1000

            # ---- layers and collision
            Press-Key 0x09   # Tab: the ripple to B
            $ok = Wait-Title $proc "layer B" 3
            Add-Result "Tab -> layer B (Q-2)" $(if ($ok) { "OK" } else { "FAIL" }) "the title bar says layer B"
            Start-Sleep -Milliseconds 1500
            Press-Key 0x09   # Tab: back to A
            $ok = Wait-Title $proc "layer A" 3
            Add-Result "Tab -> layer A (Q-3)" $(if ($ok) { "OK" } else { "FAIL" }) "the title bar says layer A"
            Start-Sleep -Milliseconds 1500
            Press-Key 0x09   # Tab, and again after 0.3 s: the ripple turns round and ends on A
            Start-Sleep -Milliseconds 300
            Press-Key 0x09
            Start-Sleep -Milliseconds 2000
            $ok = (Get-Title $proc) -like "*layer A*"
            Add-Result "Tab twice quickly (Q-4)" $(if ($ok) { "OK" } else { "FAIL" }) "the ripple turned round and the end is layer A"
            Press-Key 0x09   # Tab, and the window is resized during the ripple
            Start-Sleep -Milliseconds 250
            [void][DlWin32]::MoveWindow($hwnd, 40, 40, 1000, 700, $true)
            Start-Sleep -Milliseconds 2500
            $alive = -not $proc.HasExited
            $ok = $alive -and ((Get-Title $proc) -like "*layer B*")
            Add-Result "resize during a ripple (Q-5)" $(if ($ok) { "OK" } else { "FAIL" }) "no crash, the ripple finished on layer B"
            if ($alive)
            {
                Press-Key 0x09   # back to A
                Start-Sleep -Milliseconds 2000
                Press-Key 0x4E   # N: noclip
                $ok = Wait-Title $proc "noclip" 2
                Add-Result "N -> noclip (Q-14)" $(if ($ok) { "OK" } else { "FAIL" }) "the title bar says noclip"
                Press-Key 0x4E   # N: collide again
                $ok = Wait-Title $proc "collide" 2
                Add-Result "N -> collide (Q-14)" $(if ($ok) { "OK" } else { "FAIL" }) "the title bar says collide"
                Start-Sleep -Milliseconds 1500
            }
        }
        if (-not $proc.HasExited)
        {
            [void]$proc.CloseMainWindow()
            if (-not $proc.WaitForExit(6000)) { $proc.Kill(); Add-Result "game closes" "FAIL" "it did not close in 6 s" }
            elseif ($proc.ExitCode -ne 0) { Add-Result "game closes" "FAIL" ("exit code {0}" -f $proc.ExitCode) }
            else { Add-Result "game closes" "OK" "exit code 0" }
        }
        else
        {
            Add-Result "game closes" "FAIL" ("the game exited by itself with code {0}" -f $proc.ExitCode)
        }
    }
    # what the game wrote about itself
    $logPath = Join-Path $root "logs\dual.log"
    if (Test-Path $logPath)
    {
        $log = @(Get-Content $logPath)
        $errs = @($log | Where-Object { $_ -match "\[ERROR\]" })
        $warns = @($log | Where-Object { $_ -match "\[WARN \]" })
        $hasRipple = @($log | Where-Object { $_ -match "ripple pictures: 2 x" }).Count -gt 0
        $hasShutdown = @($log | Where-Object { $_ -match "\[Main\] shutdown" }).Count -gt 0
        if ($errs.Count -gt 0)
        {
            $errs | Select-Object -First 5 | ForEach-Object { Write-Host "   $_" }
            Add-Result "log" "FAIL" ("{0} ERROR line(s) in logs\dual.log" -f $errs.Count)
        }
        elseif (-not $hasShutdown) { Add-Result "log" "FAIL" "no 'shutdown' line: the game did not end normally" }
        elseif (-not $hasRipple) { Add-Result "log" "WARN" "no 'ripple pictures' line: the ripple did not run on the GPU (keys skipped?)" }
        else
        {
            if ($warns.Count -gt 0) { Add-Result "log" "WARN" ("0 errors, {0} warning(s): read them in logs\dual.log" -f $warns.Count) }
            else { Add-Result "log" "OK" "0 errors, 0 warnings, the ripple pictures were made" }
        }
        if ($jumpTried)
        {
            # the jump height is reported by the game when the jump ends: the feet rose [jump_height] metres (2.0 by default)
            $peaks = @($log | Where-Object { $_ -match "jump peak [0-9.]+ m" })
            if ($peaks.Count -eq 0) { Add-Result "Space jumps (Q-17)" "FAIL" "no 'jump peak' line in logs\dual.log" }
            else
            {
                $m = [regex]::Match($peaks[$peaks.Count - 1], "jump peak ([0-9.]+) m")
                $peak = [double]::Parse($m.Groups[1].Value, [System.Globalization.CultureInfo]::InvariantCulture)
                $ok = ($peak -gt 1.7) -and ($peak -lt 2.3)
                Add-Result "Space jumps (Q-17)" $(if ($ok) { "OK" } else { "FAIL" }) ("the feet rose {0:N2} m (2.0 by default)" -f $peak)
            }
        }
    }
    else { Add-Result "log" "FAIL" "logs\dual.log was not written" }
}

# ---- 5. repository safety: nothing that must not be uploaded
Write-Host "== 5. repository" -ForegroundColor Cyan
$git = Get-Command git -ErrorAction SilentlyContinue
if (-not $git) { Add-Result "repository" "SKIP" "git not found" }
elseif (-not (Test-Path (Join-Path $root ".git"))) { Add-Result "repository" "SKIP" "not a git repository" }
else
{
    # tracked files and untracked files that are not ignored: exactly what 'git add -A' would take
    $files = @(& git ls-files --cached --others --exclude-standard 2>$null)
    $bad = @($files | Where-Object {
            $_ -match '^(build|logs|\.vs|x64|Debug|Release)/' -or
            $_ -match '\.(exe|pdb|obj|ilk|zip|dll)$' -or
            $_ -match '^docs/understanding/' -or
            $_ -match '^config/local\.ini$'
        })
    $big = @()
    foreach ($f in $files)
    {
        $p = Join-Path $root $f
        if ((Test-Path $p -PathType Leaf) -and ((Get-Item $p).Length -gt 50MB)) { $big += $f }
    }
    if ($bad.Count -gt 0)
    {
        $bad | Select-Object -First 8 | ForEach-Object { Write-Host "   $_" }
        Add-Result "nothing forbidden" "FAIL" ("{0} file(s) that must not be uploaded (build output, logs, private local files, local config)" -f $bad.Count)
    }
    elseif ($big.Count -gt 0) { Add-Result "nothing forbidden" "FAIL" ("{0} file(s) over 50 MB: {1}" -f $big.Count, ($big -join ", ")) }
    else { Add-Result "nothing forbidden" "OK" ("{0} files would be uploaded; none is build output, a log, a private file or over 50 MB" -f $files.Count) }
    $hooks = (& git config --global core.hooksPath 2>$null)
    if ($hooks) { Add-Result "hooks" "OK" ("core.hooksPath = {0}" -f $hooks) }
    else { Add-Result "hooks" "WARN" "core.hooksPath is not set: the pre-commit, commit-msg and pre-push checks would not run" }
    $porcelain = @(& git status --porcelain 2>$null)
    Write-Host ("   {0} changed path(s):" -f $porcelain.Count)
    $porcelain | Select-Object -First 30 | ForEach-Object { Write-Host "   $_" }
}

# ---- 6. commit and push, only on request and only when everything passed
if ($Commit -ne "")
{
    Write-Host "== 6. commit" -ForegroundColor Cyan
    if (Failed) { Add-Result "commit" "SKIP" "a step failed: nothing was committed" }
    else
    {
        $okMessage = ($Commit -match '^(feat|fix|docs|style|refactor|perf|test|build|ci|chore|revert)(\([a-z0-9._-]+\))?: \S.*[^.]$') -and ($Commit.Length -le 72)
        if (-not $okMessage)
        {
            Add-Result "commit message" "FAIL" ("not 'type(scope): subject' within 72 characters and without a final period: {0}" -f $Commit)
        }
        else
        {
            & git add -A
            & git audit
            $auditExit = $LASTEXITCODE
            if ($auditExit -ne 0) { Add-Result "git audit" "FAIL" ("exit code {0}: nothing was committed" -f $auditExit) }
            else
            {
                & git commit -m $Commit
                if ($LASTEXITCODE -ne 0) { Add-Result "git commit" "FAIL" ("exit code {0}" -f $LASTEXITCODE) }
                else
                {
                    Add-Result "git commit" "OK" $Commit
                    if ($Push)
                    {
                        & git push
                        if ($LASTEXITCODE -ne 0) { Add-Result "git push" "FAIL" ("exit code {0}: read the message above" -f $LASTEXITCODE) }
                        else { Add-Result "git push" "OK" "pushed" }
                    }
                }
            }
        }
    }
}

# ---- the table
Write-Host ""
Write-Host "== result" -ForegroundColor Cyan
$results | Format-Table -AutoSize -Wrap | Out-String | Write-Host
$failed = @($results | Where-Object { $_.Status -eq "FAIL" }).Count
$warned = @($results | Where-Object { $_.Status -eq "WARN" }).Count
if ($failed -gt 0) { Write-Host ("{0} step(s) FAILED" -f $failed) -ForegroundColor Red; exit 1 }
if ($warned -gt 0) { Write-Host ("passed, with {0} warning(s)" -f $warned) -ForegroundColor Yellow; exit 0 }
Write-Host "all passed" -ForegroundColor Green
exit 0
