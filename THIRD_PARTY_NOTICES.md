# Third-party notices

PartePlay is distributed under the AGPL-3.0 (see [`LICENSE`](LICENSE)). That licence
covers only PartePlay's own code. The distributed VST3 binary also contains the
components below, each under its own licence.

This file exists because PartePlay **statically links** Chromaprint, and the LGPL 2.1
requires the recipient of a binary to be told which parts are under which licence and
be given the corresponding source. See [issue #4](https://github.com/rubenslyra/parteplay/issues/4).
It also covers the FFmpeg and SoundTouch executables bundled under `resources/bin/`
for the external tempo pipeline (issue #1).

Every licence string quoted here was read from the licence file actually shipped in the
source tree, not from memory. The path where each one was read is given so the claim
can be re-checked.

---

## Obligation summary

| Component | Licence | Obligation towards the user | Where the source is |
| --- | --- | --- | --- |
| Chromaprint 1.6.1 | LGPL 2.1 (see note) | Source of the linked library, plus the right to relink | `Chromaprint-Dependences/chromaprint-1.6.1.tar.gz` |
| avresample (from FFmpeg) | LGPL 2.1 | Same as above, for the FFmpeg-derived files | Inside the same tarball, `src/avresample/` |
| KissFFT | BSD-3-Clause | Attribution only | Inside the same tarball, `src/3rdparty/kissfft/` |
| JUCE 9.0.2 | AGPL-3.0 **or** commercial | Corresponding source of the whole work | This repository, plus the JUCE tag `9.0.2` |
| VST3 SDK | MIT | Attribution only | Bundled in the JUCE checkout |
| LV2 SDK (lilv, serd, sord) | ISC | Attribution only | Bundled in the JUCE checkout |
| FFmpeg (ffmpeg.exe, ffprobe.exe, av* DLLs) | LGPL v3 | Distribution of the licence text; separate process, not linked | `resources/bin/`, upstream BtbN/FFmpeg-Builds |
| SoundTouch / SoundStretch (soundstretch.exe) | LGPL v2.1 | Distribution of the licence text; separate process, not linked | `resources/bin/`, upstream surina.net |

The compliance mechanism that satisfies the LGPL source obligation is already in
place: `Chromaprint-Dependences/chromaprint-1.6.1.tar.gz` is committed to this
repository and is the exact archive the build extracts and compiles. The build never
patches it — `CMakeLists.txt` extracts it verbatim and sets
`USE_INTERNAL_AVRESAMPLE=ON` — so the committed tarball *is* the source of the
statically linked code.

Because the licence is LGPL 2.1 §6 and the linkage is static, the user must also be
able to relink the binary against a modified Chromaprint. The archive gives them
everything needed to do that.

---

## 1. Chromaprint 1.6.1

- **Version:** 1.6.1 (the version string lives in `PARTEPLAY_JUCE_VERSION`'s
  sibling `CHROMAPRINT_ARCHIVE` in `CMakeLists.txt`, and in the tarball name)
- **Upstream:** <https://github.com/acoustid/chromaprint>
- **Licence:** as a whole, **LGPL 2.1**
- **Read from:** `chromaprint-1.6.1/LICENSE.md`, extracted by the build to
  `build/<preset>/chromaprint-src/chromaprint-1.6.1/LICENSE.md`

Chromaprint's own source is MIT, but it embeds parts of FFmpeg under LGPL 2.1, so the
project as a whole is LGPL 2.1. Verbatim from that file:

> Chromaprint's own source code is licensed under the MIT license, but we include some
> parts of the FFmpeg library, which is licensed under the LGPL 2.1 license. As a whole,
> Chromaprint should be therefore considered to be licensed under the LGPL 2.1 license.

And its MIT portion:

> Copyright (C) 2010-2016  Lukas Lalinsky

Full LGPL 2.1 text: <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html>

### 1.1 FFmpeg-derived files (avresample)

- **Licence:** LGPL 2.1
- **Files:** `src/avresample/resample2.c`, added to the build by
  `chromaprint-src/chromaprint-1.6.1/src/CMakeLists.txt:78` when
  `USE_INTERNAL_AVRESAMPLE` is `ON`. PartePlay forces it `ON` in
  `CMakeLists.txt:95`.

> **Note on a misleading build message.** The Chromaprint configure step prints
> `Could not find FFMPEG` followed by `Building without audio conversion support`.
> Both are wrong for this build. The second message is only reachable when
> `AUDIO_PROCESSOR_LIB` is `swresample`
> (`chromaprint-1.6.1/CMakeLists.txt:168`); PartePlay sets it to `avresample`, which
> that branch never matches, and the internal avresample source is added
> unconditionally by `USE_INTERNAL_AVRESAMPLE`. The internal converter is compiled
> and linked. This is recorded so the next person to read the log does not file a
> false bug against it.

### 1.2 KissFFT

- **Licence:** BSD-3-Clause
- **Copyright:** Copyright (c) 2003-2010 Mark Borgerding. All rights reserved.
- **Read from:** `chromaprint-1.6.1/src/3rdparty/kissfft/COPYING`, which states
  `SPDX-License-Identifier: BSD-3-Clause`.
- **Why it is here:** no external FFT is available, so PartePlay pins
  `FFT_LIB=kissfft` (`CMakeLists.txt:97`) to use the copy bundled inside the
  Chromaprint tarball.

---

## 2. JUCE 9.0.2

- **Licence:** dual **AGPL-3.0 / commercial**. PartePlay uses the **AGPL-3.0** option.
- **Read from:** `JUCE_ROOT/LICENSE.md` in the checkout, and
  `https://juce.com/legal/juce-9-licence/`

> The JUCE Framework modules are dual-licensed under the AGPLv3 and the commercial
> JUCE licence.

This is the licence boundary that decides the product: the core is AGPL-3.0, so a
closed-source edition would require buying the commercial JUCE licence. Any future
proprietary DLL or service that links against this core risks being a derivative work
of an AGPL work — that question has to be settled legally *before* it is implemented.

### 2.1 Third-party code inside JUCE

JUCE vendors additional libraries (image, font and audio codecs among them) and is
responsible for their notices; its licence file covers them. PartePlay does not
re-enumerate them here, and this is a deliberate choice rather than an oversight:
enumerating them from memory would risk publishing wrong licence claims, which is
worse than deferring to the authoritative source.

**To do:** enumerate the vendored list from the JUCE 9.0.2 checkout as part of the
release checklist, so this section stops being a pointer and starts being a list.

---

## 3. VST3 SDK

- **Licence:** MIT
- **Copyright:** Copyright (c) 2025, Steinberg Media Technologies GmbH
- **Read from:** `JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt`

---

## 4. LV2 SDK — lilv, serd, sord

- **Licence:** ISC
- **Copyright:** Copyright 2011-2021 David Robillard <d@drobilla.net>
- **Read from:** the `COPYING` file in each of `lilv/`, `serd/` and `sord/` under
  `JUCE/modules/juce_audio_processors_headless/format_types/LV2_SDK/`

> Permission to use, copy, modify, and/or distribute this software for any purpose
> with or without fee is hereby granted, provided that the above copyright notice
> and this permission notice appear in all copies.

PartePlay does not ship an LV2 build, but the sources are compiled into the
`juce_audio_plugin_client` translation unit, so they are in the binary.

---

## 5. FFmpeg — bundled executables

- **Files:** `ffmpeg.exe`, `ffprobe.exe`, `avcodec-63.dll`, `avdevice-63.dll`,
  `avfilter-12.dll`, `avformat-63.dll`, `avutil-61.dll`, `swresample-7.dll`,
  `swscale-10.dll`
- **Where:** `resources/bin/` in the repository; `scripts/install-vst3.ps1` copies
  it to `%APPDATA%/PartePlay/bin`, which is where the plugin looks for it (the VST3
  host owns the process, so a path next to the plugin DLL is not reliable)
- **Upstream:** <https://github.com/BtbN/FFmpeg-Builds>, package
  `ffmpeg-master-latest-win64-lgpl-shared`
- **Licence:** **LGPL v3**
- **Read from:** `resources/bin/LICENSE-ffmpeg.txt`, shipped verbatim from the
  downloaded package

These are **separate programs**, invoked by `Source/Analysis/ExternalBpm.cpp` as a
child process to convert the source file to WAV. They are not linked into the
PartePlay binary, so the LGPL relinking obligation does not extend to PartePlay's own
code; the obligation is to ship the licence text (done) and to identify the version.

The build is a rolling upstream master, not a pinned release, because BtbN publishes
binary builds from `master`. It is identified by the `av*` SONAMEs above. To replace
it with a reproducible pin, drop a dated BtbN release into `resources/bin/` and update
this section and `resources/bin/README.md`.

## 6. SoundTouch / SoundStretch — bundled executable

- **File:** `soundstretch.exe`
- **Where:** `resources/bin/` in the repository; deployed to `%APPDATA%/PartePlay/bin`
  by `scripts/install-vst3.ps1`, same as FFmpeg
- **Upstream:** <https://www.surina.net/soundtouch/>, package `soundstretch-v2.3.3.zip`
- **Version:** 2.3.3
- **Copyright:** Copyright (c) Olli Parviainen
- **Licence:** **LGPL v2.1**
- **Read from:** `resources/bin/LICENSE-soundtouch.txt`, shipped verbatim from the
  SoundTouch source tarball `soundtouch-2.3.3.tar.gz` (`COPYING.TXT`)

Like FFmpeg, this is a separate program launched as a child process (the `-bpm`
switch prints `Detected BPM rate <value>`), not code linked into PartePlay.

## Not bundled

- **FFmpeg as a linked library** — not linked. Only the avresample subset vendored
  inside Chromaprint is compiled into the binary; the full FFmpeg above is an
  external process.
- **AcoustID application key** — intentionally absent. The fingerprint is computed
  fully offline; online lookup is disabled, and the key would come from the
  environment, never from the binary.
- **No other network, telemetry or analytics library** is linked.

---

*Generated as part of issue #4. If you find a component in the binary that is not
listed here, that is a defect in this file — please open an issue.*
