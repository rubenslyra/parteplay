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