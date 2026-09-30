#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <memory>
#include <mutex>

// Gera a impressao digital acustica (Chromaprint) de um arquivo de audio, para
// consulta ao AcoustID. Nada aqui presume que exista chave de API: a impressao
// digital e calculada localmente e vale mesmo sem rede. A camada de rede, quando
// existir, consome este resultado (Bloco 6.2 - falha aberta: sem chave o plugin
// continua funcionando, so sem identificacao).
namespace Fingerprint
{
    enum class State
    {
        idle,        ///< nada solicitado
        running,     ///< em curso no worker
        ready,       ///< concluido com sucesso
        failed,      ///< erro de leitura/algoritmo
        cancelled    ///< cancelado pelo usuario ou por novo pedido
    };

    // Motivo da falha, separado de `message` de propósito.
    //
    // `message` é diagnóstico em português para log de desenvolvimento e não
    // deve ser exibido: a interface tem três idiomas e não pode sair texto fixo.
    // A UI traduz este enum via Text::t, que é a regra do projeto.
    enum class Failure
    {
        none,
        fileNotFound,
        unsupportedFormat,
        noChannels,
        sampleRateTooLow,
        tooShort,
        readFailed,
        algorithmFailed,
        emptyFingerprint
    };

    struct Result
    {
        State state = State::idle;
        Failure failure = Failure::none;
        juce::String fingerprint;      ///< base64, como o AcoustID espera
        double durationSeconds = 0.0;  ///< derivado da contagem de amostras
        double sourceSampleRate = 0.0; ///< taxa nativa do arquivo
        int sourceChannels = 0;
        int64_t sampleCount = 0;
        juce::String message;          ///< diagnóstico, não é traduzido

        bool isUsable() const noexcept { return state == State::ready && fingerprint.isNotEmpty(); }
    };

    // Abaixo deste tempo o algoritmo nao produz uma impressao digital confiavel:
    // a janela de analise e maior que o proprio clipe. Rejeitar aqui e melhor do
    // que consultar o AcoustID e receber "not found" sem explicacao.
    inline constexpr double minimumUsefulSeconds = 10.0;

    // Escala o fator de conversao para PCM int16. 32767 e o maior valor com
    // sinal, e o mesmo que o Chromaprint espera: o PCM precisa ter a mesma
    // escala do fpcalc, senao a impressao digital nao bate com a deReference.
    inline constexpr float pcm16FullScale = 32767.0f;

    // Converte uma amostra ja reduzida a mono em PCM int16.
    //
    // Existe como funcao isolada, e nao como expressao dentro do laco, por causa
    // de uma armadilha de C++ que aqui custou um bug real e silencioso:
    // `(int16_t) x * 32767.0f` liga o cast ANTES da multiplicacao, porque o
    // cast liga mais forte que `*`. O sinal e truncado para {-1, 0, +1} e so
    // depois escalado, de modo que o Chromaprint recebe um PCM de tres niveis
    // em vez do audio. A impressao digital continua valida em base64 - apenas
    // nao bate com a do fpcalc, e o sintoma aparece como "o AcoustID nao acha a
    // gravacao", sem crash nem log que aponte a causa. Um teste que so verifique
    // "o resultado e base64" nao enxerga isso.
    //
    // Por isso o cast fica no fim, visivel, e o caminho e testavel.
    int16_t toPcm16 (float mono) noexcept;

    // Nucleo sincrono, separado do threading para poder ser testado sem thread.
    //
    // `cancel` pode ser nulo; quando nao nulo, e consultado a cada bloco para
    // que um arquivo longo possa ser interrompido sem esperar o fim.
    //
    // Decisao de projeto: este nucleo le o arquivo do zero em vez de reaproveitar
    // o buffer ja decodificado pelo FilePlayer. Custa uma segunda decodificacao
    // (centenas de ms em arquivo grande) e ganha: independencia do ciclo de vida
    // do player, nenhuma chance de a impressao digital sair de um buffer ja
    // transformado, e a possibilidade de gerar a impressao sem nem carregar o
    // arquivo para reproducao. Se a latencia virar problema, o alvo e passar o
    // shared_ptr do buffer do player em vez do File.
    Result compute (const juce::File& file, const std::atomic<bool>* cancel);

    // Envoltoria assincrona. A message thread so dispara e coleta: nunca espera
    // o trabalho (Bloco 2.5). O resultado e publicado por troca atomica de
    // shared_ptr, o mesmo mecanismo usado pelo player.
    class Worker
    {
    public:
        Worker();
        ~Worker();

        Worker (const Worker&) = delete;
        Worker& operator= (const Worker&) = delete;

        // Inicia o calculo. Se ja houver um em curso, ele e cancelado e o novo
        // pedido toma a vez - carregar outro arquivo nao deve exigir esperar o
        // fingerprint anterior terminar.
        void start (const juce::File& file);

        void cancel();

        // Coleta o estado atual. Seguro de chamar em qualquer momento; se ainda
        // nao houver resultado, devolve o anterior.
        Result poll() const;

        bool isBusy() const noexcept;

    private:
        // Mutex curto, usado apenas dentro do job de background e no start. Nao
        // ha bloqueio na thread de audio: o unico caminho que a thread de audio
        // toca e poll(), que le o shared_ptr publicado sem-trava.
        std::mutex publishMutex;
        int64_t generation = 0;

        juce::ThreadPool pool { 1 };
        std::shared_ptr<Result> published;

        // Um flag por pedido, e nao um flag compartilhado: assim um job antigo
        // nao pode ser "des-cancelado" por causa do reset que um novo start faz.
        std::shared_ptr<std::atomic<bool>> cancelFlag;
        std::atomic<bool> busy { false };
    };
}
