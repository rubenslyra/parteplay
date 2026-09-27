# PartePlay

Plugin VST3 que reproduz um **áudio de referência sincronizado ao transporte do hospedeiro** (MuseScore 4 ou qualquer DAW) e o **transpõe para o tom do instrumento da partitura** — sem descompasso e sem alteração de velocidade.

**Versão:** 0.1.0 · **Plataforma:** Windows · **Formato:** VST3 · **Stack:** C++ / JUCE 9.0.2 / CMake / Visual Studio 2022

---

## O problema

Quem escreve para instrumento transpositor conhece o retrabalho: a partitura está em Si♭, mas o áudio de referência está em Dó. Para o instrumento soar na tonalidade certa é preciso tratar o tom duas vezes — a da partitura e a do instrumento — e qualquer erro de meio-tom escorrega para o conjunto.

O PartePlay resolve isso em um ponto só: ele lê o tom do instrumento, aplica a transposição sobre o áudio e o alinha ao relógio do hospedeiro. O músico toca a partitura e ouve o material na tonalidade correta, no tempo correto.

## O que ele faz hoje

- **Sincronia real**: a posição do áudio é derivada do relógio do host (`timeInSamples`), não de um relógio próprio. Play, pause, seek, avanço e retrocesso acompanham sem deriva.
- **Transposição por instrumento**: tabela com Piano, Trombone, Trompete, Sax Tenor, Sax Alto, Trompa e Ajuste Manual, combinando o tom do instrumento, a afinação de referência (A4 432–445 Hz) e a afinação real detectada no arquivo.
- **Pitch-shift sem mudança de velocidade**: vocoder de fase (FFT 2048 / hop 512) em passagem única, mantendo o comprimento do buffer — por construção a sincronia não se altera.
- **Análise de tempo**: BPM, assinatura de tempo (3/4 ou 4/4), número de compassos e afinação detectada, com exportação do mapa de tempo em **MIDI 1.0** para importar no MuseScore ou na DAW.
- **Modo treino**: velocidade de 50–150% e loop de trecho em compassos, alinhado à métrica detectada.
- **Editor 5:4** com tema escuro, forma de onda, loop visível, tiles de análise e saída estéreo ou mono.

## Tabela de instrumentos

A convenção é o **menor intervalo assinado** entre o tom escrito (partitura em Dó) e o som real do instrumento:

| Instrumento | Tom | Semitons |
|---|---|---|
| Piano | Dó | 0 |
| Trombone | Dó | 0 |
| Trompete | Si♭ | −2 |
| Sax Tenor | Si♭ | −2 |
| Sax Alto | Mi♭ | +3 |
| Trompa | Fá | +5 |
| Ajuste Manual | — | livre (−12…+12) |

O fator aplicado é `2^(semitons/12) × (referência / afinação detectada)`. Razão 1,0 usa o buffer original, sem reprocessamento.

## Requisitos

- Windows 10/11
- Visual Studio 2022 (Build Tools com workload *Desktop development with C++*)
- CMake 3.22 ou superior
- JUCE 9.0.2 — o caminho fica em `JUCE_ROOT` (padrão no preset: `D:/Program Files/JUCE`)
- Para uso musical: MuseScore 4 ou DAW com suporte a VST3

## Compilação

```powershell
# Configura e compila em Release
cmake --preset msvc
cmake --build --preset msvc --config Release

# Alternativa: Ninja (CLion)
cmake --preset ninja
cmake --build --preset ninja
```

Ou use o script, que encadeia configure + build:

```powershell
.\scripts\build.ps1 -Configuration Release
```

Artefato gerado:

```
build/msvc/PartePlay_artefacts/Release/VST3/PartePlay.vst3
```

## Instalação

Feche o MuseScore (ou a DAW) antes de instalar — o binário fica travado enquanto o plugin está carregado.

```powershell
.\scripts\install-vst3.ps1
```

O script remove a instalação anterior, copia o bundle para `C:\Program Files\Common Files\VST3\PartePlay.vst3` e confere o **hash SHA-256** do binário instalado contra o do build. Exige execução como administrador.

O template de partitura do MuseScore é instalado separadamente:

```powershell
.\scripts\install-musescore-template.ps1
```

## Uso

1. Abra o MuseScore 4 e adicione o PartePlay como efeito.
2. **Carregar áudio** — WAV, FLAC, OGG ou MP3.
3. Confira BPM, métrica e compassos exibidos; se quiser, exporte o mapa de tempo em `.mid` e importe no hospedeiro.
4. Escolha o **instrumento** e ajuste a **afinação de referência** se necessário.
5. Toque: partitura e áudio juntos, na tonalidade do instrumento.
6. Para estudo, reduza a **velocidade de treino** e/ou ative o **loop de trecho**.

## Parâmetros

| ID | Faixa / Opções | Padrão |
|---|---|---|
| `instrument` | Piano (C) · Trombone (C) · Trompete (B♭) · Sax Tenor (B♭) · Sax Alto (E♭) · Trompa (F) · Ajuste Manual | Piano (C) |
| `referencePitch` | 432,0 – 445,0 Hz (passo 0,1) | 440,0 |
| `transpose` | −12 … +12 semitons | 0 |
| `trainingSpeed` | 0,50 – 1,50 | 1,00 |
| `loopEnabled` | liga/desliga | false |
| `loopStart` / `loopEnd` | 1 – 10000 (compasso) | 1 |
| `muted` | silencia a saída local | false |

Todos os parâmetros são gravados na sessão do hospedeiro e ficam disponíveis para automação.

## Arquitetura

```
Source/
  PluginProcessor   — núcleo: parâmetros, sincronia, orquestração da transposição
  PluginEditor      — editor 5:4 (painéis, LookAndFeel, layout)
  Theme.h           — tokens visuais e rotinas de pintura
  FilePlayer        — carregamento, reprodução, loop, picos da forma de onda
  PitchShifter      — engine de pitch-shift offline (isolada, substituível)
  TempoAnalyser     — BPM, métrica, compassos e afinação
  Instrument        — domínio musical: instrumentos e semitons (fonte única)
  ParameterIds.h    — identificadores de parâmetro (fonte única)
  MidiMapExporter   — mapa de tempo em MIDI 1.0
  Text              — i18n e interpolação de texto
  Waveform.h        — estrutura de picos
```

Duas decisões estruturais merecem destaque:

- **Buffers por `shared_ptr<const>`**: o áudio original e o transposto são snapshots imutáveis trocados atomicamente. Uma mudança de parâmetro não interrompe a reprodução.
- **Texto sempre por `Text::from`/`Text::format`**: `juce::String::formatted` converte o formato com `String(const char*)` e, no Windows, usa `_vsnwprintf`, que exige `wchar_t*` em `%s` — a causa raiz dos acentos quebrados. Toda interpolação passa por `Text`.

## Limitações conhecidas

- O vocoder de fase degrada transientes (material percussivo) e pode soar "fantasiado" em notas longas com deslocamentos grandes (±12 semitons).
- O recálculo do pitch-shift roda na thread de mensagens e pode congelar a interface por um instante (o áudio continua tocando).
- BPM, métrica e afinação são **heurísticos** e servem de referência de montagem; a verdade é sempre a do hospedeiro.
- A injeção direta de tempo no host é limitada pela API VST3 (relações Slave/Master) — por isso a exportação do mapa em `.mid`.

## Documentação

A documentação de desenvolvimento vive em [`docs-dev/`](docs-dev/):

| Arquivo | Conteúdo |
|---|---|
| [`Funcionalidades-Atuais.md`](docs-dev/Funcionalidades-Atuais.md) | O que o plugin faz hoje, com detalhe técnico por área |
| [`Especificacao-Tecnica.md`](docs-dev/Especificacao-Tecnica.md) | Especificação integrada e escopo das fases futuras |
| [`Analise-e-Roadmap.md`](docs-dev/Analise-e-Roadmap.md) | Riscos, decisões e notas de arquitetura |
| [`Proximos-Passos.md`](docs-dev/Proximos-Passos.md) | Handoff e validações pendentes |
| [`visual/5x4-camadas/`](docs-dev/visual/5x4-camadas/) | Referência visual em camadas do editor (canvas 2000×1600) |

## Licença

O JUCE 9 é **duplo-licenciado**: [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.html) ou a licença comercial da JUCE. O AGPLv3 é mais restritivo que a GPL — exige disponibilité do código-fonte a quem recebe o binário, o que vale para uso em rede além da distribuição.

Enquanto o projeto não decidir por uma dessas vias, o caminho compatível é: publicar o código-fonte sob AGPLv3 junto do binário, ou adquirir a licença comercial da JUCE para fechar a distribuição. Essa é uma decisão de negócio ainda em aberto.

Feito para músicos e arranjadores · Rubinho Lyra / Software Eng
