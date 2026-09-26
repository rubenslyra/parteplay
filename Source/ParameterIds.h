#pragma once

#include <JuceHeader.h>

// Identificadores únicos dos parâmetros (APVTS) — fonte única para núcleo e interface.
namespace Parameter
{
    inline const juce::String instrument     = "instrument";
    inline const juce::String referencePitch = "referencePitch";
    inline const juce::String transpose      = "transpose";
}