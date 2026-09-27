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
    playbackBuffer.reset();
    peaks            = Waveform::build (*audioBuffer, 2048);
    appliedDurationScale = 1.0;
    fileSampleRate   = reader->sampleRate;
    sourceFileName   = file.getFileName();
    fileSizeBytes    = file.getSize();
    durationSeconds  = fileSampleRate > 0.0 ? (double) reader->lengthInSamples / fileSampleRate : 0.0;

    runTempoAnalysis();
}

void FilePlayer::clear()
{
    audioBuffer.reset();
    playbackBuffer.reset();
    peaks.reset();
    appliedDurationScale = 1.0;
    fileSampleRate = 0.0;
    sourceFileName.clear();
    fileSizeBytes = 0;
    estimatedBpm = 0.0;
    beatsPerBar = 4;
    detectedTuningHz = Tuning::defaultReferenceHz;
    durationSeconds = 0.0;
    measures = 0;
    loopEnabled = false;
    loopStartMeasure = 1;
    loopEndMeasure = 1;
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
    const auto& active = (playbackBuffer != nullptr) ? playbackBuffer : audioBuffer;

    if (active == nullptr || fileSampleRate <= 0.0)
        return 0.0;

    return static_cast<double> (active->getNumSamples()) / fileSampleRate;
}

bool FilePlayer::getLoopFractions (double& startFraction, double& endFraction) const noexcept
{
    startFraction = 0.0;
    endFraction   = 0.0;

    if (! loopEnabled || estimatedBpm <= 0.0)
        return false;

    const auto& active = (playbackBuffer != nullptr) ? playbackBuffer : audioBuffer;

    if (active == nullptr || active->getNumSamples() <= 0 || fileSampleRate <= 0.0)
        return false;

    const double measureSamples = (beatsPerBar * 60.0 / estimatedBpm) * appliedDurationScale * fileSampleRate;

    if (measureSamples <= 0.0)
        return false;

    const double total = static_cast<double> (active->getNumSamples());

    startFraction = juce::jlimit (0.0, 1.0, ((loopStartMeasure - 1) * measureSamples) / total);
    endFraction   = juce::jlimit (0.0, 1.0, (static_cast<double> (loopEndMeasure) * measureSamples) / total);

    return endFraction > startFraction;
}

bool FilePlayer::fillOutput (juce::AudioBuffer<float>& dest,
                             juce::int64 hostStartSample,
                             double hostSampleRate)
{
    if (fileSampleRate <= 0.0 || hostSampleRate <= 0.0)
        return false;

    // Quando há buffer de reprodução disponível (transposto/esticado), toca-o;
    // caso contrário, usa o original. O buffer de reprodução tem o mesmo sample
    // rate do arquivo e o mapeamento transport->amostra permanece 1:1 (a duração
    // escalada já está embutida no conteúdo), preservando a sincronia.
    const auto* source = playbackBuffer != nullptr ? playbackBuffer.get() : audioBuffer.get();

    if (source == nullptr)
        return false;

    const int numOut = dest.getNumSamples();
    if (numOut <= 0)
        return true;

    const int fileLen = source->getNumSamples();
    const double interpStep = fileSampleRate / hostSampleRate;
    double srcPos = static_cast<double> (hostStartSample) * interpStep;

    // Loop de treino: repete o trecho entre os compassos [início, fim].
    if (loopEnabled && estimatedBpm > 0.0)
    {
        const double measureSamples = (beatsPerBar * 60.0 / estimatedBpm) * appliedDurationScale * fileSampleRate;
        const auto loopStart = static_cast<juce::int64> (std::lround ((loopStartMeasure - 1) * measureSamples));
        const auto loopEnd   = static_cast<juce::int64> (std::lround (static_cast<double> (loopEndMeasure) * measureSamples));
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
