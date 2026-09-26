#pragma once

#include <JuceHeader.h>

enum class Instrument
{
    PianoC = 0,
    TrumpetBb = 1,
    SaxTenorBb = 2,
    SaxAltoEb = 3,
    FrenchHornF = 4,
    Manual = 5
};

class FilePlayer
{
public:
    FilePlayer() = default;
    ~FilePlayer() = default;

    void loadFromFile (const juce::File& file);

    bool hasAudio() const noexcept                  { return audioBuffer != nullptr; }
    double getSampleRate() const noexcept           { return fileSampleRate; }
    int getNumSamples() const noexcept              { return audioBuffer != nullptr ? audioBuffer->getNumSamples() : 0; }
    const juce::String& getSourceFileName() const noexcept { return sourceFileName; }

    double getBpm() const noexcept                  { return estimatedBpm; }
    double getDurationSeconds() const noexcept      { return durationSeconds; }
    int getMeasureCount44() const noexcept          { return measures44; }

    void clear();

    bool fillOutput (juce::AudioBuffer<float>& dest,
                     juce::int64 hostStartSample,
                     double hostSampleRate);

private:
    void runTempoAnalysis();

    std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;
    double fileSampleRate = 0.0;
    juce::String sourceFileName;

    double estimatedBpm = 0.0;
    double durationSeconds = 0.0;
    int measures44 = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilePlayer)
};