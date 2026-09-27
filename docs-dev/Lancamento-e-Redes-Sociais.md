# Lançamento e Redes Sociais — PartePlay

**Data:** 27/09/2026 · **Versão:** 0.1.0
**Complementa (não substitui):** `docs-dev\Proximos-Passos.md` (handoff) · `docs-dev\Funcionalidades-Atuais.md` (estado vivo) · `docs-dev\Analise-e-Roadmap.md` (escopo).

Material de divulgação para o **vídeo de teste da primeira versão funcional**, publicado em quatro canais.
Alvo de lançamento announced: **sábado, 03/10/2026, às 10h**.

---

## 0. Status real — ler antes de publicar

O vídeo-test mostrou **duas lacunas**. Elas estão declaradas em todos os quatro posts, em voz alta.
Publicar sem isso transforma o lançamento em Issue.

| Lacuna | Observação do vídeo-test | Disposição |
|---|---|---|
| **Leitura automática de BPM não sincroniza a partitura sozinha** | Ainda é preciso ajustar manualmente o BPM da partitura para casar com a faixa de áudio | A leitura automática **deve** funcionar ao carregar a faixa no plugin, sem intervenção. Em aberto. |
| **Faixa ausente no container de instrumentos do MuseScore** | O que foi pedido ainda não está disponível | Em aberto. |

Consequência prática: o plugin está **bem avançado, ainda em desenvolvimento**. O termo
"lançamento previsto" (nunca "lançamento") é o que faz o adiamento ser lido como plano.
Detalhamento e ordem de resolução: `docs-dev\Proximos-Passos.md`.

> **Atenção:** `README.md` e `docs-dev\Funcionalidades-Atuais.md` ainda descrevem a análise de
> BPM/métrica como funcionalidade entregue. Isso diverge do comportamento observado e precisa
> ser reconciliado antes do dia 03/10.

---

## 1. Núcleo narrativo (base comum aos quatro posts)

**Motivação**
Quem escreve para instrumento transpositor vive de retrabalho: a partitura está em Si♭, mas o
áudio de referência está em Dó. O tom precisa ser tratado duas vezes — a da partitura e a do
instrumento — e qualquer erro de meio-tom não aparece como erro, aparece como o conjunto inteiro
desafinado, sem ninguém saber por quê. O PartePlay resolve isso em um ponto só: lê o tom do
instrumento, transpõe o áudio e alinha tudo ao relógio do hospedeiro.

**Desafios**
- **Sincronia** — o áudio não pode ter relógio próprio; toda posição deriva de `timeInSamples` do host.
- **Pitch-shift sem alterar duração** — vocoder de fase mantém o tamanho do buffer, então a
  sincronia não se altera por construção. Buffers imutáveis trocados atomicamente para o áudio não
  interromper a cada mudança de parâmetro.
- **Convenção musical** — a Trompa em Fá soa uma quinta acima da partitura em Dó. O código aplicava
  −5 semitons e entregava um Sol. Um domínio musical não se resolve com suposição, se resolve com
  invariante testável.
- **Encoding no Windows** — `juce::String::formatted` usa `_vsnwprintf`, que exige `wchar_t*` em
  `%s`; era a causa raiz dos acentos quebrados. Toda interpolação passou por uma camada de texto
  própria, coberta por testes.

**Decisões arquiteturais**
- Domínio musical centralizado em `Source/Instrument.h` e `Source/ParameterIds.h` — fonte única de verdade.
- Buffers por `shared_ptr<const>` com troca atômica — snapshot imutável no caminho crítico de áudio.
- Engines isoladas e substituíveis: `PitchShifter`, `TempoAnalyser`, `FilePlayer`. O vocoder de
  fase é uma implementação, não uma decisão irreversível.
- Aceitar o limite da plataforma em vez de contorná-lo: a API VST3 restringe a injeção direta de
  tempo pelos papéis Slave/Master, então o plugin exporta o mapa de tempo em **MIDI 1.0** e deixa o
  hospedeiro aplicar.
- Editor em proporção **5:4** como restrição deliberada — é a proporção da notação musical.
- Matemática musical isolada em funções puras: **110 verificações** automatizadas, sem host e sem ouvido.

**Tecnologias**
C++17 · JUCE 9.0.2 · CMake 3.22+ · Visual Studio 2022 · VST3 SDK (Steinberg) · MIDI 1.0 (MMA/AMEI) ·
CTest · MuseScore 4 · Git · UTF-8 (RFC 3629) · phase vocoder (Laroche & Dolson, 1999)

**Status**
- ✅ Transposição · mapa de tempo em MIDI · editor 5:4 · modo treino com loop por compassos ·
  domínio centralizado · i18n pt/en/es · template de partitura · suíte automatizada verde · VST3 instalado
- ⚠️ Leitura automática de BPM (pede ajuste manual) · faixa no container de instrumentos do MuseScore
- 🚀 **Lançamento previsto: sábado, 03/10, às 10h**

---

## 2. LinkedIn

**Título:** Estou construindo um VST3 que resolve um problema de músico — e ele vai ao ar sábado

**Corpo:**

Existe um retrabalho que todo quem escreve para instrumento transpositor conhece.

A partitura está em Si♭. O áudio de referência está em Dó. Para o instrumento soar na tonalidade
certa, o tom precisa ser tratado duas vezes: uma para a partitura, outra para o instrumento. E
qualquer erro de meio-tom não aparece como erro — aparece como o conjunto inteiro desafinado, sem
ninguém saber por quê.

O **PartePlay** resolve isso em um ponto só. Ele lê o tom do instrumento, transpõe o áudio de
referência e alinha tudo ao relógio do hospedeiro. Você toca a partitura e ouve o material na
tonalidade correta, no tempo correto.

**🎯 A motivação**

Eu não queria um afinador mais um visualizador. A maior parte do que existe nesse espaço trata o
sintoma. O que faltava era tratar a tonalidade como um problema de domínio — com fonte de verdade,
com invariante, com regra verificável.

**🧗 Os desafios**

O primeiro foi convencer-se de que o áudio não pode ter relógio próprio. Toda posição deriva do
`timeInSamples` do host. Sem isso, qualquer play, pause ou seek vira deriva acumulada.

O segundo foi pitch-shift que não muda a duração. Vocoder de fase resolve por construção — o buffer
mantém o tamanho, então a sincronia não se altera. Mas para o áudio não interromper a cada
movimento de parâmetro, os buffers viraram snapshots imutáveis trocados atomicamente.

O terceiro foi o mais interessante. A Trompa em Fá soa uma quinta acima do que está escrito. O
código aplicava −5 semitons e entregava um Sol em vez de um Fá. Passou despercebido até alguém
ouvir. A correção foi −5 → **+5**, e a decisão de fundo foi outra: domínio musical não se resolve
com suposição, se resolve com invariante testável. Hoje são **110 verificações automatizadas**
cobrindo tabela de instrumentos, direção da transposição, compensação de afinação e encoding.

O quarto foi encoding no Windows. `juce::String::formatted` usa `_vsnwprintf`, que exige `wchar_t*`
em `%s`. Era a causa raiz dos acentos quebrados. Toda interpolação de texto passou por uma camada
própria, com testes.

**🏛️ Decisões arquiteturais**

- Domínio musical centralizado em `Instrument.h` e `ParameterIds.h` — fonte única de verdade.
- Buffers por `shared_ptr<const>` com troca atômica — imutabilidade no caminho crítico de áudio.
- Engines isoladas e substituíveis: `PitchShifter`, `TempoAnalyser`, `FilePlayer`. O vocoder de
  fase é uma implementação, não uma decisão irreversível.
- Aceitar o limite da plataforma em vez de contorná-lo: a API VST3 restringe a injeção direta de
  tempo pelos papéis Slave/Master. Em vez de forçar, o plugin exporta o mapa de tempo em **MIDI 1.0**
  e deixa o hospedeiro aplicar.
- Editor em proporção 5:4 como restrição deliberada — é a proporção da notação musical, e a UI
  deveria refletir o domínio.
- Matemática musical isolada em funções puras, testáveis sem host e sem ouvido.

**⚙️ Tecnologias**

C++17 · JUCE 9.0.2 · CMake · Visual Studio 2022 · VST3 SDK · MIDI 1.0 · CTest · MuseScore 4 ·
phase vocoder (Laroche & Dolson, 1999)

**📍 Status — e o que ainda não está pronto**

Seria desonesto da minha parte apresentar isso como pronto. O projeto está avançado, mas ainda em
desenvolvimento, e há duas lacunas reais:

1. **A leitura automática de BPM ainda não sincroniza a partitura sozinha.** Hoje ainda é preciso
   ajustar o BPM manualmente no host. É o próximo marco: a leitura automática tem de funcionar ao
   carregar a faixa, sem intervenção.
2. **A faixa no container de instrumentos do MuseScore ainda não está disponível** como foi pedida.
   A integração com o container é o que transforma o plugin de "efeito que você encaixa" em
   "instrumento que você toca".

Ambas estão no roadmap. O alvo de lançamento é **sábado, 03/10, às 10h** — e o vídeo de teste que
gravei mostra exatamente o estado real do projeto, incluindo o que ainda não encaixa.

**🎬 O roteiro que vem a seguir**

Este vídeo é o primeiro de uma série documentando a construção:

- **Agora** — o problema, a primeira versão funcionando e as duas lacunas abertas.
- **Em seguida** — leitura automática de BPM: da heurística à sincronia sem intervenção.
- **Depois** — integração com o container de instrumentos do MuseScore.
- **Ao longo** — as decisões arquiteturais, os bugs que mudaram o rumo e o que a comunidade cobrar
  de um VST3 feito para músicos.

**Lançamento previsto: sábado, 03/10, às 10h.**

Se você escreve para metais, sopros ou saxofone e já perdeu tempo para uma referência soar na
tonalidade errada, me conta nos comentários. Esse é exatamente o caso que eu quero acertar.

`#PartePlay #VST3 #DesenvolvimentoDeAudio #DSP #JUCE #Cpp #ProducaoMusical #ArranjoMusical #Metais #Saxofone #Trompa #PluginDeAudio #MIDI #MuseScore`

---

## 3. YouTube

### Títulos (escolher um)

- `Plugin VST3 que transpõe sua referência pro tom do instrumento (V1 + o que falta)`
- `Transpondo áudio pro instrumento sem quebrar a sincronia — VST3 em C++/JUCE`

### Descrição

```
PartePlay — plugin VST3 que reproduz um áudio de referência sincronizado ao
transporte do hospedeiro e o transpõe para o tom do instrumento da partitura.
Sem descompasso. Sem alteração de velocidade.

Este é o VÍDEO DE TESTE DA PRIMEIRA VERSÃO FUNCIONAL — incluindo o que ainda
não está pronto.

════════════════════════════════════════
⏱️ CAPÍTULOS
════════════════════════════════════════
00:00  O problema: partitura em Si♭, áudio em Dó
00:35  O que o plugin faz
01:10  A aposta técnica: sincronia vem do host
01:45  O bug da Trompa (Sol em vez de Fá)
02:25  Duas lacunas reais — leia antes de julgar
03:10  Arquitetura em 60 segundos
03:55  Tecnologias
04:20  Status e próximos passos

════════════════════════════════════════
⚠️ STATUS HONESTO
════════════════════════════════════════
O projeto está avançado, mas ainda em desenvolvimento.

Ainda NÃO funciona:
• A leitura automática de BPM ainda exige ajuste manual do BPM da
  partitura para sincronizar com a faixa de áudio.
• A faixa ainda não aparece no container de instrumentos do MuseScore.

Próximos marcos:
1. Leitura automática de BPM funcionando ao carregar a faixa
2. Integração com o container de instrumentos do MuseScore
3. Ajuste fino do editor 5:4

LANÇAMENTO PREVISTO: sábado, 03/10, às 10h

════════════════════════════════════════
🧠 DECISÕES ARQUITETURAIS
════════════════════════════════════════
• Toda posição do áudio deriva de timeInSamples do host — o plugin não
  tem relógio próprio
• Pitch-shift por vocoder de fase mantém o tamanho do buffer, então a
  sincronia não se altera por construção
• Buffers como shared_ptr<const> com troca atômica: o áudio não
  interrompe a cada mudança de parâmetro
• Domínio musical em fonte única (Instrument.h / ParameterIds.h)
• Limite real da plataforma: o VST3 restringe a injeção de tempo pelos
  papéis Slave/Master — em vez de forçar, exporto o mapa em MIDI 1.0
• Matemática musical em funções puras → 110 verificações sem host e
  sem ouvido

════════════════════════════════════════
⚙️ TECNOLOGIAS
════════════════════════════════════════
C++17 · JUCE 9.0.2 · CMake · Visual Studio 2022 · VST3 SDK (Steinberg)
MIDI 1.0 (MMA/AMEI) · CTest · phase vocoder (Laroche & Dolson, 1999)

════════════════════════════════════════
🔗 LINKS
════════════════════════════════════════
• Código e issues: github.com/rubenslyra/parteplay
• LinkedIn: linkedin.com/in/rubenslyra
• Documentação técnica: pasta docs-dev/ no repositório

════════════════════════════════════════
🎵 CONVENÇÃO DE TRANSPOSIÇÃO
════════════════════════════════════════
Piano/Trombone 0 · Trompete/Sax Tenor −2 · Sax Alto +3 · Trompa +5

(Pra quem for conferir se o áudio está certo: a Trompa em Fá soa uma
quinta ACIMA do que está escrito. Se você ouvir um Sol, tem bug.)

#PartePlay #VST3 #AudioPlugin #CPP #JUCE #DSP #MusicProduction
```

### Roteiro do vídeo (blocos do que foi gravado)

| Tempo | Bloco | Fala / ação |
|---|---|---|
| 0:00–0:35 | Gancho | Partitura em Si♭ e referência em Dó lado a lado. "Todo mundo que escreve para instrumento transpositor já passou por isso: dois tons, uma referência, e nenhum lugar único onde acertar." |
| 0:35–1:10 | O que é | Abre o plugin, carrega a faixa, escolhe o instrumento. Mostrar o áudio entrar junto com a partitura. |
| 1:10–1:45 | A aposta | "A sincronia não é minha, é do host. Toda posição do áudio sai do relógio dele. Por isso play, pause e seek não derivam." |
| 1:45–2:25 | O bug | "A Trompa em Fá soa uma quinta acima do que está escrito. Eu aplicava menos cinco semitons e entregava um Sol. O código estava errado e a suíte também — só o ouvido não passou." |
| 2:25–3:10 | Lacunas | Dizer direto: "Duas coisas ainda não prontas. O BPM ainda precisa de ajuste manual. E a faixa ainda não entra no container de instrumentos do MuseScore. Não vou maquiar isso." |
| 3:10–3:55 | Arquitetura | Diagrama simples: host → processor → engines → buffer imutável. Uma frase por camada. |
| 3:55–4:20 | Tecnologias | Passagem rápida de tela. |
| 4:20–fim | CTA | "Lançamento previsto sábado, 03/10, às 10h. Se você tem trompa ou saxofone e já briga com referência na tonalidade errada, comenta aqui." |

### Comentário fixado

> Status real: a leitura automática de BPM ainda exige ajuste manual, e a faixa ainda não entra no
> container de instrumentos do MuseScore. Ambos são os próximos marcos. Lançamento previsto:
> **sábado, 03/10, às 10h**. Se você é da comunidade, seu feedback sobre o que construir primeiro
> importa mais agora.

---

## 4. Instagram

### Legenda do post

```
PartePlay — plugin VST3 que transpõe sua referência pro tom do instrumento,
sem descompasso e sem mudar a velocidade. 🎺

O problema: partitura em Si♭, áudio em Dó. Dois tons, uma referência, nenhum
lugar único onde acertar. O plugin lê o tom do instrumento, transpõe e alinha
ao relógio do hospedeiro.

O que ele já faz:
✔ transposição por instrumento (piano, trombone, trompete, sax tenor, sax alto,
  trompa, manual)
✔ afinação de referência 432–445 Hz
✔ sincronia derivada do host — sem deriva
✔ modo treino com loop por compassos
✔ mapa de tempo em MIDI 1.0
✔ editor 5:4
✔ 110 verificações automatizadas

O que ainda NÃO faz (e vamos fazer):
✗ a leitura automática de BPM ainda pede ajuste manual da partitura
✗ a faixa ainda não entra no container de instrumentos do MuseScore

O projeto está bem avançado, mas ainda em desenvolvimento.
Lançamento previsto: sábado, 03/10, às 10h.

Link na bio 👆

#parteplay #vst3 #musicproduction #musictech #dsp #cpp #juce
#audioengineering #trompete #trompa #saxofone #metais #arranjomusical
#composer #midi #musescore #plugin #desenvolvimentodeaudio
```

### Roteiro do carrossel (6 slides, um por seção)

| Slide | Título | Corpo |
|---|---|---|
| 1 | **O PROBLEMA** | Partitura em Si♭. Áudio em Dó. Dois tons, uma referência, nenhum lugar único onde acertar. Meio-tom errado não aparece como erro — aparece como o conjunto desafinado. |
| 2 | **A MOTIVAÇÃO** | Não quero um afinador nem um visualizador. Quero tratar tonalidade como domínio: fonte de verdade, invariante, regra verificável. |
| 3 | **OS DESAFIOS** | Sincronia sem relógio próprio. Pitch-shift sem mudar duração. A Trompa que soava Sol em vez de Fá. Acentos quebrados no Windows por causa do `_vsnwprintf`. |
| 4 | **AS DECISÕES** | Fonte única no domínio. Buffers imutáveis com troca atômica. Engines substituíveis. Limite do VST3 aceito, não contornado. 5:4 porque notação musical é 5:4. |
| 5 | **TECNOLOGIAS** | C++17 · JUCE 9.0.2 · CMake · VS 2022 · VST3 SDK · MIDI 1.0 · CTest · phase vocoder |
| 6 | **STATUS** | Ainda em desenvolvimento. Leitura automática de BPM e container de instrumentos do MuseScore são os próximos marcos. Lançamento previsto: **sábado, 03/10, 10h**. |

### Stories (3 telas, ~5s cada)

1. "Você já precisou ajustar o BPM da partitura à mão pra casar com a referência?" → enquete: [Sim, sempre] / [Não, meu afinador resolve]
2. "O container de instrumentos do MuseScore ainda não tem a faixa. Tá na lista." → caixinha "o que você quer ver primeiro?"
3. "Sábado, 03/10, 10h. Lançamento previsto." → link

### Roteiro do Reel (30s)

| Tempo | Visual | Texto na tela |
|---|---|---|
| 0–3s | Partitura Si♭ + áudio Dó lado a lado | "Dois tons. Uma referência. Nenhum lugar único de acertar." |
| 3–10s | Plugin aberto, faixa carregada, instrumento escolhido | "PartePlay — VST3 que transpõe a referência pro tom do instrumento." |
| 10–17s | Tocando junto com a partitura, cursor parado no compasso | "Sincronia vem do host. Sem deriva." |
| 17–22s | Painel 02 com trompa selecionada | "Trompa em Fá = +5. O bug entregava um Sol." |
| 22–30s | Código, terminal com "110 verificações, 0 falhas", editor 5:4 | "Ainda em desenvolvimento. Sábado, 03/10, 10h. Link na bio." |

---

## 5. TikTok

### Legenda

```
parteplay é um VST3 que transpõe sua referência pro tom do instrumento,
sem descompasso 🎺

ainda em desenvolvimento — o BPM ainda pede ajuste manual e a faixa ainda
não entra no container do musescore. os dois são os próximos marcos.

lançamento previsto: sábado 03/10, 10h

#parteplay #vst3 #musicproduction #audioengineering #cpp #dsp #trompete
#trompa #saxofone #metais #midi #musescore #musictech #fyp
```

### Roteiro (45–60s)

| Tempo | Visual | Texto na tela | Fala |
|---|---|---|---|
| 0–4s | Partitura em Si♭ e áudio em Dó lado a lado | `partitura: si♭` / `áudio: dó` | "Todo mundo que escreve pra instrumento transpositor conhece isso." |
| 4–12s | Abre o plugin, carrega a faixa, escolhe Trompa | `o plugin faz isso:` | "Você carrega a faixa, escolhe o instrumento, e ele transpõe e alinha no tempo." |
| 12–20s | Toca junto com a partitura, cursor parado no compasso | `sincronia vem do host` | "A sincronia não é minha, é do relógio do host. Por isso não deriva." |
| 20–28s | Mostra o bug: trompa tocando, nota errada | `trompa em fá = +5` → `bug: −5 (sól)` | "A trompa em fá sobe cinco semitons. Eu tava descendo cinco. Entregava um sol. Só o ouvido pegou." |
| 28–38s | Dois X vermelhos na tela | `ainda não funciona:` | "Ainda preciso ajustar o BPM na mão, e a faixa não entra no container do musescore ainda." |
| 38–48s | Código, terminal com "110 verificações, 0 falhas", editor 5:4 | `110 verificações` / `c++ · juce · vst3` | "C++, JUCE, VST3. Cem e dez verificações automatizadas." |
| 48–60s | Tela final com a data | `sábado 03/10 · 10h` | "Lançamento previsto sábado, três de outubro, dez horas. Segue o projeto." |

### Texto de capa (thumbnail)

```
PARTEPLAY: o tom do instrumento, automático
VST3 · C++/JUCE
03/10 10h
```

---

## 6. Checklist de publicação

- [ ] Reconciliar `README.md` e `docs-dev\Funcionalidades-Atuais.md` com o status real da análise de BPM
- [ ] Confirmar o URL do LinkedIn no README (preenchido: `linkedin.com/in/rubenslyra`)
- [ ] Adicionar o link do vídeo à bio do Instagram / descrição do TikTok
- [ ] Recortar o corte de 3s do Reel/TikTok a partir do master do YouTube
- [ ] Publicar YouTube primeiro (o vídeo é o conteúdo-mãe) e derivar os demais
- [ ] Programar os demais para 03/10, 10h
- [ ] Responder comentários de beta testers com o formulário de feedback (definir antes do dia 03/10)
- [ ] Fixar o comentário de status no YouTube e o story de lacunas no Instagram

## 7. Regras de uso do material

1. **Nunca dizer "lançamento"** — sempre "lançamento previsto".
2. **Nunca omitir as duas lacunas** se o conteúdo for sobre o estado do produto. Posts só de
   Tutorial/Feature podem citá-las no bloco final, mas a descrição do vídeo de teste não.
3. **Não prometer** container de instrumentos do MuseScore nem leitura automática de BPM como
   fonctionnalité entregue antes de estar validado no host.
4. **A tabela de transposição é conteúdo de verificação** — quem assiste consegue auditar o
   projeto. Manter a Trompa em **+5**.
