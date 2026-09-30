#pragma once

#include <JuceHeader.h>

// Tokens visuais do PartePlay — "Vidro Rítmico".
//
// Fonte única de cor, tipografia e geometria da interface. Nenhum componente
// deve fixar cor por conta própria: usar estes tokens mantém a UI sincronizada.
namespace Theme
{
    // Superfícies
    inline const juce::Colour background { 0xff040e1a };
    inline const juce::Colour panel      { 0xff051827 };
    inline const juce::Colour surface    { 0xff12202f };
    inline const juce::Colour line       { 0xff2b3d4c };

    // Texto
    inline const juce::Colour foreground { 0xffe0edf8 };
    inline const juce::Colour muted      { 0xff8ca2b3 };

    // Energia: ciano desenha ação/sinal; laranja marca apenas o que exige atenção.
    inline const juce::Colour azure      { 0xff00b3f2 };
    inline const juce::Colour ice        { 0xff63cdf1 };
    inline const juce::Colour teal       { 0xff00c7c9 };
    inline const juce::Colour signal     { 0xfff08e54 };

    constexpr float radius      = 10.0f;
    constexpr float radiusSmall = 6.0f;

    // Escala tipográfica (a hierarquia nasce do tamanho e do intervalo, não de cor).
    namespace Type
    {
        constexpr float title    = 19.0f;
        constexpr float value    = 21.0f; // cifras isoladas (mono)
        constexpr float body     = 13.0f;
        constexpr float caption  = 11.0f;
        constexpr float section  = 10.5f; // micro-rótulos em caixa alta
    }

    inline juce::Font font (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(),
                                              height,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }

    inline juce::Font monoFont (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                              height,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }

    // Painel de vidro fosco: superfície única, borda nítida e um fio de luz no topo.
    inline void paintPanel (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour (surface);
        g.fillRoundedRectangle (area, radius);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRect (juce::Rectangle<float> (area.getX() + radius, area.getY(),
                                            area.getWidth() - radius * 2.0f, 1.0f));

        g.setColour (line);
        g.drawRoundedRectangle (area.reduced (0.5f), radius, 1.0f);
    }

    // Micro-rótulo de seção ("01 / TRACK"), em caixa alta e cor apagada.
    inline void paintSectionTitle (juce::Graphics& g, juce::Rectangle<float> area,
                                   const juce::String& text)
    {
        g.setColour (muted);
        g.setFont (font (Type::section, true));
        g.drawText (text.toUpperCase(), area, juce::Justification::centredLeft);
    }

    // Selo retangular (estado/categoria). Cor = energia do aviso.
    inline void paintBadge (juce::Graphics& g, juce::Rectangle<float> area,
                            const juce::String& text, juce::Colour accent)
    {
        g.setColour (accent.withAlpha (0.12f));
        g.fillRoundedRectangle (area, radiusSmall);
        g.setColour (accent.withAlpha (0.45f));
        g.drawRoundedRectangle (area.reduced (0.5f), radiusSmall, 1.0f);

        g.setColour (accent);
        g.setFont (font (Type::caption, true));
        g.drawText (text.toUpperCase(), area.reduced (6.0f, 0.0f), juce::Justification::centred);
    }

    // Linha divisória horizontal de precisão.
    inline void paintRule (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour (line);
        g.fillRect (area.withHeight (1.0f));
    }

    // Trilho do visualizador: grade vertical de 20 px sobre gradiente azul-noite.
    inline void paintWaveBackdrop (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setGradientFill (juce::ColourGradient (surface.brighter (0.25f), area.getX(), area.getY(),
                                                 background.brighter (0.10f), area.getX(), area.getBottom(),
                                                 false));
        g.fillRoundedRectangle (area, radiusSmall);

        g.setColour (ice.withAlpha (0.07f));
        for (float x = area.getX() + 20.0f; x < area.getRight(); x += 20.0f)
            g.drawVerticalLine ((int) x, area.getY() + 1.0f, area.getBottom() - 1.0f);

        g.setColour (azure.withAlpha (0.18f));
        g.drawHorizontalLine ((int) area.getCentreY(), area.getX() + 1.0f, area.getRight() - 1.0f);

        g.setColour (line);
        g.drawRoundedRectangle (area.reduced (0.5f), radiusSmall, 1.0f);
    }

    // Anel do seletor (conic ice -> azure -> teal, aproximado por arcos).
    inline void paintRing (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto centre = area.getCentre();
        const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const auto ringArea = juce::Rectangle<float> (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);

        g.setColour (azure.withAlpha (0.35f));
        g.drawEllipse (ringArea, 2.0f);

        // JUCE 9 nao expoe drawArc: o arco conico e montado como Path.
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, r, r, 0.0f,
                           juce::MathConstants<float>::pi * 1.15f,
                           juce::MathConstants<float>::pi * 1.85f, true);
        g.setColour (ice.withAlpha (0.55f));
        g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

        const auto inner = ringArea.reduced (7.0f);
        g.setColour (panel);
        g.fillEllipse (inner);
        g.setColour (line);
        g.drawEllipse (inner, 1.0f);
    }
}
