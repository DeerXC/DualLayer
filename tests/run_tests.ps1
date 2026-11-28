# run_tests.ps1 - builds and runs the logic tests with MSVC (and builds the tools that are not part of the game, if any).
# Run it from a "Developer PowerShell for VS" in the repository folder (tools\check.ps1 runs it too, and finds that environment
# by itself):
#   .\tests\run_tests.ps1               every test
#   .\tests\run_tests.ps1 -Only stage   one test (the name in the table below)
# The tests are header-only except where Sources lists a .cpp of the game. /WX (warnings are errors) is on for the tests that
# have compiled clean on both machines; for the others a warning is printed and tools\check.ps1 reports it as WARN.
# Exit code: 0 when every test built and passed, otherwise the exit code of the first failure.
param([string]$Only = "")
$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path build\tests | Out-Null

# Name, sources, extra flags. /O2 for the tests that search or simulate a lot (they take seconds without it).
$tests = @(
    @{ Name = "core";      Sources = @("tests\test_core.cpp", "src\core\Config.cpp", "src\core\Log.cpp", "src\core\Assert.cpp"); Flags = @("/WX") },
    @{ Name = "camera";    Sources = @("tests\test_camera.cpp", "src\scene\Camera.cpp");                                      Flags = @("/WX") },
    @{ Name = "mesh";      Sources = @("tests\test_mesh.cpp", "src\render\MeshData.cpp");                                     Flags = @() },
    @{ Name = "trimesh";   Sources = @("tests\test_trimesh.cpp");                                                           Flags = @("/O2") },
    @{ Name = "ledger";    Sources = @("tests\test_ledger.cpp");                                                            Flags = @("/O2") },
    @{ Name = "batch";     Sources = @("tests\test_batch.cpp");                                                              Flags = @("/O2") },
    @{ Name = "trimath";   Sources = @("tests\test_trimath.cpp");                                                            Flags = @("/O2") },
    @{ Name = "ripple";    Sources = @("tests\test_ripple.cpp");                                                              Flags = @() },
    @{ Name = "player";    Sources = @("tests\test_player.cpp");                                                              Flags = @("/O2") },
    @{ Name = "layer_guard"; Sources = @("tests\test_layer_guard.cpp");                                                      Flags = @() },
    @{ Name = "effects";   Sources = @("tests\test_effects.cpp");                                                            Flags = @() },
    @{ Name = "guidance";  Sources = @("tests\test_guidance.cpp");                                                           Flags = @("/O2") },
    @{ Name = "firstminute"; Sources = @("tests\test_firstminute.cpp");                                                      Flags = @("/O2") },
    @{ Name = "map";       Sources = @("tests\test_map.cpp");                                                                Flags = @("/O2") },
    @{ Name = "lighting"; Sources = @("tests\test_lighting.cpp", "src\core\Config.cpp");                                    Flags = @("/O2") },
    @{ Name = "style";    Sources = @("tests\test_style.cpp", "src\core\Config.cpp");                                       Flags = @("/O2") },
    @{ Name = "town_player"; Sources = @("tests\test_town_player.cpp");                                                        Flags = @("/O2") },
    @{ Name = "overlay";   Sources = @("tests\test_overlay.cpp");                                                             Flags = @() },
    @{ Name = "town";      Sources = @("tests\test_town.cpp");                                                               Flags = @("/O2") },
    @{ Name = "lantern";   Sources = @("tests\test_lantern.cpp");                                                            Flags = @("/O2") },
    @{ Name = "things";    Sources = @("tests\test_things.cpp");                                                             Flags = @("/O2") },
    @{ Name = "route";     Sources = @("tests\test_route.cpp");                                                              Flags = @("/O2") }
)

# Tools that are kept in the repository but are not part of the game: they are only BUILT, so that they cannot rot unseen.
# None at the moment.
$tools = @()

function Build-One($entry, [string]$exe)
{
    foreach ($s in $entry.Sources)
    {
        if (-not (Test-Path $s)) { Write-Host "missing source: $s" -ForegroundColor Red; exit 2 }
    }
    $flags = @($entry.Flags)
    # WHY captured and written with Write-Host: whatever a PowerShell function lets through becomes part of its return value
    # (the compiler output and the exit code came back as one array, which never equals 0). Write-Host goes to the
    # information stream, which tools\check.ps1 collects with *>&1, so it can still read every line.
    $out = & cl /nologo /std:c++20 /EHsc /W4 @flags /I src /Fo:build\tests\ "/Fe:$exe" @($entry.Sources) 2>&1
    $code = [int]$LASTEXITCODE
    $out | ForEach-Object { Write-Host "$_" }
    return $code
}

$ran = 0
foreach ($t in $tests)
{
    if ($Only -ne "" -and $t.Name -ne $Only) { continue }
    $exe = "build\tests\test_" + $t.Name + ".exe"
    Write-Host ("== test " + $t.Name) -ForegroundColor Cyan
    $code = Build-One $t $exe
    if ($code -ne 0) { Write-Host ("test " + $t.Name + ": the build failed") -ForegroundColor Red; exit $code }
    $out = & ".\$exe" 2>&1
    $code = [int]$LASTEXITCODE
    $out | ForEach-Object { Write-Host "$_" }
    if ($code -ne 0) { Write-Host ("test " + $t.Name + ": FAILED") -ForegroundColor Red; exit $code }
    $ran++
}
if ($Only -eq "")
{
    foreach ($t in $tools)
    {
        Write-Host ("== build-only " + $t.Name) -ForegroundColor Cyan
        $code = Build-One $t ("build\tests\" + $t.Name + ".exe")
        if ($code -ne 0) { Write-Host ("tool " + $t.Name + ": the build failed") -ForegroundColor Red; exit $code }
    }
}
if ($ran -eq 0) { Write-Host "no test is called '$Only'" -ForegroundColor Red; exit 2 }
Write-Host ("all {0} test(s) passed" -f $ran) -ForegroundColor Green
exit 0
