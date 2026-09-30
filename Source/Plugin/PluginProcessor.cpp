#include "PluginProcessor.h"
#include "PluginEditor.h"
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
    pitchValue         = parameters.getRawParameterValue (Parameter::referencePitch);
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

    layout.add (std::make_unique<juce::AudioParameterFloat> (Parameter::referencePitch, Text::t ("Afinação (Hz)"),
                  juce::NormalisableRange<float> ((float) Tuning::minReferenceHz,
                                                  (float) Tuning::maxReferenceHz, 0.1f),
                  (float) Tuning::defaultReferenceHz));

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

double PlayScoreProcessor::computeCurrentPitchRatio (double baseFilePitchHz) const
{
    if (pitchValue == nullptr)
        return 1.0;

    // Só a afinação de referência: a tabela de instrumentos e a transposição
    // manual saíram da v1.x, então este ratio é a compensação entre a
    // referência escolhida e a afinação real medida no arquivo.
    return Tuning::playbackRatio (static_cast<double> (*pitchValue), baseFilePitchHz);
}

void PlayScoreProcessor::timerCallback()
{
    const double speed = (trainingSpeedValue != nullptr) ? (double) *trainingSpeedValue : 1.0;
    const double durationScale = (speed > 0.0) ? (1.0 / speed) : 1.0;

    const bool loopOn   = (loopEnabledValue != nullptr) && (*loopEnabledValue > 0.5f);
    const int  loopFrom = (loopStartValue != nullptr) ? (int) *loopStartValue : 1;
    const int  loopTo   = (loopEndValue != nullptr)   ? (int) *loopEndValue   : 1;

    const auto current = std::atomic_load (&player);

    if (current == nullptr || ! current->hasAudio())
        return;

    // setLoop e setPlaybackBuffer publicam em átomos; não precisam do stateLock,
    // e assim a message thread nunca disputa com o áudio. O setPlaybackBuffer
    // recebe o ponteiro de um buffer já transformado, não o player.
    current->setLoop (loopOn, loopFrom, loopTo);

    std::shared_ptr<const juce::AudioBuffer<float>> source = current->getSourceBuffer();
    const double basePitch = current->getDetectedTuningHz();

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
        current->setPlaybackBuffer (nullptr, 1.0);
    }
    else
    {
        // Transformação cara fora de qualquer região sincronizada; o resultado
        // só entra no áudio quando atomic_store publica o ponteiro.
        auto transformed = PitchShiftEngine::transform (*source, ratio, durationScale);
        if (transformed == nullptr)
            return;

        current->setPlaybackBuffer (std::move (transformed), durationScale);
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

    // Sem lock. O snapshot custa um incremento de refcount e é a única região
    // sincronizada do caminho de áudio; o laço de amostras em fillOutput roda
    // fora dela. A thread de mensagem pode trocar o player e o buffer transformado
    // a qualquer momento sem esperar o áudio terminar o bloco.
    const auto current = std::atomic_load (&player);

    if (current != nullptr && current->hasAudio())
    {
        const auto startSample = transportSample.load (std::memory_order_relaxed);
        const bool withinRange = current->fillOutput (buffer, startSample, getSampleRate());
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
    // Decodificação e análise fora do lock, e fora do caminho de áudio: só a
    // publicação do ponteiro é crítica. Quem estiver tocando mantém o antigo
    // vivo pelo próprio snapshot e termina o bloco sem notar a troca.
    auto newPlayer = std::make_shared<FilePlayer>();
    newPlayer->loadFromFile (file);

    if (! newPlayer->hasAudio())
        return;

    {
        const juce::ScopedLock sl (stateLock);
        loadedFileName = newPlayer->getSourceFileName();
        reachedEnd.store (false, std::memory_order_relaxed);
    }

    std::atomic_store (&player, std::move (newPlayer));
    ++fileGeneration;

    // A impressão digital sai daqui, e não do player: é trabalho de fundo e
    // não pode atrasar o carregamento. Um novo start cancela o anterior, então
    // trocar de arquivo rapidinho não empilha cálculos.
    fingerprintWorker.start (file);
}


bool PlayScoreProcessor::exportTempoMap (const juce::File& file)
{
    const auto current = std::atomic_load (&player);

    if (current == nullptr || ! current->hasAudio())
        return false;

    const double bpm = current->getBpm();

    if (bpm <= 0.0)
        return false;

    return MidiMapExporter::writeTempoMap (file, bpm, current->getBeatsPerBar());
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

bool PlayScoreProcessor::hasReachedEndOfFile() const noexcept
{
    return reachedEnd.load (std::memory_order_relaxed);
}

juce::String PlayScoreProcessor::getLoadedFileName() const
{
    const juce::ScopedLock sl (stateLock);
    return loadedFileName;
}

juce::String PlayScoreProcessor::getSongTitle() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getTaggedTitle() : juce::String();
}

juce::String PlayScoreProcessor::getSongIsrc() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getTaggedIsrc() : juce::String();
}

juce::String PlayScoreProcessor::getSongYear() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getTaggedYear() : juce::String();
}

Fingerprint::State PlayScoreProcessor::getIdentificationState() const
{
    return fingerprintWorker.poll().state;
}

Fingerprint::Failure PlayScoreProcessor::getIdentificationFailure() const
{
    return fingerprintWorker.poll().failure;
}

juce::String PlayScoreProcessor::getIdentificationMessage() const
{
    return fingerprintWorker.poll().message;
}

int PlayScoreProcessor::getIdentificationFingerprintLength() const
{
    return fingerprintWorker.poll().fingerprint.length();
}


juce::int64 PlayScoreProcessor::getFileSizeBytes() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getSourceFileSize() : 0;
}

double PlayScoreProcessor::getAudioBpm() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getBpm() : 0.0;
}

int PlayScoreProcessor::getBeatsPerBar() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getBeatsPerBar() : 4;
}

double PlayScoreProcessor::getDetectedTuningHz() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getDetectedTuningHz() : Tuning::defaultReferenceHz;
}

double PlayScoreProcessor::getDetectedTuningCents() const
{
    const double hz = getDetectedTuningHz();
    return (hz > 0.0) ? 1200.0 * std::log2 (hz / Tuning::defaultReferenceHz) : 0.0;
}

double PlayScoreProcessor::getDurationSeconds() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getDurationSeconds() : 0.0;
}

double PlayScoreProcessor::getPlaybackDurationSeconds() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getPlaybackDurationSeconds() : 0.0;
}

int PlayScoreProcessor::getMeasureCount() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getMeasureCount() : 0;
}

WaveformPtr PlayScoreProcessor::getWaveformPeaks() const
{
    const auto current = std::atomic_load (&player);
    return current != nullptr ? current->getPeaks() : nullptr;
}

bool PlayScoreProcessor::getLoopFractions (double& startFraction, double& endFraction) const
{
    const auto current = std::atomic_load (&player);

    if (current == nullptr)
    {
        startFraction = endFraction = 0.0;
        return false;
    }

    return current->getLoopFractions (startFraction, endFraction);
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
