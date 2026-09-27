# PartePlay

![Status](https://img.shields.io/badge/status-em%20desenvolvimento-c94f4d)
![Versão](https://img.shields.io/badge/vers%C3%A3o-0.1.0-blue)
![Plataforma](https://img.shields.io/badge/plataforma-Windows%2010%2F11-0078d4?logo=windows)
![Formato](https://img.shields.io/badge/formato-VST3-ff5722)
![Host](https://img.shields.io/badge/host-MuseScore%204%20%2F%20DAW-8a2be2)
![JUCE](https://img.shields.io/badge/JUCE-9.0.2-3d2b8a?logo=juce)
![Linguagem](https://img.shields.io/badge/C%2B%2B-C%2B%2B17-00599c?logo=c%2B%2B&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.22%2B-064f8c?logo=cmake&logoColor=white)
![Testes](https://img.shields.io/badge/testes-110%20verifica%C3%A7%C3%B5es%20%E2%9C%85-4c9a2c)
![Docs](https://img.shields.io/badge/docs-dev%2F-blue)
![Codificação](https://img.shields.io/badge/encoding-UTF--8%20BOM-0f7cbf)

Plugin VST3 que reproduz um **áudio de referência sincronizado ao transporte do hospedeiro** (MuseScore 4 ou qualquer DAW) e o **transpõe para o tom do instrumento da partitura** — sem descompasso e sem alteração de velocidade.

**Versão:** 0.1.0 · **Plataforma:** Windows · **Formato:** VST3 · **Stack:** C++17 / JUCE 9.0.2 / CMake / Visual Studio 2022

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

## Testes

O domínio musical e a camada de texto têm suíte automatizada (sem UI e sem audio thread), wired no CTest:

```powershell
cmake --build --preset msvc --config Release --target PartePlayTests
ctest --test-dir build\msvc -C Release --output-on-failure
```

São 110 verificações em cinco grupos, e o primeiro existe justamente para travar a correção musical sem depender do ouvido:

| Grupo | O que fixa |
|---|---|
| **V1 — Tabela de instrumentos** | Piano/Trombone 0 · Trompete/Sax Tenor −2 · Sax Alto +3 · Trompa **+5** · Manual 0. Nenhum instrumento pode voltar a −5, que soaria Sol em vez de Fá. Ida e volta de índice. |
| **V1 — Direção do ratio** | `2^(semitons/12)` com sinal correto: +5 sobe, −2 desce, 5 e −5 são inversos exatos. Aritmética em cents (uma oitava = 1200 cents). |
| **V1 — Compensação de referência** | Arquivo em A=432 com referência 440 corrige ≈ +31,8 cents; afinação inválida não explode o ratio. |
| **V2 — Encoding e i18n** | `Text::from` devolve UTF-8 intacto, `Text::format` interpola com acentos, pt-BR devolve a chave, en/es traduzem, e nenhuma saída carrega sequência duplamente codificada. |
| **V2 — Nomes exibidos** | Os 7 nomes de instrumento são distintos e todos têm símbolo, tonalidade e semitons com sinal. |

O `install-vst3.ps1` roda essa suíte antes de instalar e cancela se algo falhar (`-SkipTests` ignora).

## Instalação

Feche o MuseScore (ou a DAW) antes de instalar — o binário fica travado enquanto o plugin está carregado.

```powershell
.\scripts\install-vst3.ps1
```

O script roda os testes de domínio, remove a instalação anterior, copia o bundle para `C:\Program Files\Common Files\VST3\PartePlay.vst3` e confere o **hash SHA-256** do binário instalado contra o do build. Exige execução como administrador.

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
  Instrument        — domínio musical: instrumentos, semitons e ratio (fonte única)
  ParameterIds.h    — identificadores de parâmetro (fonte única)
  MidiMapExporter   — mapa de tempo em MIDI 1.0
  Text              — i18n e interpolação de texto
  Waveform.h        — estrutura de picos
Tests/
  DomainTests.cpp   — gate V1 (convenção e direção) e invariantes de encoding
```

Três decisões estruturais merecem destaque:

- **Buffers por `shared_ptr<const>`**: o áudio original e o transposto são snapshots imutáveis trocados atomicamente. Uma mudança de parâmetro não interrompe a reprodução.
- **Texto sempre por `Text::from`/`Text::format`**: `juce::String::formatted` converte o formato com `String(const char*)` e, no Windows, usa `_vsnwprintf`, que exige `wchar_t*` em `%s` — a causa raiz dos acentos quebrados. Toda interpolação passa por `Text`.
- **Matemática musical no domínio**: `Tuning::playbackRatio` concentra o cálculo `2^(semitons/12) × referência/afinação_do_arquivo` em código puro e testável, fora do processor. O gate V1 é verificável sem host e sem ouvido.

## Limitações conhecidas

- O vocoder de fase degrada transientes (material percussivo) e pode soar "fantasiado" em notas longas com deslocamentos grandes (±12 semitons).
- O recálculo do pitch-shift roda na thread de mensagens e pode congelar a interface por um instante (o áudio continua tocando).
- BPM, métrica e afinação são **heurísticos** e servem de referência de montagem; a verdade é sempre a do hospedeiro.
- A injeção direta de tempo no host é limitada pela API VST3 (relações Slave/Master) — por isso a exportação do mapa em `.mid`.
- Não há cobertura de testes para o áudio em si (sincronia sob time-stretch, vocoder): ainda depende de validação no host.

## Documentação

A documentação de desenvolvimento vive em [`docs-dev/`](docs-dev/):

| Arquivo | Conteúdo |
|---|---|
| [`Funcionalidades-Atuais.md`](docs-dev/Funcionalidades-Atuais.md) | O que o plugin faz hoje, com detalhe técnico por área |
| [`Especificacao-Tecnica.md`](docs-dev/Especificacao-Tecnica.md) | Especificação integrada e escopo das fases futuras |
| [`Analise-e-Roadmap.md`](docs-dev/Analise-e-Roadmap.md) | Riscos, decisões e notas de arquitetura |
| [`Proximos-Passos.md`](docs-dev/Proximos-Passos.md) | Handoff e validações pendentes |
| [`Lancamento-e-Redes-Sociais.md`](docs-dev/Lancamento-e-Redes-Sociais.md) | Material de divulgação do vídeo de teste (LinkedIn, YouTube, Instagram, TikTok) e status real do produto |
| [`visual/5x4-camadas/`](docs-dev/visual/5x4-camadas/) | Referência visual em camadas do editor (canvas 2000×1600) |

## Licença

O JUCE 9 é **duplo-licenciado**: [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.html) ou a licença comercial da JUCE. O AGPLv3 é mais restritivo que a GPL — exige disponibilité do código-fonte a quem recebe o binário, o que vale para uso em rede além da distribuição.

Enquanto o projeto não decidir por uma dessas vias, o caminho compatível é: publicar o código-fonte sob AGPLv3 junto do binário, ou adquirir a licença comercial da JUCE para fechar a distribuição. Essa é uma decisão de negócio ainda em aberto — o repositório ainda não tem arquivo `LICENSE`.

---

## Autoria

**Rubens ("Rubinho") Lyra** — arquitetura e desenvolvimento do PartePlay.

| Atribuição | O que envolveu |
|---|---|
| **Arquitetura de software** | Modelo de domínio musical centralizado (`Instrument`, `ParameterIds`), separação de engines substituíveis, decisões de thread-safety, análise de viabilidade contra as limitações reais do VST3. |
| **C++ / JUCE / DSP de áudio** | Implementação do vocoder de fase, do player sincronizado ao transport, do analisador de tempo e do editor JUCE. |
| **Integração VST3 / MuseScore** | Contrato do SDK, respeitamento do papel Slave/Master do transporte, mapa de tempo em MIDI 1.0 e o template de partitura para o MuseScore 4. |
| **UI/UX e design system** | Redesenho do editor em 5:4, sistema de tokens (`Source/Theme.h`), painéis e identidade visual. |
| **Produto, roadmap e documentação** | Escopo e priorização, mapeamento às normas ISO/IEC, handoff de trabalho e este README. |
| **DevOps / build / release** | CMake com presets, scripts de instalação com verificação de hash, suíte de testes wired no CTest. |

- GitHub: [github.com/rubenslyra/parteplay](https://github.com/rubenslyra/parteplay)
- LinkedIn: [linkedin.com/in/rubenslyra](https://www.linkedin.com/in/rubenslyra)

Contribuições são bem-vindas. O código segue o estilo do JUCE e a convenção de domínio musical registrada em `Source/Instrument.h` — alterá-la é uma decisão de produto, não um detalhe de implementação.

---

## Referências

### Normas e padrões

| Norma | Escopo no projeto |
|---|---|
| **ISO/IEC 12207** (ABNT NBR ISO/IEC 12207) | Processos de ciclo de vida de software: desenvolvimento, manutenção e documentação. |
| **ISO/IEC 25010** (ABNT NBR ISO/IEC 25010) | Modelo de qualidade do produto de software — base do rastreamento de requisitos e atributos. |
| **ISO 16:1975** — *Acoustics — Standard tuning frequency (440 Hz) for musical pitch* | Frequência de afinação padrão e a convenção de centavos. (Nota: o repositório citava "ISO 16:2005"; a edição vigente é a de 1975, com confirmação em 2004.) |
| **MIDI 1.0 — RP-001** (MIDI Manufacturers Association / AMEI) | Formato do arquivo de mapa de tempo exportado (eventos de tempo, assinatura e nome de trilha). |
| **VST3 SDK** (Steinberg Media Technologies) | Interface do plugin, contrato de transporte e o papel Slave/Master. |
| **ISO 9241-210:2019** | Ergonomia da interação homem-sistema aplicada ao editor (design centrado no usuário). |
| **UTF-8 — RFC 3629** | Codificação de todo o texto do repositório, com BOM por decisão de projeto. |
| **C++17** (ISO/IEC 14882:2017) | Norma da linguagem em que o plugin é escrito. |

### Bibliografia técnica

- **JUCE 9.0.2** — *The JUCE Framework*. [Documentação](https://docs.juce.com/master/) e [licenciamento](https://juce.com/legal/juce-9-licence/).
- **Oppenheim, A. V.; Schafer, R. W.** — *Discrete-Time Signal Processing*. 3. ed. Pearson, 2010. Capítulos 7–9: DFT, transformada rápida de Fourier e análise por blocos sobrepostos.
- **Laroche, J.; Dolson, M.** — "Improved phase vocoder time-scale modification of audio". *IEEE Transactions on Speech and Audio Processing*, v. 7, n. 3, p. 259–266, 1999. Propagação de fase usada no `PitchShifter`.
- **Flanagan, J. L.** — "The Synthesis of Complex Audio Spectra by Means of Short-Term Fourier Analysis". *IEEE Transactions on Audio and Electroacoustics*, v. 10, n. 2, p. 119–126, 1962. Análise de envelopes por fluxo de sintonia, base da detecção de transientes.
- **Duxbury, P.** — *Synthesis and Simulation of Audio: A Digital Audio Approach*. Academic Press, 2000. Phase vocoder, transposição de tom e time-stretching.
- **Sethares, W. M.** — *Tuning, Timbre, Spectrum, Scale*. 2. ed. Springer, 2002. Por que a afinação de referência (432–445 Hz) importa na prática musical.
- **Bregman, A. S.** — "On the Representation of Durational Information in Music". *Journal of the Acoustical Society of America*, v. 37, n. 2, p. 244–258, 1965. Estrutura rítmica e metrical levels, base da detecção de downbeat.
- **Dixmier, A.** — *Mathématiques et musique*. Hermann, 1982. Formalismo matemático do sistema musical aplicado à transposição.
- **Mackey, N.** — *Time–Frequency Analysis: Fourier and Wavelet Transforms and Context*. 2. ed. Cambridge University Press, 2015. Cubos de constante-Q usados na detecção de tom.
- **Zwicker, E.; Fastl, H.** — *Psychoacoustics: Facts and Models*. 3. ed. Springer, 2013. Faixa de audibilidade que o detector de tom percorre.
- **Gómez, E.; Bonada, J.** — "Sinusoids and Transients in Sound Synthesis". *Journal of New Music Research*, v. 33, n. 2, p. 137–158, 2004. Separação harmônico-percussivo: por que vocoders escorregam em material percussivo.
- **Aguirre, M. D.; Wavre, P.; Harte, C.** — "Transposing time, frequency, and timbre with maximum flexibility". *EURASIP Journal on Applied Signal Processing*, v. 2004, n. 1, p. 289–306, 2004. Referência para engines alternativas ao `PitchShiftEngine`.

### Nomenclatura instrumental

- **MIMO — *Musical Instrument Museums Online*** (Horniman Museum and partners) — Recomendações de nomenclatura de instrumentos e de transposição, alinhadas à tabela de `Source/Instrument.h`.
- **MIDI Manufacturers Association (MMA) / AMEI** — *RP-001: Detailed MIDI Specification*. Formaliza os eventos de tempo e de assinatura de tempo usados pelo `MidiMapExporter`.

### Licenças

- **GNU AGPLv3** — *GNU Affero General Public License, version 3*. Seção 13, sobre interação remota. Ver [licença da JUCE](https://juce.com/legal/juce-9-licence/) para a via comercial.
- **MIDI 1.0** é especificação pública da MMA/AMEI, de uso livre.

### Ferramentas

- **Microsoft Visual Studio 2022** — compilador MSVC 14.44, C++17 com `/utf-8`; compila o plugin e a suíte de testes.
- **CMake 3.22+** — geração de build, presets MSVC/Ninja e integração com CTest.
- **MuseScore 4** (MuseScore Foundation) — hospedeiro de referência; destino do template de partitura e do mapa de tempo em MIDI.
- **Git** — controle de versão.
