# Análise e Roadmap — PartePlay (VST3)

**Data:** 26/09/2026 | **Versão:** 0.1.0 → 0.2.0 (alvo)
**Documento-base:** "Proposta e Especificação Técnica.md" (docs-dev)
**Normas de referência:** ABNT NBR ISO/IEC 12207 · ABNT NBR ISO/IEC 25010 · ISO 16 · MIDI 1.0 (MMA/AMEI) · VST3 (Steinberg) — ver Seção 5.

---

## 1. Veredito da Especificação Técnica Integrada

A proposta une a base já existente com 4 módulos novos. Cotejando com o código atual:

| Item | Situação | Fonte no código |
|---|---|---|
| Sincronia 1:1, play/pause/seek, taxa diverge, fim sem loop, thread-safe | ✅ Já implementado | `FilePlayer::fillOutput`, `processBlock` |
| Mack visão: vocoder offline (FFT 2048, hop 512, Hann) preservando duração | ✅ Já implementado | `PitchShifter.cpp` |
| Tabela de instrumentos + calibração 432–445 + manual ±12 st | ✅ Já implementado | `PluginProcessor` (APVTS) |
| Fórmula `C = T×BPM/(60×N)` (4/4) | ✅ Já implementado | `FilePlayer::runTempoAnalysis` (`measures44`) |
| Recálculo sob demanda com fallback seguro | ✅ Já implementado | Timer 4 Hz + geração |
| **Detecção autônoma de afinação** (ex.: pico A4=432 Hz) | 🆕 A implementar | alimenta `referencePitch` em cents |
| **BPM por transientes + downbeat + 3/4×4/4** | 🆕 A implementar | sugestão para o Mapa; ver risco Sec. 3 |
| **Mascaramento da timeline (edição não-destrutiva)** | ⚠️ Redesign necessário | ver Sec. 3 |
| **Mapa de Tempo via MIDI** | 🆕 Em implementação (v1) | `MidiMapExporter` |
| Painel consolidado de Inteligência | 🆕 A implementar | consumo do analisador |

Conclusão: a proposta é **tecnicamente coerente** e 60% está entregue. Os pontos que a redigiram de modo mais otimista do que a realidade da API VST3 estão na Seção 3.

---

## 2. Decisões de arquitetura confirmadas

1. **Sincronia por construção**: transposição é **offline** pré-processada; versão transposta mantém comprimento/taxa original → o mapeamento transport→amostra nunca muda.
2. **Engine de transposição isolada**: `PitchShiftEngine::transpose` permite trocar o vocoder por outra engine (ex.: Rubber Band) sem tocar no núcleo.
3. **Análise na carga, thread de mensagens para retune**: nunca bloquear o audio thread; troca atômica via `shared_ptr` sob `audioLock`.
4. **Métrica do host é autoritativa para sincronia**: o `AudioPlayHead` do VST3 já expõe `getTempo()` e `getTimeSignature()`; a análise de métrica do **áudio** serve somente para **sugerir** o mapa e o "chão", nunca para forçar o host (limitação Slave/Master do VST3, conforme a própria proposta).

---

## 3. Pontos que exigem cuidado (redesign / limitação real)

### 3.1 Mascaramento da timeline
Um VST3 **não dispõe** do "número de compassos da partitura" do host — a interface playhead não expõe a extensão do arranjo. Além disso, no MuseScore a reprodução termina no fim da partitura (`isPlaying=false`), então o plugin já silencia naturalmente nesse fluxo; o caso real é o DAW tocando além do arranjo.
**Decisão v1:** limite derivado do próprio áudio (compassos estimados × BPM do host) ou dos *loop points* do host quando ativos. Redesign detalhado quando o modo treino entrar.

### 3.2 Drag-and-drop MIDI entre aplicações
Arrastar do editor do plugin para o canvas do MuseScore exige OLE (`IDataObject`) no Windows — o JUCE não embrulha esse fluxo.
**Decisão v1:** botão **"Exportar Mapa (.mid)"** que gera o arquivo com tempo-map (metas, sem notas); o usuário solta/importa no MuseScore, que importa MIDI nativamente. Drag-and-drop OLE fica como melhoria posterior.

### 3.3 Downbeat / 3-4×4-4
Inferência de acentuação em graves tem precisão mediana e é pesquisa de sinal razoavelmente complexa. **Decisão:** tratar como **sugestão** sempre confirmável pelo usuário; prioridade baixa até o Mapa (v1, constante 4/4) validar o fluxo.

### 3.4 Suposição de andamento constante na fórmula `C`
`C = T×BPM/(60×N)` assume BPM constante — correto para o v1 (música com template fixo). Variações de andamento entram em iteração futura (mapa multi-seção).

---

## 4. Roadmap priorizado

> Ordem por (valor para validação × esforço), com rastreabilidade à Seção 5.

| # | Item | Esforço | Valor | Atributo 25010 alvo | Status |
|---|---|---|---|---|---|
| 1 | **Exportar Mapa de Tempo (.mid)** — BPM + assinatura + compassos | M | Alto (diferencial) | Adequação funcional, compatibilidade | 🚧 Em implementação |
| 2 | **Afinação autônoma** — detectar desvio em cents e ajustar automaticamente a transposição | M | Alto | Adequação funcional, exatidão | ✅ Implementado (histograma de cents via FFT; `A=…Hz` no painel) |
| 3 | **Modo treino** — loop de trecho + velocidade sem mudar tom (reuso do vocoder p/ time-stretch) | M | Alto | Usabilidade, adequação funcional | ✅ Implementado (transform pitch+time em 1 passada; loop por compassos; velocidade 50–150%) |
| 4 | **Presets por partitura/arranjo** (salvar/restaurar APVTS + metadados) | B | Médio | Portabilidade, usabilidade | :white_large_square: |
| 5 | Melhorar BPM (transientes) + downbeat e 3/4×4/4 como sugestão | M–A | Médio | Exatidão, adequação funcional | 🚧 Downbeat/3-4×4-4 implementado (heurística); refino com aubio pendente |
| 6 | Visualizador de waveform | M | Baixo–Médio | Usabilidade | :white_large_square: |
| 7 | Mascaramento de timeline (v1 loops host) | B–M | Baixo | Usabilidade | :white_large_square: |
| 8 | Tonalidade automática + afinações regionais + i18n + lote | A | Fase de lançamento | — | 🚧 i18n base concluída (PT/EN/ES, seletor e UTF-8); tonalidade/lote pendentes |

### Critérios de aceite da iteração 1
- [ ] MuseScore 4 importa o `.mid` e herda BPM e assinatura.
- [ ] Export válido para WAV/FLAC/MP3 com BPM ≥ 60.
- [ ] Sem regressões: transposição + sincronia intocados (direção verificada em código — V1; resta a confirmação A/B no ouvido).

---

## 5. Conformidade com normas e padrões

O desenvolvimento observa as seguintes normas aplicáveis ao produto (plugin de áudio para partitura):

| Norma | Escopo no projeto |
|---|---|
| **ABNT NBR ISO/IEC 12207** | Processos de ciclo de vida de software (desenvolvimento, manutenção, documentação). |
| **ABNT NBR ISO/IEC 25010** (sucede a 9126) | Modelo de qualidade — mapeamento na Seção 7. |
| **ISO 16:2005** | Frequência de afinação padrão (A = 440 Hz) — fundamenta o padrão do slider de afinação e a fórmula de centavos. |
| **MIDI 1.0 / RP-001 (MMA & AMEI)** | Formato do arquivo de mapa de tempo exportado (eventos de tempo, assinatura e nome de trilha). |
| **VST3 SDK (Steinberg)** | Interface do plugin; respeito ao contrato Slave/Master do transport. |
| **ISO 9241** | Princípios de *ergonomia da interação humano-sistema* para a interface. |
| **ITU-R BS.1770 / EBU R128** (futuro) | Baliza para sonoridade da saída quando houver mistura/normalização. |

> 📌 Não se aplicam: **IEC 62304** (software médico) e **ISO 26262** (segurança veicular) — fora do domínio do produto; não serão adotadas, evitando custo indevido de conformidade.

**Decisão de codificação de fonte:** UTF-8 **com BOM** (RFC 3629) em todos os arquivos de texto do repositório (`.cpp/.h/.md/.ps1/.json/.gitignore`), reforçada pelo `/utf-8` do MSVC e pelo `.editorconfig`. O legado **ISO/IEC 8859-1** foi **descartado por conflito técnico**: fontes sem BOM em Latin-1 são decodificadas como ANSI/Windows-1252 quando `/utf-8` não se aplica, quebrando o texto pt-BR; a BOM torna a codificação inequívoca para compilador, PowerShell 5.1 e editores Windows.
>
> **Padrão de strings (runtime):** o construtor `juce::String(const char*)` do JUCE 9 interpreta a entrada como 8-bit ASCII, quebrando acentos. Toda literal de interface/usuário deve ser criada por **`Text::from("...")` → `juce::String::fromUTF8`** (`Source/Text.h`) — inclusive literais atualmente ASCII, para textos futuros com acentos não quebrarem silenciosamente.

### 6. Mapeamento ao ABNT NBR ISO/IEC 25010

| Atributo de qualidade | Como o projeto atende |
|---|---|
| **Adequação funcional** | Sincronia 1:1, transposição por instrumento, análise BPM, export MIDI — rastreáveis por feature / teste de aceite. |
| **Eficiência de desempenho** | Transposição fora do audio thread; playback com interpolação linear económica (nenhum DSP por-bloco). |
| **Compatibilidade** | VST3 em MuseScore 4 e DAWs; WAV/FLAC/MP3/OGG via JUCE. |
| **Usabilidade** | Interface com estado explícito (status, análise, transposição ativa); padrão ISO 9241. |
| **Confiabilidade** | Fallback automático para o buffer original em falha do vocoder; thread-safe por `audioLock` + `shared_ptr`. |
| **Manutenibilidade** | Engines isoladas (transposição, análise, exportação); domínio centralizado em `Source/Instrument.*`, `ParameterIds.h`. |
| **Portabilidade** | Código sem dependência fora do JUCE para o núcleo; migração de engine prevista (RB). |

---

## 7. Riscos e mitigação

| Risco | Prob. | Impacto | Mitigação |
|---|---|---|---|
| Qualidade do vocoder em material percussivo | M | M | Aceitar v1; engine isolada p/ troca por Rubber Band |
| Direção da transposição não confirmada no ouvido (cadeia e tabela já revisadas em código; Trompa corrigida −5 → +5) | B | A | Teste A/B com músicos antes da beta |
| Pausa da UI na thread de mensagens no retune | M | B | Computação por 2s máx.; troca atômica; migrar p/ thread própria se reclamaram |
| BPM heurístico impreciso → mapa errado | M | M | Sugerir + permitir correção manual e re-export |

---

## 8. Próximas 3 ações

1. [x] Documentar (este arquivo + atualização de Funcionalidades-Atuais).
2. [ ] **Exportar Mapa (.mid)** — implementação + build + teste no MuseScore.
3. [ ] Validação A/B **no ouvido** da direção de transposição com os acionistas do projeto (revisão de código concluída: `semitonesFor` = menor intervalo escrito→soante; Trompa corrigida de −5 para +5).