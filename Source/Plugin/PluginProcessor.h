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

    // Carrega o audio em segundo plano. O trabalho pesado (decode, forma de
    // onda, andamento, afinacao) roda num job do loadPool, fora da message
    // thread; a UI acompanha por isLoadingAudio()/getLoadProgress()/
    // getLoadStage(). Substitui o antigo loadAudioFile sincrono, que travava a
    // interface por todo o tempo da analise.
    void beginLoadAudioFile (const juce::File& file);
    bool exportTempoMap (const juce::File& file);

    bool isLoadingAudio() const noexcept;
    float getLoadProgress() const noexcept;
    int getLoadStage() const noexcept;

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

    // Andamento efetivo (detectado x escala, padrao x2) e cru (detectado), para o
    // botao de fonte no painel 05: x2 corrige a leitura do SoundStretch para a
    // metrica do editor; x1 mantem o valor real detectado.
    double getAudioBpm() const;
    int getBeatsPerBar() const;
    double getRawAudioBpm() const;
    bool isBpmDoubled() const noexcept;
    void setBpmDoubled (bool doubled);

    // Silencio inicial removido no carregamento (zeros digitais exatos), para o
    // aviso de "audio adaptado". Zero = nada foi cortado.
    double getLeadingSilenceSeconds() const;
    bool wasAudioTrimmed() const;

    // Transporte declarado pelo host. Zero significa "o host nao informou";
    // nunca tratar o ausente como um valor real de BPM ou de formula de compasso.
    double getHostBpm() const noexcept;
    int getHostTimeSignatureNumerator() const noexcept;
    int getHostTimeSignatureDenominator() const noexcept;
    double getHostPpqPosition() const noexcept;
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

    // Carga assincrona. Os jobs rodam fora da message thread e devolvem o player
    // por callAsync; a UI so le os atomicos abaixo, a 20 Hz, sem lock.
    juce::ThreadPool loadPool { 1 };
    std::atomic<bool> loadInProgress { false };
    std::atomic<float> loadProgress { 0.0f };
    std::atomic<int> loadStage { static_cast<int> (FilePlayer::LoadStage::preparing) };
    std::atomic<int> loadGeneration { 0 };

    // Guarda de vida: jobs e callAsync capturam este flag e desistem se o
    // processador ja foi destruido. Sem ele, um job em voo tocaria membros mortos.
    std::shared_ptr<std::atomic<bool>> aliveFlag { std::make_shared<std::atomic<bool>> (true) };

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

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<int> hostTimeSigNumerator { 0 };
    std::atomic<int> hostTimeSigDenominator { 0 };
    std::atomic<double> hostPpqPosition { 0.0 };

    // Fonte do BPM escolhida no painel 05. Fora do APVTS de proposito: e estado
    // de sessao da UI, nao automacao salva no projeto.
    std::atomic<bool> bpmDoubled { true };

    juce::String loadedFileName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayScoreProcessor)
};
