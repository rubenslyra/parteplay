# Continuação — PartePlay

> **Estado atual: 01/10/2026, fim do dia.** A 0.3.0 está **publicada** — tag
> `v0.3.0` assinada sobre `d997650`, release no GitHub com notas em inglês,
> `main` e `develop` reconciliados. Ver [Como retomar](#como-retomar) e
> [Próximos passos](#próximos-passos-02102026) no fim deste arquivo.
>
> Este documento é um **diário de sessão**, escrito de cima para baixo. As seções
> do meio descrevem o estado como ele estava em 29/09 e valem como registro do
> que foi feito e por quê — não como descrição de hoje. Os títulos das seções
> antigas dizem a data a que se referem.

## Objetivo

Publicar a primeira versão estável do PartePlay e abrir o repositório para
colaboração externa com o mínimo de atrito: release baixável, documentação em
inglês, Pages no ar e `main` protegida. **Cumprido em 01/10/2026**: tag `v0.3.0`
assinada, release com notas em inglês, e binários para Windows x86-64, macOS
universal e Linux x86-64 anexados pela workflow `release.yml`.

## Estado do repositório em 29/09 (histórico)
- Branch de trabalho: **`develop`** (criada a partir de `main` em `977411d`).
- `main` permanece em `977411d` = `origin/main`, intocada e com a proteção valendo.
- `develop` tem 1 commit à frente: **`0c05f85`**, assinado, com a correção do PCM
  do fingerprint, o segundo `C4244`, o teste de regressão e o preset `msvc-2026`.
- `CI` de `977411d`: run `36521048403`, **success** ✅ (a pendência 1 está resolvida;
  o run de `959cf24` está `cancelled` porque foi supersedido pelo push seguinte).
- **`develop` ainda não foi enviada ao GitHub.** Decidir se entra como push direto
  ou via PR — `main` exige aprovação de code owner e o owner não pode aprovar o
  próprio PR, então o caminho é `enforce_admins: false` com merge explícito.

> ⚠️ **O `D:` (Disco 0, PNY CS900 120GB) falhou durante a build de 29/09** —
> 202 falhas de escrita na MFT do NTFS, com perda de dados confirmada, e o
> volume está em `Full Repair Needed`. **Não rodar `chkdsk /f`** antes de copiar
> o que importa. O dossiê com os logs está em
> `C:\Users\rlyra\Desktop\PartePlay-SSD-diagnostico-2026-09-29\`.
> O laudo aponta **cabo / controladora / alimentação** como causa comum
> (os dois discos falharam na mesma janela), não só a memória flash.

## Release candidate: publicada ✅
- Tag **`v0.2.0-rc.1`**, marcada como **pre-release** (não draft).
- Build `26092901`, commit `4113f13`, Windows x64.
- Assets: `PartePlay-0.2.0-rc.1-win-x64.zip` (3.360.666 bytes) + `.sha256`.
- SHA-256 `2675867e4744bcb25f261330ef660986bcdfb82bb8e59d1d2c2b558311b6d27c`,
  conferido contra o arquivo local **e** contra o asset publicado.
- `GetPluginFactory` confirmado no binário; `moduleinfo.json` íntegro.
- URL: https://github.com/rubenslyra/parteplay/releases/tag/v0.2.0-rc.1
- Só Windows. Linux/macOS passam no CI mas **não** estão anexados — decision
  conscious: notas de release dizem isso abertamente.

## `main` protegida — aplicada ✅
| Regra | Valor |
|---|---|
| Commits assinados | ruleset `Signed commits on main`, `active` |
| Aprovação de code owner | 1, `require_code_owner_reviews: true` |
| Último push precisa aprovar | `true` |
| CI obrigatório | `strict: true` |
| Histórico linear | `true` |
| Force push / deleção | bloqueados |
| `enforce_admins` | **`false`** (ver "Armadilha 1") |

`.github/CODEOWNERS` = `* @rubenslyra`.

### Armadilha 1 — por que `enforce_admins` é `false`
GitHub **proíbe o autor de aprovar o próprio PR** (`422 Review Can not approve
your own pull request`). Como `rubenslyra` é o único com escrita, exigir aprovação
de code owner para ele = travamento circular. O PR #6 ficou `mergeable_state:
blocked` e o owner não conseguia destravar.

`enforce_admins: false` dá ao owner o merge com override explícito. Quem entrar
depois continua preso a **todas** as regras. Se quiser endurecer: criar um segundo
colaborador como aprovador — mas aí a aprovação deixa de ser só do owner.

### Armadilha 2 — `restrictions` por usuário não existe em repo pessoal
A API responde `Only organization repositories can have users and team
restrictions`. Como `rubenslyra` já é o único colaborador, a restrição é
redundante: revisão exige escrita, então só ele pode aprovar. É o CODEOWNERS que
faz o trabalho.

## Assinatura de commits — funcionando ✅
- Chave de assinatura dedicada: `~/.ssh/id_ed25519_signing` (ed25519, sem
  passphrase), fingerprint `SHA256:H5MTN7gQV99lND7DtF9RDQFzOPOGzWqb5tm/Ehfp+UE`.
- Chave de autenticação `~/.ssh/id_ed25519` **inalterada**.
- Config **local do repo** (`.git/config`), não global:
  `gpg.format=ssh`, `user.signingkey=%USERPROFILE%\.ssh\id_ed25519_signing.pub`,
  `commit.gpgsign=true`, `tag.gpgsign=true`,
  `gpg.ssh.allowedSignersFile=%USERPROFILE%\.ssh\allowed_signers`.
- `allowed_signers` lista as **duas** chaves mapeadas em `rubensvlyra@gmail.com`.
- Commit `959cf24` verificado pelo GitHub: **`verified=True`, `reason=valid`**.
- Fluxo documentado em `CONTRIBUTING.md` → "Signed commits".

**Armadilha 3:** o endpoint clássico
`PUT .../protection/required_signatures` devolve **404** nesta conta. A exigência
foi feita por **ruleset** (`POST /repos/:owner/:repo/rulesets`, regra
`required_signatures`). Se alguém recriar a proteção pela UI, checar se o ruleset
sobreviveu.

**Armadilha 4:** `id_ed25519` **não** está registrada no GitHub. Commits
antigos assinados com ela aparecem `unknown_key`. Só a chave de assinatura está
registrada (aba *Signing keys*, título `parteplay-signing`).

## Documentação e presença pública
- Descrição do repo em inglês + 18 topics (`vst3`, `juce`, `musescore`,
  `score-follower`, `phase-vocoder`, …).
- README: logo reduzido (783 KB → 52 KB, `docs/parteplay-logo.png` 256 px,
  `width="140"`), botão de download apontando para a tag da RC, screenshot da RC
  abaixo do botão.
- Pages no ar com botão → `v0.2.0-rc.1` e rótulo "release candidate".
  Confirmado `HTTP 200`.
- `publicacao-playbook.md` **removido** do repo (era material de redes sociais).
- `.gitignore` agora ignora `linkedin-*.md` e `social-*.md`.

## Issues abertas (5) — e o que já foi entregue

| # | Assunto | Entregue | Falta |
|---|---|---|---|
| 1 | Pipeline ffmpeg/ffprobe + remoção de silêncio inicial + UI de progresso | **UI de progresso e carga assíncrona** (`LoadingOverlay`, `FilePlayer::LoadStage`, `beginLoadAudioFile`); em **01/10/2026**, a **pipeline externa de andamento** (`Source/Analysis/ExternalBpm.cpp`: `ffmpeg` → WAV mono 44,1 kHz → `soundstretch -bpm`), com binários LGPL em `resources/bin/`; e a **remoção do silêncio inicial** (zeros digitais exatos, `FilePlayer::countLeadingSilence`, com aviso na UI e áudio entregue já ajustado). | Nada pendente do escopo. A decisão de 30/09/2026 de "não embarcar" foi **revertida** (licença resolvida; `THIRD_PARTY_NOTICES.md`, `resources/bin/`, `%APPDATA%/PartePlay/bin`). O fator `×2` também foi **revertido** em 01/10/2026: virou padrão do BPM, com botão no painel 05 para escolher o valor real do SoundStretch. |
| 2 | `C4244` em `TempoAnalyser.cpp:248` e `FingerprintWorker.cpp:157` | **Os dois pontos corrigidos** em `0c05f85` (`develop`), agora em `Source/Analysis/`. O do fingerprint corrigiu um bug real de escala do PCM, não só o warning. **Suíte rodada: 401 verificações, 0 falhas** (30/09, preset `msvc-2026`) — a pendência de rodar a suíte está resolvida. | **Promover `/WX` a erro nos fontes próprios** — sem isso a issue volta. |
| 3 | Cobertura automatizada: sync sob time-stretch e vocoder | — | tudo. Mas `testFingerprintPcmConversion` já serve de **precedente**: invariante quantitativa, não golden file, exatamente o que a issue pede. |
| 4 | Licenças de terceiros no binário distribuído (Chromaprint LGPL) | Issue escrita com a análise da LGPL-2.1; tarball 1.6.1 confirmado em `Chromaprint-Dependences/`. **`THIRD_PARTY_NOTICES.md` criado em 30/09/2026** e lincado no README, com cada string de licença lida do arquivo real da árvore. | Enumerar a lista de libs vendorizadas dentro da JUCE (hoje o arquivo aponta para o `LICENSE.md` da JUCE em vez de listar). |
| 5 | Credenciais plaintext + secret scanning | Análise registrada na própria issue: `secrets` está no `.gitignore`, `git log --all -- secrets` vazio, **não vazou**. | Mover para variável de ambiente, documentar em `CONTRIBUTING.md`, `gitleaks`/`detect-secrets` no CI, **rotacionar** OAuth do GitHub e chave AcoustID. |

O número de verificações foi de 313 para **325** — confirmado por observação em
30/09/2026, não por aritmética: a suíte rodou e imprimiu
`325 verificacoes, 0 falha(s)`. As 12 novas são as do teste de regressão do PCM
descrito em "Corrigido". O número já foi reconciliado no README, no
`docs/index.html` e no CHANGELOG.

Depois disso, em 30/09/2026, a reescrita do `TempoAnalyser` (testes sintéticos de
click track) e o teste de round-trip do mapa de tempo MIDI levaram a suíte a
**342** — confirmado por execução (`342 verificacoes, 0 falha(s)`) e
reconciliado no README, no `docs/index.html` e aqui.

Em seguida, ainda em 30/09/2026, a carga assíncrona (progresso do `estimateBpm`
e as 7 strings de carregamento, cada uma checada nas 4 culturas) levou a suíte de
**342** para **401** — confirmado por execução (`401 verificacoes, 0 falha(s)`) e
reconciliado no README, no `docs/index.html`, no CHANGELOG e aqui.

Em 01/10/2026, a pipeline externa de andamento (`ExternalBpm`, com os binários em
`resources/bin/`) acrescentou 14 verificações, de **401** para **415** — confirmado
por execução (`415 verificacoes, 0 falha(s)`) e reconciliado no README, no
`docs/index.html` e no CHANGELOG. As novas cobrem os conjuntos puros: parser da saída
do SoundStretch, linha de comando do `ffmpeg`, nome do WAV temporário e o interruptor
`setEnabled` (desligado no `main` do teste para o CI não depender de `ffmpeg` no PATH).

Ainda em 01/10/2026, a sincronia de compassos (corte de silêncio inicial, BARS pela
fórmula do host e escala de BPM ×2) acrescentou 46 verificações, de **415** para
**461** — confirmado por execução (`461 verificacoes, 0 falha(s)`) e reconciliado no
README, no `docs/index.html` e no CHANGELOG. Cobrem `FilePlayer::countLeadingSilence`
(zeros digitais em qualquer canal), `FilePlayer::measureCountFor` (guarda de duração,
BPM e compasso nulos) e a métrica efetiva (`setMeterOverride`/`setBpmScale`).

## Estado do disco (30/09/2026) — não resolvido

O `D:` continua em **`Full Repair Needed`**. Isto é a causa das sinalizações de erro
no Visual Studio, não o código: o build de linha de comando do zero passa limpo
(0 erros; os 9 warnings restantes são um único `C4244` dentro do Chromaprint, código
de terceiro, e os fontes próprios do projeto não têm warning nenhum).

Backup verificado em `C:\PartePlay-BACKUP-2026-09-30\` (repo sem `build/`, chaves de
assinatura, `secrets` e os binários de hoje). `git fsck` = 0, 429/429 arquivos, SHA-256
das fontes e das chaves sem divergência. Ver o `MANIFEST.txt` naquela pasta.

**`chkdsk /f` ainda NÃO foi rodado** e não deve ser, até decidir o que fazer com o
backup. `chkdsk /scan` (read-only) exige shell elevado e ainda não foi executado.


## Fixes técnicos desta sessão
1. **Build Windows quebrado:** `PartePlay_VST3` recebia só o wrapper do JUCE e
   nenhum código do plugin — build "terminava com sucesso" e sem binário. Bloco
   `if(WIN32)` adicionado; os dois targets agora consomem `PARTEPLAY_SOURCES`
   (fonte única, sem lista duplicada).
2. **O `LinkObjects` era pista falsa.** `MSVC_LINK_OBJECTS` escreve
   `<LinkObjects>false</LinkObjects>` num `<CustomBuild>` do `juceaide`, que não
   linka objeto nenhum — `false` está correto ali. O `build.ps1` que "contornava"
   isso foi deletado.
3. Mortos removidos: `fontMedium`/`fontBold` (cópias idênticas de `font()`,
   nunca chamadas), linha morta do subtitle na tabela i18n, `Resources/Fonts/`
   (1,67 MB de Noto sem nenhuma referência).
4. Badge e subtítulo do cabeçalho enxutos; painel 05 sem a métrica duplicada.
5. `CMakeLists.txt`: `PARTEPLAY_SOURCES` + whitespace limpo.

⚠️ Não apagar `build\msvc-2026\PartePlay_artefacts` **depois** do configure: contém
`Defs.txt`; sem ele o `juceaide` quebra com `Unhandled exception` / `MSB8066`.
Vale para qualquer árvore de build — `build\msvc` foi removida em 30/09 e
recriar o preset `msvc` gera os `Defs.txt` de novo, sem problema.

## Removidos por completo nesta rodada (BPM automático + seletor de idioma)

Registro de antecessores removidos (agent.md, Bloco 3, Anexo): nada foi publicado
com a herança ainda presente.

- **`TempoAnalyser` — estimador por pente puro (`combScore`) e desempate ×2/÷2.**
  Removido. Motivo: o pente puro premiava tempos rápidos por densidade (mais
  dentes dentro do envelope) e só testava ambiguidade de oitava; deixava passar
  o caso 103→69 (razão 3:2). Substituto: `combExcess` (pente normalizado pela
  média do envelope) + `tactusPrior` (log-gaussiana em 120 BPM) + uma única
  varredura 40–240 BPM. Coberto por testes sintéticos novos (click track 103 em
  3/4, 120 em 4/4, 90 em 4/4) em `Tests/DomainTests.cpp`.
- **`estimateBeatsPerBar` — busca de fase por salto `ceil(barFrames/4)`.**
  Removida em favor da busca por frame. A busca antiga testava no máximo ~4
  alinhamentos, errava o downbeat e classificava um 3/4 nítido como 4/4.
- **Seletor de idioma — seta de dropdown (`drawComboBox`).** Removida.
  Substituída por `drawLanguageFlag`: bandeira vetorial do idioma ativo,
  desenhada no código, sem asset e sem licença de terceiro.
- **Seletor de idioma — desenho manual do texto do item selecionado.**
  Removido. Motivo: duplicava o label interno do `ComboBox` e o texto aparecia
  um sobre o outro. Agora o texto é desenhado só pelo label, posicionado por
  `StudioLookAndFeel::positionComboBoxText` (zona do label reservada).

## Removidos por completo nesta rodada (carga assíncrona)

Registro de antecessores removidos (agent.md, Bloco 3, Anexo): a carga passou a
ser assíncrona e com feedback; nada dos caminhos síncronos ficou para trás.

- **`PlayScoreProcessor::loadAudioFile` (síncrono).** Removido. Lia o arquivo,
  rodava a análise e publicava o player na thread de mensagem, travando a UI (e
  sem nenhum sinal de progresso). Substituto: `beginLoadAudioFile`, que agenda um
  job em `juce::ThreadPool loadPool { 1 }`, controla estado por
  `std::atomic<bool> loadInProgress`, `std::atomic<float> loadProgress`,
  `std::atomic<int> loadStage` e um `loadGeneration` que descarta cargas
  superadas, publica o `FilePlayer` na message thread por
  `MessageManager::callAsync` e guarda um
  `std::shared_ptr<std::atomic<bool>> aliveFlag` que o destrutor desliga antes de
  `loadPool.removeAllJobs (true, 4000)`.
- **`FilePlayer::runTempoAnalysis()` (e a declaração no cabeçalho).** Removida.
  Rodava a análise de uma vez, sem reportar progresso. Agora
  `FilePlayer::loadFromFile (file, onProgress)` lê o arquivo em blocos
  (262144 amostras) e chama `estimateBpm`/`estimateBeatsPerBar`/
  `estimateTuningCents` inline, remapeando o progresso por estágio
  (`FilePlayer::LoadStage`: `preparing`/`decoding`/`waveform`/`tempo`/`tuning`/
  `publishing`).
- **UI — disparo de carga sem feedback.** Removido. Antes o seletor de arquivo
  chamava `loadAudioFile` direto e não havia indicação visual. Agora o editor
  exibe o `LoadingOverlay` (topo, cobre 100% da janela, com
  `setInterceptsMouseClicks (true, true)` para bloquear o que está atrás), com
  estágio e barra, e o esconde quando `isLoadingAudio()` volta a falso.

## Branches e governança (30/09/2026)

Três branches novas, criadas a partir de `develop` (`232dfcd`):

| Branch | Papel | Estado |
|---|---|---|
| `main` | linha publicada, protegida | `977411d` |
| `develop` | integração | `232dfcd`, 7 commits à frente do remoto |
| `release/0.2.2` | correções apenas — **patch**, sem ISRC | `133d901`, PR #7 aberto |
| `epic/identidade-do-fonograma` | ISRC/ano/aba de metadados — **0.3.0** | parada, criada de `develop` |

O número `0.2.2` **não** é mais "0.2.1". A regra de SemVer para `0.x` que ficou
escrita no `CONTRIBUTING.md`: tudo que o usuário percebe é **minor**, só defeito
é **patch**. A aba de metadados é funcionalidade, logo é `0.3.0`, e por isso está
em branch própria em vez de ser somada à de correções.

### Renumerado para 0.3.0 (01/10/2026)

A tabela acima descreve o estado de 30/09. Em 01/10 a branch foi renomeada e os
númerosfollowingaram, porque a linha não era mais só correção:

| Branch | Papel | Estado |
|---|---|---|
| `release/0.3.0` | linha de manutenção — **minor**, com saída MIDI nova | `5b3ba41`, **PR #8 aberto** |
| `epic/identidade-do-fonograma` | ISRC/ano/aba de metadados — **0.4.0** | parada, `232dfcd` |

Motivo do minor: o mapa de tempo SMF formato 0 na saída MIDI é funcionalidade
perceptível, não correção. Patch só para defeito — a política está escrita no
`CONTRIBUTING.md`, então o próximo conserto da linha é `0.3.1`.

Motivo do 0.4.0 no épico: se `0.3.0` já é minor por causa do MIDI, o número da
aba de metadados não pode ser o mesmo minor. Um minor por linha. O épico fica
para `0.4.0`.

**PR #7 foi fechado sozinho** pela renomeação da branch (o GitHub fecha o PR de
uma branch que some). **PR #8** é o mesmo conteúdo, com a branch certa:
https://github.com/rubenslyra/parteplay/pull/8

O histórico **não** foi reescrito: sem squash, sem rebase. A única exceção foi o
commit `72f3e30` → `aefee39`, refeito só para tirar BOM e acrescentar o
`Signed-off-by`. O assunto antigo `cd0056b` continua no log porque o
`docs/index.html` de um commit fala do commit que o gerou — reescrever isso
mentiria sobre o passado.

Escopo acordado: reestruturar `Source/` (feito, junto com `Source/` na branch de
release); apagar `build\msvc` e manter `build\msvc-2026` (feito); só o domínio de
identidade do fonograma entra no épico.

**PR #7 — https://github.com/rubenslyra/parteplay/pull/7** — `release/0.2.2` →
`develop`, 9 commits. Ubuntu e macOS verdes; Conventional Commits, leitura de
metadados, versão-única e SonarCloud verdes; **Windows ainda rodando** no momento
desta escrita.

**PR #8 — https://github.com/rubenslyra/parteplay/pull/8** — `release/0.3.0` →
`develop`. Mesmo conteúdo, branch renomeada. Título: *release: bring the 0.3.0
line to develop*.

Sobre Conventional Commits: o gate **falhou na primeira execução** com
`Cannot find module '@commitlint/config-conventional'`, porque o CLI não traz o
preset e o commitlint resolve `extends` a partir da raiz do repo, que não tem
`node_modules`. Passava local só por cache do `npx` — verde na máquina que
escreveu, vermelho no runner. As 12 regras do preset estão agora inline em
`commitlint.config.js`, o que elimina a dependência e deixa as regras legíveis.

### 1. Confirmar a CI de `959cf24` — ✅ resolvido
Run `36521048403` (commit `977411d`) terminou **success**. O run de `959cf24`
aparece `cancelled` porque o push seguinte o supersedeu — não foi falha.

### 2. Backup da chave privada de assinatura — **prioridade**
`~/.ssh/id_ed25519_signing` está **sem passphrase**. Se for perdida, não há
recuperação: é preciso gerar outra e reregistrar no GitHub. Backup **cifrado**
fora do disco de trabalho (cofre de senhas ou backup cifrado).

### 3. Testar a RC de verdade
O build da release **não** foi instalado nem aberto no MuseScore 4. Só foi
verificado por hash, estrutura de bundle e presença do export. Instalar:
```powershell
# 1. fechar o MuseScore
# 2. descompactar o zip em qualquer pasta temporaria e copiar o bundle .vst3
Expand-Archive .\PartePlay-0.2.0-rc.1-win-x64.zip -DestinationPath $env:TEMP\pp
Copy-Item -Recurse $env:TEMP\pp\PartePlay.vst3 "C:\Program Files\Common Files\VST3\"
```
Plano: `scripts\install-vst3.ps1` faz isso e valida o SHA-256, mas rebuilda.
Para instalar o binário **da release** sem recompilar, copiar manualmente.

### 4. Ligar Linux/macOS à release (opcional, depois de testar)
Anexar `PartePlay-0.2.0-rc.1-linux-x64.tar.gz` e
`...-macos-universal.zip` quando houver build local verificado dessas plataformas.

### 5. `THIRD_PARTY_NOTICES` — ✅ entregue, falta enumeração
Criado em 30/09/2026 (`b399b77`), lincado no README, com cada string de licença
lida do arquivo real da árvore. A issue #4 permanece aberta só pelo motivo
descrito na tabela de issues: o arquivo aponta para o `LICENSE.md` da JUCE em
vez de enumerar as libs vendorizadas dentro dela.

### 6. Rotacionar credenciais locais
`secrets` tem OAuth do GitHub e chave AcoustID em plaintext (ignorado pelo Git,
mas exposto a backup/sync de pasta). Ver issue #5. **Nunca colar os valores em
issue, commit ou log.**

### 7. Script de release automatizado (não feito)
O processo de release foi manual nesta sessão (build → zip → sha256 →
`gh release create --prerelease`). Vale um `scripts/release.ps1` que faz tag,
build, checksum e upload num comando. Verificar antes se a branch protection
agora exige assinatura em tag (`tag.gpgsign` já está `true`).

### 8. Rascunhos de redes sociais fora do repo
- `linkedin-post.md` — post reescrito (continuidade, 0.2.0, ~1.666 chars).
- `linkedin-post-comentário-resposta.md` — resposta ao Thiago, 334 chars.

Ambos gitignored. **Não commitar.** O post do LinkedIn **não** foi publicado.

## Decisões fechadas nesta sessão
1. Transposição removida em definitivo — o post e o README descrevem o 0.2.0 real.
2. `0.2.0-rc.1` como primeira release pública, pre-release, com número de build.
3. `enforce_admins: false` — desimpasse do travamento de auto-aprovação.
4. Assinatura via SSH com chave dedicada, não GPG (o GPG do Git for Windows está
   quebrado: `keyboxd` ausente).
5. Escopo do release: só Windows. Linux/macOS declarados como CI-only por ora.

## Estado do produto (para não redescrever)
- `Source/Instrument.*` **removido**; `ParameterIds.h` com 6 parâmetros, sem
  instrumento/transposição.
- `Tuning.*` centraliza a matemática de afinação. `PitchShifter` existe **só**
  para compensar afinação de referência.
- `FingerprintWorker.*` calcula Chromaprint fora da thread de áudio.
- `FilePlayer.*` publica buffers por `shared_ptr<const>` atômico, sem lock no áudio.
- `Text.*` i18n pt-BR / en-GB / en-US / es-ES.
- `Tests/DomainTests.cpp`: **461 verificações, 0 falhas** (confirmado rodando em
  01/10/2026 no preset `msvc-2026`; 415 antes do corte de silêncio/BARS, 401 em 30/09/2026).
- Mídia de demonstração em `docs/`: `screenshot-v0.3.0-release.png` substitui o
  `screenshot-vst-used.png` (removido); `screenshot-v0.3.0-release.mp4` é a demo da
  linha 0.3.0, recomprimida de 182,7 MiB para 19,0 MiB com o ffmpeg LGPL embarcado
  (`libopenh264`, resolução nativa, SSIM 0.989) — o original passava o limite de
  100 MiB/arquivo do GitHub. `*.mp4 binary` no `.gitattributes`.
- `secrets` é um **arquivo** de 311 bytes na raiz do repo (OAuth do GitHub + chave
  AcoustID), não uma pasta. O `.gitignore` cobre os dois casos.
- `develop` **está no GitHub** e ficou 7 commits à frente de `origin/develop` até
  30/09/2026, quando o **PR #7** (`release/0.2.2` → `develop`) foi aberto para
  propagar a linha de manutenção. Em 01/10 esse PR foi substituído pelo **PR #8**
  (`release/0.3.0` → `develop`) depois da renomeação. Ver "Branches e governança".
- `JUCE_ROOT` está definido como variável de ambiente no escopo do usuário
  (`D:\Program Files\JUCE`), então o configure usa o caminho de precedência
  correta e o aviso de "default fixo desta maquina" não aparece mais.
- VS2022 **BuildTools** continua instalado (`Program Files (x86)`), então o preset
  `msvc` funciona; o `msvc-2026` usa o VS 18 Enterprise. Os dois coexistem.
- `CMakePresets.json`: `msvc`, `ninja`, `linux`, `macos`, `macos-universal`.
- `agent.md` é interno e gitignored — nunca commitar.
- O GPG do Git for Windows (`C:\Program Files\Git\usr\bin\gpg.exe`) **não gera
  chaves**: `keyboxd probably not installed`. Para GPG de verdade, instalar Gpg4win.

## O portão de warnings pagou a conta (01/10/2026)

`5d7366c` transformou warnings em erro nos fontes próprios. No Windows passou
limpo na hora; **no Linux e no macOS não**, porque GCC e Claw são bem mais
estritos que o `/W4` do MSVC. O portão nomeou 38Warnings que ninguém estava
lendo. Commit `5b3ba41` limpa:

| Onde | Aviso | O que era |
|---|---|---|
| `MidiMapExporter.cpp` (10) | `-Wsign-conversion` | `writeByte (char)` recebendo `int`/`unsigned char`. Byte escrito é o momento em que um inteiro deixa de ser número e vira octeto — o cast diz isso. |
| `DomainTests.cpp` (20) | `-Wmissing-prototypes` | As 20 funções de teste estavam no escopo global, com ligação externa. Um binário de teste não devia exportar símbolo que ninguém chama. |
| `DomainTests.cpp` (7) | `-Wfloat-equal` | 4 queriam tolerância (`parseBpm`, `sourceSampleRate`) → `checkClose`. 3 querem igualdade **exata** (parser devolvendo 0 sem BPM, 0 cents para entrada inválida) → helper `exactly()`. Tolerância nesses 3 enfraqueceria o teste em silêncio. |
| `FilePlayer.cpp` (1) | `-Wfloat-equal` | `countLeadingSilence` conta silêncio **digital**, sem piso em dB. Exato de propósito; supressão local com o motivo escrito. |

O Sonar também era vermelho, e por dois motivos reais:

- **3 × `cpp:S2193`** (Bugs, Reliability B) — *"do not use a counter of type float"*.
  Os três laços do `TempoAnalyser` contavam em `double`. A varredura grossa
  pode pagar a conta hoje porque 0,5 é exato em binário, mas "somar 0,5 quatro
  centas vezes e chegar em 240,0" é justamente a suposição que quebra quando
  alguém troca o passo. O índice é o contador agora.
- **2 × `cpp:S5443`** (Vulnerabilities CRITICAL, Security **D**) — *"publicly
  writable directories"*. O teste do ffmpeg usava `C:/tmp`. Nada é lido nem
  escrito ali, mas `/tmp` é de todo mundo no POSIX, e um teste que aponta para lá
  ensina o leitor que a pasta é confiável. O aviso acertou pelo motivo errado,
  o que continua sendo um motivo.

Sobram **72 code smells** no Sonar (memória sequencial, `auto` redundante,
`printf`, `std::print`, lambdas longas). Não são bugs nem vulnerabilidades, não
seguram o gate, e **não são coisa de patch release**.

> Os três `cpp:S2193` e os dois `cpp:S5443` só apareceram porque a busca por
> `branch=release/0.3.0` devolveu **zero** issues — a consulta certa é
> `pullRequest=8`. A leitura anterior "0 vulnerabilidades, 0 hotspots, análise
> velha" estava errada por causa do parâmetro, não por causa do Sonar.

### Resultado no run `36909409128` — tudo verde ✅

| Check | Tempo |
|---|---|
| Windows | 3m07s |
| macOS | 3m30s |
| Ubuntu | 8m13s |
| macOS universal | 7m50s |
| SonarCloud | 28s |
| Conventional Commits | 17s |
| Ler metadados do projeto | 12s |
| Versão em um único lugar | 10s |

`mergeStateStatus: CLEAN`. Esta é a **primeira vez que as quatro plataformas
compilam com o portão de warnings ligado** — antes disso o macOS universal era
`skipping` porque as outras três quebravam antes.

## Como retomar

```powershell
cd D:\source\DevOUT\parteplay
gh release view v0.3.0                 # release publicado, notas em inglês
git log --oneline origin/main -1       # d997650 — o merge que a tag aponta
```

**A 0.3.0 está publicada.** Ordem do que aconteceu, para não reconstruir:

| O quê | Resultado |
|---|---|
| PR #8 `release/0.3.0` → `develop` | merged como `2e23c0c` |
| PR #9 `develop` → `main` | merged como `5fa3f74` |
| `commitlint.config.js` | `8300470` — o gate não reconhecia merge commit |
| CHANGELOG datado | `648cb89` — a seção dizia *Não publicado* |
| PR #10 `develop` → `main` | merged como `d997650` |
| Tag `v0.3.0` | anotada e assinada sobre `d997650`; GitHub: `verified=true` |

Os dois PRs para `main` foram mergeados com `--admin`: o ruleset exige uma
aprovação de code owner e o owner não pode aprovar o próprio PR. Os commits de
merge foram conferidos pela API (`verified=true`) — `%G?` local devolve `E`
porque a `allowed_signers` local resolve a chave diferente da que o GitHub
conhece, e isso nunca afetou o resultado lá.

A branch `epic/identidade-do-fonograma` precisa ser atualizada com `develop`
antes de começar o trabalho de metadados.

### Instalação ainda não validada de ponta a ponta
O `install-vst3.ps1` foi exercitado contra `msvc` e `msvc-2026` e para
corretamente no gate de privilégio — mas **esta sessão não é administrativa**,
então a cópia para `Program Files` nunca aconteceu. O binário instalado
continua sendo o de 29/09, hash `AFE01F4B17F8CBBF`, que não corresponde a
nenhuma árvore local. Para fechar isso, com o MuseScore fechado e um shell
elevado:
```powershell
.\scripts\build.ps1
.\scripts\install-vst3.ps1 -SkipTests
Get-FileHash "C:\Program Files\Common Files\VST3\PartePlay.vst3\Contents\x86_64-win\PartePlay.vst3"
```
O hash impresso pelo script tem de bater com o do bundle em `build\msvc-2026`.

### De onde vêm os binários da release
A workflow `release.yml` **não compila**. Ela baixa os artefatos do run verde de CI do
**mesmo commit da tag** e os anexa à release, com `.sha256` e um `SHA256SUMS.txt`.

Isso é deliberado, e é a diferença entre um artefato e uma promessa:

- O passo de empacotamento do `ci.yml` já tem histórico de escolher a árvore errada por
  causa do cache (`restore-keys` ignora o sha, e "Debug" ordena antes de "Release" no nome
  do diretório num projeto multi-config). Duplicar essas ~200 linhas num segundo workflow
  duplicaria também a chance de reintroduzir o bug.
- O binário que o usuário baixa é literalmente o binário que a matriz de quatro
  plataformas compilou e testou. Não é um "build de release" que alguém rodou à mão.
- Compilar de novo na hora da release custaria ~20 min por plataforma para produzir o
  mesmo byte em quatro sistemas.

**O que a release não substitui:** a validação no MuseScore continua em aberto (§_install).
Um binário que a CI compilou e testeiu não é o mesmo que um binário que alguém carregou no
host. O que a release entrega é o artefato testado; a validação de uso é o passo
administrativo que falta.

**As ferramentas externas vão separadas.** O bundle tem ~6,5 MB e não contém `resources/bin`:
o `locateTools()` procura `resources/bin` ao lado do executável, ou seja dentro de
`Contents\x86_64-win\resources\bin`, e o CMake não copia nada para lá. Sem o
`PartePlay-0.3.0-tools-win-x64.zip` (~65 MB), quem instala da release fica só com o
analisador nativo. O README diz onde descompactar.

**Para anexar um bundle novo a uma tag antiga** (a tag já está publicada e não se move):
```powershell
gh workflow run release.yml --ref main -f tag=v0.3.0
```
`workflow_dispatch` só aparece se o workflow já estiver na branch padrão — por isso o merge
dele em `main` precede o disparo.

## Próximos passos (02/10/2026)

1. **Validar no MuseScore** (shell elevado, MuseScore fechado): `.\scripts\build.ps1`
   e `.\scripts\install-vst3.ps1 -SkipTests`; conferir o hash do bundle instalado
   contra `build\msvc-2026`. É o único passo do escopo que não deu para fechar
   por falta de privilégio — e é o que destrava o binário anexado ao release.
2. **Sincronia de compassos com áudio real**: o caso relatado (BPM real ~103 contra
   137,5) só se reproduz com o arquivo do usuário — sem ele, não mexer em número no
   escuro (agent.md). Com o arquivo, comparar o BPM do SoundStretch, o ×2 padrão e o
   BARS do host.
3. ~~Push~~ **feito**: `release/0.3.0` enviada, PR #8 merged (`2e23c0c`).
4. ~~Propagar~~ **feito**: PR #9 merged em `main` (`5fa3f74`), PR #10 merged
   (`d997650`). Tag `v0.3.0` e release publicados.
5. **Retomar metadados**: atualizar `epic/identidade-do-fonograma` com `develop`
   antes do trabalho de ISRC/ano. A ficha da canção sai como **0.4.0**.
