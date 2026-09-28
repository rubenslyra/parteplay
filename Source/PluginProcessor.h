#pragma once

#include <JuceHeader.h>

#include <memory>

#include "FilePlayer.h"
#include "FingerprintWorker.h"
#include "Tuning.h"
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
    bool hasReachedEndOfFile() const noexcept;

    juce::String getLoadedFileName() const;
    juce::int64 getFileSizeBytes() const;

    // Ficha da canção — hoje lida das tags do arquivo. A camada de consulta
    // online (AcoustID + MusicBrainz) vai sobrescrever/preencher estes campos
    // depois; até lá, o que a tag não trouxer fica vazio de propósito.
    juce::String getSongTitle() const;
    juce::String getSongIsrc() const;
    juce::String getSongYear() const;

    // Estado da identificação por conteúdo.
    //
    // A impressão digital é calculada localmente ao carregar o arquivo, sem
    // rede e sem chave. Os getters abaixo são lidos pela interface a 20 Hz pela
    // message thread e não tomam lock: o worker publica o resultado por troca
    // atômica de shared_ptr, e poll() só faz a leitura.
    //
    // Importante para a interface: `ready` significa "impressão digital
    // calculada", e NÃO "faixa identificada". Nenhuma consulta ao AcoustID saiu
    // ainda — o texto da UI tem de refletir essa diferença.
    Fingerprint::State getIdentificationState() const;
    Fingerprint::Failure getIdentificationFailure() const;
    juce::String getIdentificationMessage() const;
    int getIdentificationFingerprintLength() const;

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

    double getReferencePitchHz() const noexcept;

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void resetPlaybackState();
    void timerCallback() override;
    double computeCurrentPitchRatio (double baseFilePitchHz) const;

    // O player é publicado por atomic_store e lido por atomic_load. A thread de
    // áudio nunca espera pela thread de mensagem: pega um shared_ptr (custo de um
    // incremento de refcount) e segue. Enquanto o snapshot estiver vivo, o
    // objeto não pode ser destruído por um novo carregamento.
    std::shared_ptr<FilePlayer> player;

    // Impressão digital do arquivo carregado. É trabalho de fundo: nunca roda
    // no caminho de áudio nem trava a message thread (só dispara e coleta). O
    // destrutor cancela e espera o job corrente, o que é rápido porque o
    // cancelamento é lido a cada bloco.
    Fingerprint::Worker fingerprintWorker;

    // stateLock ficou só para leituras O(1) da message thread (strings e números
    // da ficha). Nenhuma região crítica de áudio passa por aqui — ver processBlock.
    mutable juce::CriticalSection stateLock;

    // Ajuste de afinação (message thread via Timer). Sem transposição: o ratio
    // aqui é sempre a compensação da afinação de referência contra a do arquivo,
    // da ordem de poucos cents.
    double committedPitchRatio = 1.0;
    double committedDurationScale = 1.0;
    long long fileGeneration  = 0;   // incrementado a cada carregamento
    long long committedGeneration = -1;

    std::atomic<float>* pitchValue = nullptr;
    std::atomic<float>* trainingSpeedValue = nullptr;
    std::atomic<float>* loopEnabledValue = nullptr;
    std::atomic<float>* loopStartValue = nullptr;
    std::atomic<float>* loopEndValue = nullptr;
    std::atomic<float>* mutedValue = nullptr;

    std::atomic<bool> transportPlaying { false };
    std::atomic<int64_t> transportSample { 0 };
    std::atomic<double> transportSampleRate { 0.0 };
    std::atomic<bool> reachedEnd { false };

    juce::String loadedFileName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayScoreProcessor)
};
