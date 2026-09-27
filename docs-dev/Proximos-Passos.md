# Próximos Passos — PartePlay (handoff de retomada)

**Data:** 27/09/2026 | **Versão:** 0.1.0
**Complementa (não substitui):** `docs-dev\Analise-e-Roadmap.md` (escopo/roadmap) e `docs-dev\Funcionalidades-Atuais.md` (estado vivo do produto).

---

## Snapshot atual (onde paramos)

| Item | Estado |
|---|---|
| Transposição offline (vocoder, duração preservada) | ✅ commit |
| Mapa de Tempo MIDI (exportar .mid c/ BPM+assinatura) | ✅ commit |
| Análise: BPM, assinatura (3/4×4/4), afinação autônoma (A=…Hz) | ✅ commit |
| Modo Treino (loop por compassos + velocidade sem mudar tom) | ✅ commit |
| Organização de domínio (Instrument/ParameterIds/Text) + centralização | ✅ commit |
| Codificação UTF-8/BOM + i18n (PT/EN/ES, seletor, `Text::t`) | ✅ commit |
| Template MuseScore + script de instalação + README | ✅ commit |
| **Editor 5:4 reescrito** (painéis 01–06, `Theme.h`, waveform, mute) | ✅ compilado em Release |
| **V1 corrigido em código** (Trompa Fá: −5 → **+5**) | ✅ aguardando A/B no ouvido |
| **VST3 instalado** (bundle atual) | ✅ |
| Documentação limpa: README reescrito, `docs-dev/` versionado, obsoletos removidos | ✅ commit |

Comandos de rotina: `scripts\build.ps1` · `scripts\install-vst3.ps1` (admin, com MuseScore **fechado**) · `scripts\install-musescore-template.ps1`.

---

## 1) GATE de validação — fazer ANTES de implementar mais

| # | Validação | Como testar | Risco se pular |
|---|---|---|---|
| V1 | **Direção da transposição** — revisada em código (26/09) e corrigida: Trompa (Fá) −5 (soava Sol) → **+5**. Falta a confirmação no ouvido | Trompete (Si♭) deve **descer** 2 st; Trompa (Fá) deve **subir** 5 st; Sax Alto (Mi♭) deve **subir** 3 st. Conferir no painel 02 | Correção musical errada contamina todo o resto |
| V2 | **Editor 5:4 renderizado** | Abrir o plugin com uma faixa carregada: 6 painéis sem sobreposição, sem texto cortado, redimensionar para o mínimo (800×640) e conferir | Layout quebrado no tamanho mínimo |
| V3 | **Renderização pt-BR + i18n** | Trocar PT/EN/ES; conferir painel 06 (BPM, assinatura, afinação, compassos, duração) e nomes dos instrumentos | Se ainda quebrar, é fonte/host (investigar) |
| V4 | **Sincronia sob time-stretch** | Modo Treino a 70% + loop 5–12; áudio ainda bate com a partitura? | Engine generalizada precisa de confirmação |
| V5 | **Heurísticas de análise** | BPM/assinatura/afinação em 3–4 faixas (valsa 3/4, 4/4, gravação desafinada) | Mapa e correção saem errados |

---

## 2) Implementação (ordem recomendada, do roadmap)

| Ordem | Item | Esforço | Primeiro passo concreto |
|---|---|---|---|
| A | **Presets por partitura/arranjo** (roadmap item 4) | Baixo | Salvar/recuperar estado APVTS + análise p/ arquivo próprio do projeto (base p/ camada Pro) |
| B | **Testes automatizados** | Médio | Primeiro o domínio musical (`Instrument`, `Text::format`, cálculo do ratio) — são puros e não dependem de UI |
| C | **Refinar análise** (roadmap item 5) | Médio | Downbeat/assinatura como **sugestão**; considerar `aubio` atrás de interface (igual ao vocoder) |
| D | **Mascaramento de timeline** (roadmap item 7) | Baixo–Médio | v1 por *loop points* do host (ver §3.1 do Analise-e-Roadmap — playhead não expõe nº de compassos) |
| E | **Lote / afinações regionais / comercialização** (item 8) | Alto | Fase de lançamento |

> O visualizador de forma de onda (antigo item C) foi entregue no editor 5:4 (`Source/Waveform.h`).

---

## 3) Manutenção / conformidade (pendentes)

- **`CHANGELOG.md`** + notas de release concisas (o usuário pediu gerar mais tarde). Manter a partir dos commits.
- **Regra de i18n:** nenhuma string de UI solta — sempre `Text::t("chave pt-BR")`; adicionar colunas EN/ES na tabela em `Source\Text.cpp`. Interpolar **sempre** com `Text::format` (nunca `juce::String::formatted`, que quebra acentos no Windows). Padrão **UTF-8 com BOM** (RFC 3629) + `.editorconfig`.
- **Tema:** `Source\Theme.h` é a fonte de verdade das cores e do desenho; o protótipo web `docs-dev\visual\styles.css` deve acompanhá-lo.
- **Licença/comercial:** o JUCE 9 é **duplo-licenciado (AGPLv3 × licença comercial)** — não GPLv3, como constava no README antigo. A decisão de modelo (Free aberto × Pro fechado) continua em aberto e é pré-requisito para qualquer distribuição. Verificar licença ao adicionar libs externas (aubio = GPLv3; Rubber Band = GPLv3/comercial).
- **Normas aplicáveis** (mapeadas no Analise-e-Roadmap): ABNT NBR ISO/IEC 12207 e 25010 · ISO 16 (A=440) · MIDI 1.0/RP-001 · VST3 · ISO 9241.

---

## 4) Como retomar (checklist)

1. Rodar **V1–V5** no MuseScore (fechar antes de instalar; `install-vst3.ps1` precisa de admin).
2. Se V1 (direção) ainda parecer invertida no ouvido → corrigir `InstrumentTable` e revalidar **antes** de A.
3. Escolher o item da Seção 2 (default: **A — Presets**) e implementar + commitar por task.
4. Atualizar `Funcionalidades-Atuais.md` e marcar o item no `Analise-e-Roadmap.md`.
