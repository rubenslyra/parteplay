# Changelog

Todas as mudanças relevantes do PartePlay. O formato segue
[Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/) e o versionamento é
[SemVer](https://semver.org/lang/pt-BR/).

## [Não publicado]

Próxima versão: **0.2.2** (o número mora só em
`project(PartePlay VERSION x.y.z)`, no `CMakeLists.txt`).

A 0.2.0 estável nunca foi publicada: o que existe é `v0.2.0-rc.1`, cortada de
`4113f13`. Esta seção é o conteúdo da 0.2.2.

**Por que 0.2.2 e não 0.2.1.** A 0.2.2 é *patch* por decisão de escopo: corrigir
o que já existia, sem functionality nova. A ficha da canção — ISRC, ano, dados do
fonograma que passa pelo algoritmo — é funcionalidade e sai como **0.3.0**, na
branch `epic/identidade-do-fonograma`. Pular a 0.2.0 e a 0.2.1 é deliberado:
numerar a versão seguinte como patch deixa explícito o que entra e o que não
entra, e 0.2.2 é o primeiro número que não colide com nada já publicado.

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
  fingerprint offline descritos com precisão; badges de testes (325) e CI.
- **Página do produto em GitHub Pages** — `docs/index.html` autossuficiente
  (sem JS, sem builds) com `.github/workflows/pages.yml` para publicar via
  GitHub Actions.
- **`THIRD_PARTY_NOTICES.md`** — as licenças de tudo que é linkado estaticamente
  no binário distribuído, com o caminho do arquivo de onde cada string de licença
  foi lida. A LGPL 2.1 exige dizer quem é quem e oferecer a fonte; a fonte já
  estava no repositório, em `Chromaprint-Dependences/chromaprint-1.6.1.tar.gz`,
  que é o tarball upstream intacto e portanto a fonte do código linkado (issue #4).
- **Conventional Commits** — `commitlint.config.js` com lista fechada de tipos,
  cabeçalho de no máximo 72 caracteres e linha de corpo de no máximo 100, validado
  por `.github/workflows/conventional-commits.yml` no intervalo do PR. Vale para os
  commits a partir de agora: o histórico já publicado, assinado, não é reescrito.
- **Conventional Commits como chave do changelog** — `git log --oneline` passa a
  distinguir correção de mudança de formatação, que é o que permite escrever o
  changelog a partir do histórico em vez de da memória.

### Removido

- **Transposição por instrumento e a tabela de instrumentos** — `Source/Instrument.*`
  e os parâmetros `instrument`/`transpose` saem do repositório: o produto é um
  transporte escravo que entrega o áudio de referência sincronizado ao relógio do
  hospedeiro, sem reescrever a tonalidade.
- **`docs-dev/`** — planejamento e referências visuais internas não fazem parte do
  público; as decisões em aberto que ainda importam foram preservadas em backup.

### Alterado

- **A suíte de domínio passou a 325 verificações**, incluindo: códigos BCP 47,
  separador decimal por cultura, grafia das variantes do inglês, acentuação do
  es-ES, cálculo/cancelamento/rejeições do fingerprint e publicação do player sem
  lock.
- **`Format` do destino do template de PR** — `ctest --preset msvc` substitui o
  caminho direto para o binário, e o checklist exige DCO e a reconcilição da versão.
- **CI verde nas três plataformas** — depois de cinco rodadas de correção, o
  workflow está de fato verde em Windows, Ubuntu 24.04 e macOS 14, com o bundle
  universal x86_64 + arm64 verificado por `lipo` (antes o check era `|| true`,
  ou seja, não verificava nada):
  - `Obter JUCE` roda com `shell: bash`; no runner do Windows o PowerShell
    transformava a continuação de linha em argumento e o clone morria.
  - O `JUCE_ROOT` exportado pelo CI chegava corrompido ao CMake no Windows
    (`$GITHUB_ENV` com barras invertidas redirecionado em bash); os dois lados
    passam por `cygpath`.
  - `JUCE_ROOT` no CMake passa a ter precedência: variável de ambiente, depois
    cache, e só então o default da máquina — que antes vencia sempre no Windows
    e mascarava o valor real.
  - O bootstrap do `juceaide` roda num CMake recursivo que não herda os include
    dirs do projeto; no Linux, o `CPLUS_INCLUDE_PATH` é derivado do
    `pkg-config` do próprio `freetype2` para ele.
  - O Chromaprint é compilado com `-fPIC` (escopo do próprio subdiretório), sem
    o que a biblioteca estática não entra no `.so` do VST3.
  - O binário do bundle VST3 de macOS não tem extensão: a asserção de
    arquitetura procurava `*.dylib` e falhava num build que tinha dado certo.

### Corrigido

- **A impressão digital não correspondia à do `fpcalc`.** A conversão para PCM
  int16 estava escrita `(int16_t) x * 32767.0f`, e o cast liga mais forte que o
  `*`: a amostra era truncada para `{-1, 0, +1}` **antes** de ser escalada. O
  Chromaprint recebia um sinal de três níveis em vez do áudio.

  O sintoma era silencioso de um jeito difícil de diagnosticar: a impressão
  continuava sendo base64 válido, ou seja, **nenhum teste existente falhava**. O
  único efeito observável era o AcoustID não encontrar a gravação — sem crash e
  sem linha de log que apontasse para a causa. Agora a conversão é
  `Fingerprint::toPcm16`, com o limite aplicado antes da escala e o cast no fim,
  e a escala é nomeada (`pcm16FullScale`) porque precisa ser a mesma do `fpcalc`.

  Coberto por um teste de regressão que afirma uma **invariante quantitativa**
  (uma rampa de 1001 pontos tem de produzir mais de 500 níveis distintos — a
  versão com o bug produzia exatamente três), e não "a saída é base64 válido",
  que era justamente o que deixava passar. 12 verificações novas.
- **O instalador entregava um binário obsoleto ao hospedeiro.**
  `install-vst3.ps1` e `build.ps1` tinham `build\msvc` fixo no caminho. Essa é a
  árvore do VS2022 BuildTools, mas o build daqui é feito com `msvc-2026`, então o
  script lia o bundle de uma árvore enquanto o compilador escrevia em outra. O
  sintoma é invisível: o script copiava, comparava o hash do que acabara de copiar
  com o hash do que copiara, e dava sucesso. O binário instalado em
  `C:\Program Files\Common Files\VST3` era simplesmente código que não era o
  último compilado — no caso, anterior à correção do PCM acima.

  Agora o preset é parâmetro dos dois scripts e é usado de forma consistente para
  o bundle, para o executável de testes e para o `cmake --build`. Um bundle mais
  antigo que o fonte mais recente é recusado (`-AllowStale` existe para máquina
  cujo relógio não é confiável, que é o caso desta, com o `D:` em
  `Full Repair Needed`), e o que vai para o hospedeiro é impresso com preset,
  configuração, data e SHA-256.
- **Lixeira do sistema operacional dentro do bundle VST3.** Oito `desktop.ini`
  estavam dentro dos bundles nas árvores de build — o shell do Windows os grava
  quando alguém personaliza o ícone da pasta no Explorer, o que aconteceu quando
  o pacote foi aberto para inspeção. A especificação do bundle não admite arquivos
  avulsos na raiz, e `.gitignore` não protege um zip. O instalador agora limpa uma
  lista fechada desses nomes e **reverte a instalação** se algum deles chegar ao
  hospedeiro. O asset publicado `v0.2.0-rc.1` foi verificado e não os contém.
- **A versão estava escrita duas vezes.** `project(PartePlay VERSION …)` é
  declarado como fonte única quatro linhas acima de um comentário dizendo isso, e
  `juce_add_plugin` repetia o literal. Agora deriva de `${PROJECT_VERSION}`: o
  número da release e o do bundle não podem divergir em silêncio.
- **`msvc-2026` passou a ser o preset local padrão**; `msvc` (VS2022) continua no
  `CMakePresets.json` porque a CI roda em `windows-2022`, que não tem VS18.
  Existiam duas árvores de build com o mesmo código e binários diferentes, e nada
  nos scripts apontava para qualquer uma delas por padrão.
- **O bundle anunciava uma funcionalidade que não existe.** A descrição era
  "Ficha da cancao (ISRC, ano, BPM)"; `Source/Instrument.*` foi removido,
  `ParameterIds.h` tem seis parâmetros e nenhum deles é metadado, e não há leitor
  de ISRC na árvore. Como o hospedeiro mostra essa string no navegador de
  plugins, a consequência era concreta: alguém lia, procurava a função e reportava
  defeito contra algo que nunca foi entregue. Passa a descrever o que existe.
- **A CI nunca rodou para `develop`.** O gatilho era push em `main` e
  `pull_request` para `main`. Os dois commits que corrigiram o PCM do fingerprint
  foram publicados sem build e sem teste: a correção foi verificada apenas à mão,
  numa máquina com o disco em `Full Repair Needed`. `develop` entrou nos dois
  gatilhos.
- **O aviso de administrador não impedia a instalação.** Era um `Write-Warning`
  seguido de `exit 1`, que devolve sucesso a quem chamou `build.ps1`. Passa a ser
  `throw`.

### Alterado

- **`C4244` corrigido nos dois pontos da issue #2** — o estreitamento
  `double`→`float` na janela de Hann de `TempoAnalyser::estimateTuningCents`
  (a fase permanece `double`; o estreitamento acontece uma vez, explícito, em
  vez de vazar pela atribuição) e o `float`→`int16_t` do PCM do fingerprint.
  Em ambos os casos a conversão deixa de ser implícita e passa a ser uma
  decisão visível.
- **Novo preset `msvc-2026`** no `CMakePresets.json`, para Visual Studio 18 2026
  Enterprise, ao lado do `msvc` existente — que é preservado, com histórico
  próprio em `build/msvc`.

## [0.1.0] — 2026-09-03

Primeira versão pública. (O que mudou depois dela está em [Não publicado], inclusive a
remoção da transposição por instrumento e a troca do i18n de três idiomas para quatro.)

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
