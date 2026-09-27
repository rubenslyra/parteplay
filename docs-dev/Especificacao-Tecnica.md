## Especificação Técnica Integrada: PartePlay (VST3) — Fase de Inteligência Musical

Abaixo está a proposta do escopo completo do projeto, unindo a base arquitetônica existente com o módulo avançado de Recuperação de Informação Musical (MIR), matemática de timeline e resolução de impasses da API VST3.

### 1. Visão Geral e Arquitetura Base

O PartePlay atua como um reprodutor de áudio de referência que se sincroniza integralmente ao _transport_ do software hospedeiro, aplicando transposição de tom direcionada ao instrumento da partitura sem gerar descompassos ou alterações na velocidade original. Com a nova expansão, o VST3 se transforma em um Sampler Inteligente, automatizando a adequação tonal e rítmica.

- **Stack:** C++ / JUCE 9.0.2 / CMake / Visual Studio 2022.

- **Plataforma e Formato:** Windows / VST3.

### 2. Sincronia, Reprodução e Mascaramento (Edição Não-Destrutiva)

- O sistema carrega integralmente na memória arquivos nos formatos WAV, FLAC, MP3 e OGG.

- A reprodução é vinculada ao relógio do hospedeiro (`isPlaying` e `timeInSamples`), com conversão de taxa de amostragem por interpolação linear, assegurando que o _seek_ avance ou retroceda com precisão de amostra.

- **Mascaramento de Áudio da Timeline:** O arquivo de áudio nunca é destrutivamente cortado. O plugin implementará um "Ponteiro de Leitura" com limite máximo de reprodução acoplado à quantidade de compassos da partitura. Se o usuário apagar compassos no MuseScore 4 e o _playhead_ ultrapassar esse limite, o plugin suspende a leitura do buffer e emite silêncio, ocultando a sobra de áudio do final para o início.

### 3. Motor de Transposição e Análise de Frequência (Pitch-Shift)

- A transposição ocorre de forma offline utilizando um phase vocoder próprio (FFT 2048, hop 512, janela Hann), o que preserva rigorosamente a duração original e a proporção de sincronia 1:1 com a DAW.

- Há suporte para calibração de afinação de referência (432 Hz a 445 Hz) e uma tabela de compensação por instrumento (Piano, Trompete, Sax Alto, Trompa).

- **Detecção Autônoma de Afinação:** O plugin aplicará uma Transformada de Fourier (FFT) em recortes do áudio para identificar a frequência fundamental real da gravação (ex: pico da nota A4 em 432 Hz). O algoritmo calculará o desvio em _cents_ para a pré-configuração (ex: 440 Hz) e aplicará o _Pitch Shifting_ corretivo automaticamente, englobando a compensação de afinação e do instrumento na mesma equação de reamostragem.

### 4. Análise Avançada de Tempo e Métrica (O "Chão")

A atual detecção heurística (varredura de 60 a 180 BPM) evoluirá para uma varredura de transientes profunda utilizando bibliotecas MIR integradas ao C++.

- **Detecção de BPM:** O algoritmo fará o rastreamento de transientes de energia (picos de volume como percussão ou ataques de notas) medindo a distância em milissegundos para cravar o andamento.
- **Detecção de Assinatura de Tempo:** O plugin identificará o "Tempo Forte" (Downbeat) analisando padrões de acentuação de baixas frequências e repetições harmônicas. O algoritmo determinará autonomamente se o agrupamento natural da música (_chão_) exige um compasso quaternário (4/4) ou ternário (3/4).

### 5. Cálculo Matemático de Timeline

Com os parâmetros isolados, o plugin utilizará a equação universal de razão e proporção de áudio para determinar a quantidade de bússolas musicais necessárias antes do início da edição.

$$C = \frac{T \times BPM}{60 \times N}$$

Onde:

- $C$ = Número total de compassos.
- $T$ = Duração total da música em segundos originais.
- $BPM$ = Andamento extraído da análise de transientes.
- $N$ = Número de tempos por compasso deduzido da assinatura (4 para 4/4; 3 para 3/4).

_(Nota: O cálculo ignora estruturas escritas como ritornellos ou codas, balizando-se estritamente pela duração cronológica do player em relação ao arranjo linear)._

### 6. Integração Host-Plugin e UX de Sincronia

Devido à arquitetura _Slave/Master_ do padrão VST3 (onde o plugin não possui permissão de código para injetar mudanças de andamento forçadas no MuseScore 4), o fluxo de trabalho adotará a seguinte interface:

- **Geração de Mapa de Tempo (MIDI Drag and Drop):** O plugin compilará as detecções de BPM e Assinatura de Tempo em um arquivo MIDI virtualizado. A interface fornecerá um painel arrastável; o usuário fará o _Drag and Drop_ para a partitura, e o MuseScore 4 herdará instantaneamente a grade calculada (BPM, 4/4, quantidade de compassos).
- **Flexibilidade de Pauta:** Após a sincronia inicial, se o usuário alterar manualmente a métrica no MuseScore (ex: dividindo um compasso 4/4 em dois compassos 2/4 ou alterando a pulsação para 6/8), o Host dobrará a quantidade de compassos e o motor de áudio do PartePlay continuará reproduzindo o arquivo de forma íntegra, fluindo com o relógio sem ser afetado por compressão ou expansão de tempo.

### 7. Interface Gráfica e Parâmetros Expostos

- A interface preservará os controles de carregamento, lista de instrumentos, transposição e afinação (salvos via APVTS e XML binário na sessão do projeto).

- Será adicionado um painel visual consolidado de Inteligência reportando o relatório do áudio (ex: _"Áudio Detectado: 120 BPM | 3/4 | A=432Hz | 115 Compassos Estimados"_), seguido pela área de arrasto do Mapa de Tempo.

Qual biblioteca em C++ (como Aubio ou Essentia) você prefere investigar primeiro para integrar a detecção matemática dos transientes e do "chão" da harmonia?

---

## Proposta de Desenvolvimento e Especificação Técnica: PartePlay (VST3)

O PartePlay é um plugin VST3, atualmente na versão 0.1.0, desenvolvido para ambiente Windows utilizando a stack C++, JUCE 9.0.2, CMake e Visual Studio 2022. A solução atua como um reprodutor de áudio de referência que se sincroniza integralmente ao _transport_ do software hospedeiro (como o MuseScore 4), aplicando transposição de tom direcionada ao instrumento da partitura sem gerar descompassos ou alterações na velocidade original.

### 1. Núcleo de Sincronia e Reprodução

- O sistema carrega integralmente na memória arquivos de áudio nos formatos WAV, FLAC, MP3 e OGG.

- A sincronização é garantida pela leitura contínua dos estados de `isPlaying` e `timeInSamples` do hospedeiro, assegurando que comandos de play, pause e _seek_ (avanço/retrocesso) funcionem com precisão exata de amostra.

- A posição de reprodução do arquivo mantém uma proporção de 1:1 em relação ao relógio do host.

- A conversão da taxa de amostragem é realizada via interpolação linear, garantindo que arquivos com taxas divergentes do hospedeiro sejam reproduzidos corretamente e em velocidade normal.

- Ao atingir o final da última amostra de áudio, o plugin silencia automaticamente, sinaliza o fim da faixa e não executa repetição (loop).

- A arquitetura é _thread-safe_, utilizando bloqueios (`audioLock`) e ponteiros inteligentes (`shared_ptr<const>`) para trocar buffers sem realizar cópias pesadas de dados.

### 2. Motor de Transposição Inteligente (Pitch-Shift)

- A transposição ocorre de forma offline utilizando um phase vocoder próprio (FFT 2048, janela Hann), o que preserva rigorosamente a duração original do áudio.

- Como o áudio processado mantém o mesmo comprimento e taxa do arquivo de origem, a sincronia permanece intacta por construção.

- O sistema inclui uma tabela pré-configurada de instrumentos para facilitar o ajuste: Piano/Trombone (0 semitons), Trompete/Sax Tenor (-2 semitons), Sax Alto (+3 semitons) e Trompa (-5 semitons).

- Há suporte para calibração de afinação de referência entre 432 Hz e 445 Hz (com padrão em 440 Hz) e um modo de ajuste manual que permite transpor o áudio de -12 a +12 semitons.

- O recálculo da transposição é acionado sob demanda pela _message thread_ sempre que ocorrem mudanças de parâmetros, possuindo um sistema de _fallback_ que preserva o buffer original caso o cálculo resulte em erros matemáticos.

### 3. Análise de Áudio e Interface Gráfica

- Durante o carregamento do arquivo, o plugin realiza uma estimativa do andamento, varrendo frequências de 60 a 180 BPM, e calcula a quantidade correspondente de compassos considerando uma métrica padrão de 4/4.

- A interface (480x340) opera com um tema escuro e expõe controles vitais: seletor de arquivos, seleção de instrumento, afinação, transposição manual e painel de status em tempo real.

- Todos os parâmetros modificados pelo usuário, como instrumento e afinação, são persistidos nativamente no projeto do DAW através de formato XML binário.

### 4. Roadmap e Expansões Futuras

O escopo de desenvolvimento projetado para as próximas iterações inclui:

- Implementação de modo de treino, possibilitando repetição de trechos (loop) e alteração de velocidade do áudio sem afetar a afinação.

- Salvamento e recuperação de _presets_ atrelados a arranjos ou partituras específicas.

- Inclusão de um visualizador de forma de onda (_waveform_) e análise automática de tonalidade do arquivo carregado.

- Suporte a processamento em lote (_batch processing_), afinações regionais (como viola caipira) e internacionalização do painel (Português, Inglês e Espanhol) para viabilização comercial.

Qual destas funcionalidades listadas no roadmap futuro agregará mais valor imediato para a validação da ferramenta junto aos primeiros usuários do MuseScore 4?
