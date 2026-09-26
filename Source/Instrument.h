#pragma once

#include <JuceHeader.h>

// Domínio musical compartilhado entre núcleo, interface e exportação.

enum class Instrument
{
    PianoC = 0,
    TrumpetBb,
    SaxTenorBb,
    SaxAltoEb,
    FrenchHornF,
    Manual
};

namespace Tuning
{
    // ISO 16: frequência de referência padrão A = 440 Hz.
    constexpr double defaultReferenceHz = 440.0;
    constexpr double minReferenceHz     = 432.0;
    constexpr double maxReferenceHz     = 445.0;
}

namespace InstrumentTable
{
    // Compensação em semitons (áudio de concerto -> tom escrito do instrumento).
    constexpr int semitonesFor (Instrument instrument) noexcept
    {
        switch (instrument)
        {
            case Instrument::TrumpetBb:
            case Instrument::SaxTenorBb: return -2;
            case Instrument::SaxAltoEb:  return 3;
            case Instrument::FrenchHornF: return -5;
            case Instrument::PianoC:
            case Instrument::Manual:
            default:                     return 0;
        }
    }

    Instrument fromIndex (int index) noexcept;
    const juce::StringArray& getDisplayNames() noexcept;
}