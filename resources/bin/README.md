# resources/bin — binários da pipeline externa de andamento

Executáveis que o PartePlay usa para medir o BPM quando existem. O localizador em
`Source/Analysis/ExternalBpm.cpp` procura nesta pasta (`resources/bin`, ao lado do
executável), depois em `%APPDATA%/PartePlay/bin` e, por fim, no `PATH`.

## Conteúdo

| Arquivo | Origem | Versão | Licença |
| --- | --- | --- | --- |
| `ffmpeg.exe`, `ffprobe.exe` e as DLLs `avcodec-63`, `avdevice-63`, `avfilter-12`, `avformat-63`, `avutil-61`, `swresample-7`, `swscale-10` | [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds), pacote `ffmpeg-master-latest-win64-lgpl-shared` | master (rolling) | LGPL v3 — `LICENSE-ffmpeg.txt` |
| `soundstretch.exe` | [surina.net — SoundTouch / SoundStretch](https://www.surina.net/soundtouch/), pacote `soundstretch-v2.3.3.zip` | 2.3.3 | LGPL v2.1 — `LICENSE-soundtouch.txt` |

O `ffplay.exe` não está incluído: não é usado.

## Integridade (SHA-256)

```
62279df82f983c841d934e81d3181ed1670061c3845f7ba83f5b1fb722a2e635  avcodec-63.dll
b649f396ad40239f64b02c56ad090d162450d2739f12c211332b8e2eb0c15000  avdevice-63.dll
7ae291102bd19ee3afeb391734439f000834670f3e2fedbf25e261906474b2b7  avfilter-12.dll
a005ecf81fcff91bf7b4cbbbc516bbf7a7aac736713cdfdf9ee7c5df7f42bc89  avformat-63.dll
e88a419c336f555bd44471a93e31e0a202dbbfc43258498673b516447b1fb40d  avutil-61.dll
76d1292ddb08447fbe8908b6d9480c30331e7f34c706cf581cb3ad0f03e05864  ffmpeg.exe
333e75a69bf650d9e0210d6745a70b846d448fe94abe440251c164ea327a11d9  ffprobe.exe
1f900c4cdfca42f660f149e1edde24c51b31e5c3b3e4ab9d4e43a384d82e1708  soundstretch.exe
8aab72746d6c3ffe35a4094c1a2d119e691e4eae9607868ca706819edf5ce5ea  swresample-7.dll
00bd890c9e7b6e39aa710a8117a264fac00985c7cfda6b641c5db7a713238531  swscale-10.dll
```

Os textos de licença foram lidos dos próprios pacotes baixados, não de memória.
O restante das dependências está em `THIRD_PARTY_NOTICES.md`.
