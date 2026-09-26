#include "FilePlayer.h"
#include "TempoAnalyser.h"

#include <cmath>

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
    appliedDurationScale = 1.0;
    fileSampleRate   = reader->sampleRate;
    sourceFileName   = file.getFileName();
    durationSeconds  = fileSampleRate > 0.0 ? (double) reader->lengthInSamples / fileSampleRate : 0.0;

    runTempoAnalysis();
}

void FilePlayer::clear()
{
    audioBuffer.reset();
    playbackBuffer.reset();
    appliedDurationScale = 1.0;
    fileSampleRate = 0.0;
    sourceFileName.clear();
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
        const auto loopEnd   = static_cast<juce::int64> (std::lround ((double) loopEndMeasure * measureSamples));
        const auto loopLen   = loopEnd - loopStart;

        if (loopStart >= 0 && loopLen > static_cast<juce::int64> (fileSampleRate / 4.0))
        {
            const double wrapped = (double) loopStart + std::fmod (srcPos - (double) loopStart, (double) loopLen);
            srcPos = (wrapped < (double) loopStart) ? wrapped + (double) loopLen : wrapped;
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