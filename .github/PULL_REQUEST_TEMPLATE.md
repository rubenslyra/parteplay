## What changes

<!-- Problem before the solution. If it is a fix, the symptom; if a feature, what you cannot do today. -->

## How I verified

<!-- What you ran and what you saw. "ctest, 0 failures" and "checked in the host with track X" are
     the two checks that matter. Tick what you did not do as well. -->

- [ ] `ctest --preset msvc` → __ checks, __ failures
- [ ] Clean Release build
- [ ] CI green on all three platforms (Windows / Linux / macOS)
- [ ] Checked in the host (MuseScore 4 / DAW): ______
- [ ] Checked at the editor's minimum size (800×640)

## Diff scope

- [ ] One subject only — I did not mix refactor with fix or text
- [ ] No new dependency
- [ ] No UI string outside `Text::t`; the four columns filled in `Source/Text.cpp` (`pt-BR | en-GB | en-US | es-ES`)
- [ ] Any change to the translation table comes with its key list in `Tests/DomainTests.cpp` in the same commit
- [ ] No UTF-8 file without BOM; no CRLF
- [ ] `build/` and artifacts out of the diff
- [ ] No credential in the diff (`secrets` is ignored; no AcoustID value embedded)
- [ ] Commit carries `Signed-off-by` (DCO required — the check fails without it)

## Documentation

- [ ] `README.md` updated in the same commit
- [ ] `CHANGELOG.md` entry under **Unreleased**
- [ ] Any changed fact (version, test count, feature status) reconciled across all `.md`, `.cpp`, `.h`, `.json` — the version lives only in `project(PartePlay VERSION x.y.z)`
- [ ] UI numbers go through `Text::number` when they have decimals

## Breaking change

- [ ] None
- [ ] Yes: describe below (parameter index, saved state, public API)