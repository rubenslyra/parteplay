#pragma once

#include <JuceHeader.h>

#include <atomic>
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

    void loadFromFile (const juce::File& file);

    bool hasAudio() const noexcept                         { return audioBuffer != nullptr; }
    double getSampleRate() const noexcept                  { return fileSampleRate; }
    int getNumSamples() const noexcept                     { return audioBuffer != nullptr ? audioBuffer->getNumSamples() : 0; }
    const juce::String& getSourceFileName() const noexcept { return sourceFileName; }
    juce::int64 getSourceFileSize() const noexcept         { return fileSizeBytes; }

    double getBpm() const noexcept                         { return estimatedBpm; }
    int getBeatsPerBar() const noexcept                    { return beatsPerBar; }
    double getDetectedTuningHz() const noexcept            { return detectedTuningHz; }
    double getDurationSeconds() const noexcept             { return durationSeconds; }
    int getMeasureCount() const noexcept                   { return measures; }

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
    void runTempoAnalysis();
    void readTags (const juce::AudioFormatReader& reader);

    // audioBuffer e peaks sao escrita-uma-vez em loadFromFile, antes de o objeto
    // ser publicado ao processador. Por isso nao sao atomicos: o publicador
    // (std::atomic_store em loadAudioFile) cria a barreira de memoria.
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
    int measures = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilePlayer)
};
