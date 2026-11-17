# run_tests.ps1 - builds and runs the logic tests with MSVC. Run it from a "Developer PowerShell for VS" in the repository folder:
#   .\tests\run_tests.ps1
$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path build\tests | Out-Null
cl /nologo /std:c++20 /EHsc /W4 /WX /I src /Fo:build\tests\ /Fe:build\tests\test_core.exe `
    tests\test_core.cpp src\core\Config.cpp src\core\Log.cpp src\core\Assert.cpp
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\tests\test_core.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cl /nologo /std:c++20 /EHsc /W4 /WX /I src /Fo:build\tests\ /Fe:build\tests\test_camera.exe `
    tests\test_camera.cpp src\scene\Camera.cpp
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\tests\test_camera.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# test_mesh: the shapes of the models (M1). /WX is left out until it has compiled clean once on this machine; then add it like the lines above.
cl /nologo /std:c++20 /EHsc /W4 /I src /Fo:build\tests\ /Fe:build\tests\test_mesh.exe `
    tests\test_mesh.cpp src\render\MeshData.cpp
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\tests\test_mesh.exe
exit $LASTEXITCODE

