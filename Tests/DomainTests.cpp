// Testes automatizados do domínio musical e da camada de texto.
//
// Objetivo: travar em código o gate V1 (direção e convenção da transposição) e as
// invariantes de encoding/i18n, para que a val musically correta não dependa
// apenas do ouvido. Executados por CTest: `ctest --preset msvc` ou via script.
//
// Não há framework externo: o runner é uma função local `check` para manter o
// núcleo sem dependências além do JUCE.

#include <cmath>
#include <cstdio>
#include <string>

#include "Instrument.h"
#include "Text.h"

namespace
{
    int failures = 0;
    int checks = 0;

    void check (bool condition, const std::string& what)
    {
        ++checks;

        if (! condition)
        {
            ++failures;
            std::printf ("  FALHOU: %s\n", what.c_str());
        }
    }

    void checkClose (double actual, double expected, double tolerance, const std::string& what)
    {
        const bool ok = std::abs (actual - expected) <= tolerance;
        check (ok, what + " (obtido " + std::to_string (actual)
                      + ", esperado " + std::to_string (expected) + ")");
    }

    // Assinatura de mojibake: UTF-8 interpretado como Latin-1 e re-codificado.
    // "ç" (C3 A7) vira "Ã§" (C3 83 C2 A7) — procuramos o par U+00C3 seguido de
    // caractere do suplemento Latin-1, alem do caractere de substituicao U+FFFD.
    bool hasMojibake (const juce::String& s)
    {
        static const char* const mojibakeSequences[] = { "\xC3\x83\xC2\xA7",   // Ã§
                                                         "\xC3\x83\xC2\xA3",   // Ã£
                                                         "\xC3\x83\xC2\xA9",   // Ã©
                                                         "\xC3\x83\xC2\xB5",   // Ãµ
                                                         "\xC3\x83\xC3\xAD" }; // Ãí

        if (s.contains (juce::CharPointer_UTF8 ("\xEF\xBF\xBD")))
            return true;

        for (const auto* sequence : mojibakeSequences)
            if (s.contains (juce::CharPointer_UTF8 (sequence)))
                return true;

        return false;
    }

    // V1 — convenção musical: menor intervalo assinado entre o tom escrito
    // (partitura em dó) e o som real do instrumento.
    void testInstrumentTable()
    {
        std::printf ("[V1] Tabela de instrumentos\n");

        check (InstrumentTable::semitonesFor (Instrument::PianoC)      == 0,  "Piano = 0 semitons");
        check (InstrumentTable::semitonesFor (Instrument::TromboneC)   == 0,  "Trombone = 0 semitons");
        check (InstrumentTable::semitonesFor (Instrument::TrumpetBb)   == -2, "Trompete = -2 semitons (desce)");
        check (InstrumentTable::semitonesFor (Instrument::SaxTenorBb)  == -2, "Sax Tenor = -2 semitons (desce)");
        check (InstrumentTable::semitonesFor (Instrument::SaxAltoEb)   == 3,  "Sax Alto = +3 semitons (sobe)");
        check (InstrumentTable::semitonesFor (Instrument::FrenchHornF) == 5,  "Trompa = +5 semitons (sobe, nao -5)");
        check (InstrumentTable::semitonesFor (Instrument::Manual)      == 0,  "Manual = 0 semitons");

        // Nenhum instrumento pode ficar a -5: cairia em Sol em vez de subir a Fá.
        const juce::StringArray names = InstrumentTable::getDisplayNames();
        check (names.size() == InstrumentTable::numEntries, "Tabela expoe numEntries nomes");

        for (int i = 0; i < InstrumentTable::numEntries; ++i)
        {
            const auto instrument = InstrumentTable::fromIndex (i);
            check (InstrumentTable::indexFor (instrument) == i,
                   "Indice " + std::to_string (i) + " faz ida e volta");
            check (InstrumentTable::semitonesFor (instrument) != -5,
                   "Instrumento " + names[i].toStdString() + " nao usa -5 semitons");
        }
    }

    // V1 — o sinal do semitone sobrevive à conversão para ratio de frequência.
    void testRatioDirection()
    {
        std::printf ("[V1] Direcao do ratio\n");

        checkClose (Tuning::ratioForSemitones (0), 1.0, 1e-12, "0 semitons = razao 1.0");
        checkClose (Tuning::ratioForSemitones (5), 1.3348398541700344, 1e-9, "+5 semitons sobe (Fá)");
        checkClose (Tuning::ratioForSemitones (-2), 0.8908987181403393, 1e-9, "-2 semitons desce (Si bemol)");
        checkClose (Tuning::ratioForSemitones (3), 1.189207115002721, 1e-9, "+3 semitons sobe (Mi bemol)");

        check (Tuning::ratioForSemitones (5) > 1.0, "Trompa produz razao > 1");
        check (Tuning::ratioForSemitones (-2) < 1.0, "Trompete produz razao < 1");

        // Inversão de sinal é exatamente o bug corrigido na Trompa.
        check (std::abs (Tuning::ratioForSemitones (5) * Tuning::ratioForSemitones (-5) - 1.0) < 1e-12,
               "+5 e -5 sao inversos (sinal nao se anula)");

        // Aritmética musical: uma oitava = 1200 cents, logo 5 semitons = 500 cents.
        checkClose (1200.0 * std::log2 (Tuning::ratioForSemitones (5)), 500.0, 1e-6, "+5 semitons = +500 cents");
        checkClose (1200.0 * std::log2 (Tuning::ratioForSemitones (-2)), -200.0, 1e-6, "-2 semitons = -200 cents");
        checkClose (1200.0 * std::log2 (Tuning::ratioForSemitones (3)), 300.0, 1e-6, "+3 semitons = +300 cents");

        // A Trompa em Fá escrito e lido em dó: a versão fiel à oitava seria -7
        // semitons (cai uma quinta), a convenção de tessitura usa +5.
        checkClose (Tuning::ratioForSemitones (5), std::pow (2.0, -7.0 / 12.0) * std::pow (2.0, 1.0), 1e-9,
                    "+5 semitons e a mesma classe de altura que -7 (oitava acima)");
    }

    void testReferencePitchCompensation()
    {
        std::printf ("[V1] Compensacao de afinacao de referencia\n");

        checkClose (Tuning::playbackRatio (0, Tuning::defaultReferenceHz, Tuning::defaultReferenceHz),
                    1.0, 1e-12, "A=440 em arquivo A=440 e razao 1.0");

        // Arquivo afinado em 432 Hz lido com referencia 440: precisa subir 8 cents.
        checkClose (1200.0 * std::log2 (Tuning::playbackRatio (0, 440.0, 432.0)),
                    31.77, 0.05, "Arquivo em 432 Hz com referencia 440 corrige ~+31.8 cents");

        // Entrada invalida nao pode explodir o ratio.
        checkClose (Tuning::playbackRatio (0, 440.0, 0.0), 1.0, 1e-12, "Afinacao 0 Hz e tratada como sem desvio");
        checkClose (Tuning::playbackRatio (3, 440.0, -1.0), Tuning::ratioForSemitones (3), 1e-12,
                    "Afinacao negativa e tratada como sem desvio");

        checkClose (Tuning::centsBetween (440.0, 880.0), 1200.0, 1e-6, "440 -> 880 Hz = 1200 cents");
        checkClose (Tuning::centsBetween (0.0, 440.0), 0.0, 1e-12, "centsBetween tolera entrada invalida");
    }

    // V2/V3 — encoding e i18n: a causa raiz do mojibake foi String(const char*).
    void testTextEncoding()
    {
        std::printf ("[V2] Encoding e i18n\n");

        const auto withAccent = Text::from ("Afina\xC3\xA7\xC3\xA3o");
        check (withAccent == juce::String::fromUTF8 ("Afina\xC3\xA7\xC3\xA3o"), "Text::from decodifica UTF-8");
        check (withAccent.contains (juce::CharPointer_UTF8 ("\xC3\xA7")), "Text::from preserva cedilha");
        check (! hasMojibake (withAccent), "Text::from nao produz sequencia mojibake");

        const auto flat = Text::format (Text::t ("{0} \xC2\xB7 {1} MB"), { "WAV", juce::String (12.5, 1) });
        check (flat == juce::String::fromUTF8 ("WAV \xC2\xB7 12.5 MB"), "Text::format interpola e preserva acentos");

        const auto missing = Text::format (Text::t ("{0} e {1}"), { "a" });
        check (missing == juce::String::fromUTF8 ("a e {1}"), "Marcador ausente fica visivel");

        check (Text::time (65.0) == "01:05", "Text::time formata mm:ss");
        check (Text::timePrecise (65.25) == "01:05.25", "Text::timePrecise formata mm:ss.cc");
        check (Text::time (-1.0) == "00:00", "Text::time trata valor negativo");

        // pt-BR precisa devolver a propria chave com acentos; en/es traduzem.
        Text::setCulture (Text::Culture::PortugueseBR);
        const auto pt = Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)");
        check (pt == juce::String::fromUTF8 ("Afina\xC3\xA7\xC3\xA3o (Hz)"), "pt-BR devolve a chave intacta");
        check (pt.contains (juce::CharPointer_UTF8 ("\xC3\xA7")), "pt-BR preserva a cedilha");
        check (! hasMojibake (pt), "pt-BR nao produz mojibake");

        Text::setCulture (Text::Culture::English);
        const auto en = Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)");
        check (en == juce::String::fromUTF8 ("Tuning (Hz)"), "en traduz 'Afinacao (Hz)'");
        check (en != pt, "en difere de pt-BR");

        Text::setCulture (Text::Culture::Spanish);
        const auto es = Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)");
        check (es == juce::String::fromUTF8 ("Afinaci\xC3\xB3n (Hz)"), "es traduz 'Afinacion (Hz)'");
        check (es.contains (juce::CharPointer_UTF8 ("\xC3\xB3")), "es traduz com acento (Afinacion)");
        check (! hasMojibake (es), "es nao produz mojibake");

        for (const auto culture : { Text::Culture::PortugueseBR, Text::Culture::English, Text::Culture::Spanish })
        {
            Text::setCulture (culture);
            check (Text::getCulture() == culture, "Cultura ativa apos setCulture");

            const auto name = Text::t ("Afina\xC3\xA7\xC3\xA3o em {0} \xC2\xB7 {1} semitons");
            check (name.isNotEmpty() && ! hasMojibake (name),
                   "Texto traduzido sem mojibake em " + Text::cultureCode().toStdString());
        }

        Text::setCulture (Text::Culture::PortugueseBR);
    }

    // Nomes de instrumento devem ser distintos — colisão faria o selector enganar.
    void testDisplayNames()
    {
        std::printf ("[V2] Nomes exibidos\n");

        Text::setCulture (Text::Culture::PortugueseBR);
        const auto names = InstrumentTable::getDisplayNames();

        for (int i = 0; i < names.size(); ++i)
        {
            check (names[i].isNotEmpty(), "Nome " + std::to_string (i) + " nao vazio");

            for (int j = i + 1; j < names.size(); ++j)
                check (names[i] != names[j], "Nomes distintos: " + names[i].toStdString()
                                              + " vs " + names[j].toStdString());
        }

        for (int i = 0; i < names.size(); ++i)
        {
            const auto instrument = InstrumentTable::fromIndex (i);
            check (InstrumentTable::symbolFor (instrument).isNotEmpty(),
                   "Simbolo presente para " + names[i].toStdString());
            check (InstrumentTable::keyNameFor (instrument).isNotEmpty(),
                   "Nome da tonalidade presente para " + names[i].toStdString());
            check (InstrumentTable::signedSemitonesFor (instrument).isNotEmpty(),
                   "Semitons com sinal presentes para " + names[i].toStdString());
        }
    }
}

int main()
{
    std::printf ("PartePlay - testes de dominio\n\n");

    testInstrumentTable();
    testRatioDirection();
    testReferencePitchCompensation();
    testTextEncoding();
    testDisplayNames();

    std::printf ("\n%d verificacoes, %d falha(s)\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
