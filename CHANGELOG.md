# Changelog

Todas as mudanças relevantes do PartePlay. O formato segue
[Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/) e o versionamento é
[SemVer](https://semver.org/lang/pt-BR/).

## [Não publicado]

### Adicionado

- **Identificação offline por impressão digital** — o plugin calcula uma impressão
  acústica Chromaprint (PCM mono 16-bit, reamostrado para 11025 Hz) do áudio de
  referência e mostra o estado na ficha da canção (Idle → analisando → pronta /
  cancelada / falha). A consulta ao banco do AcoustID segue desabilitada: a chave
  de aplicação está validada fora do build.
- **i18n com culturas BCP 47 completas** — as entradas deixam de ser `English`/
  `Spanish` e passam a ser `pt-BR`, `en-GB`, `en-US` e `es-ES`, com nomes nativos
  regionais no seletor de idioma. Dentro do inglês, a grafia diverge por variante
  (`analysing/cancelled` no Reino Unido vs `analyzing/canceled` nos EUA), e no
  espanhol a acentuação completa entrou (`válida`, `vacía`, `aún`).
- **Números formatados pela cultura** — o separador decimal passa a ser o do
  usuário: `128.5` em en-US/en-GB, `128,5` em pt-BR/es-ES. Vale para BPM, afinação
  detectada, referência A4 e megabytes exibidos.
- **CI multiplataforma** — `.github/workflows/ci.yml` com jobs `metadata`,
  `version-single-source`, build+test+artefato em Windows/Ubuntu/macOS e um
  build universal do macOS. Presets de CMake para `msvc`, `ninja`, `linux`,
  `macos` e `macos-universal`, todos sem caminho de JUCE hardcoded.
- **`PARTEPLAY_JUCE_VERSION` em `CMakeLists.txt`** — versão da dependência
  declarada como fato único; a CI clona a tag exata em vez de derivá-la do
  número do projeto (a derivação original produzia a tag inexistente `0.1.0`).
- **README em inglês** — reescrito para o público: transporte escravo, i18n e
  fingerprint offline descritos com precisão; badges de testes (313) e CI.
- **Página do produto em GitHub Pages** — `docs/index.html` autossuficiente
  (sem JS, sem builds) com `.github/workflows/pages.yml` para publicar via
  GitHub Actions.

### Removido

- **Transposição por instrumento e a tabela de instrumentos** — `Source/Instrument.*`
  e os parâmetros `instrument`/`transpose` saem do repositório: o produto é um
  transporte escravo que entrega o áudio de referência sincronizado ao relógio do
  hospedeiro, sem reescrever a tonalidade.
- **`docs-dev/`** — planejamento e referências visuais internas não fazem parte do
  público; as decisões em aberto que ainda importam foram preservadas em backup.

### Alterado

- **A suíte de domínio passou a 313 verificações**, incluindo: códigos BCP 47,
  separador decimal por cultura, grafia das variantes do inglês, acentuação do
  es-ES, cálculo/cancelamento/rejeições do fingerprint e publicação do player sem
  lock.
- **`Format` do destino do template de PR** — `ctest --preset msvc` substitui o
  caminho direto para o binário, e o checklist exige DCO e a reconcilição da versão.

## [0.1.0] — 2026-09-03

Primeira versão pública.

### Adicionado

- Sincronia 1:1 com o relógio do host (`timeInSamples`), com play, pause, seek, avanço e
  retrocesso sem deriva.
- Transposição por instrumento offline (vocoder de fase, FFT 2048 / hop 512) preservando a
  duração do buffer — por construção a sincronia não se altera.
- Análise de tempo: BPM, assinatura (3/4 ou 4/4), número de compassos e afinação detectada.
- Exportação do mapa de tempo em MIDI 1.0 para importar no MuseScore ou na DAW.
- Modo treino: velocidade de 50–150% e loop de trecho em compassos.
- Editor 5:4 com tema escuro, forma de onda, loop visível e tiles de análise.
- i18n em pt-BR, en e es.

### Conhecido

- A leitura automática de BPM não sincroniza a partitura sozinha (lacuna L1).
- A faixa não aparece no container de instrumentos do MuseScore (lacuna L2).
