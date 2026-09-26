#pragma once

#include <JuceHeader.h>

// Identificadores únicos dos parâmetros (APVTS) — fonte única para núcleo e interface.
namespace Parameter
{
    inline const juce::String instrument     = "instrument";
    inline const juce::String referencePitch = "referencePitch";
    inline const juce::String transpose      = "transpose";

    // Modo treino
    inline const juce::String trainingSpeed  = "trainingSpeed";
    inline const juce::String loopEnabled    = "loopEnabled";
    inline const juce::String loopStart      = "loopStart";
    inline const juce::String loopEnd        = "loopEnd";
}