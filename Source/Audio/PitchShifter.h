#pragma once

#include <JuceHeader.h>

namespace PitchShiftEngine
{
    // Transforma (pitch-shift + time-stretch) offline em todo o buffer, de forma
    // independente e em uma única passada:
    //
    //   pitchRatio:    1.0  =>  tom inalterado        (>1 agudo, <1 grave)
    //   durationScale: 1.0  =>  duração inalterada    (>1 mais longo, <1 mais curto,
    //                ou seja, inverso da velocidade — 1/speed).
    //
    // O buffer resultante tem o mesmo número de canais e aproximadamente
    // `input.getNumSamples() * durationScale` amostras (trimado para o arredondamento),
    // o que mantém a sincronia por mapeamento linear com o transport.
    std::unique_ptr<juce::AudioBuffer<float>> transform (const juce::AudioBuffer<float>& input,
                                                         double pitchRatio,
                                                         double durationScale);
}