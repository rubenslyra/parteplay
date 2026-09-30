param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    # Preset de configure/build. Precisa bater com o preset usado para compilar,
    # senao o script copia um binario de outra arvore de build sem avisar - que e
    # como um plugin de tres dias atras chega ao host sem ninguem perceber.
    #
    # O padrao e msvc-2026 porque e o toolchain deste projeto em Windows. O preset
    # `msvc` (VS2022) continua no CMakePresets.json por causa da CI, que roda em
    # windows-2022 e usa aquele toolchain; ele nao e o padrao local. Para usar
    # outro, $env:PARTEPLAY_PRESET ou -Preset.
    [string]$Preset = $(if ($env:PARTEPLAY_PRESET) { $env:PARTEPLAY_PRESET } else { "msvc-2026" }),

    # Instala sem rodar a suite de testes de dominio.
    [switch]$SkipTests,

    # Instala mesmo que o bundle seja mais antigo que os fontes. Existe para o
    # caso de o relogio do disco estar errado; o padrao e recusar.
    [switch]$AllowStale
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot

# Arquivos que o Windows e ferramentas de terceiros deixam dentro de um diretorio
# e que NAO fazem parte de um bundle VST3. A spec do bundle nao permite arquivo
# avulso na raiz, e um .gitignore nao protege o zip: se o lixeira estiver na arvore
# de build, ela vai para o pacote publicado. Lista fechada, nao curinga.
$litter = @("desktop.ini", "Thumbs.db", ".DS_Store", "ehthumbs.db")

# Prefixo para encurtar os caminhos nas mensagens. Nota de sintaxe: dentro de
# "$()" o PowerShell 5.1 nao aceita aspas duplas aninhadas, entao as comparacoes
# usam a variavel e aspas simples. Nao voltar a escrever "$root\" dentro de uma
# string: o parser acusa "cadeia de caracteres nao tem o terminador".
$prefix = $root + '\'

$vst3 = Join-Path $root "build\$Preset\PartePlay_artefacts\$Configuration\VST3\PartePlay.vst3"

if (-not (Test-Path -LiteralPath $vst3))
{
    throw "VST3 nao encontrado em '$vst3'. Rode scripts\build.ps1 -Preset $Preset antes, " +
          "ou use o mesmo preset com que compilou."
}

# --- Higiene do bundle: nada de lixeira entra no que vai para o host ------------
$found = @()
foreach ($name in $litter)
{
    $found += Get-ChildItem -Path $vst3 -Recurse -Force -Filter $name -ErrorAction SilentlyContinue
}

if ($found)
{
    Write-Host "> Limpando $($found.Count) arquivo(s) de lixeira do bundle"
    foreach ($item in $found)
    {
        Write-Host "    - $($item.FullName.Replace($prefix, ''))"
        Remove-Item -LiteralPath $item.FullName -Force
    }
}

# --- Antiquidade: o binario tem de ser mais novo que os fontes ------------------
$binario = Join-Path $vst3 "Contents\x86_64-win\PartePlay.vst3"

if (-not (Test-Path -LiteralPath $binario))
{
    throw "Bundle sem binario em '$binario'. O build esta incompleto - rode de novo."
}

$binarioTime = (Get-Item -LiteralPath $binario).LastWriteTime

$maisNovoFonte = Get-ChildItem -Path "$root\Source" -Recurse -File |
                 Sort-Object LastWriteTime -Descending |
                 Select-Object -First 1

if ($maisNovoFonte -and $binarioTime -lt $maisNovoFonte.LastWriteTime)
{
    $aviso = "Bundle mais ANTIGO que os fontes:" +
             "`n    binario : $($binarioTime.ToString('yyyy-MM-dd HH:mm:ss'))" +
             "`n    fonte   : $($maisNovoFonte.Name) em $($maisNovoFonte.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss'))"

    if (-not $AllowStale)
    {
        throw "$aviso`nO host passaria a carregar um plugin desatualizado. " +
              "Recompile com scripts\build.ps1 -Preset $Preset, ou use -AllowStale se o relogio do disco estiver errado."
    }

    Write-Warning $aviso
}

Write-Host "> Bundle: $($vst3.Replace($prefix, ''))"
Write-Host "> Config : $Preset / $Configuration / $($binarioTime.ToString('yyyy-MM-dd HH:mm'))"

$muse = Get-Process -Name "MuseScore*" -ErrorAction SilentlyContinue
if ($muse)
{
    throw "Feche o MuseScore antes de instalar (o binario do plugin fica bloqueado)."
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))
{
    throw "Executar como Administrador para copiar em 'C:\Program Files\Common Files\VST3'."
}

# --- Gate automatizado ---------------------------------------------------------
if (-not $SkipTests)
{
    $testsExe = Join-Path $root "build\$Preset\PartePlayTests_artefacts\$Configuration\PartePlayTests.exe"

    if (-not (Test-Path -LiteralPath $testsExe))
    {
        Write-Host "> PartePlayTests.exe ausente; compilando apenas o alvo de testes."
        cmake --build "$root\build\$Preset" --config $Configuration --target PartePlayTests

        if ($LASTEXITCODE -ne 0)
        {
            throw "Falha ao compilar os testes (exit $LASTEXITCODE)."
        }
    }

    if (-not (Test-Path -LiteralPath $testsExe))
    {
        throw "PartePlayTests.exe continua ausente em '$testsExe'."
    }

    Write-Host "> Rodando testes de dominio (gate V1)..."
    & $testsExe

    if ($LASTEXITCODE -ne 0)
    {
        throw "Testes de dominio falharam: instalacao cancelada. Use -SkipTests para ignorar."
    }
}

# --- Instalacao ----------------------------------------------------------------
$destDir = "C:\Program Files\Common Files\VST3"
$dest    = Join-Path $destDir "PartePlay.vst3"
$destBin = Join-Path $dest "Contents\x86_64-win\PartePlay.vst3"

New-Item -ItemType Directory -Path $destDir -Force | Out-Null

# O bundle .vst3 e um diretorio: Copy-Item sem -Recurse cria um subdir aninhado e
# mantem o binario antigo no lugar. Espelha a arvore inteira a partir do zero.
if (Test-Path -LiteralPath $dest)
{
    Remove-Item -LiteralPath $dest -Recurse -Force
}

Copy-Item -LiteralPath $vst3 -Destination $dest -Recurse -Force

if (-not (Test-Path -LiteralPath $destBin))
{
    throw "Instalacao falhou: binario ausente em '$destBin'."
}

$hashSrc = (Get-FileHash -LiteralPath $binario).Hash
$hashDst = (Get-FileHash -LiteralPath $destBin).Hash

if ($hashSrc -ne $hashDst)
{
    throw "Instalacao falhou: hash do binario instalado difere do build."
}

# O que foi para o host tem de bater com o que a arvore de build produziu.
$residuo = Get-ChildItem -Path $dest -Recurse -Force -File |
           Where-Object { $litter -contains $_.Name }

if ($residuo)
{
    Remove-Item -LiteralPath $dest -Recurse -Force
    throw "Lixeira chegou ao host e a instalacao foi revertida: $($residuo.Name -join ', ')"
}

Write-Host ""
Write-Host "> Instalado em $dest"
Write-Host "> SHA-256 $hashSrc"
Write-Host "> $Preset / $Configuration / $($binarioTime.ToString('yyyy-MM-dd HH:mm'))"
Write-Host "> Recarregue o host para o VST3 mudar."