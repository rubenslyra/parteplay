#pragma once

#include <JuceHeader.h>
#include <cmath>

// Domínio musical compartilhado entre núcleo, interface e exportação.
// Fonte única da tabela de instrumentos: nenhum outro módulo (UI, tema, docs)
// deve duplicar estes valores.

enum class Instrument
{
    PianoC = 0,
    TromboneC,
    TrumpetBb,
    SaxTenorBb,
    SaxAltoEb,
    FrenchHornF,
    Manual
};

namespace Tuning
{
    // ISO 16: frequência de referência padrão A = 440 Hz.
    constexpr double defaultReferenceHz = 440.0;
    constexpr double minReferenceHz     = 432.0;
    constexpr double maxReferenceHz     = 445.0;

    // Fator de multiplicação de frequência para um deslocamento em semitons.
    // Positivo sobe o tom, negativo desce — o sinal é o da convenção de
    // InstrumentTable::semitonesFor e nunca é invertido em nenhum elo da cadeia.
    inline double ratioForSemitones (int semitones) noexcept
    {
        return std::pow (2.0, static_cast<double> (semitones) / 12.0);
    }

    // Cents entre duas frequências de referência (1200·log2(f2/f1)).
    inline double centsBetween (double fromHz, double toHz) noexcept
    {
        if (fromHz <= 0.0 || toHz <= 0.0)
            return 0.0;

        return 1200.0 * std::log2 (toHz / fromHz);
    }

    // Razão total aplicada ao áudio: transposição do instrumento somada à
    // compensação da afinação de referência em relação à afinação real do
    // arquivo. Frequência inválida no arquivo é tratada como "sem desvio".
    inline double playbackRatio (int semitones, double referenceHz, double filePitchHz) noexcept
    {
        const double referenceCompensation = (filePitchHz > 0.0)
            ? static_cast<double> (referenceHz) / filePitchHz
            : 1.0;

        return ratioForSemitones (semitones) * referenceCompensation;
    }
}

namespace InstrumentTable
{
    // Entradas selecionáveis (índice do parâmetro "instrument").
    constexpr int numEntries = 7;

    // Deslocamento aplicado ao áudio de referência (tom de concerto) para que ele
    // coincida com o som real do instrumento quando o músico lê uma partitura em dó.
    // Convenção: menor intervalo assinado entre o escrito e o soante — ler Dó produz
    // Si♭ (-2), Mi♭ (+3), Fá (+5). Equivalente em classe de altura ao deslocamento
    // fiel à oitava (Si♭ -2, Mi♭ -9, Fá -7), porém mantém o áudio na tessitura útil.
    constexpr int semitonesFor (Instrument instrument) noexcept
    {
        switch (instrument)
        {
            case Instrument::TrumpetBb:
            case Instrument::SaxTenorBb:  return -2;
            case Instrument::SaxAltoEb:   return 3;
            case Instrument::FrenchHornF: return 5;
            case Instrument::PianoC:
            case Instrument::TromboneC:
            case Instrument::Manual:
            default:                      return 0;
        }
    }

    Instrument fromIndex (int index) noexcept;
    int        indexFor (Instrument instrument) noexcept;

    // Nomes exibidos na interface (já traduzidos). Não são armazenados em cache:
    // a troca de idioma precisa valer imediatamente.
    juce::String      displayName (Instrument instrument);
    juce::StringArray getDisplayNames();

    // Símbolo da tonalidade ("C", "B♭", "E♭", "F") — independente de idioma.
    juce::String symbolFor (Instrument instrument);

    // Nome da tonalidade no idioma ativo ("Dó", "Si♭", ...).
    juce::String keyNameFor (Instrument instrument);

    // Semitons com sinal, para exibição ("+5", "-2", "0").
    juce::String signedSemitonesFor (Instrument instrument);
}
