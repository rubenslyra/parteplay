#include "TempoAnalyser.h"

#include <cmath>
#include <vector>
#include <limits>

namespace
{
    juce::Array<float> buildOnsetEnvelope (const juce::AudioBuffer<float>& buffer, const int hop)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples <= hop * 2 || numChannels < 1)
            return {};

        const int numFrames = numSamples / hop;
        juce::Array<float> energy;
        energy.ensureStorageAllocated (numFrames + 1);

        for (int frame = 0; frame <= numFrames; ++frame)
        {
            const int start = frame * hop;
            const int end = juce::jmin (start + hop, numSamples);

            double acc = 0.0;
            for (int s = start; s < end; ++s)
            {
                float v = 0.0f;
                for (int c = 0; c < numChannels; ++c)
                    v += buffer.getSample (c, s);
                v /= (float) numChannels;

                acc += (double) v * (double) v;
            }

            energy.add ((float) acc);
        }

        juce::Array<float> onset;
        onset.ensureStorageAllocated (energy.size() - 1);

        for (int i = 1; i < energy.size(); ++i)
            onset.add (juce::jmax (0.0f, energy.getReference (i) - energy.getReference (i - 1)));

        return onset;
    }

    // Saliencia do pente normalizada pelo nivel medio do envelope.
    //
    // O pente puro premiava tempos rapidos: quanto mais curto o periodo, mais
    // dentes caiam dentro do envelope e a media subia por densidade, nao por
    // periodicidade. Dividir pela media do envelope transforma o numero em
    // "quanto este tempo se destaca do fundo", que e a grandeza comparavel
    // entre tempos diferentes - e e isso que desfaz a confusao 3:2 relatada
    // (69,0 x 103,5): os dois sao candidatos na mesma busca, nao um corrigido
    // por um fator testado a parte.
    double combExcess (const juce::Array<float>& onset, double envelopeMean, double framesPerBeat)
    {
        if (framesPerBeat < 1.5 || envelopeMean <= 1e-9)
            return -1.0;

        double total = 0.0;
        int count = 0;

        for (int k = 0; k < 300; ++k)
        {
            const int idx = (int) std::lround (k * framesPerBeat);
            if (idx >= onset.size())
                break;

            // Janela de +-1 frame. Sem ela o pente perde batidas por drift: o
            // indice ideal k*framesPerBeat e arredondado a cada dente, e o onset
            // cai no frame vizinho quando os dois arredondam em sentidos opostos.
            // A perda e sistematica (cresce com k) e derruba justamente o tempo
            // certo, que tem mais dentes do que o candidato errado.
            float best = onset.getReference (juce::jlimit (0, onset.size() - 1, idx - 1));
            best = juce::jmax (best, onset.getReference (idx));
            best = juce::jmax (best, onset.getReference (juce::jlimit (0, onset.size() - 1, idx + 1)));

            total += best;
            ++count;
        }

        if (count < 4)
            return -1.0;

        return (total / (double) count) / envelopeMean - 1.0;
    }

    // Prior de tactus: a percepcao ancora o pulso perto de 120 BPM, com
    // tolerancia larga em escala logaritmica (Moelants; Parncutt). E o desempate
    // de referencia da literatura para a ambiguidade de oitava e de metrica -
    // sem ele, 69,0 e 103,5 pontuam quase igual e o maximo bruto escolhe errado.
    double tactusPrior (double bpm)
    {
        const double x = std::log2 (bpm / 120.0) / 0.9;
        return std::exp (-0.5 * x * x);
    }
}

double TempoAnalyser::estimateBpm (double sampleRate, const juce::AudioBuffer<float>& buffer,
                                   const ProgressFn& onProgress)
{
    if (sampleRate <= 0.0)
        return 0.0;

    constexpr int hop = 1024;
    const juce::Array<float> onset = buildOnsetEnvelope (buffer, hop);

    if (onset.size() < 16)
        return 0.0;

    double sum = 0.0;
    for (int i = 0; i < onset.size(); ++i)
        sum += (double) onset.getReference (i);

    const double envelopeMean = sum / (double) onset.size();
    const double framesPerSecond = sampleRate / (double) hop;

    auto excessAt = [&] (double bpm)
    {
        if (bpm < 40.0 || bpm > 240.0)
            return -1.0;

        return combExcess (onset, envelopeMean, framesPerSecond * 60.0 / bpm);
    };

    // A varredura cobre a ambiguidade metrica INTEIRA - nao so x2 e /2 como
    // antes. 51,8, 69,0, 103,5, 138,0 e 207,0 sao todos candidatos na mesma
    // busca, e o prior de tactus decide entre eles. Passo de 0,5 BPM no bruto,
    // refinado a 0,05 em volta do pico vencedor.
    double coarseBpm = 0.0;
    double coarseWeight = -1.0;

    const int coarseSteps = (int) std::lround ((240.0 - 40.0) / 0.5);

    // Contador inteiro, nao acumulando 0,5 em double. O som acumulativo daria
    // o mesmo resultado hoje - 0,5 e exato em binario - mas "somar 0,5 mil vezes
    // e chegar em 240,0" e exatamente o tipo de suposicao que quebra quando
    // alguem troca o passo por 0,05 ou 0,03. O passo e o indice.
    for (int step = 0; step <= coarseSteps; ++step)
    {
        const double bpm = 40.0 + 0.5 * (double) step;
        const double excess = excessAt (bpm);
        if (excess <= 0.0)
            continue;

        const double weight = excess * tactusPrior (bpm);
        if (weight > coarseWeight)
        {
            coarseWeight = weight;
            coarseBpm = bpm;
        }

        if (onProgress)
            onProgress (0.05f + 0.65f * (float) step / (float) coarseSteps);
    }

    if (coarseBpm <= 0.0)
        return 0.0;

    double bestBpm = coarseBpm;
    double bestWeight = coarseWeight;

    // Mesma razao: 41 passos de 0,05 em torno do pico, contados por indice.
    const int fineSteps = 40;

    for (int step = 0; step <= fineSteps; ++step)
    {
        const double bpm = coarseBpm - 1.0 + 0.05 * (double) step;
        const double excess = excessAt (bpm);
        if (excess <= 0.0)
            continue;

        const double weight = excess * tactusPrior (bpm);
        if (weight > bestWeight)
        {
            bestWeight = weight;
            bestBpm = bpm;
        }

        if (onProgress)
            onProgress (0.70f + 0.28f * (float) step / (float) fineSteps);
    }

    jassert (bestBpm >= 40.0 && bestBpm <= 240.0);

    if (onProgress)
        onProgress (1.0f);

    return std::round (bestBpm * 2.0) * 0.5;
}

int TempoAnalyser::estimateBeatsPerBar (double sampleRate, const juce::AudioBuffer<float>& buffer, double bpm)
{
    if (sampleRate <= 0.0 || bpm <= 0.0)
        return 4;

    constexpr int hop = 1024;
    const juce::Array<float> onset = buildOnsetEnvelope (buffer, hop);

    if (onset.size() < 32)
        return 4;

    const double framesPerSecond = sampleRate / (double) hop;
    const double framesPerBeat = framesPerSecond * 60.0 / bpm;

    if (framesPerBeat < 2.0)
        return 4;

    // Clareza do downbeat para uma hipótese de compasso (N tempos por compasso):
    // alinha grades de compasso em várias fases e mede o destaque do tempo forte
    // (primeira batida) sobre as demais, normalizado pela energia média.
    auto clarityFor = [&onset] (int beatsPerBar, double beatFrames) -> double
    {
        const double barFrames = beatFrames * beatsPerBar;
        const int numBars = (int) std::floor (onset.size() / barFrames);

        if (numBars < 4)
            return -1.0;

        double bestClarity = -1.0;

        // Fase a cada frame. A busca antiga saltava ceil(barFrames/4) - ate ~19
        // frames -, testava so 4 alinhamentos e errava o downbeat verdadeiro, o
        // que fazia um 3/4 nitido cair para 4/4.
        // A fase e um indice de frame, entao conta em int e converte no uso.
        const int phaseCount = (int) std::ceil (barFrames);

        for (int phaseStep = 0; phaseStep < phaseCount; ++phaseStep)
        {
            const double phase = (double) phaseStep;
            std::vector<double> strengths (static_cast<size_t> (beatsPerBar), 0.0);

            for (int bar = 0; bar < numBars; ++bar)
            {
                const double base = phase + (double) bar * barFrames;

                for (int k = 0; k < beatsPerBar; ++k)
                {
                    const int idx = (int) std::lround (base + (double) k * beatFrames);
                    if (idx >= 0 && idx < onset.size())
                        strengths[static_cast<size_t> (k)] += onset.getReference (idx);
                }
            }

            double mean = 0.0;
            double maxValue = 0.0;
            double minValue = std::numeric_limits<double>::max();

            for (double value : strengths)
            {
                mean += value;
                maxValue = juce::jmax (maxValue, value);
                minValue = juce::jmin (minValue, value);
            }

            mean /= (double) beatsPerBar;

            if (mean <= 1e-9)
                continue;

            double otherMax = 0.0;
            for (size_t k = 1; k < strengths.size(); ++k)
                otherMax = juce::jmax (otherMax, strengths[k]);

            const double downbeatEdge = (strengths.front() - otherMax) / mean;
            const double spread       = (maxValue - minValue) / mean;
            const double clarity      = downbeatEdge + 0.5 * spread;

            bestClarity = juce::jmax (bestClarity, clarity);
        }

        return bestClarity;
    };

    const double clarity3 = clarityFor (3, framesPerBeat);
    const double clarity4 = clarityFor (4, framesPerBeat);

    // Padrão quaternário; adota o ternário somente com evidência clara.
    if (clarity3 > 0.0 && clarity3 > clarity4 * 1.25 + 0.1)
        return 3;

    return 4;
}

double TempoAnalyser::estimateTuningCents (double sampleRate, const juce::AudioBuffer<float>& buffer,
                                           const ProgressFn& onProgress)
{
    constexpr int fftOrder  = 12;
    constexpr int fftSize   = 1 << fftOrder;   // 4096
    constexpr int hopSize   = fftSize / 2;     // 2048
    constexpr int halfRange = 60;              // faixa +-60 cents
    constexpr int histSize  = halfRange * 2 + 1;

    if (sampleRate <= 0.0 || buffer.getNumChannels() < 1 || buffer.getNumSamples() < fftSize)
        return 0.0;

    float globalPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        globalPeak = juce::jmax (globalPeak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));

    if (globalPeak <= 1e-6f)
        return 0.0;

    juce::dsp::FFT fft (fftOrder);

    // Janela de Hann. twoPi e double, entao a fase e double e so o resultado
    // final e guardado em float. A conversao e intencional (o destino e
    // float por escolha) e fica explicita: sem o static_cast o MSVC emite
    // C4244 nesta linha, e e a unica atribuicao do projeto em que um double
    // chega num float sem cast.
    std::vector<float> window (static_cast<size_t> (fftSize));
    for (int i = 0; i < fftSize; ++i)
    {
        const double phase = juce::MathConstants<double>::twoPi * i / (fftSize - 1);
        window[static_cast<size_t> (i)] = static_cast<float> (0.5 - 0.5 * std::cos (phase));
    }

    std::vector<float> spectrum (static_cast<size_t> (fftSize * 2), 0.0f);
    std::vector<int> histogram (static_cast<size_t> (histSize), 0);

    const double hzPerBin = sampleRate / fftSize;
    const int minBin = juce::jmax (1, (int) std::lround (80.0 / hzPerBin));
    const int maxBin = juce::jmin (fftSize / 2 - 1, (int) std::lround (2500.0 / hzPerBin));

    const auto magnitudeAt = [&spectrum] (int bin)
    {
        return std::hypot (spectrum[static_cast<size_t> (bin * 2)],
                           spectrum[static_cast<size_t> (bin * 2 + 1)]);
    };

    const int framesPerChannel = (buffer.getNumSamples() - fftSize) / hopSize + 1;
    const double totalFrames = (double) framesPerChannel * (double) buffer.getNumChannels();
    double doneFrames = 0.0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* data = buffer.getReadPointer (ch);
        const int numSamples = buffer.getNumSamples();

        for (int offset = 0; offset <= numSamples - fftSize; offset += hopSize)
        {
            if (onProgress && totalFrames > 0.0)
                onProgress ((float) (doneFrames / totalFrames));
            ++doneFrames;
            std::fill (spectrum.begin(), spectrum.end(), 0.0f);

            double energy = 0.0;
            for (int i = 0; i < fftSize; ++i)
            {
                const float x = data[offset + i];
                spectrum[static_cast<size_t> (i * 2)] = x * window[static_cast<size_t> (i)];
                energy += (double) x * (double) x;
            }

            const double rms = std::sqrt (energy / fftSize);
            if (rms < (double) globalPeak * 0.02)
                continue;

            fft.performRealOnlyForwardTransform (spectrum.data(), false);

            float frameMax = 0.0f;
            for (int k = minBin; k <= maxBin; ++k)
                frameMax = juce::jmax (frameMax, magnitudeAt (k));

            if (frameMax <= 1e-6f)
                continue;

            for (int k = minBin; k <= maxBin; ++k)
            {
                const float mag = magnitudeAt (k);

                if (mag < frameMax * 0.05f)
                    continue;

                const bool leftOk  = k - 1 < minBin || magnitudeAt (k - 1) <= mag;
                const bool rightOk = k + 1 > maxBin || magnitudeAt (k + 1) <= mag;
                if (! leftOk || ! rightOk)
                    continue;

                const double freq   = (double) k * hzPerBin;
                const double midi   = 69.0 + 12.0 * std::log2 (freq / 440.0);
                const double nearest = 440.0 * std::pow (2.0, (std::round (midi) - 69.0) / 12.0);
                const double cents  = 1200.0 * std::log2 (freq / nearest);

                const int idx = (int) std::lround (cents) + halfRange;
                if (idx >= 0 && idx < histSize)
                    ++histogram[static_cast<size_t> (idx)];
            }
        }
    }

    std::vector<int> smooth (static_cast<size_t> (histSize), 0);
    int total = 0;
    for (int i = 0; i < histSize; ++i)
    {
        total += histogram[static_cast<size_t> (i)];
        smooth[static_cast<size_t> (i)] = (histogram[static_cast<size_t> (i - 1 < 0 ? 0 : i - 1)]
                                        + histogram[static_cast<size_t> (i)]
                                        + histogram[static_cast<size_t> (i + 1 >= histSize ? i : i + 1)]) / 3;
    }

    if (total < 40)
        return 0.0;

    int best = halfRange;
    int bestCount = 0;
    for (int i = 0; i < histSize; ++i)
    {
        if (smooth[static_cast<size_t> (i)] > bestCount)
        {
            bestCount = smooth[static_cast<size_t> (i)];
            best = i;
        }
    }

    if (bestCount < total / 8)
        return 0.0;

    if (onProgress)
        onProgress (1.0f);

    return (double) (best - halfRange);
}