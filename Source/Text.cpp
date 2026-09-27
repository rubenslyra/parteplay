#include "Text.h"

#include <cmath>
#include <cstring>

namespace
{
    struct Translation
    {
        const char* pt; // também é a chave
        const char* en;
        const char* es;
    };

    // Fonte única de textos da interface. Adicionar novas strings aqui, não em
    // literal solto no código de UI. A chave é sempre o texto pt-BR.
    const Translation translations[] =
    {
        // Cabeçalho
        { "PartePlay",                                  "PartePlay",                              "PartePlay" },
        { "Espaço de trabalho do áudio de referência",  "Reference audio workspace",              "Espacio de trabajo del audio de referencia" },
        { "VST3 · Transporte escravo",                  "VST3 · Slave transport",                 "VST3 · Transporte esclavo" },
        { "Idioma:",                                    "Language:",                            "Idioma:" },

        // 01 — Faixa
        { "01 / Faixa",                                 "01 / Track",                             "01 / Pista" },
        { "Carregar áudio",                             "Load audio",                             "Cargar audio" },
        { "Trocar áudio",                               "Change audio",                           "Cambiar audio" },
        { "Selecionar o áudio de referência",           "Select the reference audio",             "Seleccionar el audio de referencia" },
        { "Nenhum áudio carregado",                     "No audio loaded",                        "Ningún audio cargado" },
        { "WAV, FLAC ou MP3",                           "WAV, FLAC or MP3",                       "WAV, FLAC o MP3" },
        { "{0} · {1} MB",                               "{0} · {1} MB",                           "{0} · {1} MB" },
        { "Carregue um áudio para ver a forma de onda", "Load audio to see the waveform",         "Cargue un audio para ver la forma de onda" },
        { "Sincronizado ao transporte do hospedeiro",   "Synced to the host transport",           "Sincronizado con el transporte del host" },

        // 02 — Transporte
        { "02 / Transporte",                            "02 / Transport",                         "02 / Transporte" },
        { "Estado",                                     "State",                                  "Estado" },
        { "Tocando",                                    "Playing",                                "Sonando" },
        { "Pausado",                                    "Paused",                                 "En pausa" },
        { "Fim do arquivo",                             "End of file",                            "Fin del archivo" },
        { "Aguardando o hospedeiro",                    "Waiting for the host",                   "Esperando al host" },
        { "Posição",                                    "Position",                               "Posición" },
        { "Repetir trecho",                             "Repeat section",                         "Repetir sección" },
        { "Início (compasso)",                          "Start (bar)",                            "Inicio (compás)" },
        { "Fim (compasso)",                             "End (bar)",                              "Fin (compás)" },
        { "Velocidade (treino)",                        "Training speed",                         "Velocidad (entrenamiento)" },
        { "Play, pause e busca são controlados pelo hospedeiro.",
          "Play, pause and seek are controlled by the host.",
          "Play, pausa y búsqueda los controla el host." },
        { " %",                                         " %",                                     " %" },

        // 03 — Instrumento
        { "03 / Instrumento",                           "03 / Instrument",                        "03 / Instrumento" },
        { "Afinação em {0} · {1} semitons",             "Tuned in {0} · {1} semitones",           "Afinación en {0} · {1} semitonos" },
        { "Piano (C)",                                  "Piano (C)",                              "Piano (C)" },
        { "Trombone (C)",                               "Trombone (C)",                           "Trombón (C)" },
        { "Trompete (Bb)",                              "Trumpet (Bb)",                           "Trompeta (Bb)" },
        { "Sax Tenor (Bb)",                             "Tenor Sax (Bb)",                         "Saxo tenor (Bb)" },
        { "Sax Alto (Eb)",                              "Alto Sax (Eb)",                          "Saxo alto (Eb)" },
        { "Trompa (F)",                                 "French Horn (F)",                        "Trompa (F)" },
        { "Ajuste Manual",                              "Manual",                                 "Ajuste manual" },
        { "Dó",                                         "C",                                      "Do" },
        { "Si♭",                                        "B♭",                                     "Si♭" },
        { "Mi♭",                                        "E♭",                                     "Mi♭" },
        { "Fá",                                         "F",                                      "Fa" },
        { "Personalizada",                              "Custom",                                 "Personalizada" },

        // 04 — Afinação e transposição
        { "04 / Afinação & Transposição",               "04 / Tuning & Transposition",            "04 / Afinación y transposición" },
        { "Referência · A4",                            "Reference · A4",                         "Referencia · A4" },
        { "Restaurar 440 Hz",                           "Restore 440 Hz",                         "Restaurar 440 Hz" },
        { "Transposição",                               "Transposition",                          "Transposición" },
        { "Transposição manual (semitons)",             "Manual transposition (semitones)",       "Transposición manual (semitonos)" },
        { "{0} st",                                     "{0} st",                                 "{0} st" },
        { "A4 / {0} Hz",                                "A4 / {0} Hz",                            "A4 / {0} Hz" },
        { "Arquivo",                                    "File",                                   "Archivo" },
        { "A = {0} Hz ({1} cents)",                     "A = {0} Hz ({1} cents)",                 "A = {0} Hz ({1} cents)" },

        // 05 — BPM e métrica
        { "05 / BPM & Métrica",                         "05 / BPM & Meter",                       "05 / BPM y métrica" },
        { "BPM",                                        "BPM",                                    "BPM" },
        { "Assinatura",                                 "Time signature",                         "Compás" },
        { "Compassos",                                  "Bars",                                   "Compases" },
        { "Duração",                                    "Duration",                               "Duración" },
        { "{0}/4",                                      "{0}/4",                                  "{0}/4" },
        { "--",                                         "--",                                     "--" },
        { "Exportar Mapa (.mid)",                       "Export Tempo Map (.mid)",                "Exportar mapa (.mid)" },
        { "Exportar mapa de tempo (MIDI)",              "Export tempo map (MIDI)",                "Exportar mapa de tempo (MIDI)" },
        { "Mapa de tempo exportado: {0}",               "Tempo map exported: {0}",                "Mapa de tempo exportado: {0}" },
        { "Falha ao exportar o mapa de tempo.",         "Failed to export the tempo map.",        "Fallo al exportar el mapa de tempo." },
        { "Estimado do áudio — sugestão; a métrica do hospedeiro é a definitiva.",
          "Estimated from the audio — a suggestion; the host meter is authoritative.",
          "Estimado del audio — sugerencia; la métrica del host es la definitiva." },

        // Rodapé
        { "PartePlay · Feito para músicos e arranjadores",
          "PartePlay · Made for musicians and arrangers",
          "PartePlay · Hecho para músicos y arreglistas" },
        { "Produção Rubinho Lyra / Software Eng",       "Production Rubinho Lyra / Software Eng", "Producción Rubinho Lyra / Software Eng" },

        // Nomes de parâmetro (visíveis no hospedeiro)
        { "Instrumento",                                "Instrument",                             "Instrumento" },
        { "Afinação (Hz)",                              "Tuning (Hz)",                            "Afinación (Hz)" },
        { "Transposição (semitons)",                    "Transposition (semitones)",              "Transposición (semitonos)" },
        { "Loop",                                       "Loop",                                   "Bucle" },
        { "Loop início (compasso)",                     "Loop start (bar)",                       "Inicio del bucle (compás)" },
        { "Loop fim (compasso)",                        "Loop end (bar)",                         "Fin del bucle (compás)" },
        { "Silenciar saída",                            "Mute output",                            "Silenciar salida" },

        // Exportação (nome da trilha MIDI)
        { "PartePlay - Mapa de Tempo",                  "PartePlay - Tempo Map",                  "PartePlay - Mapa de tempo" },
    };

    Text::Culture currentCulture = Text::Culture::PortugueseBR;
    bool cultureInitialised = false;

    Text::Culture detectCulture()
    {
        const juce::String lang = juce::SystemStats::getUserLanguage();

        if (lang.startsWith ("pt"))
            return Text::Culture::PortugueseBR;
        if (lang.startsWith ("es"))
            return Text::Culture::Spanish;

        return Text::Culture::PortugueseBR;
    }

    const char* pick (const Translation& t)
    {
        switch (currentCulture)
        {
            case Text::Culture::English:       return t.en;
            case Text::Culture::Spanish:       return t.es;
            case Text::Culture::PortugueseBR:
            default:                           return t.pt;
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

juce::String Text::cultureCode() noexcept
{
    switch (getCulture())
    {
        case Culture::English:   return from ("en");
        case Culture::Spanish:   return from ("es");
        case Culture::PortugueseBR:
        default:                 return from ("pt-BR");
    }
}

juce::String Text::cultureName() noexcept
{
    switch (getCulture())
    {
        case Culture::English:   return from ("English");
        case Culture::Spanish:   return from ("Español");
        case Culture::PortugueseBR:
        default:                 return from ("Português");
    }
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
