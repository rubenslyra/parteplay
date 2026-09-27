# Funcionalidades Atuais — PartePlay (Plugin VST3)

**Data:** 26/09/2026 | **Versão:** 0.1.0 | **Plataforma:** Windows | **Formato:** VST3
**Stack:** C++ / JUCE 9.0.2 / CMake / Visual Studio 2022
**Proposta:** Reproduzir um áudio de referência sincronizado ao transport do hospedeiro, transposto para o tom do instrumento da partitura — sem descompasso e sem mudança de velocidade.

---

## 1. Visão geral do que o plugin faz hoje

1. Carrega um arquivo de áudio (WAV / FLAC / MP3 / OGG) inteiro em memória.
2. Sincroniza a reprodução com o relógio do hospedeiro (MuseScore 4 / DAW) — play, pause e **seek** são seguidos com precisão de amostra.
3. Detecta o tempo (BPM) e estima compassos do áudio no carregamento.
4. Transpõe o tom do áudio (pitch-shift) **sem mudar a velocidade**, de forma offline, quando o usuário escolhe o instrumento/afinação/transposição.
5. Expõe estado e parâmetros em uma interface gráfica simples e persiste-os na sessão.

---

## 2. Reprodução e sincronia (núcleo)

| Funcionalidade | Detalhe |
|---|---|
| **Sincronia via transport do hospedeiro** | Lê `isPlaying` + `timeInSamples` do playhead a cada bloco. A posição no arquivo decorre **1:1** do relógio do host (`timeInSamples × ratio`). |
| **Conversão de taxa de amostragem** | Áudio é tocado na taxa do host com interpolação **linear** (`fileSampleRate / hostSampleRate`). Arquivo com taxa ≠ host toca correto e em velocidade normal. |
| **Seek / avançar / retroceder** | Consequência direta da sincronia: buscar em qualquer ponto da partitura reposiciona o áudio no instante correspondente. |
| **Play / Pause** | Estados detectados bloco a bloco; fora de play o buffer é silenciado e o estado zerado. |
| **Fim de arquivo** | Ao ultrapassar o último sample: silencia, sinaliza "Fim do arquivo" e **não faz loop**. |
| **Estrutura mono/stéreo** | Canal de entrada opcional, saída estéreo ou mono (bus layout compatível). |
| **Thread-safety** | Acesso ao áudio protegido por `audioLock`; buffers trocam-se por `shared_ptr<const>` (snapshots baratos, sem cópia de dados). |

### Comportamento de borda (detalhe)
- Ao **carregar arquivo novo**: substitui o player inteiro sob lock, zera flag de fim e força recálculo da transposição (contador de geração).
- Durante reprodução, qualquer **mudança de parâmetro não interrompe o áudio** — a versão transposta é trocada atomicamente quando fica pronta.

---

## 3. Transposição inteligente (Fase 2)

| Funcionalidade | Detalhe |
|---|---|
| **Motor de pitch-shift offline** | Phase vocoder próprio (FFT 2048, hop 512, janela Hann, propagação de fase estilo Laroche/Dolson) mantendo **duração igual ao original**. Isolado em `PitchShiftEngine::transpose` — a engine pode ser trocada (ex.: Rubber Band) sem tocar no resto. |
| **Preserva sincronia por construção** | Como o buffer transposto tem o **mesmo comprimento/taxa** do original, o mapeamento transport→amostra não muda; a reprodução só troca o conteúdo. |
| **Tabela de instrumentos** | Piano/Trombone (C, 0 st) • Trompete/Sax Tenor (Si♭, −2) • Sax Alto (Mi♭, +3) • Trompa (Fá, **+5**) • Ajuste Manual. Convenção: menor intervalo assinado entre o escrito (partitura em dó) e o som real do instrumento. |
| **Afinação de referência** | Slider 432–445 Hz (padrão 440 Hz), passo 0,1 Hz. |
| **Transposição manual** | Slider −12 a +12 semitons, ativo somente no modo "Ajuste Manual". |
| **Fórmula combinada** | `ratio = 2^(semitons/12) × (referência / afinação_detectada)` — compensação de instrumento, afinação de referência **e** afinação real do arquivo em um único fator. |
| **Recálculo sob demanda** | Timer (4 Hz, message thread) detecta mudança no alvo (tolerância 1e-4) ou arquivo novo → recalcula fora do audio thread. Razão = 1,0 usa o buffer original direto. |
| **Robustez** | Se o resultado conter NaN/inf, o buffer original é mantido (fallback seguro). |

### Limitações conhecidas do motor (para decisão)
- Vocoder de fase **degrada transientes** (materiais percussivos) e pode apresentar "phasiness" em notas longas para deslocamentos grandes (±12 st).
- **Sentido da transposição validado em código (V1, 26/09/2026)**: a cadeia `semitonesFor → computeCurrentPitchRatio → vocoder → status da UI` preserva o sinal (ratio > 1 = sobe) e a convenção da tabela é "menor intervalo assinado escrito→soante". Corrigida a **Trompa (Fá)**: era −5 (caía em **Sol**) e passou a **+5** (fiel à oitava: −7). O comentário de `Instrument.h` descrevia o sentido inverso e foi reescrito. **Pendente:** confirmação A/B no ouvido com músicos.
- O recálculo roda na thread de mensagens: um retune bloqueia a UI do editor por um curto período (≈ sub-segundo a poucos segundos); o áudio continua tocando.

---

## 4. Análise de tempo (carga)

- **BPM estimado** ao carregar: envelope de onset + comb filter, varredura 60–180 BPM, com ajuste de oitava (dobro/metade). Precisão arredondada a 0,5 BPM.
- **Assinatura de tempo ("o chão")**: detecção de downbeat (tempo forte) com grades de compasso alinhadas por fase; retorna **3 (3/4)** ou **4 (4/4)**. Heurística — o 4/4 é o padrão e só cede ao 3/4 quando a evidência de acentuação ternária é clara.
- **Afinação detectada ("A=…Hz")**: estima o desvio global do arquivo em cents relativo a A=440 via histograma de picos espectrais pela nota temperada mais próxima (FFT). Aplicada automaticamente na transposição; 440 Hz quando inconclusivo.
- **Compassos (N/4)** estimados = `duração × BPM / 60 / N` com o N detectado (eq. da timeline `C = T×BPM/(60×N)`).
- **Duração** exibida em `mm:ss`.
> Obs.: a análise é heurística e serve de referência de setup; não altera a reprodução. O mapa exportado é uma **sugestão** — a métrica definitiva é sempre a do hospedeiro/partitura.

### 4.1 Exportação de Mapa de Tempo (MIDI)

- **Botão "Exportar Mapa (.mid)"** na interface, habilitado quando há áudio com BPM detectado.
- Gera um arquivo **MIDI 1.0 / RP-001 (formato 0, sem notas)** com o mapa de tempo do áudio: **andamento (BPM)** e **assinatura de tempo detectada (N/4)**, consolidados em `Source/MidiMapExporter.cpp` (engine isolada).
- NomUn do arquivo sugerido: `<áudio> - mapa de tempo.mid`.
- Uso: o usuário importa/solta o `.mid` no MuseScore/DAW; o hospedeiro herda a grade calculada. A vasta limitação VST3 (Slave/Master) impede injeção direta de tempo — por isso o arquivo.
- **Decisão v1:** BPM constante (mapa mono-seção); `totalBars` não é codificado (MIDI não define esse evento).

---

## 5. Modo Treino

- **Velocidade ajustável sem mudar tom**: slider 50–150% (blend de 1%). Rastreia a duração em tempo real via time-stretch (pitch preservado).
- **Loop de trecho por compassos**: toggle "Repetir trecho" + sliders "Loop início/fim (compasso)". O trecho repete continuamente, alinhado à métrica detectada (BPM/assinatura).
- **Engine única**: o vocoder agora faz `transform(pitchRatio, durationScale)` em **uma passada** — evita o dobro de artefatos e mantém sincronia por mapeamento 1:1 (o conteúdo já vem com a duração escalada).
- **Estado no status**: `Treino | 70% | Loop 5-12 | mm:ss.dd`.

> Obs.: a posição do compasso depende da precisão do BPM detectado; o loop é uma repetição contínua até desligar.

## 6. Interface gráfica (editor)

Editor redesenhado em **proporção 5:4** (1000×800 inicial, mínimo 800×640, proporção fixa), tema escuro "Vidro Rítmico" com tokens em `Source/Theme.h`. Painéis numerados de 01 a 06:

| Painel | Conteúdo |
|---|---|
| **01 / Faixa** | Nome do arquivo (com tooltip), metadados (extensão · tamanho em MB), **Carregar/Trocar áudio**, forma de onda com picos, cursor de reprodução e marcações do loop, posição/duração e dica de operação. |
| **02 / Transporte** | Tocar/Parar, volta ao início, **Exportar Mapa (.mid)**, tiles de posição e velocidade de treino, estado (Pronto/Tocando/Fim do arquivo), botão de **Mute** da saída local. |
| **03 / Instrumento** | Anel com o símbolo do tom, nome do instrumento, afinação em semitons, grade de 7 botões de seleção rápida. |
| **04 / Afinação** | Referência A4 (432–445 Hz), transposição manual (−12…+12 st), detecção de afinação do arquivo em cents, botão de reset. |
| **05 / BPM e Métrica** | BPM detectado, assinatura de tempo, número de compassos e duração efetiva. |
| **06 / Análise e Saída** | Estado da análise (Aguardando áudio / BPM não detectado), canal de saída (Estéreo/Mono), precisão da detecção de afinação e mensagens de exportação. |

- Atualização de estado a 20 Hz; controles vinculados aos parâmetros via `AudioProcessorValueTreeState`.
- Toda interpolação de texto passa por `Text::from`/`Text::format` — nunca `juce::String::formatted`, que quebra acentos no Windows.


---

## 7. Parâmetros expostos (automação / persistência)

| ID no APVTS | Tipo | Faixa / Opções | Padrão |
|---|---|---|---|
| `instrument` | choice | Piano (C) • Trombone (C) • Trompete (B♭) • Sax Tenor (B♭) • Sax Alto (E♭) • Trompa (F) • Ajuste Manual | Piano (C) |
| `referencePitch` | float | 432,0 – 445,0 Hz (passo 0,1) | 440,0 |
| `transpose` | int | −12 … +12 st | 0 |
| `trainingSpeed` | float | 0,50 – 1,50 (passo 0,01) | 1,0 |
| `loopEnabled` | bool | liga/desliga | false |
| `loopStart` | int | 1 – 10000 (compasso) | 1 |
| `loopEnd` | int | 1 – 10000 (compasso) | 1 |
| `muted` | bool | silencia a saída local sem parar o transport | false |

- Estado persistido no DAW (`getStateInformation` / `setStateInformation` — XML binário).
- Sem entradas/saídas MIDI; um único programa ("Default").

---

## 8. Fluxo de uso típico

```
Abrir MuseScore 4 → adicionar PartePlay (mixer/efeito)
→ Carregar Áudio
→ (opcional) conferir BPM/compassos exibidos
→ escolher Instrumento e/ou ajustar Afinação / Transposição
→ Play → partitura + áudio sincronizados, no tom do instrumento
→ seek/voltar → áudio acompanha
→ Exportar Mapa (.mid) → importar no MuseScore para herdar a grade (BPM/assinatura)
→ Modo Treino → reduzir velocidade e/ou repetir trecho para estudo
```

---

## 9. Estrutura e build

```
Source/
  PluginProcessor.cpp/.h   — núcleo, parâmetros, sincronia, orquestração da transposição
  PluginEditor.cpp/.h      — editor 5:4 (painéis, LookAndFeel, layout)
  Theme.h                  — tokens visuais e rotinas de pintura (fonte única do tema)
  FilePlayer.cpp/.h        — player de arquivo (buffer + playback + "output fill" + picos)
  PitchShifter.cpp/.h      — engine de transposição offline (isolada)
  TempoAnalyser.cpp/.h     — análise BPM, métrica e afinação
  Instrument.cpp/.h        — domínio musical (instrumentos, afinação) — fonte única
  ParameterIds.h           — identificadores dos parâmetros (fonte única)
  MidiMapExporter.cpp/.h   — exportação do mapa de tempo (MIDI)
  Text.cpp/.h              — i18n e interpolação de texto sem mojibake
  Waveform.h               — estrutura de picos da forma de onda
scripts/
  build.ps1                — configure + build (Release/Debug) via preset "msvc"
  install-vst3.ps1         — copia o .vst3 para C:\Program Files\Common Files\VST3 (admin)
```

- Presets CMake: `msvc` (Visual Studio 2022 x64) e `ninja` (CLion).
- Artefato de saída: `build/msvc/PartePlay_artefacts/Release/VST3/PartePlay.vst3`.
- Destino de instalação: `C:\Program Files\Common Files\VST3\PartePlay.vst3`.

---

## 10. Funcionalidades ainda NÃO implementadas (para referência da fusão)

- Presets por partitura/arranjo (salvar/recuperar).
- Processamento em lote (transpor arquivos sem DAW).
- Análise automática de tonalidade do áudio.
- Afinações regionais (viola caipira, cordas).
- Testes automatizados (unitários de domínio musical e de integração host↔áudio).
- Multilíngue (PT/EN/ES) e campanha/comercialização.