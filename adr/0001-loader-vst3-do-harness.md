# ADR 0001 — Loader do VST3 para o harness de carga

- **Status:** aceito em 02/10/2026 — confirmado por execução: o probe carrega o
  bundle Release real (9/9, `exit=0`) e recusa as duas corrupções de binário da
  T7 (`exit=5`). Três afirmações deste ADR antes estavam erradas e foram
  corrigidas por observação, não por dedução; ver "O que a T7 mudou neste ADR".
- **Data:** 2026-10-02
- **Task:** `epic/vst3-load-harness-ci`, T1
- **Autor:** a partir de `CONTINUACAO.md:725`

## Contexto

O plugin compila e passa 461 checks de domínio, mas **não é carregado em lugar
nenhum** (`CONTINUACAO.md:545-556`). O gate de carga precisa abrir o bundle
`.vst3` real, instanciar a classe e processar áudio, falhando alto se não abrir.

A escolha é entre usar o loader do próprio SDK VST3 ou o loader do JUCE. O
backlog registra "Recomendado: **SDK** — o parser do manifesto é o do próprio
SDK, e testar com o loader do JUCE seria testar o JUCE".

**Esse argumento não se sustenta para o objetivo desta epic, e a recomendação
muda.** Ver Trade-offs.

### Fatos verificados

O backlog diz "o SDK já vem vendorizado no JUCE". Verdade, mas o caminho não é
o assumido. Em `JUCE_ROOT` = `D:\Program Files\JUCE` (JUCE 9.0.2):

| Fato | Evidência |
|---|---|
| A SDK existe, dentro do módulo `juce_audio_processors_headless` | `modules/juce_audio_processors_headless/format_types/VST3_SDK/` |
| A raiz `juce_audio_plugin_client/VST3/` **não** tem SDK | só tem `juce_VST3ManifestHelper.cpp` e `juce_VST3ModuleInfo.h` |
| `Module::create` está onde o backlog diz | `public.sdk/source/vst/hosting/module.h:160` |
| `getFactory` e `createInstance` | `module.h:170` e `module.h:185-188` |
| A SDK **não tem CMakeLists.txt** | — |
| O módulo expõe um loader VST3 próprio, sem GUI | `juce_VST3PluginFormatHeadless.h:45` |
| `createPluginInstance` é `private`/override | `juce_VST3PluginFormatHeadless.h:68` — o ponto de entrada público é `AudioPluginFormat::createPluginInstance` |
| `VST3PluginInstanceHeadless` não expõe o wrapper | `juce_VST3PluginFormatImpl.h:2025`, forward-declared em `:570`, sem `getWrapper()` público |

O alvo de teste hoje (`CMakeLists.txt:297-322`) liga `juce_core`,
`juce_audio_basics`, `juce_audio_formats`, `juce_dsp`, `juce_events` e
`chromaprint`. **Não** liga `juce_audio_processors_headless`, e suas fontes são
lista hard-coded (`:299-308`).

### Ground truth do bundle local

Lido de
`build/msvc-2026/PartePlay_artefacts/Release/VST3/PartePlay.vst3/Contents/Resources/moduleinfo.json`:

```
Audio Module Class         ABCDEF019182FAEB5052504C50545059
Component Controller Class ABCDEF011234ABCD5052504C50545059
SDKVersion                 VST 3.8.0
```

Estáveis: Debug e Release produzem CID **idêntico**, logo a constante pode ser
hard-coded sem risco de mudar a cada build. A cauda `5052504C50545059` é
"PPLPPLY" em ASCII — a semente é plantada, não aleatória. Nenhum desses
identificadores aparece em `Source/` nem em `CMakeLists.txt`: o JUCE deriva o
UID do nome do plugin, e o repo não expõe essa constante. Note que o manifesto
**não contém** o `uniqueId`: ele traz o CID hexadecimal
(`ABCDEF019182FAEB…`), e o JUCE converte o CID em `uniqueId` numérico. A
constante `expectedUniqueId` do harness foi observada em execução, e a
`expectedAudioModuleCid` continua impressa no log como referência legível —
nenhuma das duas é lida do manifesto em tempo de execução, porque o manifesto
não é a fonte da verdade nesse caminho (ver "O que a T7 mudou neste ADR").

Observação, agora **verificada** em 02/10/2026: sob
`PartePlay_artefacts/RelWithDebInfo/VST3/PartePlay.vst3` e
`.../MinSizeRel/VST3/PartePlay.vst3` existem pastas que contêm **apenas um
`desktop.ini` de 80 bytes** — sem `moduleinfo.json` e, principalmente, **sem
binário**. Não são bundles sem manifesto: são pastas vazias, quase certamente
criadas pelo Explorer ao navegar a árvore (`desktop.ini` é artefato de
personalização de pasta do Windows). Nada a consertar no empacotamento; o
harness as rejeitaria com `NO_PLUGIN_IN_PATH`, que é a resposta certa para uma
pasta que não é um bundle.

## Arquitetura proposta

Uma fronteira só: o harness é um `juce_add_console_app` que carrega o bundle
como dado externo — nunca linka o código do plugin.

```
PartePlayTests            (exe de console, sem link com o plugin)
  └─ DomainTests.cpp      461 checks, inalterado
  └─ LoadHarness.cpp      << novo: T2..T8, alvo separado
       └─ VST3PluginFormatHeadless   (juce_audio_processors_headless)
            └─ .vst3 bundle          (dado, não código)
```

O harness **não** linka `PARTEPLAY_SOURCES`. Se linkasse, o teste provaria a
cópia compilada no executável, não o artefato que o usuário baixa — e a
inteira razão desta epic é verificar o artefato.

## Fluxo de dados

Oito etapas, cada uma com ID estável e falha nomeada. A contagem subiu de 7
para 8 quando o probe rodou: o preflight virou etapa própria, porque sem ele o
passo de descoberta acusaria a causa errada.

| # | Etapa | Falha nomeada |
|---|---|---|
| 1 | Preflight: existe um formato VST3 registrado? | `VST3_SUPPORT_COMPILED_OUT` |
| 2 | Resolver o `.vst3` a partir do argv | `PATH_NOT_FOUND` |
| 3 | `findAllTypesForFile` e coletar descrições | `NO_PLUGIN_IN_PATH` |
| 4 | Conferir `uniqueId` contra a constante | `CLASS_NOT_FOUND`, `UNEXPECTED_CLASS_ID` |
| 5 | `createPluginInstance` | `INSTANTIATE_FAILED`, `INIT_FAILED` |
| 6 | Conferir estado após `prepareToPlay` | `INIT_FAILED` |
| 7 | `processBlock` em buffer não silencioso, rejeitar saída não finita | `OUTPUT_NON_FINITE`, `PROBE_SIGNAL_TOO_QUIET` |
| 8 | `locateTools()` dentro do bundle | `FOREIGN_BINARY_IN_BUNDLE` |
| 9 | Caso negativo: binário corrompido **tem** de ser recusado | `NEGATIVE_CASE_UNEXPECTED_SUCCESS`, `NEGATIVE_CORRUPTION_FAILED`, `NEGATIVE_TARGET_NOT_FOUND`, `NEGATIVE_SPAWN_FAILED`, `NEGATIVE_CHILD_TIMEOUT`, `NEGATIVE_REACHED_INSTANTIATION` |

Sobre a etapa 3: `NO_PLUGIN_IN_PATH` é o sinal de **caminho** errado, e nunca de
conteúdo corrompido. Ver "O que a T7 mudou neste ADR".

### O que o probe mudou neste ADR

O probe rodou contra o bundle Release local antes de qualquer uma destas
tarefas de código. Três coisas aqui estavam erradas e foram corrigidas por
observação, não por dedução:

**A constante da classe não é o CID.** `createIdentifierString()` devolve
`VST3-PartePlay-776e3313-58b41b17`, não o hex do manifesto — então a comparação
com `ABCDEF019182FAEB…` é impossível por essa API. A chave comparável é
`PluginDescription::uniqueId`, que o JUCE documenta como estável entre
plataformas (existe para substituir `deprecatedUid`, que gerava valores
diferentes por plataforma). Valor observado: **1488198423**.

**A asserção da T6 é finitude, nunca amplitude.** O probe mediu
`input_rms=0.394595` e `output_rms=0.000000`. Isso está **certo**: o plugin é
tocador de arquivo de referência e o harness não carrega arquivo nenhum, então
não há o que tocar. Um limiar de nível na saída reprovaria um plugin
perfeitamente saudável. O buffer precisa ser não silencioso **na entrada**,
que é o que prova que o sinal chega ao `processBlock`; o que se exige da saída é
que seja finita.

**O harness não é registrado em CTest.** Decisão, não omissão: o bundle só
existe depois da etapa de empacotamento do CI, então não há caminho para
resolver no configure time. Ele roda como passo da workflow logo após o
empacotamento, recebendo o bundle pelo argv.

### O que a T7 mudou neste ADR

A T7 (caso negativo) rodou **quatro** corrupções de conteúdo contra o bundle
copiado, e três delas **foram aceitas pelo loader**. Isso não é detalhe de
implementação; corrige uma afirmação que este ADR fazia:

| Corrupção aplicada | Resultado observado |
|---|---|
| `moduleinfo.json` virando lixo (1111 → 35 bytes) | **aceita**, `exit=0`, `PROBE_OK` |
| `moduleinfo.json` apagado | **aceita**, `exit=0`, `PROBE_OK` |
| pasta renomeada para `Outro.vst3`, binário ainda `PartePlay.vst3` | **aceita**, `exit=0`, `PROBE_OK` |
| binário truncado para 0 bytes | rejeitada, `exit=5`, `INSTANTIATE_FAILED` |
| cabeçalho do binário zerado, tamanho preservado | rejeitada, `exit=5`, `INSTANTIATE_FAILED` |

**`NO_PLUGIN_IN_PATH` não é o sinal de bundle corrompido neste loader.** Ele
dispara quando o *caminho* está errado — que é o que os probes de path avulso já
provam — e nunca quando o *conteúdo* está quebrado. Três consequências:

1. **Nome e `uniqueId` vêm da factory do binário, não do manifesto.** Com o
   manifesto destruído, o passo 3 continua devolvendo `name=PartePlay` e
   `uid=1488198423`. Um negativo baseado em "corromper o manifesto" seria um
   teste permanentemente vermelho, e a culpa seria do teste.
2. **`moduleinfo.json` é dispensável** nesse caminho headless. Ele existe para o
   navegador de plugins do host, não para o carregamento.
3. **O nome do binário interno não precisa casar com o da pasta.** O SDK oficial
   reprova (`module_win32.cpp:158` reaproveita `p.filename()`), mas o módulo
   headless do JUCE varre `Contents/<arch>/` e carrega o que encontrar. Portanto a
   T2 pode passar a raiz do bundle sem medo — e o harness faz isso.

Por isso as duas variantes da T7 miram o **binário**, e ambas aceitam
exatamente `exit=5`. `exit=5` e `INSTANTIATE_FAILED` são a única prova de que o
harness recusa artefato quebrado, porque o passo 5 é onde o código é carregado.

### O caso negativo é um subprocesso, e não uma chamada em processo

`AudioPluginFormatManager` guarda as instâncias que já criou e as reaproveita
por `pluginID`. O bundle corrompido tem **exatamente o mesmo `pluginID`** do
bundle bom (`1488198423`). Um negativo em processo receberia de volta a instância
já carregada e **passaria com o bundle quebrado** — exatamente o falso verde que
a T7 existe para impedir. Por isso o pai executa a si mesmo como filho, contra a
cópia corrompida, e julga o **código de saída** e o `reason` do filho.

A recursão é impedida pela flag `--negative-child`, que faz o filho rodar só os
passos 1 a 8. Sem ela o filho criaria outra cópia corrompida, e outra, até
esgotar o temp.

Cada corrupção é **verificada depois de aplicada** (tamanho e cabeçalho
conferidos). Sem essa checagem, uma escrita que falhasse silenciosamente faria o
filho rodar contra um bundle intacto, carregar com sucesso, e o passo verde seria
mentira. O mesmo vale para o nome da pasta copiada, que é preservado porque é
ele que o SDK reaproveita para montar o caminho do binário.

### Observabilidade — a parte de debug

Cada etapa emite uma linha `[step]`, uma ou mais linhas de detalhe e uma linha
`[ok]`, com prefixo estável para `grep`:

```
[step] 03/09 discover-types
       found=1
       cand name=PartePlay version=0.3.0 id=VST3-PartePlay-776e3313-58b41b17 uid=1488198423
[ok]   03/09 discover-types
```

e na falha:

```
[fail] 07/09 process-block  reason=OUTPUT_NON_FINITE  first_bad_sample=2048
[fail] summary  steps_ok=6  failed_at=07/09  reason=OUTPUT_NON_FINITE
```

O código de saída é o índice da etapa que falhou, e cada etapa também escreve
`reason=<nome>`. Isso dá três coisas de graça no CI: o nome do step no log
mostra onde parou, o `grep -c '^\[ok\]'` dá quantas etapas passaram sem
interpretar prosa, e o `reason` separa modos de falha que de outro modo
colapsariam no mesmo "plugin not found".

`99` é reservado para falha de infraestrutura do harness — uma `std::exception`
inesperada, por exemplo. Não corresponde a nenhuma etapa, e precisa ser
distinguível de `0`: um `catch` que devolvesse zero transformaria um crash em
verde, que é o oposto do que um harness deve fazer.

A discriminação foi verificada, não presumida — cinco entradas distintas, cinco
saídas:

| Entrada | reason | exit | steps_ok |
|---|---|---|---|
| caminho inexistente | `PATH_NOT_FOUND` | 2 | 1 |
| `CMakeLists.txt` (não é bundle) | `NO_PLUGIN_IN_PATH` | 3 | 2 |
| sem argumento | `PATH_NOT_FOUND` | 2 | 1 |
| bundle íntegro (Release local) | — | 0 | 9 |
| binário truncado / cabeçalho inválido | `INSTANTIATE_FAILED` | 5 | 4 |

Um dump verbose fica atrás de flag (`--verbose` ou `PARTEPLAY_HARNESS_VERBOSE`),
nunca por padrão, porque o log vai para artefato de CI.

## Trade-offs

**Escolhido: `VST3PluginFormatHeadless`.**

Contra o backlog, que recomendava a SDK crua:

- *A favor:* console-safe por construção (nada de `AppKit`/`Win32` num app de
  console); uma dependência de link em vez de lista manual de fontes da SDK,
  que nem tem `CMakeLists.txt`; menos defines de plataforma para acertar.
- *Contra:* perdemos asserções no nível do `IAudioProcessor`.
  `VST3PluginInstanceHeadless` é detalhe de implementação e não expõe o
  wrapper (`juce_VST3PluginFormatImpl.h:2025`), então `initializeComponent` e
  `setBusArrangements` são chamados **pelo wrapper do JUCE**, não por nós. Não
  podemos afirmar que nosso `setBusArrangements` foi o aceito.

Isso é aceitável porque o modo de falha que a epic persegue está no *nosso*
código — `create`, `initializeComponent`, `process` — e o wrapper chega lá
chamando esses mesmos métodos. O que fica fora de prova direta é o parser de
manifesto, e o custo está registrado no passo 2: `NO_PLUGIN_IN_PATH` não
distingue "manifesto ilegível" de "classe ausente". Etapa 3 existe para
separar as duas.

Descartado: `juce::VST3PluginFormat` (a com GUI) — arrastaria `juce_gui_basics`
para dentro de um teste de console.

Fallback se uma task futura exigir asserção no nível do SDK: a SDK crua está
em caminho conhecido e verificado acima, com `Module::create` em
`hosting/module.h:160`.

## Análise matemática

O buffer de entrada usa uma janela de $N$ amostras com taxa $f_s$:

$$x_{\mathrm{rms}} = \sqrt{\frac{1}{N}\sum_{i=0}^{N-1} x_i^2}$$

Um degrau de amplitude $A$ tem $x_{\mathrm{rms}} = \frac{A}{\sqrt{2}}$. Com
$sr = 44100$, $blockSize = 512$ e limiar $\tau = 10^{-3}$:

$$x_{\mathrm{rms}} = 0{,}5 \cdot \frac{1}{\sqrt{2}} \approx 0{,}3536 \;\gg\; \tau$$

O probe mediu $0{,}394595$, acima do previsto porque o sinal real não é um
degrau: é 440 Hz mais uma oitava acima a 50%, o que eleva o RMS. A folga
continua sendo de duas ordens de grandeza — o limiar não é o que está sob
teste. O que está sob teste é que o sinal **chega** ao `processBlock`, e um
buffer de silêncio passaria em qualquer limiar baixo, que é justamente o
falso-positivo que a T6 proíbe.

Repare na direção da assimetria: o limiar vale para a **entrada**, que é
sinal gerado pelo harness. A **saída** não recebe limiar nenhum, porque
silêncio nela é o resultado correto quando nenhum arquivo foi carregado — e
foi exatamente o que o probe mediu. Aplicar um limiar de nível na saída seria
reprovar um plugin saudável.

Duracão do teste. Com $B$ amostras por bloco e $K$ blocos:

$$T = \frac{B \cdot K}{f_s}$$

Para $K = 8$, $B = 512$, $f_s = 44100$: $T = \frac{4096}{44100} \approx 92{,}9\ \text{ms}$.

Oito blocos é o piso para o wrapper concluir a cadeia
`prepareToPlay` → `setBusArrangements` → `process` antes de qualquer áudio
aparecer. É o número que garante que o teste não falha por falta de latência
em block pequeno em CI, e é barato: $O(K)$ amostras, custo desprezível contra
os 2,34 s que a suíte de domínio já leva.

Finitude: $\forall i, |x_i| < \infty$, checada com `std::isfinite` por amostra.
Custo $O(BK)$, com curto-circuito para manter o primeiro índice ruim no log — é
o que a linha `[fail]` de `OUTPUT_NON_FINITE` usa.

Custo da etapa 9. O caso negativo copia o bundle duas vezes e executa o harness
duas vezes, então o custo é $O(2S)$ em disco e dois processos a mais, onde $S$ é
o tamanho do bundle — medido em **6,74 MB**, e 6,44 MB na variante do cabeçalho,
que lê e regrava o binário inteiro. O custo é de I/O de arquivo, não de CPU:
deve ficar na casa das dezenas de centenas de milissegundos por variante no CI.
É o preço de sair de "o harness passa quando o alvo está bom" para "o harness
sabe recusar artefato quebrado", e é o único custo do epic que cresce com o
tamanho do plugin em vez de com o número de etapas.

## O que este ADR não decide

T9. O backlog manda "reusar o `cmake --install` que já existe" e ele não
existe — o empacotamento são ~200 linhas em `.github/workflows/ci.yml:181-258`,
que já produzem o bundle (`ci.yml:198`, `:234`, `:303`). T9 vira um passo
**depois** do empacotamento, não um alvo de CMake.