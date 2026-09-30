#include "TempoAnalyser.h"

#include <cmath>
#include <vector>
#include <limits>

namespace
{
    juce::Array<float> buildOnsetEnvelope (const juce::AudioBuffer<float>& buffer, const int hop)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples <= hop * 2 || numChannels < 1)
            return {};

        const int numFrames = numSamples / hop;
        juce::Array<float> energy;
        energy.ensureStorageAllocated (numFrames + 1);

        for (int frame = 0; frame <= numFrames; ++frame)
        {
            const int start = frame * hop;
            const int end = juce::jmin (start + hop, numSamples);

            double acc = 0.0;
            for (int s = start; s < end; ++s)
            {
                float v = 0.0f;
                for (int c = 0; c < numChannels; ++c)
                    v += buffer.getSample (c, s);
                v /= (float) numChannels;

                acc += (double) v * (double) v;
            }

            energy.add ((float) acc);
        }

        juce::Array<float> onset;
        onset.ensureStorageAllocated (energy.size() - 1);

        for (int i = 1; i < energy.size(); ++i)
            onset.add (juce::jmax (0.0f, energy.getReference (i) - energy.getReference (i - 1)));

        return onset;
    }

    double combScore (const juce::Array<float>& onset, const double framesPerBeat)
    {
        if (framesPerBeat < 1.5)
            return -1.0;

        double total = 0.0;
        int count = 0;

        for (int k = 0; k < 200; ++k)
        {
            const int idx = (int) std::lround (k * framesPerBeat);
            if (idx >= onset.size())
                break;

            total += onset.getReference (idx);
            ++count;
        }

        return count > 0 ? (total / (double) count) : -1.0;
    }
}

double TempoAnalyser::estimateBpm (double sampleRate, const juce::AudioBuffer<float>& buffer)
{
    if (sampleRate <= 0.0)
        return 0.0;

    constexpr int hop = 1024;
    const juce::Array<float> onset = buildOnsetEnvelope (buffer, hop);

    if (onset.size() < 16)
        return 0.0;

    const double framesPerSecond = sampleRate / (double) hop;

    double bestBpm = 0.0;
    double bestScore = -1.0;

    for (double bpm = 60.0; bpm <= 180.0; bpm += 0.25)
    {
        const double framesPerBeat = framesPerSecond * 60.0 / bpm;
        const double score = combScore (onset, framesPerBeat);

        if (score > bestScore)
        {
            bestScore = score;
            bestBpm = bpm;
        }
    }

    if (bestBpm <= 0.0)
        return 0.0;

    const double baseBpm = bestBpm;
    const double baseScore = bestScore;

    auto scoreFor = [&] (double bpm)
    {
        if (bpm < 60.0 || bpm > 180.0)
            return -1.0;
        return combScore (onset, framesPerSecond * 60.0 / bpm);
    };

    const double doubleScore = scoreFor (baseBpm * 2.0);
    if (doubleScore > 0.0 && doubleScore * 0.97 >= baseScore)
    {
        bestBpm = baseBpm * 2.0;
        bestScore = doubleScore;
    }
    else
    {
        const double halfScore = scoreFor (baseBpm / 2.0);
        if (halfScore > 0.0 && halfScore > baseScore * 1.4)
        {
            bestBpm = baseBpm / 2.0;
            bestScore = halfScore;
        }
    }

    const double finalScore = combScore (onset, framesPerSecond * 60.0 / bestBpm);
    if (finalScore > bestScore)
        bestScore = finalScore;

    jassert (bestBpm >= 60.0 && bestBpm <= 180.0);

    return std::round (bestBpm * 2.0) * 0.5;
}

int TempoAnalyser::estimateBeatsPerBar (double sampleRate, const juce::AudioBuffer<float>& buffer, double bpm)
{
    if (sampleRate <= 0.0 || bpm <= 0.0)
        return 4;

    constexpr int hop = 1024;
    const juce::Array<float> onset = buildOnsetEnvelope (buffer, hop);

    if (onset.size() < 32)
        return 4;

    const double framesPerSecond = sampleRate / (double) hop;
    const double framesPerBeat = framesPerSecond * 60.0 / bpm;

    if (framesPerBeat < 2.0)
        return 4;

    // Clareza do downbeat para uma hipótese de compasso (N tempos por compasso):
    // alinha grades de compasso em várias fases e mede o destaque do tempo forte
    // (primeira batida) sobre as demais, normalizado pela energia média.
    auto clarityFor = [&onset] (int beatsPerBar, double beatFrames) -> double
    {
        const double barFrames = beatFrames * beatsPerBar;
        const int numBars = (int) std::floor (onset.size() / barFrames);

        if (numBars < 4)
            return -1.0;

        const int phaseSteps = juce::jlimit (1, 64, (int) std::ceil (barFrames / 4.0));

        double bestClarity = -1.0;

        for (double phase = 0.0; phase < barFrames; phase += phaseSteps)
        {
            std::vector<double> strengths (static_cast<size_t> (beatsPerBar), 0.0);

            for (int bar = 0; bar < numBars; ++bar)
            {
                const double base = phase + (double) bar * barFrames;

                for (int k = 0; k < beatsPerBar; ++k)
                {
                    const int idx = (int) std::lround (base + (double) k * beatFrames);
                    if (idx >= 0 && idx < onset.size())
                        strengths[static_cast<size_t> (k)] += onset.getReference (idx);
                }
            }

            double mean = 0.0;
            double maxValue = 0.0;
            double minValue = std::numeric_limits<double>::max();

            for (double value : strengths)
            {
                mean += value;
                maxValue = juce::jmax (maxValue, value);
                minValue = juce::jmin (minValue, value);
            }

            mean /= (double) beatsPerBar;

            if (mean <= 1e-9)
                continue;

            double otherMax = 0.0;
            for (size_t k = 1; k < strengths.size(); ++k)
                otherMax = juce::jmax (otherMax, strengths[k]);

            const double downbeatEdge = (strengths.front() - otherMax) / mean;
            const double spread       = (maxValue - minValue) / mean;
            const double clarity      = downbeatEdge + 0.5 * spread;

            bestClarity = juce::jmax (bestClarity, clarity);
        }

        return bestClarity;
    };

    const double clarity3 = clarityFor (3, framesPerBeat);
    const double clarity4 = clarityFor (4, framesPerBeat);

    // Padrão quaternário; adota o ternário somente com evidência clara.
    if (clarity3 > 0.0 && clarity3 > clarity4 * 1.25 + 0.1)
        return 3;

    return 4;
}

double TempoAnalyser::estimateTuningCents (double sampleRate, const juce::AudioBuffer<float>& buffer)
{
    constexpr int fftOrder  = 12;
    constexpr int fftSize   = 1 << fftOrder;   // 4096
    constexpr int hopSize   = fftSize / 2;     // 2048
    constexpr int halfRange = 60;              // faixa +-60 cents
    constexpr int histSize  = halfRange * 2 + 1;

    if (sampleRate <= 0.0 || buffer.getNumChannels() < 1 || buffer.getNumSamples() < fftSize)
        return 0.0;

    float globalPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        globalPeak = juce::jmax (globalPeak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));

    if (globalPeak <= 1e-6f)
        return 0.0;

    juce::dsp::FFT fft (fftOrder);

    // Janela de Hann. twoPi e double, entao a fase e double e so o resultado
    // final e guardado em float. A conversao e intencional (o destino e
    // float por escolha) e fica explicita: sem o static_cast o MSVC emite
    // C4244 nesta linha, e e a unica atribuicao do projeto em que um double
    // chega num float sem cast.
    std::vector<float> window (static_cast<size_t> (fftSize));
    for (int i = 0; i < fftSize; ++i)
    {
        const double phase = juce::MathConstants<double>::twoPi * i / (fftSize - 1);
        window[static_cast<size_t> (i)] = static_cast<float> (0.5 - 0.5 * std::cos (phase));
    }

    std::vector<float> spectrum (static_cast<size_t> (fftSize * 2), 0.0f);
    std::vector<int> histogram (static_cast<size_t> (histSize), 0);

    const double hzPerBin = sampleRate / fftSize;
    const int minBin = juce::jmax (1, (int) std::lround (80.0 / hzPerBin));
    const int maxBin = juce::jmin (fftSize / 2 - 1, (int) std::lround (2500.0 / hzPerBin));

    const auto magnitudeAt = [&spectrum] (int bin)
    {
        return std::hypot (spectrum[static_cast<size_t> (bin * 2)],
                           spectrum[static_cast<size_t> (bin * 2 + 1)]);
    };

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* data = buffer.getReadPointer (ch);
        const int numSamples = buffer.getNumSamples();

        for (int offset = 0; offset <= numSamples - fftSize; offset += hopSize)
        {
            std::fill (spectrum.begin(), spectrum.end(), 0.0f);

            double energy = 0.0;
            for (int i = 0; i < fftSize; ++i)
            {
                const float x = data[offset + i];
                spectrum[static_cast<size_t> (i * 2)] = x * window[static_cast<size_t> (i)];
                energy += (double) x * (double) x;
            }

            const double rms = std::sqrt (energy / fftSize);
            if (rms < (double) globalPeak * 0.02)
                continue;

            fft.performRealOnlyForwardTransform (spectrum.data(), false);

            float frameMax = 0.0f;
            for (int k = minBin; k <= maxBin; ++k)
                frameMax = juce::jmax (frameMax, magnitudeAt (k));

            if (frameMax <= 1e-6f)
                continue;

            for (int k = minBin; k <= maxBin; ++k)
            {
                const float mag = magnitudeAt (k);

                if (mag < frameMax * 0.05f)
                    continue;

                const bool leftOk  = k - 1 < minBin || magnitudeAt (k - 1) <= mag;
                const bool rightOk = k + 1 > maxBin || magnitudeAt (k + 1) <= mag;
                if (! leftOk || ! rightOk)
                    continue;

                const double freq   = (double) k * hzPerBin;
                const double midi   = 69.0 + 12.0 * std::log2 (freq / 440.0);
                const double nearest = 440.0 * std::pow (2.0, (std::round (midi) - 69.0) / 12.0);
                const double cents  = 1200.0 * std::log2 (freq / nearest);

                const int idx = (int) std::lround (cents) + halfRange;
                if (idx >= 0 && idx < histSize)
                    ++histogram[static_cast<size_t> (idx)];
            }
        }
    }

    std::vector<int> smooth (static_cast<size_t> (histSize), 0);
    int total = 0;
    for (int i = 0; i < histSize; ++i)
    {
        total += histogram[static_cast<size_t> (i)];
        smooth[static_cast<size_t> (i)] = (histogram[static_cast<size_t> (i - 1 < 0 ? 0 : i - 1)]
                                        + histogram[static_cast<size_t> (i)]
                                        + histogram[static_cast<size_t> (i + 1 >= histSize ? i : i + 1)]) / 3;
    }

    if (total < 40)
        return 0.0;

    int best = halfRange;
    int bestCount = 0;
    for (int i = 0; i < histSize; ++i)
    {
        if (smooth[static_cast<size_t> (i)] > bestCount)
        {
            bestCount = smooth[static_cast<size_t> (i)];
            best = i;
        }
    }

    if (bestCount < total / 8)
        return 0.0;

    return (double) (best - halfRange);
}