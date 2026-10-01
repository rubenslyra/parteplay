#pragma once

#include <JuceHeader.h>

// Identificadores únicos dos parâmetros (APVTS) — fonte única para núcleo e interface.
//
// A v1.x não tem mais `instrument` nem `transpose`: a afinação de referência
// contra a afinação real do arquivo é a única decisão de tom que o plugin
// toma por conta própria. Os dois identificadores saíram junto com a tabela de
// instrumentos e com o slider de transposição manual.
namespace Parameter
{
    inline const juce::String referencePitch = "referencePitch";

    // Modo treino
    inline const juce::String trainingSpeed  = "trainingSpeed";
    inline const juce::String loopEnabled    = "loopEnabled";
    inline const juce::String loopStart      = "loopStart";
    inline const juce::String loopEnd        = "loopEnd";

    // Saída local
    inline const juce::String muted          = "muted";
}
