#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <functional>
#include <memory>

#include "Tuning.h"
#include "Waveform.h"

// Guarda o áudio de referência (fonte) e a versão transformada (afinação/esticada),
// além da análise feita na carga. O mapeamento transport->amostra é 1:1 por construção:
// a transformação preserva a taxa e a duração escalada já está embutida no conteúdo.
class FilePlayer
{
public:
    FilePlayer() = default;
    ~FilePlayer() = default;

    // Progresso da carga para a interface. O estagio e um identificador, nao
    // texto: quem traduz e a UI, dona da tabela de strings. O callback roda na
    // thread de fundo, entao nao pode tocar em nada da interface.
    enum class LoadStage { preparing, decoding, waveform, tempo, tuning, publishing };
    using LoadProgressFn = std::function<void (float fraction, LoadStage stage)>;

    void loadFromFile (const juce::File& file, const LoadProgressFn& onProgress = {});

    bool hasAudio() const noexcept                         { return audioBuffer != nullptr; }
    double getSampleRate() const noexcept                  { return fileSampleRate; }
    int getNumSamples() const noexcept                     { return audioBuffer != nullptr ? audioBuffer->getNumSamples() : 0; }
    const juce::String& getSourceFileName() const noexcept { return sourceFileName; }
    juce::int64 getSourceFileSize() const noexcept         { return fileSizeBytes; }

    // Andamento: `getRawBpm` e o detectado (SoundStretch/analise nativa) e
    // `getBpm` aplica a escala escolhida pelo usuario (padrao x2). BARS e o loop
    // de treino consomem os dois, entao a escala entra numa unica fonte.
    double getRawBpm() const noexcept                      { return estimatedBpm; }
    double getBpm() const noexcept                         { return estimatedBpm * bpmScale.load (std::memory_order_relaxed); }
    int getBeatsPerBar() const noexcept                    { return effectiveBeatsPerBar(); }
    double getDetectedTuningHz() const noexcept            { return detectedTuningHz; }
    double getDurationSeconds() const noexcept             { return durationSeconds; }
    int getMeasureCount() const noexcept;

    // Ajuste de andamento: x2 (padrao) corrige a leitura crua do SoundStretch
    // para a metrica do editor; x1 mantem o valor real detectado. Alterar apos a
    // carga recalcula BARS e o mapeamento do loop sem recarregar o arquivo.
    void setBpmScale (double scale) noexcept               { bpmScale.store (scale > 0.0 ? scale : 1.0, std::memory_order_relaxed); }
    double getBpmScale() const noexcept                    { return bpmScale.load (std::memory_order_relaxed); }

    // Metrica efetiva: o numerador declarado pelo host quando existe (>0); senao
    // o compasso estimado do audio. Mantem BARS e loop alinhados ao editor.
    void setMeterOverride (int numerator) noexcept         { meterOverride.store (numerator > 0 ? numerator : 0, std::memory_order_relaxed); }
    int getMeterOverride() const noexcept                  { return meterOverride.load (std::memory_order_relaxed); }

    // Silencio inicial removido (zeros digitais exatos). Zero = o audio comeca no
    // primeiro sample nao nulo; a UI avisa quando o audio foi adaptado.
    double getLeadingSilenceSeconds() const noexcept       { return fileSampleRate > 0.0 ? (double) leadingSilenceSamples / fileSampleRate : 0.0; }
    bool wasTrimmed() const noexcept                       { return leadingSilenceSamples > 0; }

    // Puros, para teste: contagem de silencio inicial (sem limiar inventado) e
    // BARS a partir da duracao util, do BPM efetivo e do numerador do compasso.
    static juce::int64 countLeadingSilence (const juce::AudioBuffer<float>& buffer);
    static int measureCountFor (double durationSeconds, double bpm, int beatsPerBar);

    // Ficha da canção lida das tags embutidas no arquivo. É o primeiro elo da
    // cadeia "tags -> consulta online -> manual": o que não estiver na tag
    // Simply fica vazio, e a camada de consulta preenche depois. Nenhum campo
    // aqui é inventado — vazio significa "o arquivo não diz".
    const juce::String& getTaggedTitle() const noexcept    { return taggedTitle; }
    const juce::String& getTaggedIsrc() const noexcept     { return taggedIsrc; }
    const juce::String& getTaggedYear() const noexcept     { return taggedYear; }

    // Duração do buffer efetivamente reproduzido (fonte ou versão esticada do treino).
    double getPlaybackDurationSeconds() const noexcept;

    // Forma de onda da fonte (snapshot imutável; nullptr sem áudio).
    WaveformPtr getPeaks() const noexcept                  { return peaks; }

    void clear();

    // Const por construcao de contrato: fillOutput nao escreve nenhum membro,
    // so preenche `dest`. E o que permite a thread de audio working sem lock.
    bool fillOutput (juce::AudioBuffer<float>& dest,
                     juce::int64 hostStartSample,
                     double hostSampleRate) const;

    // Acesso à fonte original e ao buffer de reprodução (transposto / esticado).
    // Os shared_ptrs permitem capturar um snapshot barato (sem copiar o áudio) e
    // manter o dado vivo enquanto outra thread ainda o estiver usando.
    std::shared_ptr<const juce::AudioBuffer<float>> getSourceBuffer() const noexcept { return audioBuffer; }

    // Publicação do buffer transformado. A thread de mensagem escreve; a de áudio
    // lê via atomic_load, que devolve uma cópia do shared_ptr — a cópia mantém o
    // buffer vivo durante todo o fillOutput, então o áudio nunca espera a
    // transformação terminar nem corre risco de ler memória liberada.
    void setPlaybackBuffer (std::shared_ptr<const juce::AudioBuffer<float>> buffer, double durationScale)
    {
        std::atomic_store (&playbackBuffer, std::move (buffer));
        appliedDurationScale.store (durationScale, std::memory_order_relaxed);
    }

    void setLoop (bool enabled, int startMeasure, int endMeasure) noexcept
    {
        loopStartMeasure.store (juce::jmax (1, startMeasure), std::memory_order_relaxed);
        loopEndMeasure.store   (juce::jmax (juce::jmax (1, startMeasure), endMeasure), std::memory_order_relaxed);
        loopEnabled.store (enabled, std::memory_order_relaxed);
    }

    bool isLoopActive() const noexcept { return loopEnabled.load (std::memory_order_relaxed); }

    // Trecho de loop como fração [0..1] da forma de onda (domínio do buffer ativo).
    // Retorna false quando o loop está desligado ou ainda não há base de tempo.
    bool getLoopFractions (double& startFraction, double& endFraction) const noexcept;

private:
    int effectiveBeatsPerBar() const noexcept
    {
        const auto override = meterOverride.load (std::memory_order_relaxed);
        return override > 0 ? override : beatsPerBar;
    }

    void readTags (const juce::AudioFormatReader& reader);

    // audioBuffer e peaks sao escrita-uma-vez em loadFromFile, antes de o objeto
    // ser publicado ao processador. Por isso nao sao atomicos: o publicador
    // (std::atomic_store em beginLoadAudioFile) cria a barreira de memoria.
    std::shared_ptr<const juce::AudioBuffer<float>> audioBuffer;
    WaveformPtr peaks;

    // Ja publicadas: escritas pela thread de mensagem, lidas pela de audio.
    std::shared_ptr<const juce::AudioBuffer<float>> playbackBuffer;
    std::atomic<double> appliedDurationScale { 1.0 };
    std::atomic<bool> loopEnabled { false };
    std::atomic<int> loopStartMeasure { 1 };
    std::atomic<int> loopEndMeasure { 1 };

    // Somente leitura depois da carga.
    double fileSampleRate = 0.0;
    juce::String sourceFileName;
    juce::int64 fileSizeBytes = 0;

    juce::String taggedTitle;
    juce::String taggedIsrc;
    juce::String taggedYear;

    double estimatedBpm = 0.0;
    int beatsPerBar = 4;
    double detectedTuningHz = 440.0;
    double durationSeconds = 0.0;
    juce::int64 leadingSilenceSamples = 0;

    // Escritos pela message thread apos a publicacao, lidos pela thread de audio
    // no laco do treino: por isso sao atomicos, e nao membros simples.
    std::atomic<double> bpmScale { 2.0 };
    std::atomic<int> meterOverride { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilePlayer)
};
