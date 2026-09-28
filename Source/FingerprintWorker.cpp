#include "FingerprintWorker.h"

#include <chromaprint.h>

#include <cstring>
#include <vector>

namespace Fingerprint
{
    // O Chromaprint aceita 1 ou 2 canais, e o AcoustID espera o fingerprint no
    // formato do fpcalc, que e mono. Qualquer coisa acima de 2 canais (5.1, por
    // exemplo) e reduzida para mono aqui; caso contrario a chamada a
    // chromaprint_start seria rejeitada e nao haveria fingerprint algum.
    static constexpr int channelsToChromaprint = 1;

    // Blocos de leitura: pequeno o bastante para que o cancelamento seja
    // percebido logo, grande o bastante para nao pagar o custo por bloco em
    // arquivo longo.
    static constexpr int readBlockSize = 4096;

    // O Chromaprint recusa_amostras abaixo deste valor (kMinSampleRate = 1000
    // em audio_processor.h). Arquivos de tauxa muito baixa sao rejeitados com
    // mensagem clara em vez de falhar dentro da biblioteca sem informacao.
    static constexpr int minimumAcceptableSampleRate = 1000;

    static bool isCancelled (const std::atomic<bool>* cancel) noexcept
    {
        return cancel != nullptr && cancel->load();
    }

    Result compute (const juce::File& file, const std::atomic<bool>* cancel)
    {
        Result result;

        if (isCancelled (cancel))
        {
            result.state = State::cancelled;
            return result;
        }

        if (! file.existsAsFile())
        {
            result.state = State::failed;
            result.failure = Failure::fileNotFound;
            result.message = "arquivo nao encontrado";
            return result;
        }

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
        if (reader == nullptr)
        {
            result.state = State::failed;
            result.failure = Failure::unsupportedFormat;
            result.message = "formato de audio nao suportado";
            return result;
        }

        const auto sampleRate = reader->sampleRate;
        const auto numChannels = static_cast<int> (reader->numChannels);
        const auto totalSamples = static_cast<int64_t> (reader->lengthInSamples);

        result.sourceSampleRate = sampleRate;
        result.sourceChannels = numChannels;
        result.sampleCount = totalSamples;

        if (numChannels <= 0)
        {
            result.state = State::failed;
            result.failure = Failure::noChannels;
            result.message = "arquivo sem canais de audio";
            return result;
        }

        // A duracao vem da contagem de amostras, nunca de
        // chromaprint_get_item_duration_ms: aquele valor e uma constante do
        // algoritmo (o tamanho da janela de analise), nao a duracao da musica,
        // e usalo faria toda consulta ao AcoustID sair com o mesmo numero.
        if (totalSamples > 0 && sampleRate > 0.0)
            result.durationSeconds = (double) totalSamples / sampleRate;

        if (sampleRate <= (double) minimumAcceptableSampleRate)
        {
            result.state = State::failed;
            result.failure = Failure::sampleRateTooLow;
            result.message = "taxa de amostragem abaixo do minimo do algoritmo";
            return result;
        }

        if (result.durationSeconds < minimumUsefulSeconds)
        {
            result.state = State::failed;
            result.failure = Failure::tooShort;
            result.message = "audio curto demais para identificacao acustica";
            return result;
        }

        auto* ctx = chromaprint_new (CHROMAPRINT_ALGORITHM_DEFAULT);
        if (ctx == nullptr)
        {
            result.state = State::failed;
            result.failure = Failure::algorithmFailed;
            result.message = "nao foi possivel iniciar o algoritmo";
            return result;
        }

        // Alimentar na taxa NATIVA do arquivo, e nao reamostrar para 11025 Hz
        // aqui. O Chromaprint compila com USE_INTERNAL_AVRESAMPLE e reamostra
        // para a taxa interna sozinho (AudioProcessor::Reset). Reamostrar antes
        // produziria uma impressao digital diferente da que o fpcalc gera, e o
        // AcoustID nao encontraria a gravacao.
        //
        // Atencao: a API do Chromaprint devolve 1 em caso de SUCESSO e 0 em caso
        // de falha (FAIL_IF). Condicionar a == 0 inverte o fluxo.
        bool ok = chromaprint_start (ctx, static_cast<int> (sampleRate), channelsToChromaprint) != 0;

        if (ok)
        {
            // Bloco unico de canais, com todos os canais do arquivo reduzidos
            // para mono por media. Um buffer separado por bloco evita manter o
            // arquivo inteiro em memoria so para a media.
            juce::AudioBuffer<float> block (numChannels, readBlockSize);
            std::vector<int16_t> pcm ((size_t) readBlockSize);

            for (int64_t position = 0; ok && position < totalSamples; position += readBlockSize)
            {
                if (isCancelled (cancel))
                {
                    result.state = State::cancelled;
                    break;
                }

                const auto wanted = (int) jmin ((int64_t) readBlockSize, totalSamples - position);
                if (! reader->read (&block, 0, wanted, position, true, true))
                {
                    result.state = State::failed;
                    result.failure = Failure::readFailed;
                    result.message = "falha ao ler amostras";
                    break;
                }

                // Media de todos os canais: um sinal percussivo qualquer em
                // multicanal continua audivel no mono, e e o que o AcoustID
                // indexa.
                for (int i = 0; i < wanted; ++i)
                {
                    float sum = 0.0f;
                    for (int c = 0; c < numChannels; ++c)
                        sum += block.getSample (c, i);

                    const auto mono = sum / (float) numChannels;

                    // jlimit antes da conversao: soma de canais pode passar de
                    // 1.0 e estouraria o int16.
                    pcm[(size_t) i] = (int16_t) juce::jlimit (-1.0, 1.0, (double) mono) * 32767.0f;
                }

                if (chromaprint_feed (ctx, pcm.data(), wanted) == 0)
                {
                    result.state = State::failed;
                    result.failure = Failure::algorithmFailed;
                    result.message = "falha ao processar o audio";
                    ok = false;
                }
            }
        }

        if (result.state == State::failed || result.state == State::cancelled)
        {
            chromaprint_free (ctx);
            return result;
        }

        if (! ok)
        {
            chromaprint_free (ctx);
            result.state = State::failed;
            result.failure = Failure::algorithmFailed;
            result.message = "falha ao processar o audio";
            return result;
        }

        chromaprint_finish (ctx);

        char* encoded = nullptr;
        if (chromaprint_get_fingerprint (ctx, &encoded) == 0 || encoded == nullptr)
        {
            chromaprint_dealloc (encoded);
            chromaprint_free (ctx);
            result.state = State::failed;
            result.failure = Failure::algorithmFailed;
            result.message = "falha ao extrair a impressao digital";
            return result;
        }

        result.fingerprint = juce::String::fromUTF8 (encoded, (int) std::strlen (encoded));
        chromaprint_dealloc (encoded);
        chromaprint_free (ctx);

        if (result.fingerprint.isEmpty())
        {
            result.state = State::failed;
            result.failure = Failure::emptyFingerprint;
            result.message = "impressao digital vazia";
            return result;
        }

        result.state = State::ready;
        result.message.clear();
        return result;
    }

    // ---------------------------------------------------------------- Worker

    Worker::Worker() = default;

    Worker::~Worker()
    {
        cancel();
        // Espera o job corrente terminar antes de este objeto morrer, senao o
        // job ainda leria published e as flags depois da destruicao. Este e o
        // unico ponto em que a message thread espera, e so pelo fim do trabalho
        // ja em andamento, nunca pelo calculo em si. Timeout -1 = ate terminar.
        //
        // O job e um std::function e ignora signalJobShouldExit, entao
        // removeAllJobs nao aborta o calculo no meio: o cancelamento efetivo
        // vem do flag, que o compute consulta a cada bloco.
        pool.removeAllJobs (true, -1);
    }

    void Worker::start (const juce::File& file)
    {
        // Derruba o pedido anterior, se houver.
        cancel();

        int64_t myGeneration = 0;
        auto flag = std::make_shared<std::atomic<bool>> (false);

        auto running = std::make_shared<Result>();
        running->state = State::running;

        {
            const std::lock_guard<std::mutex> lock (publishMutex);
            myGeneration = ++generation;
            cancelFlag = flag;
            std::atomic_store (&published, running);
        }

        busy.store (true);

        pool.addJob ([this, file, flag, myGeneration]
        {
            auto computed = compute (file, flag.get());

            const std::lock_guard<std::mutex> lock (publishMutex);

            // Pedido obsoleto: nao publica, e nao mexe em busy, porque o start
            // do pedido novo ja o marcou como ocupado.
            if (myGeneration != generation)
                return;

            std::atomic_store (&published, std::make_shared<Result> (std::move (computed)));
            busy.store (false);
        });
    }

    void Worker::cancel()
    {
        auto flag = std::atomic_load (&cancelFlag);
        if (flag != nullptr)
            flag->store (true);

        const std::lock_guard<std::mutex> lock (publishMutex);
        auto latest = std::atomic_load (&published);

        if (latest != nullptr && latest->state == State::running)
        {
            auto cancelled = std::make_shared<Result>();
            cancelled->state = State::cancelled;
            std::atomic_store (&published, cancelled);
        }

        busy.store (false);
    }

    Result Worker::poll() const
    {
        auto latest = std::atomic_load (&published);
        if (latest == nullptr)
            return Result();

        return *latest;
    }

    bool Worker::isBusy() const noexcept
    {
        return busy.load();
    }
}
