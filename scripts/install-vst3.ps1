param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))
{
    Write-Warning "Executar como Administrador para copiar para Program Files."
    exit 1
}

$vst3 = Get-ChildItem -Path "$root\build\msvc\PartePlay_artefacts\$Configuration\VST3" `
                       -Filter "PartePlay.vst3" -ErrorAction SilentlyContinue

if (-not $vst3)
{
    throw "VST3 nao encontrado em build\msvc\PartePlay_artefacts\$Configuration\VST3. Execute scripts\build.ps1 antes."
}

$destDir = "C:\Program Files\Common Files\VST3"
New-Item -ItemType Directory -Path $destDir -Force | Out-Null
Copy-Item -Path $vst3.FullName -Destination "$destDir\PartePlay.vst3" -Force

Write-Host "> VST3 instalado em $destDir\PartePlay.vst3"