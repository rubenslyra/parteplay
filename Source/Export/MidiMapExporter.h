#pragma once

#include <JuceHeader.h>

namespace MidiMapExporter
{
    // Gera um arquivo MIDI (formato 0, MIDI 1.0 / RP-001) contendo apenas o mapa
    // de tempo do áudio: andamento (BPM), assinatura de tempo e nome de trilha.
    // O arquivo não tem notas — serve para o hospedeiro herdar a grade do áudio.
    //
    //   bpm          andamento em batidas por minuto (> 0).
    //   beatsPerBar  numerador da assinatura (4 para 4/4; 3 para 3/4).
    bool writeTempoMap (const juce::File& file, double bpm, int beatsPerBar);
}