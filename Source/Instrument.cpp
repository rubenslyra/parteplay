#include "Instrument.h"
#include "Text.h"

namespace InstrumentTable
{
    Instrument fromIndex (int index) noexcept
    {
        switch (index)
        {
            case 1:  return Instrument::TrumpetBb;
            case 2:  return Instrument::SaxAltoEb;
            case 3:  return Instrument::FrenchHornF;
            case 4:  return Instrument::Manual;
            default: return Instrument::PianoC;
        }
    }

    const juce::StringArray& getDisplayNames() noexcept
    {
        static const juce::StringArray names
        {
            Text::t ("Piano / Trombone (C)"),
            Text::t ("Trompete / Sax Tenor (Bb)"),
            Text::t ("Sax Alto (Eb)"),
            Text::t ("Trompa (F)"),
            Text::t ("Ajuste Manual")
        };

        return names;
    }
}