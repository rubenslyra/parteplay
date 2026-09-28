# Playbook de Publicação — PartePlay (atualizado pós-refatoração)
# USO INTERNO. Textos públicos em INGLÊS. Data alvo: sábado, 3/out/2026, 10h BRT.

---

## BLOCO 0 — REGRAS DE OURO (não pode errar em nenhuma publicação)
1. ❌ **Nunca mencionar "horn / trompa"** (saiu do escopo v1.x).
2. História do bug = só o caso do **sax em E♭: +3 em vez de −9** (mesma classe de altura, oitava errada, passou no ouvido). Crédito: **Thiago Gonçalves (LinkedIn, 27/09/2026)**.
3. ✅ **Sempre declarar as 2 lacunas:** (L1) BPM automático ainda pede ajuste manual; (L2) faixa não aparece no container de instrumentos do MuseScore.
4. Dizer **"planned release / lançamento previsto"**, nunca "released/lançado". Versão: 0.1.0-rc.1 até a tag.
5. Testes: **345 verificações / 8 grupos** (confirme com `ctest` antes de publicar; se der outro número, use o real).
6. Repo: `github.com/rubenslyra/parteplay` · Autor: Rubens Lyra.
7. Instrumentos v1.x: piano, trompete, trombone (pistos/vara), flugelhorn, sax tenor/alto/soprano, ajuste manual.

---

## BLOCO 1 — LINKEDIN (post principal)
**Título:** I'm building a VST3 that fixes a working musician's problem — goes live Oct 3

**Corpo:**
If you play a transposing instrument, you know the rework: the part is in B♭, the reference recording is in C. Pitch gets corrected twice — and a half-tone error never looks like an error. It looks like the whole section is out of tune, with no one knowing why.

**PartePlay** fixes it in one place. It's a VST3 plugin that reads your instrument's key, transposes the reference audio, and locks it to the host transport — no drift, no tempo change.

A viewer caught a bug my ears missed: an E♭ sax was transposing +3 instead of −9 (same pitch class, wrong octave — it passes by ear). Thank you, Thiago Gonçalves. The fix: the table is now *interval + direction + octave*, the semitone is derived, and every instrument has a test. +3 and +5 are forbidden. 345 automated checks, 8 groups.

v1.x targets popular-music winds. Orchestral instruments are out by design.

Honest status — two gaps are open: (1) automatic BPM still needs manual adjustment; (2) the track doesn't yet appear in MuseScore's instrument panel. Both are the next milestones.

Planned release: **Saturday, Oct 3, 2026, 10h BRT**. Open source. Link in comments — try it, break it, open an issue. PRs welcome.

`#PartePlay #VST3 #AudioPlugin #CPP #JUCE #DSP #MusicProduction #MuseScore #MIDI #OpenSource #Saxophone #Brass`

---

## BLOCO 2 — YOUTUBE
**Títulos (escolha 1):**
- A VST3 that transposes your reference to your instrument's key (v1 + what's missing)
- Transposing reference audio without breaking sync — VST3 in C++/JUCE

**Descrição (curta):**
```
PartePlay — a VST3 plugin that plays a reference track in sync with the host
transport, transposed to your instrument's key. No drift. No tempo change.

This is the test video of the first working version — including what isn't ready.

Chapters: 00:00 The problem · 00:35 What it does · 01:10 Sync from the host ·
01:45 The octave bug (+3 vs −9) · 02:25 Two real gaps · 03:10 Architecture ·
03:55 Tech · 04:20 Status & next steps

⚠️ Not ready yet: automatic BPM sync (manual adjustment still needed) · track
not yet in MuseScore's instrument panel. Both are next milestones.
Planned release: Sat, Oct 3, 2026, 10h BRT.

Tech: C++17 · JUCE 9.0.2 · VST3 · MIDI 1.0 · phase vocoder
Links: github.com/rubenslyra/parteplay · linkedin.com/in/rubenslyra
```

**Comentário fixado:**
> Real status: automatic BPM still needs manual adjustment, and the track isn't
> in MuseScore's instrument panel yet. Both are next milestones. Planned release:
> **Sat, Oct 3, 2026, 10h BRT**. Your feedback on what to build first matters most now.

---

## BLOCO 3 — INSTAGRAM
**Legenda:**
```
PartePlay — a VST3 plugin that transposes your reference to your instrument's
key. No drift. No tempo change. 🎷

✔ per-instrument transposition (piano · trumpet · trombones · flugelhorn ·
  tenor/alto/soprano sax · manual)
✔ host-synced playback — play/pause/seek with no drift
✔ training mode: 50–150% speed + measure loop
✔ MIDI tempo-map export
✔ 345 automated checks across 8 groups

✗ automatic BPM sync still needs manual adjustment
✗ track not yet in MuseScore's instrument panel

Still in development. Planned release: Sat, Oct 3, 2026, 10h BRT.
Link in bio 👆

#parteplay #vst3 #musicproduction #dsp #cpp #juce #saxophone #trumpet
#musescore #midi #audioplugin #musictech
```

**Carrossel (6 slides, 1 linha cada):**
1. THE PROBLEM — Part in B♭, reference in C. A half-tone error = whole section out of tune.
2. THE FIX — One plugin reads the instrument key, transposes, locks to the host clock.
3. THE BUG — E♭ sax at +3 instead of −9. Same pitch class, wrong octave. Passed by ear.
4. THE FIX (technical) — interval + direction + octave, per-instrument tests. +3/+5 forbidden.
5. THE TECH — C++17 · JUCE 9 · VST3 · MIDI 1.0 · phase vocoder
6. THE STATUS — 2 gaps open, planned release Oct 3. Link in bio.

---

## BLOCO 4 — TIKTOK (roteiro 45–60s, vertical)
**0–3s (gancho, na tela):** "If you play sax or trumpet, your reference track is in the wrong key."
**3–15s:** Mostra o plugin carregando áudio + tocando junto com partitura. Voz: "This is PartePlay — a VST3 plugin that transposes your reference track to your instrument's key, locked to the transport. No drift, no speed change."
**15–25s (na tela: "+3 → −9"):** Voz: "A viewer caught a bug my ears missed — an E-flat sax transposing plus three instead of minus nine. Same note, wrong octave. Now every instrument has a test."
**25–35s (na tela: "2 things not ready"):** Voz: "Honest — two things aren't ready: automatic BPM still needs manual adjustment, and the track isn't in MuseScore's instrument panel yet. Both are next."
**35–45s (na tela: "Oct 3 · Open source"):** Voz: "It goes open source October 3rd. Try it, break it, tell me what's wrong. Link in bio."
**Texto final na tela:** "github.com/rubenslyra/parteplay"

---

## BLOCO 5 — TREINO (como praticar antes de gravar)
1. Leia em voz alta o post do LinkedIn **1 vez**. Marque as palavras que travar.
2. Grave o roteiro do TikTok (45s) **sem ler** — só com a frase mestre na cabeça.
3. Assista e cheque **3 itens**: (a) não falou "horn/trompa", (b) declarou as 2 lacunas, (c) disse a data certa: "October third, ten A-M, Brazil time".
4. **Frase mestre (decore):** *"PartePlay plays a reference track inside your DAW, synced to the transport and transposed to your instrument's key — no drift, no tempo change."*
5. **Frase do bug (repita 5x):** *"An E-flat sax was transposing plus three instead of minus nine — same pitch class, wrong octave."*

---

## BLOCO 6 — PRONÚNCIA (para brasileiro)
| Palavra | Como falar (português) | Observação |
|---|---|---|
| transposition | trans-pa-ZI-shon | acento em "ZI" |
| octave | ÓK-tiv | rima com "active", NÃO é "óiteiv" |
| semitone | SÉ-mi-toun | |
| transport (substantivo) | TRÃNS-port | acento na 1ª sílaba |
| vocoder | VOU-kou-der | |
| pitch | pítch | o "i" é curto, como em "pit" |
| drift | drifft | rima com "lift" |
| saxophone | SÁK-so-foun | |
| trumpet | TRÃM-pet | |
| trombone | trom-BOUN | o final é "boun" como "bone" |
| flugelhorn | FLÚ-gel-horn | (só se mencionar — evite) |
| VST / DAW / MIDI | soletrar: V-S-T · D-A-W · M-I-D-I | não ler como palavra |
| MuseScore | Miuscór | "muse" = "mius" |
| reference | RÉ-fe-rens | acento na 1ª |
| instrument | ÍN-stru-ment | |
| E-flat / B-flat | i-flét / bi-flét | o "flat" é "flét" |

---

## BLOCO 7 — CHECKLIST ANTES DE PUBLICAR (1 min)
- [ ] Rode `ctest` → confirme o número de verificações
- [ ] Repo público (T-22) + tag `v0.1.0` (T-23)
- [ ] Nenhum "horn / +5 / fifth above" em nenhum texto
- [ ] As 2 lacunas (L1, L2) declaradas em todos os canais
- [ ] Link do repo correto na bio/comentários
- [ ] Data: Saturday, Oct 3, 2026, 10h BRT
