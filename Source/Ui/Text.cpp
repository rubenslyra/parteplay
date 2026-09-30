#include "Text.h"

#include <cmath>
#include <cstring>

namespace
{
    struct Translation
    {
        const char* pt; // também é a chave
        const char* enGB;
        const char* enUS;
        const char* esES;
    };

    // Fonte única de textos da interface. Adicionar novas strings aqui, não em
    // literal solto no código de UI. A chave é sempre o texto pt-BR; a coluna
    // en-GB carrega a grafia britânica (analysing, cancelled) e a en-US a
    // americana (analyzing, canceled), porque profissionais de áudio dos dois
    // países escrevem diferente — não existe um "English" único. O es-ES carrega
    // a acentuação completa (válida, vacía, aún).
    const Translation translations[] =
    {
        // Cabeçalho
        { "PartePlay",                                  "PartePlay",                              "PartePlay",                              "PartePlay" },
        { "ESPAÇO DE TRABALHO DO ÁUDIO DE REFERÊNCIA",  "REFERENCE AUDIO WORKSPACE",              "REFERENCE AUDIO WORKSPACE",              "ESPACIO DE TRABAJO DEL AUDIO DE REFERENCIA" },
        { "VST3",                                       "VST3",                                   "VST3",                                   "VST3" },
        { "Idioma:",                                    "Language:",                              "Language:",                              "Idioma:" },

        // 01 — Faixa
        { "01 / Faixa",                                 "01 / Track",                             "01 / Track",                             "01 / Pista" },
        { "Carregar áudio",                             "Load audio",                             "Load audio",                             "Cargar audio" },
        { "Trocar áudio",                               "Change audio",                           "Change audio",                           "Cambiar audio" },
        { "Selecionar o áudio de referência",           "Select the reference audio",             "Select the reference audio",             "Seleccionar el audio de referencia" },
        { "Nenhum áudio carregado",                     "No audio loaded",                        "No audio loaded",                        "Ningún audio cargado" },
        { "WAV, FLAC ou MP3",                           "WAV, FLAC or MP3",                       "WAV, FLAC or MP3",                       "WAV, FLAC o MP3" },
        { "{0} · {1} MB",                               "{0} · {1} MB",                           "{0} · {1} MB",                           "{0} · {1} MB" },
        { "Carregue um áudio para ver a forma de onda", "Load audio to see the waveform",         "Load audio to see the waveform",         "Cargue un audio para ver la forma de onda" },
        { "Sincronizado ao transporte do hospedeiro",   "Synced to the host transport",           "Synced to the host transport",           "Sincronizado con el transporte del host" },

        // 02 — Transporte
        { "02 / Transporte",                            "02 / Transport",                         "02 / Transport",                         "02 / Transporte" },
        { "Estado",                                     "State",                                  "State",                                  "Estado" },
        { "Tocando",                                    "Playing",                                "Playing",                                "Reproduciendo" },
        { "Pausado",                                    "Paused",                                 "Paused",                                 "En pausa" },
        { "Fim do arquivo",                             "End of file",                            "End of file",                            "Fin del archivo" },
        { "Aguardando o hospedeiro",                    "Waiting for the host",                   "Waiting for the host",                   "Esperando al host" },
        { "Posição",                                    "Position",                               "Position",                               "Posición" },
        { "Repetir trecho",                             "Repeat section",                         "Repeat section",                         "Repetir sección" },
        { "Início (compasso)",                          "Start (bar)",                            "Start (bar)",                            "Inicio (compás)" },
        { "Fim (compasso)",                             "End (bar)",                              "End (bar)",                              "Fin (compás)" },
        { "Velocidade (treino)",                        "Training speed",                         "Training speed",                         "Velocidad (entrenamiento)" },
        { "Play, pause e busca são controlados pelo hospedeiro.",
          "Play, pause and seek are controlled by the host.",
          "Play, pause and seek are controlled by the host.",
          "Play, pausa y búsqueda los controla el host." },
        { " %",                                         " %",                                     " %",                                     " %" },

        // 03 — Ficha da canção
        { "03 / Ficha da canção",                       "03 / Song details",                      "03 / Song details",                      "03 / Ficha de la canción" },
        { "Sem título nas tags",                        "No title in tags",                       "No title in tags",                       "Sin título en las etiquetas" },
        { "Lido das tags do arquivo",                   "Read from the file's tags",              "Read from the file's tags",              "Leído de las etiquetas del archivo" },
        { "ISRC do fonograma",                          "Sound-recording ISRC",                   "Sound-recording ISRC",                   "ISRC de la grabación" },
        { "Ano de publicação",                          "Release year",                           "Release year",                           "Año de publicación" },
        { "BPM original",                               "Original BPM",                           "Original BPM",                           "BPM original" },
        { " · estimado",                                " · estimated",                           " · estimated",                           " · estimado" },
        { "Impressão digital",                          "Audio fingerprint",                      "Audio fingerprint",                      "Huella digital" },
        { "analisando…",                                "analysing…",                             "analyzing…",                             "analizando…" },
        { "pronta",                                     "ready",                                  "ready",                                  "lista" },
        { "cancelada",                                  "cancelled",                              "canceled",                               "cancelada" },
        { "arquivo ausente",                            "file missing",                           "file missing",                           "archivo ausente" },
        { "formato inválido",                           "invalid format",                         "invalid format",                         "formato no válido" },
        { "sem canais",                                 "no channels",                            "no channels",                            "sin canales" },
        { "amostragem baixa",                           "low sample rate",                        "low sample rate",                        "muestreo bajo" },
        { "áudio curto",                                "audio too short",                        "audio too short",                        "audio demasiado corto" },
        { "falha na leitura",                           "failed to read",                         "failed to read",                         "fallo de lectura" },
        { "impressão vazia",                            "empty fingerprint",                      "empty fingerprint",                      "huella vacía" },
        { "falha",                                      "failed",                                 "failed",                                 "fallo" },
        { "Impressão digital calculada localmente. Cruzá-la com o AcoustID exige rede e ainda não está habilitado.",
          "The fingerprint was computed locally. Matching it against AcoustID needs a network connection and is not enabled yet.",
          "The fingerprint was computed locally. Matching it against AcoustID needs a network connection and is not enabled yet.",
          "La huella digital se ha calculado localmente. Compararla con AcoustID requiere una conexión de red y aún no está habilitada." },
        { "Carregue um arquivo de áudio para ler a ficha.",
          "Load an audio file to read its details.",
          "Load an audio file to read its details.",
          "Cargue un archivo de audio para leer la ficha." },
        { "Campos sem valor não constam no arquivo. A impressão digital é calculada localmente; cruzá-la com o AcoustID exige rede e ainda não está habilitado.",
          "Fields without a value are not in the file. The fingerprint is computed locally; matching it against AcoustID needs a network connection and is not enabled yet.",
          "Fields without a value are not in the file. The fingerprint is computed locally; matching it against AcoustID needs a network connection and is not enabled yet.",
          "Los campos sin valor no constan en el archivo. La huella digital se ha calculado localmente; compararla con AcoustID requiere una conexión de red y aún no está habilitada." },

        // 04 — Afinação
        { "04 / Afinação",                               "04 / Tuning",                            "04 / Tuning",                            "04 / Afinación" },
        { "Referência · A4",                            "Reference · A4",                         "Reference · A4",                         "Referencia · A4" },
        { "Restaurar 440 Hz",                           "Restore 440 Hz",                         "Restore 440 Hz",                         "Restaurar 440 Hz" },
        { "A4 / {0} Hz",                                "A4 / {0} Hz",                            "A4 / {0} Hz",                            "A4 / {0} Hz" },
        { "Arquivo",                                    "File",                                   "File",                                   "Archivo" },
        { "A = {0} Hz ({1} cents)",                     "A = {0} Hz ({1} cents)",                 "A = {0} Hz ({1} cents)",                 "A = {0} Hz ({1} cents)" },

        // 05 — BPM e métrica
        { "05 / BPM & Compassos",                       "05 / BPM & Bars",                        "05 / BPM & Bars",                        "05 / BPM y compases" },
        { "BPM",                                        "BPM",                                    "BPM",                                    "BPM" },
        { "Compassos",                                  "Bars",                                   "Bars",                                   "Compases" },
        { "Duração",                                    "Duration",                               "Duration",                               "Duración" },
        { "--",                                         "--",                                     "--",                                     "--" },
        { "Exportar Mapa (.mid)",                       "Export Tempo Map (.mid)",                "Export Tempo Map (.mid)",                "Exportar mapa (.mid)" },
        { "Exportar mapa de tempo (MIDI)",              "Export tempo map (MIDI)",                "Export tempo map (MIDI)",                "Exportar mapa de tempo (MIDI)" },
        { "Mapa de tempo exportado: {0}",               "Tempo map exported: {0}",                "Tempo map exported: {0}",                "Mapa de tempo exportado: {0}" },
        { "Falha ao exportar o mapa de tempo.",         "Failed to export the tempo map.",        "Failed to export the tempo map.",        "Fallo al exportar el mapa de tempo." },
        { "Estimado do áudio — sugestão; a métrica do hospedeiro é a definitiva.",
          "Estimated from the audio — a suggestion; the host meter is authoritative.",
          "Estimated from the audio — a suggestion; the host meter is authoritative.",
          "Estimado del audio — sugerencia; la métrica del host es la definitiva." },

        // Rodapé
        { "PartePlay · Feito para músicos e arranjadores",
          "PartePlay · Made for musicians and arrangers",
          "PartePlay · Made for musicians and arrangers",
          "PartePlay · Hecho para músicos y arreglistas" },
        { "Produção Rubinho Lyra / Software Eng",
          "Production Rubinho Lyra / Software Eng",
          "Production Rubinho Lyra / Software Eng",
          "Producción Rubinho Lyra / Software Eng" },

        // Nomes de parâmetro (visíveis no hospedeiro)
        { "Afinação (Hz)",                              "Tuning (Hz)",                            "Tuning (Hz)",                            "Afinación (Hz)" },
        { "Loop",                                       "Loop",                                   "Loop",                                   "Bucle" },
        { "Loop início (compasso)",                     "Loop start (bar)",                       "Loop start (bar)",                       "Inicio del bucle (compás)" },
        { "Loop fim (compasso)",                        "Loop end (bar)",                         "Loop end (bar)",                         "Fin del bucle (compás)" },
        { "Silenciar saída",                            "Mute output",                            "Mute output",                            "Silenciar salida" },

        // Exportação (nome da trilha MIDI)
        { "PartePlay - Mapa de Tempo",                  "PartePlay - Tempo Map",                  "PartePlay - Tempo Map",                  "PartePlay - Mapa de tempo" },
    };

    Text::Culture currentCulture = Text::Culture::PortugueseBR;
    bool cultureInitialised = false;

    Text::Culture detectCulture()
    {
        const juce::String lang = juce::SystemStats::getUserLanguage();

        if (lang.startsWith ("pt"))
            return Text::Culture::PortugueseBR;

        if (lang.startsWith ("en-GB") || lang.startsWith ("en-UK"))
            return Text::Culture::EnglishUK;
        if (lang.startsWith ("en"))
            return Text::Culture::EnglishUS;

        if (lang.startsWith ("es"))
            return Text::Culture::SpanishES;

        // Público internacional do plugin: sem pt/es/en declarado, o inglês
        // americano é o padrão de comunicação global.
        return Text::Culture::EnglishUS;
    }

    const char* pick (const Translation& t)
    {
        switch (currentCulture)
        {
            case Text::Culture::EnglishUK:   return t.enGB;
            case Text::Culture::EnglishUS:   return t.enUS;
            case Text::Culture::SpanishES:   return t.esES;
            case Text::Culture::PortugueseBR:
            default:                         return t.pt;
        }
    }

    juce::String paddedTwo (int value)
    {
        return juce::String (value).paddedLeft ('0', 2);
    }
}

void Text::setCulture (Culture culture)
{
    currentCulture = culture;
    cultureInitialised = true;
}

Text::Culture Text::getCulture() noexcept
{
    if (! cultureInitialised)
    {
        currentCulture = detectCulture();
        cultureInitialised = true;
    }

    return currentCulture;
}

juce::String Text::cultureCode (Culture culture) noexcept
{
    switch (culture)
    {
        case Culture::EnglishUK:   return from ("en-GB");
        case Culture::EnglishUS:   return from ("en-US");
        case Culture::SpanishES:   return from ("es-ES");
        case Culture::PortugueseBR:
        default:                   return from ("pt-BR");
    }
}

juce::String Text::cultureName (Culture culture) noexcept
{
    switch (culture)
    {
        case Culture::EnglishUK:   return from ("English (UK)");
        case Culture::EnglishUS:   return from ("English (US)");
        case Culture::SpanishES:   return from ("Español (España)");
        case Culture::PortugueseBR:
        default:                   return from ("Português (Brasil)");
    }
}

Text::Culture Text::cultureFromCode (const juce::String& code) noexcept
{
    if (code.equalsIgnoreCase (from ("en-GB")))
        return Culture::EnglishUK;
    if (code.equalsIgnoreCase (from ("en-US")))
        return Culture::EnglishUS;
    if (code.equalsIgnoreCase (from ("es-ES")))
        return Culture::SpanishES;

    return Culture::PortugueseBR;
}

juce::String Text::cultureCode() noexcept
{
    return cultureCode (getCulture());
}

juce::String Text::cultureName() noexcept
{
    return cultureName (getCulture());
}

juce::String Text::from (const char* utf8)
{
    return juce::String::fromUTF8 (utf8);
}

juce::String Text::t (const char* utf8)
{
    return t (juce::String::fromUTF8 (utf8));
}

    juce::String Text::t (const juce::String& key)
    {
        const char* keyUtf8 = key.toRawUTF8();

        for (const auto& row : translations)
        {
            if (std::strcmp (row.pt, keyUtf8) == 0)
                return juce::String::fromUTF8 (pick (row));
        }

        return key;
    }

    bool Text::has (const juce::String& key)
    {
        const char* keyUtf8 = key.toRawUTF8();

        for (const auto& row : translations)
        {
            if (std::strcmp (row.pt, keyUtf8) == 0)
                return true;
        }

        return false;
    }


    bool Text::has (const char* utf8)
    {
        for (const auto& row : translations)
        {
            if (std::strcmp (row.pt, utf8) == 0)
                return true;
        }

        return false;
    }

    juce::String Text::number (double value, int decimalPlaces)
    {
        // juce::String(double, int) sempre usa '.' como separador, independente do
        // locale; a troca abaixo é o ponto em que a vírgula entra para pt-BR e es-ES.
        const auto text = juce::String (value, decimalPlaces);

        const auto dot = text.indexOfChar ('.');
        if (dot < 0)
            return text;

        const auto separator = (getCulture() == Culture::PortugueseBR || getCulture() == Culture::SpanishES)
                                   ? from (",")
                                   : from (".");

        return text.substring (0, dot) + separator + text.substring (dot + 1);
    }

    juce::String Text::number (int value)
    {
        return juce::String (value);
    }

    juce::String Text::format (const juce::String& model,
                           const std::initializer_list<juce::String>& values)
{
    auto result = model;
    int index = 0;

    for (const auto& value : values)
    {
        result = result.replace (from ("{") + juce::String (index++) + from ("}"), value);
    }

    return result;
}

juce::String Text::time (double seconds)
{
    if (! std::isfinite (seconds) || seconds < 0.0)
        seconds = 0.0;

    const int total = static_cast<int> (seconds);
    return paddedTwo (total / 60) + from (":") + paddedTwo (total % 60);
}

juce::String Text::timePrecise (double seconds)
{
    if (! std::isfinite (seconds) || seconds < 0.0)
        seconds = 0.0;

    const int total = static_cast<int> (seconds);
    const int cents = juce::jlimit (0, 99, static_cast<int> ((seconds - (double) total) * 100.0));

    return time (seconds) + from (".") + paddedTwo (cents);
}