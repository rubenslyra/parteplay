#include "FilePlayer.h"
#include "ExternalBpm.h"
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

juce::int64 FilePlayer::countLeadingSilence (const juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return 0;

    // Zeros digitais exatos: qualquer amostra nao nula em qualquer canal
    // interrompe a contagem. Nao ha limiar em dB aqui de proposito.
    //
    // A comparacao e exata e continua sendo. -Wfloat-equal existe porque ponto
    // flutuante nao promete igualdade depois de uma conta - aqui nao houve
    // conta nenhuma entre o valor lido e o zero, e o valor veio do decoder. Trocar
    // por tolerancia trocaria silencio digital por um piso, que e a decisao
    // errada. A supressao fica local e escrita, porque e uma excecao e nao a regra.
   #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
   #endif
    for (int i = 0; i < numSamples; ++i)
        for (int ch = 0; ch < numChannels; ++ch)
            if (buffer.getReadPointer (ch)[i] != 0.0f)
                return static_cast<juce::int64> (i);
   #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
   #endif

    return static_cast<juce::int64> (numSamples);
}

int FilePlayer::measureCountFor (double durationSeconds, double bpm, int beatsPerBar)
{
    if (durationSeconds <= 0.0 || bpm <= 0.0 || beatsPerBar <= 0)
        return 0;

    const double beats = durationSeconds * bpm / 60.0;
    return static_cast<int> (std::llround (beats / static_cast<double> (beatsPerBar)));
}

int FilePlayer::getMeasureCount() const noexcept
{
    return measureCountFor (durationSeconds, getBpm(), getBeatsPerBar());
}

void FilePlayer::loadFromFile (const juce::File& file, const LoadProgressFn& onProgress)
{
    jassert (! file.isDirectory());

    const auto report = [&onProgress] (float fraction, LoadStage stage)
    {
        if (onProgress)
            onProgress (juce::jlimit (0.0f, 1.0f, fraction), stage);
    };

    report (0.0f, LoadStage::preparing);

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return;

    auto newBuffer = std::make_shared<juce::AudioBuffer<float>> (
        static_cast<int> (reader->numChannels),
        static_cast<int> (reader->lengthInSamples));

    // Leitura em blocos: e o unico jeito de a barra avancar durante o decode de
    // um arquivo longo. O read() corrente e a parte lenta da carga fora da
    // analise de andamento.
    const auto totalSamples = static_cast<juce::int64> (reader->lengthInSamples);
    const auto chunkSamples = static_cast<juce::int64> (1 << 18);
    bool ok = true;

    for (juce::int64 start = 0; start < totalSamples; start += chunkSamples)
    {
        const auto thisChunk = static_cast<int> (juce::jmin (chunkSamples, totalSamples - start));

        if (! reader->read (newBuffer.get(), static_cast<int> (start), thisChunk, start, true, true))
        {
            ok = false;
            break;
        }

        report (totalSamples > 0
                    ? 0.02f + 0.43f * (float) ((double) (start + thisChunk) / (double) totalSamples)
                    : 0.45f,
                LoadStage::decoding);
    }

    if (! ok)
        return;

    report (0.47f, LoadStage::waveform);

    // Passo A: remove o silencio inicial em zeros digitais exatos antes de
    // qualquer medicao. O audio entregue ao player e o audio ajustado, entao
    // duracao, forma de onda, BARS e loop ja nascem alinhados ao sample 0.
    leadingSilenceSamples = countLeadingSilence (*newBuffer);

    if (leadingSilenceSamples > 0
        && leadingSilenceSamples < static_cast<juce::int64> (newBuffer->getNumSamples()))
    {
        const int remaining = static_cast<int> (static_cast<juce::int64> (newBuffer->getNumSamples())
                                                 - leadingSilenceSamples);
        auto trimmed = std::make_shared<juce::AudioBuffer<float>> (newBuffer->getNumChannels(), remaining);

        for (int ch = 0; ch < trimmed->getNumChannels(); ++ch)
            trimmed->copyFrom (ch, 0, *newBuffer,
                               juce::jmin (ch, newBuffer->getNumChannels() - 1),
                               static_cast<int> (leadingSilenceSamples), remaining);

        newBuffer = std::move (trimmed);
    }
    else
    {
        leadingSilenceSamples = 0;
    }

    audioBuffer      = std::move (newBuffer);
    std::atomic_store (&playbackBuffer, std::shared_ptr<const juce::AudioBuffer<float>>());
    peaks            = Waveform::build (*audioBuffer, 2048);
    appliedDurationScale.store (1.0, std::memory_order_relaxed);
    fileSampleRate   = reader->sampleRate;
    sourceFileName   = file.getFileName();
    fileSizeBytes    = file.getSize();

    // Duracao util: o buffer ja sem o silencio inicial, nao o comprimento do arquivo.
    durationSeconds  = (fileSampleRate > 0.0 && audioBuffer != nullptr)
                           ? static_cast<double> (audioBuffer->getNumSamples()) / fileSampleRate
                           : 0.0;

    readTags (*reader);

    estimatedBpm = 0.0;
    beatsPerBar = 4;
    detectedTuningHz = Tuning::defaultReferenceHz;

    if (audioBuffer != nullptr && fileSampleRate > 0.0)
    {
        report (0.50f, LoadStage::tempo);

        // A busca de andamento domina o tempo restante; os dois caminhos remapeiam
        // o progresso para 0,50..0,85. A pipeline externa (ffmpeg + soundstretch)
        // e a fonte autoritativa quando os binarios existem; o analisador nativo e
        // o fallback quando eles faltam ou falham.
        const auto tools = ExternalBpm::locateTools();

        if (tools.valid())
            estimatedBpm = ExternalBpm::estimateBpm (
                file, tools,
                [&report] (float p) { report (0.50f + 0.35f * p, LoadStage::tempo); });

        if (estimatedBpm <= 0.0)
            estimatedBpm = TempoAnalyser::estimateBpm (
                fileSampleRate, *audioBuffer,
                [&report] (float p) { report (0.50f + 0.35f * p, LoadStage::tempo); });

        if (estimatedBpm > 0.0)
            beatsPerBar = TempoAnalyser::estimateBeatsPerBar (fileSampleRate, *audioBuffer, estimatedBpm);

        report (0.85f, LoadStage::tuning);

        const double tuningCents = TempoAnalyser::estimateTuningCents (
            fileSampleRate, *audioBuffer,
            [&report] (float p) { report (0.85f + 0.13f * p, LoadStage::tuning); });
        detectedTuningHz = Tuning::defaultReferenceHz * std::pow (2.0, tuningCents / 1200.0);

    }

    report (1.0f, LoadStage::publishing);
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
    leadingSilenceSamples = 0;
    setLoop (false, 1, 1);
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

    const double effectiveBpm = getBpm();
    const int meter = getBeatsPerBar();

    if (! loopEnabled.load (std::memory_order_relaxed) || effectiveBpm <= 0.0 || meter <= 0)
        return false;

    const auto transformed = std::atomic_load (&playbackBuffer);
    const auto& active = (transformed != nullptr) ? transformed : audioBuffer;

    if (active == nullptr || active->getNumSamples() <= 0 || fileSampleRate <= 0.0)
        return false;

    const double measureSamples = (meter * 60.0 / effectiveBpm)
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
    const double effectiveBpm = getBpm();

    if (loopEnabled.load (std::memory_order_relaxed) && effectiveBpm > 0.0)
    {
        const double measureSamples = (getBeatsPerBar() * 60.0 / effectiveBpm) * scale * fileSampleRate;
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
