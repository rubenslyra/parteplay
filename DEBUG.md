# Debug — opening the plugin and reading the window data

This is the runbook for **looking at the window** when something looks wrong: the text is too small,
a control is clipped, a menu row is unreadable, or you simply want to confirm which build the host is
actually running.

It is written for the case that costs the most time: the plugin is loaded in a host, the host is
closed, you rebuild, and the host silently keeps the old binary. **Skip to
["Reinstall without losing time"](#reinstall-without-losing-time)** if you are only here to rebuild
and reload.

---

## 1. What the overlay is

`Ctrl+D` toggles a diagnostic panel over the editor. It is compiled **only in Debug builds** — the
gating macro is `PARTEPLAY_DEBUG_OVERLAY`, defined in `Source/Plugin/PluginEditor.h`:

```cpp
#if JUCE_DEBUG || defined(_DEBUG)
 #define PARTEPLAY_DEBUG_OVERLAY 1
#else
 #define PARTEPLAY_DEBUG_OVERLAY 0
#endif
```

Because the macro is evaluated at compile time, the released binary carries **neither the overlay code
nor the shortcut**. You can verify this on any build — the string is either in the binary or not:

```powershell
$bin = "build\msvc-2026\PartePlay_artefacts\Release\VST3\PartePlay.vst3\Contents\x86_64-win\PartePlay.vst3"
[System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($bin)).Contains("PARTEPLAY DEBUG")
# False -> the published binary is clean
```

## 2. Building and installing a Debug build

Close MuseScore (or the DAW) first — the binary is locked while the plugin is loaded.

```powershell
.\scripts\build.ps1 -Configuration Debug
.\scripts\install-vst3.ps1 -Configuration Debug
```

The install script needs an elevated shell (it writes to `C:\Program Files\Common Files\VST3`). If
you are not already elevated, right-click the shell and choose *Run as administrator*, or let the
script elevate itself.

Artifact:

```
build/msvc-2026/PartePlay_artefacts/Debug/VST3/PartePlay.vst3
```

**The Debug install replaces the Release one.** They occupy the same path, so there is only ever one
PartePlay on the machine. To go back:

```powershell
.\scripts\install-vst3.ps1 -Configuration Release
```

## 3. Reading the overlay

Press `Ctrl+D` once the plugin window has focus. The panel reports:

| Field | Meaning | When it explains a problem |
| --- | --- | --- |
| `janela` | Editor size in **logical** units, plus the minimum enforced by the constrainer | If the current size equals the minimum, the host cannot make the window bigger — raising a font will clip instead of scale |
| `pixels` | The same size multiplied by JUCE's global scale factor | Shows how many physical pixels the window actually occupies |
| `tela` | Primary display logical size, its scale factor and its DPI | A high DPI with a low scale factor is the usual reason a 15 px label still looks tiny |
| `versao` | `PARTEPLAY_VERSION`, i.e. the version compiled in | **The fastest way to confirm the host is not running a stale binary** |
| `idioma` | Language combo box size, the derived popup row height, and the active culture | Compares the closed combo against the open menu in one glance |
| `fonte` | The live `Theme` font tokens (control / body / caption / section) | Confirms a `Theme` edit reached the running binary |

### The popup row height is not a token

Worth knowing before you tune it, because it is the part that surprises people:

- `LookAndFeel_V2::positionComboBoxText` gives the combo's label a height of `comboHeight - 2`.
- `LookAndFeel_V2::getOptionsForComboBoxPopupMenu` builds the menu with
  `.withStandardItemHeight (label.getHeight())`.

So **the popup row height is the combo height minus 2**. Growing the combo grows the menu; there is no
separate size to set, and `Theme` has no entry for it. The overlay prints it because it is invisible in
the theme and moves on its own.

### Scale factor caveat

`Component` does not expose a DPI scale factor, and in a plugin the host applies the DPI anyway. The
`tela` line reports the primary display's own scale and DPI, which is what you need to reason about it;
`tela` and `pixels` will legitimately disagree in a multi-monitor setup.

## 4. Reinstall without losing time

This is the failure that costs hours. The host loads the VST3 **once, at startup**. Rebuilding while
MuseScore is open changes the file on disk and nothing else — the panel keeps serving the old binary,
and a "fix" that is already correct looks broken.

So, every time:

1. Close MuseScore entirely.
2. Build.
3. Install (elevated).
4. Reopen MuseScore.

To confirm the installed binary is really the one you just built:

```powershell
$built = "build\msvc-2026\PartePlay_artefacts\Debug\VST3\PartePlay.vst3\Contents\x86_64-win\PartePlay.vst3"
$inst  = "C:\Program Files\Common Files\VST3\PartePlay.vst3\Contents\x86_64-win\PartePlay.vst3"
(Get-FileHash $built).Hash
(Get-FileHash $inst).Hash
```

Identical hashes, then the host is about to run your build. `install-vst3.ps1` performs this comparison
itself and aborts on a mismatch.

## 5. The version has exactly one source

The overlay's `versao` field comes from `PARTEPLAY_VERSION`, a macro fed from
`project(PartePlay VERSION …)` in `CMakeLists.txt`. It is not typed by hand anywhere, and the CI
rejects any `X.Y.Z` literal under `Source/` or `Tests/`:

```
Versao em um unico lugar
```

If you find yourself wanting to change the version to test something, edit `CMakeLists.txt` instead —
a literal anywhere else fails the build on purpose, and a hardcoded string in the editor would survive
the release unnoticed.