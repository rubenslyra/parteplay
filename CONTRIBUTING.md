# Contributing to PartePlay

Thanks for your interest. This guide is short on purpose: everything you need for a PR
that passes first review.

**License:** the code is [AGPL-3.0](LICENSE). By contributing you agree to that.
One extra note: JUCE 9 is dual-licensed (AGPLv3 **or** a commercial JUCE license) — see the
"License" section of the [README](README.md).

---

## Before you start

- **Setup:** Windows 10/11 · Visual Studio 2022 (Build Tools with *Desktop development with C++*)
  · CMake ≥ 3.22 · JUCE 9.0.2 (its path goes in the `JUCE_ROOT` variable).
- **Issue before PR:** if the change affects behaviour or product, open an issue describing the
  problem first. A change to the musical model, layout or the parameter contract is a product
  decision, not an implementation detail.
- **Scope:** the v1.x target is anyone writing scores from reference audio. The plugin is a
  **slave transport**: it plays the reference audio locked to the host clock and reads its
  metadata and tuning — it does not transpose the signal.

## Build and test

```powershell
# Configure and build Release
cmake --preset msvc
cmake --build --preset msvc --config Release

# Domain suite — run before any PR
cmake --build --preset msvc --config Release --target PartePlayTests
ctest --preset msvc
```

CI runs the same suite on Windows, Ubuntu and macOS (`cmake --preset linux / macos /
macos-universal`). A PR whose gate is red does not pass.

## Code conventions

| Topic | Rule |
|---|---|
| **Encoding** | UTF-8 **with BOM** in every text file (`.cpp/.h/.md/.ps1/.json/.gitignore`). `.editorconfig` and the MSVC `/utf-8` flag depend on it. |
| **Line endings** | LF. `.gitattributes` normalizes; never commit CRLF. |
| **UI strings** | Always `Text::t("pt-BR key")` — never a stray literal. Add the four columns in `Source\Text.cpp`: `pt-BR | en-GB | en-US | es-ES`. |
| **Interpolation** | Always `Text::format`. **Never** `juce::String::formatted`: on Windows it uses `_vsnwprintf` with `String(const char*)` and breaks accents. |
| **Untranslated literal** | `Text::from("...")`, for the same reason as above. |
| **Numbers** | Any visible decimal goes through `Text::number(value, digits)`, so pt-BR and es-ES get `128,5` and en gets `128.5`. |
| **Style** | JUCE: abbreviated type names, space before call parens (`foo (a, b)`), trailing `const`, member prefix `m` only where already used. |
| **Music domain** | `Source/Tuning.*` is the source of tuning/ratio values. Nothing else duplicates them — neither the UI, nor the theme, nor docs. |
| **Comments** | Explain *why*, not *what*. A comment that restates the next line is noise. |

## Pull request

- **One subject per PR.** Mixing refactor, fix and text in one diff hides the review.
- **Describe the problem before the solution**, and say how you verified it. "Ran `ctest`, 0
  failures" and "checked in MuseScore 4 with track X" are the two checks that matter here.
- **Update the docs with the code.** A behaviour change updates `README.md` in the same commit.
  If a fact changes (version, test count, feature status) reconcile it everywhere — the version
  lives only in `project(PartePlay VERSION x.y.z)`.
- **Follow the Conventional Commits format.** `type: subject`, and the types are a closed list
  defined in `commitlint.config.js`. Every commit is checked over the PR range by
  `.github/workflows/conventional-commits.yml`. This is what lets the changelog be written from
  the history instead of from memory.
- **Descriptive commits.** The history is read with DCO: every commit carries `Signed-off-by`.
- **Never commit artifacts.** `build/`, heavy captures and binaries are already on `.gitignore`.

### Commit format

```
fix(scripts): install the bundle from the preset that was built
```

The subject is one imperative line, at most 72 characters, and says *what* changed. The body
explains *why* and is optional for small commits, but a non-obvious fix without one is not
ready for review. The type is a closed list, not a free tag:

| Type | For |
|---|---|
| `feat` | functionality the user notices |
| `fix` | a defect |
| `docs` | README, CHANGELOG, `docs/`, this file |
| `refactor` | internal change, no observable behaviour difference |
| `perf` | performance |
| `test` | the suite and its infrastructure |
| `build` | `CMakeLists.txt`, `CMakePresets.json`, `scripts/` |
| `ci` | workflows and gates |
| `chore` | maintenance that fits none of the above |

A scope is optional and names the module: `(scripts)`, `(licence)`, `(fingerprint)`. To run the
check locally:

```
npx --yes @commitlint/cli --from HEAD~1 --to HEAD --verbose
```

There is no `package.json` on purpose. The project is C++ and CMake and does not acquire a Node
toolchain for a commit linter; `npx` installs it on demand.

The rule applies to commits from now on. The existing history is signed and public and is not
rewritten — the value of the format is in everything after it.

### Signed commits

`main` requires **cryptographically signed commits**. A DCO `Signed-off-by` trailer is a legal
statement and is not a signature — you need both. Unsigned commits are rejected at push time.

The repository expects SSH signing, which needs no extra software on any platform:

```
git config gpg.format ssh
git config user.signingkey ~/.ssh/id_ed25519.pub    # your own key, not a shared one
git config commit.gpgsign true
git config tag.gpgsign true
```

Then add the matching **public** key to your GitHub account under
*Settings → SSH and GPG keys → **Signing keys** → New SSH key*. Getting this wrong is the usual
reason a commit shows as unverified: an *authentication* key authenticates `git push`, a *signing*
key is what validates a commit, and GitHub does not accept one in place of the other.

Check your work before pushing:

```
git log -1 --show-signature     # want "Good \"git\" signature"
```

Use a key dedicated to signing rather than the one you authenticate with. A signing key leaked from
somebody else's clone should not let anyone commit as you. Set
`gpg.ssh.allowedSignersFile` to a file of `email key` lines if you want `git log` to verify
signatures locally.

For a large change, open the issue first and align on the design before writing code.

## Branches and releases

### Topology

```
main                  the released line. Protected: signed commits, code owner review,
                      CI required, no force push.
develop               the integration line. Everything lands here first.
release/0.3.0         a maintenance line. Patch fixes only, no functionality.
epic/<slug>           a body of work too large for one PR. Parks until it is scheduled.
feat|fix|docs|test|build|chore/<slug>
                      one task, one branch, cut from develop or from its epic.
```

`main` is never committed to directly. `develop` is the truth about what currently builds; `main`
is a record of what was released. Anything that reaches `main` came through `develop` and through
CI.

### One issue, one branch

A task is a GitHub issue with acceptance criteria that someone else could check. The branch name
carries the issue number, so `git log` and the issue tracker point at each other:

```
feat/42-leading-silence
fix/61-stale-install
```

The branch lives at most as long as the task. It is deleted after the merge, on both sides.

### The ceremonies, as artefacts

This is a small project and the ceremonies are not meetings. Each one is a thing that exists in
the repository, so that a contributor who was not present still has the same information:

- **Backlog** — the open issues, each with a definition of done written in the issue and not in
  somebody's head. A task with no acceptance criteria is not ready to start.
- **Sprint planning** — picking the tasks off the backlog into a branch. Nothing starts without
  a branch and an issue number.
- **Daily stand-up** — none, unless there is something to unblock. A one-line note on the issue
  saying "blocked on X" is the same artefact with less ceremony.
- **Review** — the pull request. One subject, the problem before the solution, and how it was
  verified. This is where the acceptance criteria from planning get checked.
- **Done** — merged, CI green, issue closed, branch deleted, and the changelog updated from the
  Conventional Commit subjects.

The rule that makes the rest of this mean anything: **a task is not done until the repository is
clean.** A change that leaves an artifact, a stale build tree, a document that contradicts the
code, or a number that was not reconciled is not finished, it is abandoned mid-way. The commit
that adds a feature and the commit that reconciles the documentation it invalidated are one unit
of work, not two tasks.

### Versioning

[SemVer](https://semver.org/), with the project still at `0.x`, which decides what the numbers
mean:

| Change | Version |
|---|---|
| Anything a user can notice, new feature included | minor — `0.3.0` |
| A defect fix, and only defect fixes | patch — `0.3.1` |

A `0.3.x` line takes corrections and nothing else. If a fix needs a new behaviour to be
correct, it is a feature, and it belongs on an `epic/` branch for the next minor. This is why
the song sheet (ISRC, ISWC, year, phonogram data) is `0.4.0` on `epic/identidade-do-fonograma`
and not part of the `0.3.0` line.

The number itself lives only in `project(PartePlay VERSION x.y.z)`, and the bundle version is
derived from it, so the release and the artefact cannot drift. `CHANGELOG.md` records which
version each entry belongs to, and a release means moving `[Não publicado]` to a version
heading and tagging it.

The published build is `v0.2.0-rc.1`. Stable `0.2.0` was never released, and neither was
`0.2.1`, which is why the next number is `0.3.0`.

**A line is renumbered while it is still a branch, never after the tag.** A tag is a promise
about a tree; renaming a version after the tag means the artefact people downloaded points at
a number that no longer exists, and the changelog ends up with two releases claiming the same
range. Renumbering before the tag costs one line in `CMakeLists.txt` and the reconciliation of
the four documents that quote the number. That is why the `0.2.2` line became `0.3.0`: it was
opened as a patch by scope, then took user-visible work — the external tempo pipeline, leading
silence removal, bar sync to the host meter, the BPM source button, progress on load, and a MIDI
export that now carries a tempo map — and the rule above has no honest way to call that a patch.

### Patch release policy

A patch release is what ships when something already released does not behave as documented.
The line it is cut from is frozen except for corrections.

**A patch contains only:**

- a fix for a defect that can be described as "before this, a user hit X; now they do not";
- a fix for a defect introduced by the previous patch of the same line;
- a change that makes the product *stop* doing something wrong, not a change that makes it do
  something new;
- documentation and CI corrections that state the truth about the code.

**A patch never contains:**

- a new control, a new panel, a new output or a new file the user did not have to click before;
- a change to what the plugin writes — exported files, the parameter contract, state layout;
- a new dependency, or a new binary shipped to the user's machine;
- a reformat, a rename, or a refactor that does not exist to serve a fix. Those go in with the
  next minor.

The test is not "is it small" and not "is it urgent". It is **would a user on the previous
patch notice anything they did not have before**. If yes, it is a minor, and it waits.

**Cutting one:**

1. Freeze the line. No new work lands on `release/0.x` except fixes.
2. Every commit since the last tag carries a `Signed-off-by` and a Conventional Commits subject
   whose type is `fix`, `build`, `ci`, `docs`, `test` or `chore`. A `feat` subject on the line is
   the signal that the line has to be renumbered instead — catch it at review, not at the tag.
3. Green on the whole platform matrix for the merge to `develop`, and green again for
   `develop` → `main`. `main` is protected: signed commits, code owner review, required CI.
4. Move `[Não publicado]` in the changelog to the version heading, reconciling every document
   that quotes the number — README, the badge, `docs/index.html`, this file.
5. Tag it. `tag.gpgsign` is on, so the tag is signed too.
6. Verify the published artefact rather than the local one: the SHA-256 computed locally must
   equal the SHA-256 of the asset after upload, and the bundle must contain
   `GetPluginFactory` and an intact `moduleinfo.json`. A checksum that was only checked before
   the upload proves the upload did not corrupt it, not that GitHub served the right bytes.
7. Move the download button and the Pages release link to the new tag. They point at the last
   published artefact, not at the working tree.

Until the tag exists the number is provisional, and the branch name carries it
(`release/0.3.1`). Renaming the branch to match a renumbered version is part of step 4, not an
extra step.

## Not accepted

- A new dependency without discussing its license first. The domain core has no dependency
  outside JUCE on purpose — `aubio` (GPLv3) and *Rubber Band* (GPLv3/commercial) change that
  calculus and need a recorded decision first.
- A UI string outside `Text::t`, or a translation column missing.
- A commit without `Signed-off-by`.
- A commit outside the Conventional Commits format, or a type not in the list.
- A PR that leaves the repository dirty: an artifact, a stale build tree, or a document that
  contradicts the code it describes.

## Where things live

`Source/` is grouped by module: `Core/`, `Audio/`, `Analysis/`, `Ui/`, `Plugin/`, `Export/`.
Each folder is on the include path, so includes are `#include "Arquivo.h"` with no path.

| I need to touch… | Go to |
|---|---|
| Tuning / ratio / pitch detection | `Source/Core/Tuning.h` · `Source/Core/Tuning.cpp` |
| Plugin parameters | `Source/Core/ParameterIds.h` · `Source/Plugin/PluginProcessor.cpp` |
| Playback, loop, waveform peaks | `Source/Audio/FilePlayer.*` · `Source/Audio/Waveform.h` |
| Pitch shifting | `Source/Audio/PitchShifter.*` |
| BPM, meter, bars, tuning detection | `Source/Analysis/TempoAnalyser.*` |
| Offline fingerprint | `Source/Analysis/FingerprintWorker.*` |
| Texts and screen labels | `Source/Ui/Text.cpp` (table `pt-BR | en-GB | en-US | es-ES`) |
| Colours and drawing | `Source/Ui/Theme.h` |
| Panel layout | `Source/Plugin/PluginEditor.cpp` |
| MIDI export | `Source/Export/MidiMapExporter.*` |
| Build, install, presets | `scripts/build.ps1` · `scripts/install-vst3.ps1` · `CMakeLists.txt` |
| Domain tests | `Tests/DomainTests.cpp` (no framework; the runner is the `check` helper) |

## Housekeeping

The repository is expected to be clean, and "clean" is checkable:

- **No artifacts.** `build/` is ignored. A `desktop.ini` inside a built bundle is *not* harmless:
  the Windows shell writes it when someone customises a folder in Explorer, the VST3 bundle spec
  allows no stray files in its root, and a `.gitignore` does not protect a zip.
  `scripts/install-vst3.ps1` sweeps a closed list of these before installing and reverts the
  install if any of them reaches the host. If you see one, that is a bug report.
- **No stale build trees.** Two toolchains building the same code into two folders produced two
  Release binaries with different hashes and no way to tell which was current. One toolchain, one
  tree, and `-Preset` is explicit on both scripts.
- **No number written twice.** Version in `CMakeLists.txt`, test count in the suite output,
  feature status in the issue. A fact that changed and was not reconciled is an unfinished task.
- **No document promising what the code does not do.** The bundle description and the README are
  read by people deciding whether to install this; a feature named there and missing from the
  binary becomes a bug report against something that was never delivered.

Cleanup after delivery is part of the task, not a follow-up.