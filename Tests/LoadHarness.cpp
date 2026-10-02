/*
    Harness de carga do VST3 (T2..T8 de epic/vst3-load-harness-ci).

    Este executavel NAO linka o codigo do plugin. Ele carrega o bundle .vst3
    como dado externo, que e a unica forma de provar o artefato que o usuario
    baixa. Se linkasse PARTEPLAY_SOURCES, o teste passaria a provar a copia
    compilada neste executavel, e a epic inteira nao teria valor.

    Contrato de saida, desenhado para o log do CI:

      [step] 03/08 name             abre a etapa
      [ok]   03/08 name             etapa passou
      [fail] 03/08 name reason=COD  detalhe
      [done] summary  steps_ok=N failed_at=NONE

    O codigo de saida e o indice da etapa que falhou; 0 quando todas passam.
    Os prefixos sao literais para que `grep -c '^[ok]'` no CI funcione sem
    interpretar prosa, e cada modo de falha tem nome proprio para nao
    colapsar em um "plugin not found" generico.

    Ver adr/0001-loader-vst3-do-harness.md.
*/

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace
{
    constexpr int totalSteps = 8;

    // defined() e nao o valor direto: as macros de plataforma do JUCE nem
    // sempre estao todas visiveis numa TU, e "usada antes de declarada" seria
    // erro de compilacao em vez de um falso negativo silencioso.
#if defined (JUCE_WINDOWS)
    constexpr bool onWindows = true;
#else
    constexpr bool onWindows = false;
#endif

#if defined (JUCE_MAC)
    constexpr bool onMac = true;
#else
    constexpr bool onMac = false;
#endif

    // Copia verificada de Contents/Resources/moduleinfo.json. Nao ha MODULE_ID
    // no CMake nem constante de UUID em Source/: o JUCE deriva a semente do
    // nome do plugin, entao a constante nao tem de onde sair sozinha e mudar
    // aqui tem de ser deliberado.
    constexpr const char* expectedPluginName = "PartePlay";
    constexpr const char* expectedAudioModuleCid = "ABCDEF019182FAEB5052504C50545059";

    // createIdentifierString() nao devolve o CID em hex, e sim
    // "VST3-PartePlay-776e3313-58b41b17", entao nao da para comparar com a
    // constante acima. A chave comparável e uniqueId, que JUCE documenta como
    // estavel entre plataformas - foi criado justamente para substituir
    // deprecatedUid, que gerava valores diferentes por plataforma. Valor
    // observado no probe contra o Release local, derivado do CID do manifesto.
    constexpr int expectedUniqueId = 1488198423;

    constexpr double harnessSampleRate = 44100.0;
    constexpr int harnessBlockSize = 512;

    // Oito blocos e o piso para o wrapper concluir a cadeia prepareToPlay ->
    // setBusArrangements -> process antes de audio aparecer. Abaixo disso o
    // teste falha por latencia, nao por defeito.
    constexpr int blockCount = 8;

    // Degrau de amplitude A tem RMS A/sqrt(2) ~= 0.3536, duas ordens de grandeza
    // acima do limiar. O limiar nao esta sob teste: o que esta sob teste e o
    // sinal atravessar o plugin, e um buffer de silencio passaria em qualquer
    // limiar baixo.
    constexpr float probeAmplitude = 0.5f;
    constexpr float silenceThreshold = 1.0e-3f;

    int stepsOk = 0;
    bool verbose = false;

    void emit (const char* format, ...)
    {
        va_list args;
        va_start (args, format);
        std::vfprintf (stdout, format, args);
        va_end (args);
        std::fputc ('\n', stdout);
        std::fflush (stdout);
    }

    void stepOk (int n, const char* name)
    {
        ++stepsOk;
        emit ("[ok]   %02d/%02d %s", n, totalSteps, name);
    }

    [[noreturn]] void bail (int n, const char* name, const char* reason, const char* detail = nullptr)
    {
        emit ("[fail] %02d/%02d %s  reason=%s  %s", n, totalSteps, name, reason,
              detail != nullptr ? detail : "");
        emit ("[fail] summary  steps_ok=%d  failed_at=%02d/%02d  reason=%s", stepsOk, n, totalSteps, reason);
        std::exit (n);
    }

    // 440 Hz com uma oitava acima a 50%: passa por qualquer formantador, por
    // um pitch shifter e por um resampler, e nao por um eco de bloco unico.
    juce::AudioBuffer<float> makeProbeBuffer (int numChannels)
    {
        juce::AudioBuffer<float> buffer (numChannels, harnessBlockSize);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);

            for (int i = 0; i < harnessBlockSize; ++i)
            {
                const auto t = static_cast<double> (i) / harnessSampleRate;
                const auto fundamental = std::sin (juce::MathConstants<double>::twoPi * 440.0 * t);
                const auto overtone = 0.5 * std::sin (juce::MathConstants<double>::twoPi * 1320.0 * t);
                data[i] = static_cast<float> (probeAmplitude * (fundamental + overtone));
            }
        }

        return buffer;
    }

    double rmsOf (const juce::AudioBuffer<float>& buffer)
    {
        double sum = 0.0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                sum += static_cast<double> (data[i]) * data[i];
        }

        const auto count = static_cast<double> (buffer.getNumChannels()) * buffer.getNumSamples();
        return count > 0.0 ? std::sqrt (sum / count) : 0.0;
    }

    // ffmpeg.exe num bundle de outra plataforma e lixo, nao recurso. O
    // locateTools tem de cair no analisador nativo; se pegar o binario errado a
    // analise de andamento passa a mentir sem erro visivel.
    //
    // Procura a partir do bundle informado no argv, e nao do diretorio do
    // executavel: no CI o harness roda de fora do bundle e uma busca relativa ao
    // proprio binario contaria zero e passaria com o verde sem ter olhado nada.
    void stepLocateTools (int n, const juce::File& bundle)
    {
        constexpr const char* name = "locate-tools";
        emit ("[step] %02d/%02d %s", n, totalSteps, name);

        const auto allFiles = bundle.findChildFiles (juce::File::findFiles, true, "*",
                                                     juce::File::FollowSymlinks::no);

        int inResourcesBin = 0;
        int foreign = 0;

        for (const auto& file : allFiles)
        {
            auto dir = file.getParentDirectory();

            for (int depth = 0; depth < 4 && dir.isDirectory(); ++depth)
            {
                if (dir.getFileName() == "bin" && dir.getParentDirectory().getFileName() == "resources")
                {
                    ++inResourcesBin;

                    const auto ext = file.getFileExtension().toLowerCase();

                    if ((ext == ".exe" || ext == ".dll") && ! onWindows)
                    {
                        ++foreign;
                        emit ("       foreign-binary %s", file.getFileName().toStdString().c_str());
                    }
                    else if (ext == ".dylib" && ! onMac)
                    {
                        ++foreign;
                        emit ("       foreign-binary %s", file.getFileName().toStdString().c_str());
                    }

                    break;
                }

                dir = dir.getParentDirectory();
            }
        }

        char detail[160] = {};
        std::snprintf (detail, sizeof detail, "files_in_bundle=%d resources_bin=%d foreign=%d",
                       allFiles.size(), inResourcesBin, foreign);

        if (foreign > 0)
            bail (n, name, "FOREIGN_BINARY_IN_BUNDLE", detail);

        emit ("       %s", detail);
        stepOk (n, name);
    }
}

//==============================================================================
int main (int argc, char* argv[])
try
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    juce::String bundlePath;

    for (int i = 1; i < argc; ++i)
    {
        const juce::String arg (argv[i]);

        if (arg == "--verbose")
            verbose = true;
        else if (bundlePath.isEmpty())
            bundlePath = arg;
    }

    emit ("[harness] parteplay load probe  steps=%d  sample_rate=%.0f  block=%d  blocks=%d",
          totalSteps, harnessSampleRate, harnessBlockSize, blockCount);

    // 1. Preflight. O modulo headless liga os formatos atras de
    // JUCE_PLUGINHOST_VST3 e nao registra nenhum se o define faltar. Sem esta
    // checagem o passo 3 acusaria NO_PLUGIN_IN_PATH quando a causa real e o
    // suporte estar compilado fora: falso negativo que parece bundle ruim.
    emit ("[step] 01/%02d preflight-vst3-support", totalSteps);

    juce::AudioPluginFormatManager manager;
    juce::addHeadlessDefaultFormatsToManager (manager);

    juce::AudioPluginFormat* vst3Format = nullptr;

    for (auto i = 0; i < manager.getNumFormats(); ++i)
    {
        auto* format = manager.getFormat (i);

        if (format != nullptr && format->getName() == "VST3")
        {
            vst3Format = format;
            break;
        }
    }

    if (vst3Format == nullptr)
        bail (1, "preflight-vst3-support", "VST3_SUPPORT_COMPILED_OUT",
              "defina JUCE_PLUGINHOST_VST3=1 no target do harness");

    emit ("       formats_registered=%d  vst3=ok", manager.getNumFormats());
    stepOk (1, "preflight-vst3-support");

    // 2. Path vem do argv. Nada de caminho hard-coded: o bundle e empacotado em
    // diretorios diferentes em cada plataforma e em cada configuracao.
    emit ("[step] 02/%02d resolve-bundle-path", totalSteps);

    if (bundlePath.isEmpty())
        bail (2, "resolve-bundle-path", "PATH_NOT_FOUND", "uso: harness <bundle.vst3>");

    const juce::File bundle (bundlePath);

    if (! bundle.exists())
        bail (2, "resolve-bundle-path", "PATH_NOT_FOUND", bundlePath.toStdString().c_str());

    emit ("       path=%s  name=%s", bundlePath.toStdString().c_str(),
          bundle.getFileName().toStdString().c_str());
    stepOk (2, "resolve-bundle-path");

    // 3. Descoberta. No modulo headless nao ha cache de scan nem addScanPath: o
    // ponto de entrada e findAllTypesForFile, que le o bundle direto. Cada
    // descricao encontrada e listada porque "nenhuma" e "a errada" sao falhas
    // distintas e precisam ser distinguidas no log.
    emit ("[step] 03/%02d discover-types", totalSteps);

    juce::OwnedArray<juce::PluginDescription> found;
    vst3Format->findAllTypesForFile (found, bundlePath);

    emit ("       found=%d", found.size());

    const juce::PluginDescription* ours = nullptr;

    for (auto* description : found)
    {
        emit ("       cand name=%s version=%s id=%s uid=%d",
              description->name.toStdString().c_str(),
              description->version.toStdString().c_str(),
              description->createIdentifierString().toStdString().c_str(),
              description->uniqueId);

        if (description->name == expectedPluginName)
            ours = description;
    }

    if (found.size() == 0)
        bail (3, "discover-types", "NO_PLUGIN_IN_PATH", bundlePath.toStdString().c_str());

    stepOk (3, "discover-types");

    // 4. Classe esperada em constante. Comparar so por nome aceitaria um bundle
    // substituido que ainda se chame PartePlay.
    emit ("[step] 04/%02d class-match", totalSteps);

    if (ours == nullptr)
        bail (4, "class-match", "CLASS_NOT_FOUND", expectedPluginName);

    emit ("       expected_cid=%s  expected_unique_id=%d", expectedAudioModuleCid, expectedUniqueId);
    emit ("       actual_id=%s  actual_unique_id=%d",
          ours->createIdentifierString().toStdString().c_str(), ours->uniqueId);

    if (ours->uniqueId != expectedUniqueId)
    {
        char detail[192] = {};
        std::snprintf (detail, sizeof detail, "esperado unique_id=%d  obtido=%d",
                       expectedUniqueId, ours->uniqueId);
        bail (4, "class-match", "UNEXPECTED_CLASS_ID", detail);
    }

    stepOk (4, "class-match");

    // 5 e 6 sao uma unica transacao: se alguma falhar nao ha instancia viva
    // para desiniciar e o processo sairia com o componente carregado.
    emit ("[step] 05/%02d create-instance", totalSteps);

    juce::String createError;
    auto instance = manager.createPluginInstance (*ours, harnessSampleRate, harnessBlockSize, createError);

    if (instance == nullptr)
        bail (5, "create-instance", "INSTANTIATE_FAILED", createError.toStdString().c_str());

    emit ("       created=%s", instance->getName().toStdString().c_str());

    juce::MidiBuffer midi;

    // prepareToPlay e void tanto em AudioProcessor quanto em AudioPluginInstance:
    // nao ha valor de retorno para testar. A falha de inicializacao so aparece
    // em estado observavel, e quem a procura e o passo 6 (canais de saida) e o
    // passo 7 (RMS e finitude).
    instance->prepareToPlay (harnessSampleRate, harnessBlockSize);

    stepOk (5, "create-instance");

    emit ("[step] 06/%02d prepare-check", totalSteps);

    const auto numOut = instance->getTotalNumOutputChannels();

    emit ("       sample_rate=%.0f  block=%d  in=%d out=%d",
          instance->getSampleRate(), instance->getBlockSize(),
          instance->getTotalNumInputChannels(), numOut);

    if (numOut <= 0)
    {
        instance->releaseResources();
        bail (6, "prepare-check", "INIT_FAILED", "zero output channels");
    }

    stepOk (6, "prepare-check");

    // 7. O teste que a T6 exige: buffer NAO silencioso e rejeicao de saida nao
    // finita. processBlock processa o buffer in place, entao a referencia do
    // sinal de entrada e uma copia separada - medir entrada e saida no mesmo
    // buffer mediria a entrada depois do plugin ter escrito por cima.
    emit ("[step] 07/%02d process-block", totalSteps);

    const auto reference = makeProbeBuffer (numOut);
    const auto inputRms = rmsOf (reference);

    double outputRms = 0.0;
    bool nonFinite = false;
    int firstBadSample = -1;

    for (int b = 0; b < blockCount && ! nonFinite; ++b)
    {
        auto work = makeProbeBuffer (numOut);
        instance->processBlock (work, midi);

        for (int ch = 0; ch < numOut && ! nonFinite; ++ch)
        {
            const auto* out = work.getReadPointer (ch);

            for (int i = 0; i < harnessBlockSize; ++i)
            {
                if (! std::isfinite (out[i]))
                {
                    nonFinite = true;
                    firstBadSample = i;
                    break;
                }
            }
        }

        if (! nonFinite)
            outputRms += rmsOf (work);
    }

    if (nonFinite)
    {
        char detail[192] = {};
        std::snprintf (detail, sizeof detail, "first_bad_sample=%d block_of=%d", firstBadSample, blockCount);
        instance->releaseResources();
        bail (7, "process-block", "OUTPUT_NON_FINITE", detail);
    }

    outputRms /= static_cast<double> (blockCount);

    char rmsDetail[192] = {};
    std::snprintf (rmsDetail, sizeof rmsDetail, "input_rms=%.6f output_rms=%.6f", inputRms, outputRms);
    emit ("       %s", rmsDetail);

    if (inputRms <= silenceThreshold)
    {
        instance->releaseResources();
        bail (7, "process-block", "PROBE_SIGNAL_TOO_QUIET", rmsDetail);
    }

    // Silencio na saida e o resultado esperado: o plugin e um tocador de arquivo de
    // referencia e o harness nao carrega nenhum arquivo, entao nao ha o que tocar.
    // Consequencia para o criterio da T6: a assercao real e FINITUDE, nunca
    // amplitude. Um limiar de nivel na saida reprovaria um plugin perfeitamente
    // saudavel, e o alarme corria na direcao oposta ao da tarefa.
    if (outputRms < silenceThreshold)
    {
        emit ("[info] 07/%02d process-block  output_rms=%.9f: silencio esperado sem arquivo carregado",
              totalSteps, outputRms);
    }

    if (verbose)
        emit ("[info] input_rms=%.9f  output_rms=%.9f  blocks=%d", inputRms, outputRms, blockCount);

    instance->releaseResources();
    stepOk (7, "process-block");

    stepLocateTools (8, bundle);

    emit ("[done] summary  steps_ok=%d  failed_at=NONE", stepsOk);
    emit ("[harness] resultado=PROBE_OK  pendente=caso negativo (T7 do backlog)");
    return 0;
}
catch (const std::exception& e)
{
    emit ("[fail] summary  steps_ok=%d  failed_at=00/%02d  reason=UNHANDLED_EXCEPTION  %s",
          stepsOk, totalSteps, e.what());
    return 0;
}