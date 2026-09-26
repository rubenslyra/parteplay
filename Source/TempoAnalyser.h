#pragma once

#include <JuceHeader.h>

namespace TempoAnalyser
{
    double estimateBpm (double sampleRate, const juce::AudioBuffer<float>& buffer);

    // Estima a assinatura de tempo ("o chão"): número de tempos por compasso
    // (3 para 3/4, 4 para 4/4), a partir do padrão de acentuação do downbeat.
    // Heurística: 4 retorna por padrão; 3 só quando a evidência é clara.
    int estimateBeatsPerBar (double sampleRate, const juce::AudioBuffer<float>& buffer, double bpm);
}