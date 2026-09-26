#pragma once

#include <JuceHeader.h>

namespace TempoAnalyser
{
    double estimateBpm (double sampleRate, const juce::AudioBuffer<float>& buffer);
}