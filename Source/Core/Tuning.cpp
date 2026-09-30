#include "Tuning.h"
#include "Text.h"

namespace NoteName
{
    // Notacao com bemois: as tonalidades que aparecem no projeto sao bemois, e
    // sustride ali seria duplo tratamento ("A#3" para o mesmo Bb3).
    juce::String forMidiNote (int midiNote)
    {
        static const char* const pitchClasses[] = { "C",  "Db", "D",  "Eb", "E",  "F",
                                                    "Gb", "G",  "Ab", "A",  "Bb", "B" };

        const int pitchClass = ((midiNote % 12) + 12) % 12;
        const int octave     = (midiNote / 12) - 1;   // MIDI 60 = C4

        return Text::from (pitchClasses[pitchClass]) + juce::String (octave);
    }
}
