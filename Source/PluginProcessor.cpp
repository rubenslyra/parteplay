#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::String instrumentId = "instrument";
    const juce::String referencePitchId = "referencePitch";
    const juce::String transposeId = "transpose";

    const juce::StringArray instrumentChoices
    {
        "Piano / Trombone (C)",
        "Trompete / Sax Tenor (Bb)",
        "Sax Alto (Eb)",
        "Trompa (F)",
        "Ajuste Manual"
    };
}

PlayScoreProcessor::PlayScoreProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "Parameters", createParameterLayout())
{
    instrumentValue = parameters.getRawParameterValue (instrumentId);
    pitchValue      = parameters.getRawParameterValue (referencePitchId);
    transposeValue  = parameters.getRawParameterValue (transposeId);

    player = std::make_unique<FilePlayer>();
}

PlayScoreProcessor::~PlayScoreProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout PlayScoreProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (instrumentId, "Instrumento",
                                                              instrumentChoices, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (referencePitchId, "Afinação (Hz)",
                  juce::NormalisableRange<float> (432.0f, 445.0f, 0.1f), 440.0f));

    layout.add (std::make_unique<juce::AudioParameterInt> (transposeId, "Transposição (semitons)",
                  -12, 12, 0));

    return layout;
}

Instrument PlayScoreProcessor::instrumentFromIndex (int index)
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

int PlayScoreProcessor::semitonesForInstrument (Instrument instrument)
{
    switch (instrument)
    {
        case Instrument::TrumpetBb:
        case Instrument::SaxTenorBb: return -2;
        case Instrument::SaxAltoEb:  return 3;
        case Instrument::FrenchHornF:return -5;
        case Instrument::PianoC:
        case Instrument::Manual:
        default:                     return 0;
    }
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

    if (pitchValue != nullptr)
        referencePitchHz.store (static_cast<double> (*pitchValue), std::memory_order_relaxed);

    if (instrumentValue != nullptr && transposeValue != nullptr)
    {
        const auto selectedInstrument = instrumentFromIndex (static_cast<int> (*instrumentValue));
        const int semitones = (selectedInstrument == Instrument::Manual)
            ? static_cast<int> (*transposeValue)
            : semitonesForInstrument (selectedInstrument);
        activeSemitones.store (semitones, std::memory_order_relaxed);
    }

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

    const juce::ScopedLock sl (audioLock);

    if (player->hasAudio())
    {
        const auto startSample = transportSample.load (std::memory_order_relaxed);
        const bool withinRange = player->fillOutput (buffer, startSample, getSampleRate());
        reachedEnd.store (! withinRange, std::memory_order_relaxed);
    }
}

void PlayScoreProcessor::resetPlaybackState()
{
    transportPlaying.store (false, std::memory_order_relaxed);
    reachedEnd.store (false, std::memory_order_relaxed);
}

void PlayScoreProcessor::loadAudioFile (const juce::File& file)
{
    auto newPlayer = std::make_unique<FilePlayer>();
    newPlayer->loadFromFile (file);

    if (! newPlayer->hasAudio())
        return;

    const juce::ScopedLock sl (audioLock);
    player->clear();
    player = std::move (newPlayer);
    loadedFileName = player->getSourceFileName();
    reachedEnd.store (false, std::memory_order_relaxed);
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

juce::String PlayScoreProcessor::getLoadedFileName() const noexcept
{
    return loadedFileName;
}

double PlayScoreProcessor::getAudioBpm() const noexcept
{
    return player != nullptr ? player->getBpm() : 0.0;
}

double PlayScoreProcessor::getDurationSeconds() const noexcept
{
    return player != nullptr ? player->getDurationSeconds() : 0.0;
}

int PlayScoreProcessor::getMeasureCount() const noexcept
{
    return player != nullptr ? player->getMeasureCount44() : 0;
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