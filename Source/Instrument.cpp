#include "Instrument.h"
#include "Text.h"

namespace InstrumentTable
{
    Instrument fromIndex (int index) noexcept
    {
        switch (index)
        {
            case 1:  return Instrument::TromboneC;
            case 2:  return Instrument::TrumpetBb;
            case 3:  return Instrument::SaxTenorBb;
            case 4:  return Instrument::SaxAltoEb;
            case 5:  return Instrument::FrenchHornF;
            case 6:  return Instrument::Manual;
            default: return Instrument::PianoC;
        }
    }

    int indexFor (Instrument instrument) noexcept
    {
        switch (instrument)
        {
            case Instrument::PianoC:      return 0;
            case Instrument::TromboneC:   return 1;
            case Instrument::TrumpetBb:   return 2;
            case Instrument::SaxTenorBb:  return 3;
            case Instrument::SaxAltoEb:   return 4;
            case Instrument::FrenchHornF: return 5;
            case Instrument::Manual:
            default:                      return 6;
        }
    }

    juce::String displayName (Instrument instrument)
    {
        switch (instrument)
        {
            case Instrument::TromboneC:   return Text::t ("Trombone (C)");
            case Instrument::TrumpetBb:   return Text::t ("Trompete (Bb)");
            case Instrument::SaxTenorBb:  return Text::t ("Sax Tenor (Bb)");
            case Instrument::SaxAltoEb:   return Text::t ("Sax Alto (Eb)");
            case Instrument::FrenchHornF: return Text::t ("Trompa (F)");
            case Instrument::Manual:      return Text::t ("Ajuste Manual");
            case Instrument::PianoC:
            default:                      return Text::t ("Piano (C)");
        }
    }

    juce::StringArray getDisplayNames()
    {
        juce::StringArray names;

        for (int i = 0; i < numEntries; ++i)
            names.add (displayName (fromIndex (i)));

        return names;
    }

    juce::String symbolFor (Instrument instrument)
    {
        switch (instrument)
        {
            case Instrument::TrumpetBb:
            case Instrument::SaxTenorBb:  return Text::from ("B♭");
            case Instrument::SaxAltoEb:   return Text::from ("E♭");
            case Instrument::FrenchHornF: return Text::from ("F");
            case Instrument::Manual:      return Text::from ("±");
            case Instrument::PianoC:
            case Instrument::TromboneC:
            default:                      return Text::from ("C");
        }
    }

    juce::String keyNameFor (Instrument instrument)
    {
        switch (instrument)
        {
            case Instrument::TrumpetBb:
            case Instrument::SaxTenorBb:  return Text::t ("Si♭");
            case Instrument::SaxAltoEb:   return Text::t ("Mi♭");
            case Instrument::FrenchHornF: return Text::t ("Fá");
            case Instrument::Manual:      return Text::t ("Personalizada");
            case Instrument::PianoC:
            case Instrument::TromboneC:
            default:                      return Text::t ("Dó");
        }
    }

    juce::String signedSemitonesFor (Instrument instrument)
    {
        const int semitones = semitonesFor (instrument);
        return (semitones > 0 ? Text::from ("+") : Text::from ("")) + juce::String (semitones);
    }
}
