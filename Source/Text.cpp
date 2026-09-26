#include "Text.h"

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
        // Cabeçalho / ações
        { "PartePlay",                       "PartePlay",                        "PartePlay" },
        { "Reprodução de referência sincronizada ao transport", "Reference playback synced to the transport", "Reproducción de referencia sincronizada al transport" },
        { "Carregar Áudio",                  "Load Audio",                       "Cargar audio" },
        { "Selecionar o áudio de referência","Select the reference audio",       "Seleccionar el audio de referencia" },
        { "Nenhum áudio carregado",          "No audio loaded",                  "Ningún audio cargado" },
        { "Exportar Mapa (.mid)",            "Export Tempo Map (.mid)",          "Exportar mapa de tempo (.mid)" },
        { "Exportar mapa de tempo (MIDI)",   "Export tempo map (MIDI)",          "Exportar mapa de tempo (MIDI)" },
        { "Mapa de tempo exportado: ",       "Tempo map exported: ",             "Mapa de tempo exportado: " },
        { "Falha ao exportar o mapa de tempo.","Failed to export the tempo map.", "Fallo al exportar el mapa de tempo." },

        // Controles
        { "Instrumento",                     "Instrument",                       "Instrumento" },
        { "Instrumento:",                    "Instrument:",                      "Instrumento:" },
        { "Afinação (Hz)",                   "Tuning (Hz)",                      "Afinación (Hz)" },
        { "Afinação (Hz):",                  "Tuning (Hz):",                     "Afinación (Hz):" },
        { "Transposição (semitons)",         "Transposition (semitones)",        "Transposición (semitonos)" },
        { "Transposição (semitons):",        "Transposition (semitones):",       "Transposición (semitonos):" },
        { "Velocidade (treino)",             "Training speed",                   "Velocidad (entrenamiento)" },
        { "Velocidade (treino %):",          "Training speed (%):",              "Velocidad (entrenamiento %):" },
        { " %",                              " %",                               " %" },
        { "Repetir trecho",                  "Repeat section",                   "Repetir sección" },
        { "Idioma:",                         "Language:",                        "Idioma:" },
        { "Loop",                            "Loop",                             "Bucle" },
        { "Loop início (compasso)",          "Loop start (bar)",                 "Inicio del bucle (compás)" },
        { "Loop início (compasso):",         "Loop start (bar):",                "Inicio del bucle (compás):" },
        { "Loop fim (compasso)",             "Loop end (bar)",                   "Fin del bucle (compás)" },
        { "Loop fim (compasso):",            "Loop end (bar):",                  "Fin del bucle (compás):" },

        // Instrumentos
        { "Piano / Trombone (C)",            "Piano / Trombone (C)",             "Piano / Trombón (C)" },
        { "Trompete / Sax Tenor (Bb)",       "Trumpet / Tenor Sax (Bb)",         "Trompeta / Saxo tenor (Bb)" },
        { "Sax Alto (Eb)",                   "Alto Sax (Eb)",                    "Saxo alto (Eb)" },
        { "Trompa (F)",                      "French Horn (F)",                  "Trompa (F)" },
        { "Ajuste Manual",                   "Manual",                           "Ajuste manual" },

        // Status / análise
        { "BPM: -- | Assinatura: -- | Afinação: --Hz | Compassos: -- | Duração: --:--",
          "BPM: -- | Time: -- | Tuning: --Hz | Bars: -- | Duration: --:--",
          "BPM: -- | Compás: -- | Afinación: --Hz | Compases: -- | Duración: --:--" },
        { "BPM: %s | Assinatura: %d/4 | Afinação: %.1fHz | Compassos: %d | Duração: %02d:%02d",
          "BPM: %s | Time: %d/4 | Tuning: %.1fHz | Bars: %d | Duration: %02d:%02d",
          "BPM: %s | Compás: %d/4 | Afinación: %.1fHz | Compases: %d | Duración: %02d:%02d" },
        { "Pausado",                         "Paused",                           "En pausa" },
        { "Fim do arquivo",                  "End of file",                      "Fin del archivo" },
        { "Tocando | %02d:%05.2f | Transposição %+d st",
          "Playing | %02d:%05.2f | Transposition %+d st",
          "Sonando | %02d:%05.2f | Transposición %+d st" },
        { "Treino |",                        "Training |",                       "Entrenamiento |" },
        { " ",                               " ",                                " " },
        { " | Loop ",                        " | Loop ",                         " | Bucle " },
        { "-",                               "-",                                "-" },
        { " | Pausado",                      " | Paused",                        " | En pausa" },
        { "%s | %02d:%05.2f",                "%s | %02d:%05.2f",                 "%s | %02d:%05.2f" },

        // Logo / créditos
        { "Rubinho Lyra Software Eng",       "Rubinho Lyra Software Eng",        "Rubinho Lyra Software Eng" },

        // Exportação (nome da trilha MIDI)
        { "PartePlay - Mapa de Tempo",       "PartePlay - Tempo Map",            "PartePlay - Mapa de tempo" },
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
        case Culture::English:   return "en";
        case Culture::Spanish:   return "es";
        case Culture::PortugueseBR:
        default:                 return "pt-BR";
    }
}

juce::String Text::cultureName() noexcept
{
    switch (getCulture())
    {
        case Culture::English:   return "English";
        case Culture::Spanish:   return "Español";
        case Culture::PortugueseBR:
        default:                 return "Português";
    }
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