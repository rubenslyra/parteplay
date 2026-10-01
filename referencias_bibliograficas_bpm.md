# Referências Bibliográficas e Acadêmicas: Padrões de BPM e Precisão de Tempo em Áudio Digital

Este documento reúne referências bibliográficas, especificações técnicas e manuais de engenharia de áudio que fundamentam os conceitos de resolução temporal, ponto flutuante, amostragem e a representação de BPM em DAWs e softwares de notação musical.

---

## 1. Unidades de Medida e Resolução de Tempo (MIDI, Ticks e PPQ)

### Especificação Oficial MIDI 1.0 & MIDI 2.0
* **Referência:** MIDI ASSOCIATION. **The MIDI 1.0 Detailed Specification**. Los Angeles: MIDI Association, 1996-2020.
* **Referência Atualizada:** MIDI ASSOCIATION. **MIDI 2.0 Application Standards**. Los Angeles: MIDI Association, 2020.
* **Contexto:** É a fonte primária para entender o conceito de **PPQ (Pulses Per Quarter Note)** e *Ticks*. Explica como o tempo musical é fatiado em pulsos discretos para sincronização entre hardwares e softwares (sequenciadores).
* **Acesso:** Disponível oficialmente em [midi.org](https://midi.org).

### Livro-Texto de Processamento Digital de Sinais (DSP) aplicado à Música
* **Referência:** ROADS, Curtis. **The Computer Music Tutorial**. 2. ed. Cambridge: MIT Press, 2023.
* **Contexto:** O capítulo sobre "Digital Audio Concepts" e "MIDI and Digital Audio Sychronization" é a maior referência mundial para entender a matemática por trás da conversão de tempo (BPM) para amostras (samples) e a física do clock digital.

### Sincronização e Resolução Temporal
* **Referência:** HUBER, David Miles; RUNSTEIN, Robert E. **Modern Recording Techniques**. 9. ed. New York: Routledge, 2017.
* **Contexto:** Explica detalhadamente os conceitos de *Clock*, *Jitter*, *Sample Rate* e como os sistemas de áudio digital (DAWs) mantêm a estabilidade temporal interna ao traduzirem divisões de tempo musical em microssegundos.

---

## 2. Padrões de BPM e Precisão Numérica (Ponto Fixo vs. Ponto Flutuante)

### Padrão IEEE 754 (Precisão de Ponto Flutuante)
* **Referência:** INSTITUTE OF ELECTRICAL AND ELECTRONICS ENGINEERS (IEEE). **IEEE 754-2019 - IEEE Standard for Floating-Point Arithmetic**. New York: IEEE, 2019.
* **Contexto:** Norma global que define como números de ponto flutuante (32-bit float e 64-bit double) são computados. Essencial para justificar o uso de representações com múltiplas casas decimais (ex: `,000`) em motores de áudio modernos para evitar erros de arredondamento acumulados (*drift*).

### Arquitetura de Áudio em DAWs (Ex: FL Studio, Ableton, Pro Tools)
* **Referência:** BOULANGER, Richard; LAZZARINI, Victor. **The Audio Programming Book**. Cambridge: MIT Press, 2010.
* **Contexto:** Destaca como os plugins (VST/AU) e as DAWs calculam parâmetros de tempo continuamente usando floats. Demonstra matematicamente por que a representação em ponto fixo (inteiros) gera imperfeições cumulativas na reprodução de loops de áudio de tamanhos arbitrários.

### Manuais Técnicos e Documentação de Desenvolvimento (SDKs)
* **Referência:** STEINBERG MEDIA TECHNOLOGIES. **VST 3 API Documentation**. Hamburgo: Steinberg, 2020.
* **Contexto:** A documentação do padrão VST demonstra como os dados de tempo do host (*Host Time Info*) são passados para os plugins usando estruturas baseadas em variáveis `double` (ponto flutuante de dupla precisão), fornecendo o BPM exato com várias casas decimais.
* **Referência:** MUSESCORE BVBA. **MuseCore Element API & Plugin Development Guide**. v4.x. Disponível na documentação interna de desenvolvedores do MuseScore.
* **Contexto:** Essencial para entender como o MuseScore 4 expõe os dados de tempo e andamento da partitura através de sua API do sistema de propriedades (`playTicks`, `tempo`).

---

## 3. Distinção entre Unidades: Tempo (Ticks/Samples) vs. Altura (Cents)

### Acústica e Medidas Musicais
* **Referência:** CAMPBELL, Murray; GREATED, Clive. **The Musician's Guide to Acoustics**. Oxford: Oxford University Press, 1994.
* **Contexto:** Define formalmente a unidade **Cent** como a medida logarítmica de proporção tonal (afinação), criada por Alexander John Ellis, servindo de base bibliográfica para diferenciar que "centravos/cents" pertencem ao domínio da frequência (Hz), enquanto o BPM e suas frações pertencem ao domínio do tempo.

---
*Este documento serve como base técnica e científica para o desenvolvimento de especificações de engenharia de software musical.*
