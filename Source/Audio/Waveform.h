#pragma once

#include <JuceHeader.h>

#include <memory>
#include <vector>

// Amostragem min/max do áudio para o visualizador de forma de onda.
//
// Imutável depois de construída e publicada por std::shared_ptr: a thread de
// mensagens lê um snapshot sem travar a thread de áudio e sem risco de o dado
// mudar durante a pintura.
struct WaveformPeaks
{
    std::vector<float> min;
    std::vector<float> max;

    bool isEmpty() const noexcept   { return max.empty(); }
    int  numBuckets() const noexcept { return static_cast<int> (max.size()); }
};

using WaveformPtr = std::shared_ptr<const WaveformPeaks>;

namespace Waveform
{
    // Reduz o buffer a `buckets` pares min/max (varredura com passo limitado).
    WaveformPtr build (const juce::AudioBuffer<float>& buffer, int buckets);
}
