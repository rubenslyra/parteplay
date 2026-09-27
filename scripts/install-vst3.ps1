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

$muse = Get-Process -Name "MuseScore*" -ErrorAction SilentlyContinue
if ($muse)
{
    throw "Feche o MuseScore antes de instalar (o binario do plugin fica bloqueado)."
}

$destDir   = "C:\Program Files\Common Files\VST3"
$dest      = Join-Path $destDir "PartePlay.vst3"
$sourceBin = Join-Path $vst3.FullName "Contents\x86_64-win\PartePlay.vst3"
$destBin   = Join-Path $dest "Contents\x86_64-win\PartePlay.vst3"

New-Item -ItemType Directory -Path $destDir -Force | Out-Null

# O bundle .vst3 e um diretorio: Copy-Item sem -Recurse cria um subdir aninhado e
# mantem o binario antigo no lugar. Espelha a arvore inteira a partir do zero.
if (Test-Path -LiteralPath $dest)
{
    Remove-Item -LiteralPath $dest -Recurse -Force
}

Copy-Item -LiteralPath $vst3.FullName -Destination $dest -Recurse -Force

if (-not (Test-Path -LiteralPath $destBin))
{
    throw "Instalacao falhou: binario ausente em $destBin"
}

$hashSrc = (Get-FileHash -LiteralPath $sourceBin).Hash
$hashDst = (Get-FileHash -LiteralPath $destBin).Hash

if ($hashSrc -ne $hashDst)
{
    throw "Instalacao falhou: hash do binario instalado difere do build."
}

Write-Host "> VST3 instalado em $dest (binario confere com o build $Configuration)"