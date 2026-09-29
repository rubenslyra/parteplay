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
- **Descriptive commits.** The history is read with DCO: every commit carries `Signed-off-by`.
- **Never commit artifacts.** `build/`, heavy captures and binaries are already on `.gitignore`.

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

## Not accepted

- A new dependency without discussing its license first. The domain core has no dependency
  outside JUCE on purpose — `aubio` (GPLv3) and *Rubber Band* (GPLv3/commercial) change that
  calculus and need a recorded decision first.
- A UI string outside `Text::t`, or a translation column missing.
- A commit without `Signed-off-by`.

## Where things live

| I need to touch… | Go to |
|---|---|
| Tuning / ratio / pitch detection | `Source/Tuning.h` · `Source/Tuning.cpp` |
| Texts and screen labels | `Source/Text.cpp` (table `pt-BR | en-GB | en-US | es-ES`) |
| Colours and drawing | `Source/Theme.h` |
| Panel layout | `Source/PluginEditor.cpp` |
| Plugin parameters | `Source/ParameterIds.h` · `PluginProcessor.cpp` |
| Offline fingerprint | `Source/FingerprintWorker.*` |
| Domain tests | `Tests/DomainTests.cpp` (no framework; the runner is the `check` helper) |