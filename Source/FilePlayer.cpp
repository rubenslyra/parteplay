#include "FilePlayer.h"
#include "TempoAnalyser.h"

#include <cmath>

namespace Waveform
{
    WaveformPtr build (const juce::AudioBuffer<float>& buffer, int buckets)
    {
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples <= 0 || numChannels <= 0 || buckets <= 0)
            return nullptr;

        const int count = juce::jmin (buckets, numSamples);
        auto peaks = std::make_shared<WaveformPeaks>();
        peaks->min.assign (static_cast<size_t> (count), 0.0f);
        peaks->max.assign (static_cast<size_t> (count), 0.0f);

        const double samplesPerBucket = static_cast<double> (numSamples) / static_cast<double> (count);

        // Passo limitado: mantém o custo da carga proporcional a buckets, não ao arquivo.
        const int stride = juce::jmax (1, static_cast<int> (samplesPerBucket / 64.0));

        for (int b = 0; b < count; ++b)
        {
            const int start = static_cast<int> (static_cast<double> (b) * samplesPerBucket);
            const int end   = juce::jmin (numSamples,
                                          static_cast<int> (static_cast<double> (b + 1) * samplesPerBucket));

            float low = 0.0f;
            float high = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* data = buffer.getReadPointer (ch);

                for (int i = start; i < end; i += stride)
                {
                    low  = juce::jmin (low, data[i]);
                    high = juce::jmax (high, data[i]);
                }
            }

            peaks->min[static_cast<size_t> (b)] = low;
            peaks->max[static_cast<size_t> (b)] = high;
        }

        return peaks;
    }
}

namespace
{
    // Nomes de tag variam por contêiner. O JUCE normaliza o ID3 de forma
    // razoável, mas MP4/M4A usa os códigos atômicos e Vorbis/FLAC usa nomes
    // livres — por isso cada campo lista os apelidos em vez de um só.
    juce::String firstTagValue (const juce::StringPairArray& values,
                                std::initializer_list<const char*> keys)
    {
        // operator[] devolve string vazia para chave ausente, e a busca ignora
        // maiusculas — que e o que queremos, porque gravadores de tag discordam
        // sobre "Title" vs "TITLE".
        for (auto* key : keys)
        {
            const auto value = values[juce::String (key)].trim();

            if (value.isNotEmpty())
                return value;
        }

        return {};
    }

    // ISRC vem como "BR-ABC-12-34567" ou colado ("BRABC1234567"). Guardamos o
    // formato colado, que é o canônico, e a apresentação com hífen é só UI.
    juce::String normaliseIsrc (const juce::String& raw)
    {
        return raw.replaceCharacters ("- ", "").toUpperCase();
    }

    // O ano costuma vir como data completa ("1985-06-30", "1985"). Fica só o
    // ano; o resto é informação de lançamento que pertence à busca online.
    juce::String extractYear (const juce::String& raw)
    {
        if (raw.isEmpty())
            return {};

        const auto trimmed = raw.trim();
        for (int i = 0; i < juce::jmin (4, trimmed.length()); ++i)
            if (! juce::CharacterFunctions::isDigit (trimmed[i]))
                return {};

        return trimmed.substring (0, 4);
    }
}

void FilePlayer::readTags (const juce::AudioFormatReader& reader)
{
    const auto& values = reader.metadataValues;

    // O codigo atomico MP4 de copyright e' "©nam"/"©day" (U+00A9). As
    // sequencias \x sao separadas por concatenacao de literais porque "\xA9d"
    // seria lido como um unico escape de 4 digitos e estouraria o intervalo.
    taggedTitle = firstTagValue (values, { "TITLE", "TIT2", "INAM", "\xC2\xA9" "nam" });
    taggedIsrc  = normaliseIsrc (firstTagValue (values, { "ISRC", "TSRC" }));
    taggedYear  = extractYear (firstTagValue (values, { "DATE", "YEAR", "ICRD", "TYER", "\xC2\xA9" "day" }));
}

void FilePlayer::loadFromFile (const juce::File& file)
{
    jassert (! file.isDirectory());

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return;

    auto newBuffer = std::make_shared<juce::AudioBuffer<float>> (
        static_cast<int> (reader->numChannels),
        static_cast<int> (reader->lengthInSamples));

    const bool ok = reader->read (newBuffer.get(), 0,
                                  static_cast<int> (reader->lengthInSamples), 0, true, true);
    if (! ok)
        return;

    audioBuffer      = std::move (newBuffer);
    std::atomic_store (&playbackBuffer, std::shared_ptr<const juce::AudioBuffer<float>>());
    peaks            = Waveform::build (*audioBuffer, 2048);
    appliedDurationScale.store (1.0, std::memory_order_relaxed);
    fileSampleRate   = reader->sampleRate;
    sourceFileName   = file.getFileName();
    fileSizeBytes    = file.getSize();
    durationSeconds  = fileSampleRate > 0.0 ? (double) reader->lengthInSamples / fileSampleRate : 0.0;

    readTags (*reader);

    runTempoAnalysis();
}

void FilePlayer::clear()
{
    audioBuffer.reset();
    std::atomic_store (&playbackBuffer, std::shared_ptr<const juce::AudioBuffer<float>>());
    peaks.reset();
    appliedDurationScale.store (1.0, std::memory_order_relaxed);
    fileSampleRate = 0.0;
    sourceFileName.clear();
    fileSizeBytes = 0;
    taggedTitle.clear();
    taggedIsrc.clear();
    taggedYear.clear();
    estimatedBpm = 0.0;
    beatsPerBar = 4;
    detectedTuningHz = Tuning::defaultReferenceHz;
    durationSeconds = 0.0;
    measures = 0;
    setLoop (false, 1, 1);
}

void FilePlayer::runTempoAnalysis()
{
    estimatedBpm = 0.0;
    beatsPerBar = 4;
    detectedTuningHz = Tuning::defaultReferenceHz;
    measures = 0;

    if (audioBuffer == nullptr || fileSampleRate <= 0.0)
        return;

    estimatedBpm = TempoAnalyser::estimateBpm (fileSampleRate, *audioBuffer);

    if (estimatedBpm > 0.0)
        beatsPerBar = TempoAnalyser::estimateBeatsPerBar (fileSampleRate, *audioBuffer, estimatedBpm);

    const double tuningCents = TempoAnalyser::estimateTuningCents (fileSampleRate, *audioBuffer);
    detectedTuningHz = Tuning::defaultReferenceHz * std::pow (2.0, tuningCents / 1200.0);

    if (estimatedBpm > 0.0 && durationSeconds > 0.0)
        measures = (int) std::llround (durationSeconds * estimatedBpm / 60.0 / (double) beatsPerBar);

    if (measures < 0)
        measures = 0;
}

double FilePlayer::getPlaybackDurationSeconds() const noexcept
{
    const auto transformed = std::atomic_load (&playbackBuffer);
    const auto& active = (transformed != nullptr) ? transformed : audioBuffer;

    if (active == nullptr || fileSampleRate <= 0.0)
        return 0.0;

    return static_cast<double> (active->getNumSamples()) / fileSampleRate;
}

bool FilePlayer::getLoopFractions (double& startFraction, double& endFraction) const noexcept
{
    startFraction = 0.0;
    endFraction   = 0.0;

    if (! loopEnabled.load (std::memory_order_relaxed) || estimatedBpm <= 0.0)
        return false;

    const auto transformed = std::atomic_load (&playbackBuffer);
    const auto& active = (transformed != nullptr) ? transformed : audioBuffer;

    if (active == nullptr || active->getNumSamples() <= 0 || fileSampleRate <= 0.0)
        return false;

    const double measureSamples = (beatsPerBar * 60.0 / estimatedBpm)
                                  * appliedDurationScale.load (std::memory_order_relaxed) * fileSampleRate;

    if (measureSamples <= 0.0)
        return false;

    const double total = static_cast<double> (active->getNumSamples());
    const int firstMeasure = loopStartMeasure.load (std::memory_order_relaxed);
    const int lastMeasure  = loopEndMeasure.load (std::memory_order_relaxed);

    startFraction = juce::jlimit (0.0, 1.0, ((firstMeasure - 1) * measureSamples) / total);
    endFraction   = juce::jlimit (0.0, 1.0, (static_cast<double> (lastMeasure) * measureSamples) / total);

    return endFraction > startFraction;
}

bool FilePlayer::fillOutput (juce::AudioBuffer<float>& dest,
                             juce::int64 hostStartSample,
                             double hostSampleRate) const
{
    if (fileSampleRate <= 0.0 || hostSampleRate <= 0.0)
        return false;

    // Snapshot do buffer transformado. A cópia do shared_ptr é o que garante que
    // a thread de mensagem possa trocar o ponteiro a qualquer momento sem que o
    // áudio leia memória liberada: enquanto `transformed` estiver vivo aqui, o
    // objeto está vivo. atomic_load é a única região sincronizada — o laço de
    // amostras abaixo roda inteiramente fora dela.
    const auto transformed = std::atomic_load (&playbackBuffer);

    // Quando há buffer de reprodução disponível (transposto/esticado), toca-o;
    // caso contrário, usa o original. O buffer de reprodução tem o mesmo sample
    // rate do arquivo e o mapeamento transport->amostra permanece 1:1 (a duração
    // escalada já está embutida no conteúdo), preservando a sincronia.
    const auto* source = transformed != nullptr ? transformed.get() : audioBuffer.get();

    if (source == nullptr)
        return false;

    const double scale = appliedDurationScale.load (std::memory_order_relaxed);
    const int numOut = dest.getNumSamples();
    if (numOut <= 0)
        return true;

    const int fileLen = source->getNumSamples();
    const double interpStep = fileSampleRate / hostSampleRate;
    double srcPos = static_cast<double> (hostStartSample) * interpStep;

    // Loop de treino: repete o trecho entre os compassos [início, fim].
    if (loopEnabled.load (std::memory_order_relaxed) && estimatedBpm > 0.0)
    {
        const double measureSamples = (beatsPerBar * 60.0 / estimatedBpm) * scale * fileSampleRate;
        const auto loopStart = static_cast<juce::int64> (std::lround ((loopStartMeasure.load (std::memory_order_relaxed) - 1) * measureSamples));
        const auto loopEnd   = static_cast<juce::int64> (std::lround (static_cast<double> (loopEndMeasure.load (std::memory_order_relaxed)) * measureSamples));
        const auto loopLen   = loopEnd - loopStart;

        if (loopStart >= 0 && loopLen > static_cast<juce::int64> (fileSampleRate / 4.0))
        {
            const double wrapped = static_cast<double> (loopStart) + std::fmod (srcPos - static_cast<double> (loopStart), static_cast<double> (loopLen));
            srcPos = (wrapped < static_cast<double> (loopStart)) ? wrapped + static_cast<double> (loopLen) : wrapped;
        }
    }

    bool reachedEnd = false;

    for (int ch = 0; ch < dest.getNumChannels(); ++ch)
    {
        const float* src = source->getReadPointer (juce::jmin (ch, source->getNumChannels() - 1));
        float* out = dest.getWritePointer (ch);

        double pos = srcPos;

        for (int i = 0; i < numOut; ++i)
        {
            const int idx0 = static_cast<int> (pos);

            if (idx0 < 0 || idx0 >= fileLen)
            {
                out[i] = 0.0f;
                pos = -1.0;
                reachedEnd = true;
                continue;
            }

            const int idx1 = juce::jmin (idx0 + 1, fileLen - 1);
            const float frac = static_cast<float> (pos - static_cast<double> (idx0));
            out[i] = src[idx0] + frac * (src[idx1] - src[idx0]);
            pos += interpStep;
        }
    }

    return ! reachedEnd;
}
