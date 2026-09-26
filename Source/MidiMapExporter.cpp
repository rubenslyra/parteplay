#include "MidiMapExporter.h"

#include <cmath>

namespace
{
    constexpr int ticksPerQuarter = 480;
    constexpr int denominatorExponent = 2;            // 2^2 = 4 (denominador semínima)
    constexpr int clocksPerClick = 24;
    constexpr int thirtySecondsPerQuarter = 8;

    const juce::String trackName = "PartePlay - Mapa de Tempo";

    void writeVarLen (juce::MemoryOutputStream& out, unsigned int value)
    {
        unsigned char buffer[4];
        int count = 0;

        do
        {
            buffer[count++] = static_cast<unsigned char> (value & 0x7F);
            value >>= 7;
        }
        while (value > 0);

        for (int i = count - 1; i >= 0; --i)
            out.writeByte (buffer[i] | static_cast<unsigned char> (i > 0 ? 0x80 : 0x00));
    }

    void writeDelta (juce::MemoryOutputStream& out)
    {
        writeVarLen (out, 0);
    }

    void writeMetaTrackName (juce::MemoryOutputStream& out)
    {
        writeDelta (out);
        out.writeByte (0xFF);
        out.writeByte (0x03);
        out.writeByte (static_cast<unsigned char> (trackName.getNumBytesAsUTF8()));
        out.write (trackName.toRawUTF8(), trackName.getNumBytesAsUTF8());
    }

    void writeMetaTempo (juce::MemoryOutputStream& out, double bpm)
    {
        const auto microsecondsPerQuarter = static_cast<int> (std::lround (60000000.0 / bpm));

        writeDelta (out);
        out.writeByte (0xFF);
        out.writeByte (0x51);
        out.writeByte (0x03);
        out.writeByte (static_cast<unsigned char> ((microsecondsPerQuarter >> 16) & 0xFF));
        out.writeByte (static_cast<unsigned char> ((microsecondsPerQuarter >> 8)  & 0xFF));
        out.writeByte (static_cast<unsigned char> (microsecondsPerQuarter & 0xFF));
    }

    void writeMetaTimeSignature (juce::MemoryOutputStream& out, int beatsPerBar)
    {
        writeDelta (out);
        out.writeByte (0xFF);
        out.writeByte (0x58);
        out.writeByte (0x04);
        out.writeByte (static_cast<unsigned char> (beatsPerBar)); // nn
        out.writeByte (static_cast<unsigned char> (denominatorExponent)); // dd
        out.writeByte (static_cast<unsigned char> (clocksPerClick));      // cc
        out.writeByte (static_cast<unsigned char> (thirtySecondsPerQuarter)); // bb
    }

    void writeMetaEndOfTrack (juce::MemoryOutputStream& out)
    {
        writeDelta (out);
        out.writeByte (0xFF);
        out.writeByte (0x2F);
        out.writeByte (0x00);
    }
}

bool MidiMapExporter::writeTempoMap (const juce::File& file, double bpm, int beatsPerBar)
{
    if (file == juce::File()
        || ! std::isfinite (bpm) || bpm <= 0.0
        || beatsPerBar < 1)
        return false;

    // Trilha (conteúdo do MTrk).
    juce::MemoryOutputStream track;
    writeMetaTrackName (track);
    writeMetaTempo (track, bpm);
    writeMetaTimeSignature (track, beatsPerBar);
    writeMetaEndOfTrack (track);

    // Cabeçalho + trilha.
    juce::MemoryOutputStream content;
    content.writeIntBigEndian (0x4D546864); // "MThd"
    content.writeIntBigEndian (6);
    content.writeShortBigEndian ((short) 0);              // formato 0
    content.writeShortBigEndian ((short) 1);              // 1 trilha
    content.writeShortBigEndian ((short) ticksPerQuarter); // PPQ

    content.writeIntBigEndian (0x4D54726B); // "MTrk"
    content.writeIntBigEndian (static_cast<int> (track.getDataSize()));
    content.write (track.getData(), track.getDataSize());

    juce::FileOutputStream stream (file);
    if (! stream.openedOk())
        return false;

    if (! stream.write (content.getData(), content.getDataSize()))
        return false;

    stream.flush();
    return true;
}