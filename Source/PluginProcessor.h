#pragma once

#include <JuceHeader.h>

#include "FilePlayer.h"
#include "Instrument.h"
#include "Waveform.h"

class PlayScoreProcessor : public juce::AudioProcessor,
                           private juce::Timer
{
public:
    PlayScoreProcessor();
    ~PlayScoreProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadAudioFile (const juce::File& file);
    bool exportTempoMap (const juce::File& file);

    // Estado para a interface. Tudo que toca o player passa por stateLock: a
    // interface lê da thread de mensagens enquanto o áudio pode estar em fillOutput.
    bool isTransportPlaying() const noexcept;
    juce::int64 getTransportSample() const noexcept;
    double getTransportSampleRate() const noexcept;
    int getActiveSemitones() const noexcept;
    bool hasReachedEndOfFile() const noexcept;

    juce::String getLoadedFileName() const;
    juce::int64 getFileSizeBytes() const;
    double getAudioBpm() const;
    int getBeatsPerBar() const;
    double getDetectedTuningHz() const;
    double getDetectedTuningCents() const;
    double getDurationSeconds() const;
    double getPlaybackDurationSeconds() const;
    int getMeasureCount() const;
    WaveformPtr getWaveformPeaks() const;
    bool getLoopFractions (double& startFraction, double& endFraction) const;

    double getTrainingSpeed() const noexcept;
    bool isLoopEnabled() const noexcept;
    int getLoopStartMeasure() const noexcept;
    int getLoopEndMeasure() const noexcept;
    bool isOutputMuted() const noexcept;

    Instrument getCurrentInstrument() const noexcept;
    int getCurrentInstrumentIndex() const noexcept;
    int getManualSemitones() const noexcept;
    double getReferencePitchHz() const noexcept;

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    int getCurrentSemitones() const;
    void resetPlaybackState();
    void timerCallback() override;
    double computeCurrentPitchRatio (double baseFilePitchHz) const;

    std::unique_ptr<FilePlayer> player;
    mutable juce::CriticalSection stateLock;

    // Controle da transposição (message thread via Timer).
    double committedPitchRatio = 1.0;
    double committedDurationScale = 1.0;
    long long fileGeneration  = 0;   // incrementado a cada carregamento
    long long committedGeneration = -1;

    std::atomic<float>* instrumentValue = nullptr;
    std::atomic<float>* pitchValue = nullptr;
    std::atomic<float>* transposeValue = nullptr;
    std::atomic<float>* trainingSpeedValue = nullptr;
    std::atomic<float>* loopEnabledValue = nullptr;
    std::atomic<float>* loopStartValue = nullptr;
    std::atomic<float>* loopEndValue = nullptr;
    std::atomic<float>* mutedValue = nullptr;

    std::atomic<bool> transportPlaying { false };
    std::atomic<int64_t> transportSample { 0 };
    std::atomic<double> transportSampleRate { 0.0 };
    std::atomic<int> activeSemitones { 0 };
    std::atomic<bool> reachedEnd { false };

    juce::String loadedFileName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayScoreProcessor)
};
