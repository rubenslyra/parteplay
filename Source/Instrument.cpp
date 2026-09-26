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
            Text::from ("Piano / Trombone (C)"),
            Text::from ("Trompete / Sax Tenor (Bb)"),
            Text::from ("Sax Alto (Eb)"),
            Text::from ("Trompa (F)"),
            Text::from ("Ajuste Manual")
        };

        return names;
    }
}