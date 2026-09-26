#pragma once

#include <JuceHeader.h>

namespace TempoAnalyser
{
    double estimateBpm (double sampleRate, const juce::AudioBuffer<float>& buffer);

    // Estima a assinatura de tempo ("o chão"): número de tempos por compasso
    // (3 para 3/4, 4 para 4/4), a partir do padrão de acentuação do downbeat.
    // Heurística: 4 retorna por padrão; 3 só quando a evidência é clara.
    int estimateBeatsPerBar (double sampleRate, const juce::AudioBuffer<float>& buffer, double bpm);

    // Estima o desvio global de afinação do áudio em cents relativos a A=440 Hz
    // (positivo = mais agudo). Histograma de desvios de picos espectrais pela nota
    // temperada mais próxima. Retorna 0,0 quando inconclusivo (faixa +-60 cents).
    double estimateTuningCents (double sampleRate, const juce::AudioBuffer<float>& buffer);
}