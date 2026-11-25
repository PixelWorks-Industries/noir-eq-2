$ErrorActionPreference = 'Stop'
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target NOIR_EQ_VST3 --parallel
Write-Host "Build complete. Find the VST3 bundle under build\NOIR_EQ_artefacts\Release\VST3\"
