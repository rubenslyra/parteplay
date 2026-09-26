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
    double getDetectedTuningHz() const noexcept     { return detectedTuningHz; }
    double getDurationSeconds() const noexcept      { return durationSeconds; }
    int getMeasureCount() const noexcept            { return measures; }

    void clear();

    bool fillOutput (juce::AudioBuffer<float>& dest,
                     juce::int64 hostStartSample,
                     double hostSampleRate);

    // Acesso à fonte original e ao buffer de reprodução (transposto / esticado).
    // Os shared_ptrs permitem capturar um snapshot barato (sem copiar o áudio) e
    // manter o dado vivo enquanto outra thread ainda o estiver usando.
    std::shared_ptr<const juce::AudioBuffer<float>> getSourceBuffer() const noexcept { return audioBuffer; }

    void setPlaybackBuffer (std::shared_ptr<const juce::AudioBuffer<float>> buffer, double durationScale)
    {
        playbackBuffer     = std::move (buffer);
        appliedDurationScale = durationScale;
    }

    void setLoop (bool enabled, int startMeasure, int endMeasure) noexcept
    {
        loopEnabled     = enabled;
        loopStartMeasure = juce::jmax (1, startMeasure);
        loopEndMeasure  = juce::jmax (loopStartMeasure, endMeasure);
    }

    bool isLoopActive() const noexcept { return loopEnabled; }

private:
    void runTempoAnalysis();

    std::shared_ptr<const juce::AudioBuffer<float>> audioBuffer;
    std::shared_ptr<const juce::AudioBuffer<float>> playbackBuffer;
    double appliedDurationScale = 1.0;
    double fileSampleRate = 0.0;
    juce::String sourceFileName;

    double estimatedBpm = 0.0;
    int beatsPerBar = 4;
    double detectedTuningHz = 440.0;
    double durationSeconds = 0.0;
    int measures = 0;

    bool loopEnabled = false;
    int loopStartMeasure = 1;
    int loopEndMeasure = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilePlayer)
};