#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Instrument.h"
#include "ParameterIds.h"
#include "PitchShifter.h"
#include "MidiMapExporter.h"
#include "Text.h"

#include <cmath>

PlayScoreProcessor::PlayScoreProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "Parameters", createParameterLayout())
{
    instrumentValue    = parameters.getRawParameterValue (Parameter::instrument);
    pitchValue         = parameters.getRawParameterValue (Parameter::referencePitch);
    transposeValue     = parameters.getRawParameterValue (Parameter::transpose);
    trainingSpeedValue = parameters.getRawParameterValue (Parameter::trainingSpeed);
    loopEnabledValue   = parameters.getRawParameterValue (Parameter::loopEnabled);
    loopStartValue     = parameters.getRawParameterValue (Parameter::loopStart);
    loopEndValue       = parameters.getRawParameterValue (Parameter::loopEnd);
    mutedValue         = parameters.getRawParameterValue (Parameter::muted);

    player = std::make_unique<FilePlayer>();

    startTimerHz (4);
}

PlayScoreProcessor::~PlayScoreProcessor()
{
    stopTimer();
}

juce::AudioProcessorValueTreeState::ParameterLayout PlayScoreProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (Parameter::instrument, Text::t ("Instrumento"),
                                                              InstrumentTable::getDisplayNames(), 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (Parameter::referencePitch, Text::t ("Afinação (Hz)"),
                  juce::NormalisableRange<float> ((float) Tuning::minReferenceHz,
                                                  (float) Tuning::maxReferenceHz, 0.1f),
                  (float) Tuning::defaultReferenceHz));

    layout.add (std::make_unique<juce::AudioParameterInt> (Parameter::transpose, Text::t ("Transposição (semitons)"),
                  -12, 12, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (Parameter::trainingSpeed, Text::t ("Velocidade (treino)"),
                  juce::NormalisableRange<float> (0.5f, 1.5f, 0.01f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (Parameter::loopEnabled, Text::t ("Loop"),
                  false));

    layout.add (std::make_unique<juce::AudioParameterInt> (Parameter::loopStart, Text::t ("Loop início (compasso)"),
                  1, 10000, 1));
    layout.add (std::make_unique<juce::AudioParameterInt> (Parameter::loopEnd, Text::t ("Loop fim (compasso)"),
                  1, 10000, 1));

    layout.add (std::make_unique<juce::AudioParameterBool> (Parameter::muted, Text::t ("Silenciar saída"),
                  false));

    return layout;
}

int PlayScoreProcessor::getCurrentSemitones() const
{
    if (instrumentValue == nullptr || transposeValue == nullptr)
        return 0;

    const auto selected = InstrumentTable::fromIndex (static_cast<int> (*instrumentValue));
    return (selected == Instrument::Manual)
        ? static_cast<int> (*transposeValue)
        : InstrumentTable::semitonesFor (selected);
}

double PlayScoreProcessor::computeCurrentPitchRatio (double baseFilePitchHz) const
{
    if (pitchValue == nullptr)
        return 1.0;

    return Tuning::playbackRatio (getCurrentSemitones(),
                                  static_cast<double> (*pitchValue),
                                  baseFilePitchHz);
}

void PlayScoreProcessor::timerCallback()
{
    const double speed = (trainingSpeedValue != nullptr) ? (double) *trainingSpeedValue : 1.0;
    const double durationScale = (speed > 0.0) ? (1.0 / speed) : 1.0;

    const bool loopOn   = (loopEnabledValue != nullptr) && (*loopEnabledValue > 0.5f);
    const int  loopFrom = (loopStartValue != nullptr) ? (int) *loopStartValue : 1;
    const int  loopTo   = (loopEndValue != nullptr)   ? (int) *loopEndValue   : 1;

    std::shared_ptr<const juce::AudioBuffer<float>> source;
    double basePitch = Tuning::defaultReferenceHz;

    {
        // Loop e leitura da afinação detectada sob o mesmo lock que o áudio usa.
        const juce::ScopedLock sl (stateLock);

        if (player == nullptr || ! player->hasAudio())
            return;

        player->setLoop (loopOn, loopFrom, loopTo);
        basePitch = player->getDetectedTuningHz();
        source    = player->getSourceBuffer();
    }

    const double ratio = computeCurrentPitchRatio (basePitch);

    if (committedGeneration == fileGeneration
        && std::abs (ratio - committedPitchRatio) < 1e-4
        && std::abs (durationScale - committedDurationScale) < 1e-4)
        return;

    if (source == nullptr)
        return;

    if (std::abs (ratio - 1.0) < 1e-4 && std::abs (durationScale - 1.0) < 1e-4)
    {
        // Identidade: reprodução direta do original.
        const juce::ScopedLock sl (stateLock);
        player->setPlaybackBuffer (nullptr, 1.0);
    }
    else
    {
        // Transformação cara fora do lock; publicação atômica do resultado.
        auto transformed = PitchShiftEngine::transform (*source, ratio, durationScale);
        if (transformed == nullptr)
            return;

        const juce::ScopedLock sl (stateLock);
        player->setPlaybackBuffer (std::move (transformed), durationScale);
    }

    committedGeneration = fileGeneration;
    committedPitchRatio = ratio;
    committedDurationScale = durationScale;
}

const juce::String PlayScoreProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PlayScoreProcessor::acceptsMidi() const     { return false; }
bool PlayScoreProcessor::producesMidi() const    { return false; }
bool PlayScoreProcessor::isMidiEffect() const    { return false; }
double PlayScoreProcessor::getTailLengthSeconds() const { return 0.0; }

int PlayScoreProcessor::getNumPrograms()                     { return 1; }
int PlayScoreProcessor::getCurrentProgram()                  { return 0; }
void PlayScoreProcessor::setCurrentProgram (int)             {}
const juce::String PlayScoreProcessor::getProgramName (int)  { return "Default"; }
void PlayScoreProcessor::changeProgramName (int, const juce::String&) {}

void PlayScoreProcessor::prepareToPlay (double, int) {}

void PlayScoreProcessor::releaseResources() {}

bool PlayScoreProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::stereo()
        || output == juce::AudioChannelSet::mono();
}

void PlayScoreProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    activeSemitones.store (getCurrentSemitones(), std::memory_order_relaxed);

    auto playHead = getPlayHead();
    if (playHead == nullptr)
    {
        resetPlaybackState();
        return;
    }

    auto position = playHead->getPosition();
    if (! position.hasValue())
    {
        resetPlaybackState();
        return;
    }

    const auto& info = *position;
    const bool playing = info.getIsPlaying();
    transportPlaying.store (playing, std::memory_order_relaxed);

    if (const auto time = info.getTimeInSamples())
    {
        transportSample.store (static_cast<int64_t> (*time), std::memory_order_relaxed);
        transportSampleRate.store (getSampleRate(), std::memory_order_relaxed);
    }

    if (! playing || getSampleRate() <= 0.0)
    {
        reachedEnd.store (false, std::memory_order_relaxed);
        return;
    }

    if (player == nullptr)
        return;

    const juce::ScopedLock sl (stateLock);

    if (player->hasAudio())
    {
        const auto startSample = transportSample.load (std::memory_order_relaxed);
        const bool withinRange = player->fillOutput (buffer, startSample, getSampleRate());
        reachedEnd.store (! withinRange, std::memory_order_relaxed);
    }

    if (isOutputMuted())
        buffer.clear();
}

void PlayScoreProcessor::resetPlaybackState()
{
    transportPlaying.store (false, std::memory_order_relaxed);
    reachedEnd.store (false, std::memory_order_relaxed);
}

void PlayScoreProcessor::loadAudioFile (const juce::File& file)
{
    // Decodificação e análise fora do lock: só a troca do ponteiro é crítica.
    auto newPlayer = std::make_unique<FilePlayer>();
    newPlayer->loadFromFile (file);

    if (! newPlayer->hasAudio())
        return;

    const juce::ScopedLock sl (stateLock);
    player = std::move (newPlayer);
    loadedFileName = player->getSourceFileName();
    reachedEnd.store (false, std::memory_order_relaxed);
    ++fileGeneration;
}

bool PlayScoreProcessor::exportTempoMap (const juce::File& file)
{
    double bpm = 0.0;
    int beatsPerBar = 4;

    {
        const juce::ScopedLock sl (stateLock);

        if (player == nullptr || ! player->hasAudio())
            return false;

        bpm         = player->getBpm();
        beatsPerBar = player->getBeatsPerBar();
    }

    if (bpm <= 0.0)
        return false;

    return MidiMapExporter::writeTempoMap (file, bpm, beatsPerBar);
}

juce::AudioProcessorEditor* PlayScoreProcessor::createEditor()
{
    return new PlayScoreEditor (*this);
}

bool PlayScoreProcessor::hasEditor() const { return true; }

bool PlayScoreProcessor::isTransportPlaying() const noexcept
{
    return transportPlaying.load (std::memory_order_relaxed);
}

juce::int64 PlayScoreProcessor::getTransportSample() const noexcept
{
    return transportSample.load (std::memory_order_relaxed);
}

double PlayScoreProcessor::getTransportSampleRate() const noexcept
{
    return transportSampleRate.load (std::memory_order_relaxed);
}

int PlayScoreProcessor::getActiveSemitones() const noexcept
{
    return activeSemitones.load (std::memory_order_relaxed);
}

bool PlayScoreProcessor::hasReachedEndOfFile() const noexcept
{
    return reachedEnd.load (std::memory_order_relaxed);
}

juce::String PlayScoreProcessor::getLoadedFileName() const
{
    const juce::ScopedLock sl (stateLock);
    return loadedFileName;
}

juce::int64 PlayScoreProcessor::getFileSizeBytes() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getSourceFileSize() : 0;
}

double PlayScoreProcessor::getAudioBpm() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getBpm() : 0.0;
}

int PlayScoreProcessor::getBeatsPerBar() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getBeatsPerBar() : 4;
}

double PlayScoreProcessor::getDetectedTuningHz() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getDetectedTuningHz() : Tuning::defaultReferenceHz;
}

double PlayScoreProcessor::getDetectedTuningCents() const
{
    const double hz = getDetectedTuningHz();
    return (hz > 0.0) ? 1200.0 * std::log2 (hz / Tuning::defaultReferenceHz) : 0.0;
}

double PlayScoreProcessor::getDurationSeconds() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getDurationSeconds() : 0.0;
}

double PlayScoreProcessor::getPlaybackDurationSeconds() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getPlaybackDurationSeconds() : 0.0;
}

int PlayScoreProcessor::getMeasureCount() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getMeasureCount() : 0;
}

WaveformPtr PlayScoreProcessor::getWaveformPeaks() const
{
    const juce::ScopedLock sl (stateLock);
    return player != nullptr ? player->getPeaks() : nullptr;
}

bool PlayScoreProcessor::getLoopFractions (double& startFraction, double& endFraction) const
{
    const juce::ScopedLock sl (stateLock);

    if (player == nullptr)
    {
        startFraction = endFraction = 0.0;
        return false;
    }

    return player->getLoopFractions (startFraction, endFraction);
}

double PlayScoreProcessor::getTrainingSpeed() const noexcept
{
    return trainingSpeedValue != nullptr ? (double) *trainingSpeedValue : 1.0;
}

bool PlayScoreProcessor::isLoopEnabled() const noexcept
{
    return loopEnabledValue != nullptr && *loopEnabledValue > 0.5f;
}

int PlayScoreProcessor::getLoopStartMeasure() const noexcept
{
    return loopStartValue != nullptr ? (int) *loopStartValue : 1;
}

int PlayScoreProcessor::getLoopEndMeasure() const noexcept
{
    return loopEndValue != nullptr ? (int) *loopEndValue : 1;
}

bool PlayScoreProcessor::isOutputMuted() const noexcept
{
    return mutedValue != nullptr && *mutedValue > 0.5f;
}

Instrument PlayScoreProcessor::getCurrentInstrument() const noexcept
{
    return InstrumentTable::fromIndex (getCurrentInstrumentIndex());
}

int PlayScoreProcessor::getCurrentInstrumentIndex() const noexcept
{
    return instrumentValue != nullptr ? static_cast<int> (*instrumentValue) : 0;
}

int PlayScoreProcessor::getManualSemitones() const noexcept
{
    return transposeValue != nullptr ? static_cast<int> (*transposeValue) : 0;
}

double PlayScoreProcessor::getReferencePitchHz() const noexcept
{
    return pitchValue != nullptr ? static_cast<double> (*pitchValue) : Tuning::defaultReferenceHz;
}

void PlayScoreProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PlayScoreProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr)
        return;

    const auto newState = juce::ValueTree::fromXml (*xml);
    if (newState.isValid())
        parameters.replaceState (newState);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlayScoreProcessor();
}
