param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    # Preset de configure/build. Tem de ser o mesmo nos dois passos: compilar com
    # um preset e instalar de outro foi o que deixou um plugin obsoleto no host
    # sem ninguem perceber.
    [string]$Preset = $(if ($env:PARTEPLAY_PRESET) { $env:PARTEPLAY_PRESET } else { "msvc" }),

    # Nao instala no VST3 do sistema; so compila e roda os testes.
    [switch]$NoInstall
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try
{
    Write-Host "> Configurando (preset: $Preset)"
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "Falha na configuracao do CMake (exit $LASTEXITCODE)" }

    Write-Host "> Compilando ($Configuration)"
    cmake --build --preset $Preset --config $Configuration
    if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao (exit $LASTEXITCODE)" }

    if ($NoInstall)
    {
        Write-Host "> Build concluido. Instalacao ignorada (-NoInstall)."
        exit 0
    }

    Write-Host "> Instalando em 'C:\Program Files\Common Files\VST3'"
    & "$PSScriptRoot\install-vst3.ps1" -Configuration $Configuration -Preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "Falha ao instalar o VST3 (exit $LASTEXITCODE)" }

    Write-Host "> Concluido."
}
finally
{
    Pop-Location
}
