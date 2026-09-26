# Instala o template de partitura do PartePlay no MuseScore.
# Copia 'musescore/templates/PartePlay/PartePlay - Arranjo.mscx' para:
#   1) o diretório de templates do usuário do MuseScore (sem admin), se existir;
#   2) a instalação do MuseScore (Portable/Program Files), subcategoria 01-General (admin).
#
# Após rodar: feche e reabra o MuseScore → Novo => a partitura aparece no assistente.

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$templateSrc = Join-Path $root "musescore\templates\PartePlay\PartePlay - Arranjo.mscx"

if (-not (Test-Path $templateSrc))
{
    throw "Template nao encontrado: $templateSrc"
}

$installedAnywhere = $false

# 1) Diretórios de templates do usuário (candidatos comuns no Windows)
$userCandidates = @(
    "$env:USERPROFILE\Documents\MuseScore4\Templates",
    "$env:APPDATA\MuseScore\MuseScore4\templates",
    "$env:APPDATA\MuseScore4\templates",
    "$env:LOCALAPPDATA\MuseScore\MuseScore4\templates",
    "$env:LOCALAPPDATA\MuseScore4\templates"
)

foreach ($dir in $userCandidates)
{
    if (Test-Path $dir)
    {
        $dest = Join-Path $dir "PartePlay"
        New-Item -ItemType Directory -Path $dest -Force | Out-Null
        Copy-Item -Path $templateSrc -Destination (Join-Path $dest "PartePlay - Arranjo.mscx") -Force
        Write-Host "> Template do usuario instalado em: $dest"
        $installedAnywhere = $true
        break
    }
}

# 2) Instalação do MuseScore (Program Files / Portable) — precisa de admin
$appCandidates = @(
    "${env:ProgramFiles}", "${env:ProgramFiles(x86)}",
    "D:\Program Files\MuseScorePortable",
    "C:\Program Files\MuseScorePortable"
)

foreach ($base in $appCandidates)
{
    if ([string]::IsNullOrWhiteSpace($base) -or -not (Test-Path $base))
        { continue }

    $templateDir = Get-ChildItem -Path $base -Recurse -Directory -Filter "templates" `
        -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName "categories.json") } `
        | Select-Object -First 1

    if ($templateDir)
    {
        $dest = Join-Path $templateDir.FullName "01-General"
        New-Item -ItemType Directory -Path $dest -Force | Out-Null
        Copy-Item -Path $templateSrc -Destination (Join-Path $dest "PartePlay - Arranjo.mscx") -Force
        Write-Host "> Template do aplicativo instalado em: $dest"
        $installedAnywhere = $true
        break
    }
}

if (-not $installedAnywhere)
{
    Write-Host "> Nenhum diretorio de templates do MuseScore foi encontrado."
    Write-Host "> Alternativa (sempre funciona): abra diretamente o arquivo:"
    Write-Host ">   $templateSrc"
    exit 1
}

Write-Host "> Concluido. Reinicie o MuseScore e escolha 'PartePlay - Arranjo' em Novo."
Write-Host "> Lembrete: o VST3 PartePlay NAO e gravado na partitura (escopo de sessao do Mixer);"
Write-Host "> abra o Mixer e anexe o efeito PartePlay ao instrumento para trabalhar."
exit 0