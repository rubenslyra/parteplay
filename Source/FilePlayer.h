#pragma once

#include <JuceHeader.h>

#include "Instrument.h"

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
    int getBeatsPerBar() const noexcept             { return beatsPerBar; }
    double getDurationSeconds() const noexcept      { return durationSeconds; }
    int getMeasureCount() const noexcept            { return measures; }

    void clear();

    bool fillOutput (juce::AudioBuffer<float>& dest,
                     juce::int64 hostStartSample,
                     double hostSampleRate);

    // Acesso à fonte original e ao buffer transposto. Os shared_ptrs permitem
    // capturar um snapshot barato (sem copiar o áudio) e manter o dado vivo
    // enquanto outra thread ainda o estiver usando.
    std::shared_ptr<const juce::AudioBuffer<float>> getSourceBuffer() const noexcept { return audioBuffer; }
    double getAppliedPitchRatio() const noexcept { return appliedPitchRatio; }

    void setTransposedBuffer (std::shared_ptr<const juce::AudioBuffer<float>> buffer, double pitchRatio)
    {
        transposedBuffer  = std::move (buffer);
        appliedPitchRatio = pitchRatio;
    }

private:
    void runTempoAnalysis();

    std::shared_ptr<const juce::AudioBuffer<float>> audioBuffer;
    std::shared_ptr<const juce::AudioBuffer<float>> transposedBuffer;
    double appliedPitchRatio = 1.0;
    double fileSampleRate = 0.0;
    juce::String sourceFileName;

    double estimatedBpm = 0.0;
    int beatsPerBar = 4;
    double durationSeconds = 0.0;
    int measures = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilePlayer)
};