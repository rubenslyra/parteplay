param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Continue"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

Write-Host "> Configurando CMake (preset: msvc)"
cmake --preset msvc
if ($LASTEXITCODE -ne 0) { throw "Falha na configuracao do CMake (exit $LASTEXITCODE)" }

Write-Host "> Compilando ($Configuration)"
cmake --build --preset msvc --config $Configuration
if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao (exit $LASTEXITCODE)" }

Write-Host "> Instalando VST3 em C:\Program Files\Common Files\VST3"
& "$PSScriptRoot\install-vst3.ps1" -Configuration $Configuration
if ($LASTEXITCODE -ne 0) { throw "Falha ao instalar o VST3 (exit $LASTEXITCODE)" }

Write-Host "> Concluido."