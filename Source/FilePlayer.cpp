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
    transposedBuffer.reset();
    appliedPitchRatio = 1.0;
    fileSampleRate   = reader->sampleRate;
    sourceFileName   = file.getFileName();
    durationSeconds  = fileSampleRate > 0.0 ? (double) reader->lengthInSamples / fileSampleRate : 0.0;

    runTempoAnalysis();
}

void FilePlayer::clear()
{
    audioBuffer.reset();
    transposedBuffer.reset();
    appliedPitchRatio = 1.0;
    fileSampleRate = 0.0;
    sourceFileName.clear();
    estimatedBpm = 0.0;
    beatsPerBar = 4;
    durationSeconds = 0.0;
    measures = 0;
}

void FilePlayer::runTempoAnalysis()
{
    estimatedBpm = 0.0;
    beatsPerBar = 4;
    measures = 0;

    if (audioBuffer == nullptr || fileSampleRate <= 0.0)
        return;

    estimatedBpm = TempoAnalyser::estimateBpm (fileSampleRate, *audioBuffer);

    if (estimatedBpm > 0.0)
        beatsPerBar = TempoAnalyser::estimateBeatsPerBar (fileSampleRate, *audioBuffer, estimatedBpm);

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

    // Quando há buffer transposto disponível, toca a versão transposta; caso
    // contrário, usa o original. Ambos têm a mesma taxa e o mesmo comprimento,
    // portanto o mapeamento transport->amostra do arquivo permanece válido.
    const auto* source = transposedBuffer != nullptr ? transposedBuffer.get() : audioBuffer.get();

    if (source == nullptr)
        return false;

    const int numOut = dest.getNumSamples();
    if (numOut <= 0)
        return true;

    const int fileLen = source->getNumSamples();
    const double interpStep = fileSampleRate / hostSampleRate;
    double srcPos = static_cast<double> (hostStartSample) * interpStep;

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