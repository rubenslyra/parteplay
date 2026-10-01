#pragma once

#include <JuceHeader.h>
#include <cmath>

// Domínio de afinação — o que sobra da v1.x depois da retirada da tabela de
// instrumentos. A afinação de referência e a compensacao da afinação real do
// arquivo continuam(valendo: nao ha mais transposicao por instrumento, mas o
// usuario ainda decide a referencia (A4 432-445 Hz) contra a qual o arquivo e
// medido.
//
// Este modulo e a fonte unica desses valores: nucleo, interface, testes e
// exportacao nao devem duplicar a conta.

namespace Tuning
{
    // ISO 16: frequencia de referencia padrao A = 440 Hz.
    constexpr double defaultReferenceHz = 440.0;
    constexpr double minReferenceHz     = 432.0;
    constexpr double maxReferenceHz     = 445.0;

    // Cents entre duas frequencias de referencia (1200·log2(f2/f1)).
    inline double centsBetween (double fromHz, double toHz) noexcept
    {
        if (fromHz <= 0.0 || toHz <= 0.0)
            return 0.0;

        return 1200.0 * std::log2 (toHz / fromHz);
    }

    // Razao a aplicar ao audio para levar a afinacao real do arquivo ate a
    // referencia escolhida. Frequencia invalida no arquivo e tratada como
    // "sem desvio" (razao 1), para que um arquivo sem pitch detectavel nao
    // desloche a reproducao.
    //
    // Sem transposicao por instrumento, a correcao e sempre pequena (poucos
    // cents, no maximo ~1%). Por isso ela e aplicada por reamostragem, e nao
    // por vocoder: um phase vocoder de FFT 2048 custaria latencia e artefatos
    // para corrigir um desvio que nao chega a 1% — e ainda variaria a duracao
    // do audio, o que e aceitavel nesses valores mas evitavel.
    inline double playbackRatio (double referenceHz, double filePitchHz) noexcept
    {
        if (filePitchHz <= 0.0)
            return 1.0;

        return referenceHz / filePitchHz;
    }
}

namespace NoteName
{
    // Do central na notacao cientifica: MIDI 60 = C4.
    constexpr int middleC = 60;

    // Nome cientifico COM oitava, na notacao internacional (C4, Bb3, Eb3, Gb2).
    // A oitava faz parte do nome de proposito: sem ela, um erro de oitava fica
    // invisivel. Nao e traduzido — e a notacao que consta em partituras,
    // arquivos MIDI e manuais de instrumento.
    juce::String forMidiNote (int midiNote);
}
