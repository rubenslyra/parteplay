#pragma once

#include <JuceHeader.h>

#include <functional>

// Pipeline externa de andamento: o ffmpeg converte o arquivo para WAV mono a
// 44,1 kHz e o soundstretch (biblioteca SoundTouch) mede o BPM por deteccao de
// batidas. Quando os dois executaveis existem, esta e a fonte autoritativa do
// andamento; o TempoAnalyser nativo so entra quando faltam as ferramentas ou
// elas falham. A decisao e do usuario (issue #1).
//
// As funcoes puras — temporaryWavFor, buildFfmpegArgs, buildSoundStretchArgs e
// parseBpm — nao tocam no sistema nem iniciam processo: sao elas que os testes
// de dominio travam. O resto depende dos executaveis reais.
namespace ExternalBpm
{
    struct Tools
    {
        juce::File ffmpeg;
        juce::File soundstretch;

        bool valid() const noexcept;
    };

    // Procura os dois executaveis. Devolve Tools invalido quando nao acha os dois.
    //
    // Ordem: PARTEPLAY_TOOLS_DIR, depois resources/bin ao lado do executavel
    // (Standalone), depois a pasta do usuario (%APPDATA%/PartePlay/bin), por fim
    // o PATH do sistema. Nao copia nem escreve nada — se achar no PATH, usa de la.
    Tools locateTools();

    // Liga/desliga a pipeline. Os testes desligam para o resultado nao depender de
    // haver ffmpeg no PATH da maquina que roda o CI.
    void setEnabled (bool enabled) noexcept;
    bool isEnabled() noexcept;

    // Caminho do WAV temporario usado no meio do processo. Fica no diretorio de
    // temporarios do sistema, derivado do nome do arquivo de origem.
    juce::File temporaryWavFor (const juce::File& source);

    // Linhas de comando, sem o caminho do executavel (quem monta o argv[0] e o
    // estimateBpm). Separadas para poderem ser conferidas em teste.
    juce::StringArray buildFfmpegArgs (const juce::File& source, const juce::File& wav);
    juce::StringArray buildSoundStretchArgs (const juce::File& wav);

    // Extrai o BPM da saida do soundstretch. Aceita qualquer linha que mencione
    // "bpm" e devolve o primeiro numero decimal dela; 0,0 quando nao ha numero.
    // A redacao exata da linha e conferida contra o binario embarcado; por isso o
    // parser nao depende de uma frase literal.
    double parseBpm (const juce::String& output);

    using ProgressFn = std::function<void (float)>;

    // Roda ffmpeg -> WAV -> soundstretch -bpm e devolve o BPM. Devolve 0,0 em
    // qualquer falha (ferramenta ausente, conversao falha, saida ilegivel) e
    // sempre apaga o WAV temporario, inclusive nos caminhos de erro.
    double estimateBpm (const juce::File& source, const Tools& tools,
                        const ProgressFn& onProgress = {});
}
