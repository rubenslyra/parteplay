#pragma once

#include <JuceHeader.h>

namespace PitchShiftEngine
{
    // Aplica transposição (pitch-shift) offline em todo o buffer, preservando a duração.
    //
    //   pitchRatio:  1.0  =>  inalterado
    //               > 1.0  =>  mais agudo
    //               < 1.0  =>  mais grave
    //
    // O buffer resultante mantém o mesmo número de canais e de amostras da entrada.
    // Retorna nullptr quando não é possível processar.
    std::unique_ptr<juce::AudioBuffer<float>> transpose (const juce::AudioBuffer<float>& input,
                                                         double pitchRatio);
}