#include "PitchShifter.h"

#include <cmath>
#include <vector>
#include <algorithm>

// Phase vocoder de transposição (sem mudança de tempo).
//
// Abordagem clássica de propagação de fase (Laroche/Dolson, estilo smbPitchShift):
//   - segmenta o sinal em janelas de análise com avanço (hop) constante;
//   - por janela, calcula a fase instantânea de cada bin a partir da diferença de
//     fase observada entre janelas;
//   - avança a fase de síntese usando a frequência instantânea escalada por pitchRatio,
//     mantendo a magnitude;
//   - reconstrói por overlap-add normalizado pela soma dos quadrados das janelas,
//     o que garante ganho unitário na reconstrução.
//
// Como a posição das janelas no tempo coincide com a do sinal original, a duração é
// preservada — condição essencial para a sincronia com o transport do hospedeiro.
//
// Engine isolada nesta função: para migrar para outra engine (ex.: Rubber Band),
// basta substituir esta implementacao sem alterar o restante do plugin.

namespace
{
    constexpr int fftOrder = 11;
    constexpr int fftSize  = 1 << fftOrder; // 2048
    constexpr int hopSize  = fftSize / 4;   // 512
    constexpr int numBins  = fftSize / 2 + 1;

    bool isValidRatio (double ratio) noexcept
    {
        return std::isfinite (ratio) && ratio > 0.01 && ratio < 100.0;
    }

    void wrapToPi (double& phase) noexcept
    {
        constexpr double pi    = juce::MathConstants<double>::pi;
        constexpr double twoPi = juce::MathConstants<double>::twoPi;

        while (phase >  pi)   phase -= twoPi;
        while (phase < -pi)   phase += twoPi;
    }

    std::vector<float> makeWindow()
    {
        std::vector<float> window (static_cast<size_t> (fftSize));

        for (int i = 0; i < fftSize; ++i)
            window[static_cast<size_t> (i)] =
                0.5f - 0.5f * std::cos (juce::MathConstants<double>::twoPi * i / (fftSize - 1));

        return window;
    }

    // Inverso da soma dos quadrados das janelas que cobrem cada amostra da saída.
    // Aplicado por amostra dá ganho unitário ao overlap-add (reconstrução COLA).
    // Os quadros de síntese são espaçados por synthHop (=> duração escalada).
    std::vector<float> makeNormalisation (int numOut, double synthHop,
                                          int numIn, int frameCount,
                                          const std::vector<float>& window)
    {
        std::vector<float> sum (static_cast<size_t> (numOut), 0.0f);

        juce::ignoreUnused (numIn);

        for (int frame = 0; frame < frameCount; ++frame)
        {
            const int offset = (int) std::lround ((double) frame * synthHop);
            if (offset >= numOut)
                break;

            const int valid = juce::jmin (fftSize, numOut - offset);

            for (int i = 0; i < valid; ++i)
            {
                const auto w = window[static_cast<size_t> (i)];
                sum[static_cast<size_t> (offset + i)] += w * w;
            }
        }

        for (auto& value : sum)
            value = (value > 1e-6f) ? (1.0f / value) : 0.0f;

        return sum;
    }

    bool containsNonFinite (const float* data, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
            if (! std::isfinite (data[i]))
                return true;

        return false;
    }

    void transformChannel (const float* input, int numIn, float* output, int numOut,
                           double pitchRatio, double synthHop, const juce::dsp::FFT& fft,
                           const std::vector<float>& window,
                           const std::vector<float>& normalisation)
    {
        std::fill (output, output + numOut, 0.0f);

        std::vector<float> spectrum (static_cast<size_t> (fftSize * 2), 0.0f);
        std::vector<double> phaseAcc   (static_cast<size_t> (numBins), 0.0);
        std::vector<double> prevPhase  (static_cast<size_t> (numBins), 0.0);
        std::vector<bool>   started    (static_cast<size_t> (numBins), false);

        const double binOmega = juce::MathConstants<double>::twoPi / fftSize;

        for (int frame = 0;; ++frame)
        {
            const int analysisOffset = frame * hopSize;
            if (analysisOffset >= numIn)
                break;

            const int synthOffset = (int) std::lround ((double) frame * synthHop);

            for (int i = 0; i < fftSize; ++i)
            {
                const int idx = analysisOffset + i;
                const float x = (idx < numIn) ? input[idx] : 0.0f;

                spectrum[static_cast<size_t> (i * 2)] = x * window[static_cast<size_t> (i)];
            }

            fft.performRealOnlyForwardTransform (spectrum.data(), false);

            for (int k = 0; k < numBins; ++k)
            {
                const int bin = k * 2;
                const float re = spectrum[static_cast<size_t> (bin)];
                const float im = spectrum[static_cast<size_t> (bin + 1)];

                const float magnitude = std::hypot (re, im);
                const double phase    = std::atan2 (im, re);
                const double omega    = binOmega * k;

                if (! started[static_cast<size_t> (k)])
                {
                    phaseAcc[static_cast<size_t> (k)]  = phase;
                    prevPhase[static_cast<size_t> (k)] = phase;
                    started[static_cast<size_t> (k)]   = true;
                }
                else
                {
                    double phaseDiff = phase - prevPhase[static_cast<size_t> (k)] - omega * hopSize;
                    wrapToPi (phaseDiff);

                    const double trueFreq = omega + phaseDiff / hopSize;
                    phaseAcc[static_cast<size_t> (k)] += trueFreq * pitchRatio * synthHop;

                    prevPhase[static_cast<size_t> (k)] = phase;
                }

                spectrum[static_cast<size_t> (bin)]     = magnitude * std::cos (phaseAcc[static_cast<size_t> (k)]);
                spectrum[static_cast<size_t> (bin + 1)] = magnitude * std::sin (phaseAcc[static_cast<size_t> (k)]);
            }

            fft.performRealOnlyInverseTransform (spectrum.data());

            const int valid = juce::jmax (0, juce::jmin (fftSize, numOut - synthOffset));

            for (int i = 0; i < valid; ++i)
            {
                const int idx = synthOffset + i;
                output[idx]  += spectrum[static_cast<size_t> (i)]
                              * window[static_cast<size_t> (i)]
                              * normalisation[static_cast<size_t> (idx)];
            }
        }
    }
}

std::unique_ptr<juce::AudioBuffer<float>> PitchShiftEngine::transform (
    const juce::AudioBuffer<float>& input, double pitchRatio, double durationScale)
{
    if (input.getNumChannels() < 1 || input.getNumSamples() < 1
        || ! isValidRatio (pitchRatio) || ! std::isfinite (durationScale) || durationScale <= 0.0)
        return nullptr;

    juce::ScopedNoDenormals noDenormals;

    const int numIn  = input.getNumSamples();
    const int numOut = juce::jmax (1, (int) std::lround ((double) numIn * durationScale));
    const double synthHop = juce::jmax (1.0, (double) hopSize * durationScale);
    const int frameCount  = (numIn + hopSize - 1) / hopSize;

    auto result = std::make_unique<juce::AudioBuffer<float>> (input.getNumChannels(), numOut);
    result->clear();

    const juce::dsp::FFT fft (fftOrder);
    const std::vector<float> window = makeWindow();
    const std::vector<float> normalisation = makeNormalisation (numOut, synthHop, numIn, frameCount, window);

    for (int ch = 0; ch < input.getNumChannels(); ++ch)
    {
        transformChannel (input.getReadPointer (ch), numIn, result->getWritePointer (ch), numOut,
                          pitchRatio, synthHop, fft, window, normalisation);

        if (containsNonFinite (result->getReadPointer (ch), numOut))
            return nullptr;
    }

    return result;
}