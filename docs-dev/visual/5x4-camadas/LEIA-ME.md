# PartePlay — referência visual em camadas · 5:4

Canvas: **2000 × 1600 px**. As sete camadas PNG têm exatamente essa dimensão e a mesma origem (0,0). Sobreponha na ordem numérica; o fundo é opaco e todas as demais têm transparência. `00_composicao_5x4.png` é a prévia final, não uma camada adicional.

1. `01_fundo.png` — cenário azul-noite, moldura, cabeçalho, marca e crédito.
2. `02_waveform_track.png` — faixa carregada **ilustrativa**, visualizador de forma de onda, cursor e marcações.
3. `03_instrumentos.png` — visualizador do instrumento ativo (Trompete).
4. `04_transporte.png` — painel e trilho do transporte, sem os botões.
5. `05_afinacao.png` — painel da afinação A4 e transposição.
6. `06_bpm_metrica.png` — painel ilustrativo de BPM, métrica e compassos.
7. `07_botoes.png` — todos os botões, seletores e comandos dos painéis.

A faixa, os 128 BPM, o 4/4 e os 108 compassos são **dados de apresentação**, não uma análise real. No app, BPM e métrica só devem ser exibidos como sugestão calculada após a análise do áudio. A referência web não transpõe nem sincroniza com MuseScore; o motor e a ponte vivem no projeto JUCE. A interface deve manter o crédito “Rubinho Lyra / Software Eng”.

A composição foi redesenhada em 5:4, sem esticar a captura 16:9 anterior. A implementação em JUCE está em `Source/PluginEditor.cpp` (layout 1000×800, proporção fixa 5:4) e a fonte de verdade do tema é `Source/Theme.h`; as cores do protótipo web `visual/styles.css` devem acompanhar esse arquivo.
