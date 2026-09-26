#include "TempoAnalyser.h"

#include <cmath>

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