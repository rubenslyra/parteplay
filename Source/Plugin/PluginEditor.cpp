#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "FilePlayer.h"
#include "ParameterIds.h"
#include "Text.h"
#include "Theme.h"
#include "Tuning.h"
#include "Waveform.h"

#include <cmath>

// Injetada pelo CMake a partir de project(...VERSION). O fallback existe para o
// arquivo continuar compilando fora do build - e nao traz digitos de proposito:
// a CI reprova qualquer literal X.Y.Z em Source/, que e o que mantem a versao
// mostrada na interface amarrada a fonte unica.
#ifndef PARTEPLAY_VERSION
 #define PARTEPLAY_VERSION "?"
#endif

// Overlay modal da carga. Componente de tela cheia: cobre a janela, recebe os
// cliques (bloqueando o que esta atras) e desenha um cartao central com a barra.
// Fica no escopo global - e nao no anonimo - porque o cabecalho guarda um
// unique_ptr<LoadingOverlay> por nome; um tipo de namespace anonimo nao casaria
// com a declaracao antecipada do cabecalho.
class LoadingOverlay : public juce::Component
{
public:
    LoadingOverlay()
    {
        setInterceptsMouseClicks (true, true);
    }

    void setState (float newProgress, const juce::String& newStage)
    {
        const auto clamped = juce::jlimit (0.0f, 1.0f, newProgress);
        if (std::abs (clamped - progress) < 1e-4f && newStage == stage)
            return;

        progress = clamped;
        stage = newStage;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        // Veu: cobre 100% da janela e escurece o que esta atras, que e o sinal
        // visual de "bloqueado".
        g.fillAll (juce::Colour (0xff000000).withAlpha (0.72f));

        auto area = getLocalBounds();
        const int cardW = juce::jmin (520, juce::jmax (320, area.getWidth() - 80));
        const int cardH = 196;
        auto card = juce::Rectangle<int> (cardW, cardH).withCentre (area.getCentre());

        g.setColour (Theme::panel.withAlpha (0.98f));
        g.fillRoundedRectangle (card.toFloat(), 14.0f);
        g.setColour (Theme::teal.withAlpha (0.85f));
        g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 14.0f, 1.5f);

        auto inner = card.reduced (28, 22);

        g.setColour (Theme::foreground);
        g.setFont (Theme::font (Theme::Type::title, true));
        g.drawText (Text::t ("Carregando o áudio"), inner.removeFromTop (28), juce::Justification::centredLeft);

        g.setColour (Theme::muted);
        g.setFont (Theme::font (Theme::Type::body));
        g.drawText (stage, inner.removeFromTop (22), juce::Justification::centredLeft);

        inner.removeFromTop (12);

        auto bar = inner.removeFromTop (14).toFloat();
        g.setColour (Theme::background.brighter (0.12f));
        g.fillRoundedRectangle (bar, 7.0f);

        const float fillWidth = juce::jmax (6.0f, bar.getWidth() * progress);
        g.setColour (Theme::teal);
        g.fillRoundedRectangle (bar.withWidth (fillWidth), 7.0f);

        inner.removeFromTop (10);
        g.setColour (Theme::muted);
        g.setFont (Theme::font (Theme::Type::caption, true));
        g.drawText (juce::String (juce::roundToInt (progress * 100.0f)) + " %",
                    inner.removeFromTop (18), juce::Justification::centredLeft);
    }

private:
    float progress = 0.0f;
    juce::String stage;
};

// Botao de creditos. Icone desenhado no codigo ("i" em circulo), sem asset e sem
// fonte de icones: mesma razao da bandeira vetorial do idioma.
class InfoButton : public juce::Button
{
public:
    InfoButton() : juce::Button ("creditos") {}

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override
    {
        auto area = getLocalBounds().toFloat().reduced (2.0f);
        const float diameter = juce::jmin (area.getWidth(), area.getHeight());
        const auto circle = juce::Rectangle<float> (diameter, diameter).withCentre (area.getCentre());

        const bool hot = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;
        g.setColour (hot ? Theme::teal.withAlpha (0.22f) : Theme::surface);
        g.fillEllipse (circle);
        g.setColour (hot ? Theme::teal : Theme::line);
        g.drawEllipse (circle.reduced (0.5f), 1.4f);

        g.setColour (hot ? Theme::foreground : Theme::ice);
        g.setFont (Theme::font (Theme::Type::control, true));
        g.drawText ("i", circle, juce::Justification::centred);
    }
};

// Conteudo do painel de creditos. Nomes proprios e URLs sao fatos (nao se
// traduzem); as descricoes passam por Text::t e acompanham o idioma ativo.
juce::String creditsBody()
{
    juce::String text;
    const juce::String bullet = Text::from ("\xE2\x80\xA2 ");

    const auto section = [&text] (const juce::String& title)
    {
        if (text.isNotEmpty())
            text << "\n";

        text << title.toUpperCase() << "\n";
    };

    const auto entry = [&text, &bullet] (const juce::String& line, const juce::String& home)
    {
        text << bullet << line << "\n";
        text << "      " << home << "\n";
    };

    section (Text::t ("Bibliotecas de áudio"));
    entry (Text::t ("SoundTouch / SoundStretch — processamento de áudio de código aberto por Olli Parviainen (Finlândia); análise de tempo e andamento."),
           "https://www.surina.net/soundtouch/  ·  LGPL-2.1");
    entry (Text::t ("FFmpeg / FFprobe / libavcodec — sistema multimídia multiplataforma iniciado por Fabrice Bellard em 2000; conversão de contêineres e codecs."),
           "https://ffmpeg.org/  ·  LGPL-2.1 / GPL");
    entry (Text::t ("Chromaprint — cálculo local da impressão digital acústica (LGPL-2.1)."),
           "https://acoustid.org/chromaprint  ·  LGPL-2.1");

    section (Text::t ("Interface e padrões"));
    entry (Text::t ("JUCE — ambiente C++ multiplataforma para plugins de áudio; arquitetura idealizada por Julian 'Jules' Storer."),
           "https://juce.com/  ·  AGPL-3.0 / comercial");
    entry (Text::t ("Steinberg VST3 (IComponent / IEditController) — especificação de plugin para compatibilidade em tempo real com hostes de áudio."),
           "https://steinbergmedia.github.io/vst3_doc/  ·  GPLv3 / proprietária");

    section (Text::t ("Ambiente de compilação"));
    entry (Text::t ("Compilado com Microsoft Visual Studio 2026 e Visual Studio Code em C++17."),
           "https://visualstudio.microsoft.com/");

    text << "\n" << Text::t ("Componentes de terceiros e suas licenças. O PartePlay é distribuído sob a GNU AGPL-3.0.") << "\n";

    return text;
}

// Overlay de creditos: cobre a janela, bloqueia o que esta atras e mostra um
// cartao rolavel com as tecnologias e as licencas. Fecha no botao, no Esc ou ao
// clicar fora do cartao.
class CreditsOverlay : public juce::Component
{
public:
    CreditsOverlay()
    {
        setInterceptsMouseClicks (true, true);

        closeButton.onClick = [this] { setVisible (false); };
        addAndMakeVisible (closeButton);

        body.setMultiLine (true);
        body.setReadOnly (true);
        body.setCaretVisible (false);
        body.setScrollbarsShown (true);
        body.setPopupMenuEnabled (false);
        body.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        body.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        body.setColour (juce::TextEditor::textColourId, Theme::foreground);
        body.setFont (Theme::font (Theme::Type::body));
        addAndMakeVisible (body);

        refreshTexts();
    }

    void refreshTexts()
    {
        closeButton.setButtonText (Text::t ("Fechar"));
        body.setText (creditsBody(), juce::dontSendNotification);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff000000).withAlpha (0.72f));

        const auto card = cardArea();
        g.setColour (Theme::panel.withAlpha (0.99f));
        g.fillRoundedRectangle (card.toFloat(), 14.0f);
        g.setColour (Theme::teal.withAlpha (0.85f));
        g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 14.0f, 1.5f);

        auto inner = card.reduced (24, 20);

        g.setColour (Theme::foreground);
        g.setFont (Theme::font (Theme::Type::title, true));
        g.drawText (Text::t ("Créditos e tecnologias"), inner.removeFromTop (28),
                    juce::Justification::centredLeft);

        g.setColour (Theme::line);
        g.fillRect (inner.removeFromTop (1));
    }

    void resized() override
    {
        auto card = cardArea().reduced (24, 20);
        card.removeFromTop (34);

        auto footer = card.removeFromBottom (34);
        closeButton.setBounds (footer.removeFromRight (120));

        card.removeFromBottom (8);
        body.setBounds (card);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! cardArea().contains (e.getPosition()))
            setVisible (false);
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key.getKeyCode() == juce::KeyPress::escapeKey)
        {
            setVisible (false);
            return true;
        }

        return false;
    }

    void visibilityChanged() override
    {
        if (isVisible())
            grabKeyboardFocus();
    }

private:
    juce::Rectangle<int> cardArea() const
    {
        const auto area = getLocalBounds();
        const int cardW = juce::jmin (640, juce::jmax (360, area.getWidth() - 80));
        const int cardH = juce::jmin (600, juce::jmax (320, area.getHeight() - 80));
        return juce::Rectangle<int> (cardW, cardH).withCentre (area.getCentre());
    }

    juce::TextEditor body;
    juce::TextButton closeButton;
};

namespace
{
    //==============================================================================
    // Larguras reservadas no canto direito do cabecalho, para o titulo nao
    // invadir o seletor de idioma nem os selos. Espelham PlayScoreEditor::resized
    // e PlayScoreEditor::paint: se um mudar sem o outro, o texto e o botao se
    // sobrepoem em vez de o botao sumir.
    constexpr int cultureSelectorWidth  = 220;
    constexpr int cultureLabelWidth    = 82;
    constexpr int cultureSelectorHeight = 38;

    // Botao de creditos: quadrado, logo a direita do seletor de idioma (ao lado
    // da bandeira). Entra em todas as contas de largura reservada do cabecalho.
    constexpr int creditsButtonWidth    = 38;
    constexpr int creditsButtonGap      = 8;

    //==============================================================================
    // Rodape: versao + credito. A versao vem da macro do CMake, entao trocar o
    // project(...VERSION) troca o que aparece aqui - nao ha numero escrito no
    // .cpp, que e o que a CI exige.
    juce::String footerCredit()
    {
        return juce::String (PARTEPLAY_VERSION) + " · " + Text::t ("Produção: Rubens Lyra");
    }

    //==============================================================================
    // Bandeira vetorial do idioma ativo. Substitui a seta do seletor: a seta nao
    // dizia nada ao usuario, enquanto a bandeira diz o idioma de relance. Tudo
    // desenhado no codigo, sem asset e sem dependencia de arquivo ou licenca.
    constexpr int languageFlagZone = 46;

    void drawLanguageFlag (juce::Graphics& g, juce::Rectangle<float> area, Text::Culture culture)
    {
        if (area.getWidth() < 6.0f || area.getHeight() < 4.0f)
            return;

        // Proporcao 3:2, centrada na zona reservada do combo.
        const float width  = juce::jmin (area.getWidth(), area.getHeight() * 1.5f);
        const float height = width / 1.5f;
        const juce::Rectangle<float> flag (area.getCentreX() - width * 0.5f,
                                           area.getCentreY() - height * 0.5f, width, height);

        const juce::Colour blue  (0xff012169);
        const juce::Colour red   (0xffc8102e);
        const juce::Colour white (0xffffffff);

        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (flag, 2.0f);
        g.reduceClipRegion (clip);

        if (culture == Text::Culture::PortugueseBR)
        {
            g.setColour (juce::Colour (0xff009739));
            g.fillRect (flag);

            juce::Path diamond;
            diamond.startNewSubPath (flag.getCentreX(), flag.getY());
            diamond.lineTo (flag.getRight(), flag.getCentreY());
            diamond.lineTo (flag.getCentreX(), flag.getBottom());
            diamond.lineTo (flag.getX(), flag.getCentreY());
            diamond.closeSubPath();
            g.setColour (juce::Colour (0xfffedd00));
            g.fillPath (diamond);

            const float diameter = height * 0.46f;
            g.setColour (blue);
            g.fillEllipse (juce::Rectangle<float> (flag.getCentreX() - diameter * 0.5f,
                                                   flag.getCentreY() - diameter * 0.5f,
                                                   diameter, diameter));
        }
        else if (culture == Text::Culture::EnglishUK)
        {
            g.setColour (blue);
            g.fillRect (flag);

            juce::Path diagonals;
            diagonals.startNewSubPath (flag.getX(), flag.getY());
            diagonals.lineTo (flag.getRight(), flag.getBottom());
            diagonals.startNewSubPath (flag.getRight(), flag.getY());
            diagonals.lineTo (flag.getX(), flag.getBottom());

            g.setColour (white);
            g.strokePath (diagonals, juce::PathStrokeType (height * 0.34f));
            g.setColour (red);
            g.strokePath (diagonals, juce::PathStrokeType (height * 0.16f));

            juce::Path cross;
            cross.startNewSubPath (flag.getCentreX(), flag.getY());
            cross.lineTo (flag.getCentreX(), flag.getBottom());
            cross.startNewSubPath (flag.getX(), flag.getCentreY());
            cross.lineTo (flag.getRight(), flag.getCentreY());

            g.setColour (white);
            g.strokePath (cross, juce::PathStrokeType (height * 0.34f));
            g.setColour (red);
            g.strokePath (cross, juce::PathStrokeType (height * 0.2f));
        }
        else if (culture == Text::Culture::EnglishUS)
        {
            g.setColour (white);
            g.fillRect (flag);

            const float stripe = height / 13.0f;
            g.setColour (red);
            for (int i = 0; i < 13; i += 2)
                g.fillRect (flag.getX(), flag.getY() + static_cast<float> (i) * stripe,
                            flag.getWidth(), stripe + 0.5f);

            const juce::Rectangle<float> canton (flag.getX(), flag.getY(),
                                                 flag.getWidth() * 0.42f, stripe * 7.0f);
            g.setColour (blue);
            g.fillRect (canton);

            g.setColour (white);
            const float dot = juce::jmax (1.0f, stripe * 0.28f);
            for (int row = 0; row < 4; ++row)
                for (int col = 0; col < 5; ++col)
                    g.fillEllipse (canton.getX() + (static_cast<float> (col) + 0.5f)
                                                   * canton.getWidth() / 5.0f - dot * 0.5f,
                                   canton.getY() + (static_cast<float> (row) + 0.5f)
                                                   * canton.getHeight() / 4.0f - dot * 0.5f,
                                   dot, dot);
        }
        else // SpanishES
        {
            g.setColour (juce::Colour (0xffaa151b));
            g.fillRect (flag);
            g.setColour (juce::Colour (0xfff1bf00));
            g.fillRect (flag.withTrimmedTop (height * 0.25f).withTrimmedBottom (height * 0.25f));
        }

        g.setColour (Theme::line);
        g.drawRoundedRectangle (flag, 2.0f, 1.0f);
    }

    //==============================================================================
    // Aparência única da interface (tokens em Theme.h).
    class StudioLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        StudioLookAndFeel()
        {
            setColour (juce::TextButton::buttonColourId,      Theme::panel);
            setColour (juce::TextButton::buttonOnColourId,    Theme::azure.withAlpha (0.25f));
            setColour (juce::TextButton::textColourOffId,     Theme::foreground);
            setColour (juce::TextButton::textColourOnId,      Theme::ice);

            setColour (juce::ToggleButton::textColourId,      Theme::foreground);
            setColour (juce::ToggleButton::tickColourId,      Theme::azure);
            setColour (juce::ToggleButton::tickDisabledColourId, Theme::line);

            setColour (juce::ComboBox::backgroundColourId,    Theme::panel);
            setColour (juce::ComboBox::textColourId,          Theme::foreground);
            setColour (juce::ComboBox::outlineColourId,       Theme::line);
            setColour (juce::ComboBox::arrowColourId,         Theme::azure);
            setColour (juce::ComboBox::buttonColourId,        Theme::azure);

            setColour (juce::PopupMenu::backgroundColourId,   Theme::panel);
            setColour (juce::PopupMenu::textColourId,         Theme::foreground);
            setColour (juce::PopupMenu::headerTextColourId,   Theme::muted);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::azure.withAlpha (0.28f));
            setColour (juce::PopupMenu::highlightedTextColourId, Theme::foreground);

            setColour (juce::Slider::thumbColourId,           Theme::ice);
            setColour (juce::Slider::trackColourId,           Theme::azure.withAlpha (0.45f));
            setColour (juce::Slider::backgroundColourId,      Theme::panel);
            setColour (juce::Slider::rotarySliderFillColourId, Theme::azure);
            setColour (juce::Slider::rotarySliderOutlineColourId, Theme::line);

            setColour (juce::Label::textColourId,             Theme::foreground);
            setColour (juce::Label::outlineColourId,          Theme::line);
            setColour (juce::Label::backgroundColourId,       Theme::panel);

            setColour (juce::TooltipWindow::backgroundColourId, Theme::panel);
            setColour (juce::TooltipWindow::textColourId,     Theme::foreground);
            setColour (juce::TooltipWindow::outlineColourId,  Theme::line);

            setColour (juce::FileChooserDialogBox::titleTextColourId, Theme::foreground);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                   const juce::Colour& background, bool over, bool down) override
        {
            auto area = button.getLocalBounds().toFloat().reduced (0.5f);
            const bool on = button.getToggleState();

            auto fill = on ? Theme::azure.withAlpha (0.22f) : background;

            if (! button.isEnabled())
                fill = Theme::panel;
            else if (down)
                fill = fill.brighter (0.14f);
            else if (over)
                fill = fill.brighter (0.06f);

            g.setColour (fill);
            g.fillRoundedRectangle (area, Theme::radiusSmall);

            g.setColour (on ? Theme::azure.withAlpha (0.85f) : Theme::line);
            g.drawRoundedRectangle (area, Theme::radiusSmall, 1.0f);
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            const bool on = button.getToggleState();
            const auto fontSize = static_cast<float> (juce::jlimit (10, 14, button.getHeight() / 2));

            g.setColour (! button.isEnabled() ? Theme::muted.withAlpha (0.55f)
                                              : (on ? Theme::ice : Theme::foreground));
            g.setFont (Theme::font (fontSize, on));
            g.drawText (button.getButtonText(), button.getLocalBounds().reduced (8, 1),
                        juce::Justification::centred);
        }

        void drawTickBox (juce::Graphics& g, juce::Component&, float x, float y, float w, float h,
                          bool ticked, bool, bool over, bool) override
        {
            const juce::Rectangle<float> box (x, y + (h - w) * 0.5f, w, w);

            g.setColour (Theme::panel);
            g.fillRoundedRectangle (box, 4.0f);
            g.setColour (ticked ? Theme::azure : (over ? Theme::ice.withAlpha (0.6f) : Theme::line));
            g.drawRoundedRectangle (box.reduced (0.5f), 4.0f, 1.0f);

            if (ticked)
            {
                juce::Path tick;
                tick.startNewSubPath (box.getX() + box.getWidth() * 0.24f, box.getCentreY());
                tick.lineTo (box.getCentreX() - box.getWidth() * 0.04f, box.getY() + box.getHeight() * 0.74f);
                tick.lineTo (box.getX() + box.getWidth() * 0.78f, box.getY() + box.getHeight() * 0.26f);

                g.setColour (Theme::ice);
                g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
            }
        }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float, float,
                               const juce::Slider::SliderStyle, juce::Slider& slider) override
        {
            const float trackHeight = 6.0f;
            auto track = juce::Rectangle<float> (static_cast<float> (x),
                                                 static_cast<float> (y) + static_cast<float> (height) * 0.5f - trackHeight * 0.5f,
                                                 static_cast<float> (width), trackHeight);

            g.setColour (Theme::panel);
            g.fillRoundedRectangle (track, 3.0f);

            g.setColour (slider.isEnabled() ? Theme::azure.withAlpha (0.55f) : Theme::line);
            g.fillRoundedRectangle (track.withRight (sliderPos), 3.0f);

            g.setColour (Theme::line);
            g.drawRoundedRectangle (track.reduced (0.5f), 3.0f, 1.0f);

            const float radius = static_cast<float> (juce::jmin (14, height - 2)) * 0.5f;
            const juce::Rectangle<float> thumb (sliderPos - radius, track.getCentreY() - radius,
                                                radius * 2.0f, radius * 2.0f);

            g.setColour (slider.isEnabled() ? Theme::ice : Theme::muted);
            g.fillEllipse (thumb);
            g.setColour (Theme::azure);
            g.drawEllipse (thumb.reduced (0.5f), 1.0f);
        }

        // O texto do item selecionado. Sem isto o seletor de idioma aparecia como
        // um retangulo vazio com uma seta: a JUCE delega ao LookAndFeel o desenho
        // do texto tambem, e o override antigo desenhava fundo, contorno e seta -
        // e nada mais.
        // Sem nome no parametro: e um override cujo corpo nao usa a caixa, e C4100 nao
        // e um aviso de codigo esquecido, e de nome que sobrou.
        juce::Font getComboBoxFont (juce::ComboBox&) override
        {
            return Theme::font (Theme::Type::control);
        }

        // O popup herda o token de controle em vez da fonte padrao da JUCE. Como a
        // altura da linha do popup e a altura do combo menos dois, aumentar o
        // token aumenta a fonte e a linha ao mesmo tempo.
        juce::Font getPopupMenuFont() override
        {
            return Theme::font (Theme::Type::control);
        }

        void setFlagCulture (Text::Culture culture)
        {
            flagCulture = culture;
        }

        // Reserva a zona da bandeira. Sem isto o label interno corre ate width-30
        // e o texto do idioma encosta na bandeira.
        void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
        {
            label.setBounds (10, 1,
                             juce::jmax (0, box.getWidth() - languageFlagZone - 10),
                             box.getHeight() - 2);
            label.setFont (getComboBoxFont (box));
            label.setJustificationType (juce::Justification::centredLeft);
        }

        void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                           juce::ComboBox& box) override
        {
            auto area = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width),
                                                static_cast<float> (height)).reduced (0.5f);

            g.setColour (Theme::panel);
            g.fillRoundedRectangle (area, Theme::radiusSmall);
            g.setColour (box.isEnabled() ? Theme::line : Theme::line.withAlpha (0.5f));
            g.drawRoundedRectangle (area, Theme::radiusSmall, 1.0f);

            // So a bandeira. O texto do item selecionado e desenhado pelo label
            // interno, posicionado por positionComboBoxText. O desenho manual de
            // texto que existia aqui foi REMOVIDO por completo: ele duplicava o
            // rotulo e aparecia um sobre o outro.
            drawLanguageFlag (g, area.removeFromRight (static_cast<float> (languageFlagZone)),
                              flagCulture);
        }

    private:
        Text::Culture flagCulture = Text::Culture::PortugueseBR;
    };

    //==============================================================================
    void styleCaption (juce::Label& label)
    {
        label.setFont (Theme::font (Theme::Type::section, true));
        label.setColour (juce::Label::textColourId, Theme::muted);
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void styleBody (juce::Label& label)
    {
        label.setFont (Theme::font (Theme::Type::body));
        label.setColour (juce::Label::textColourId, Theme::foreground);
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void styleMono (juce::Label& label, float height = Theme::Type::body)
    {
        label.setFont (Theme::monoFont (height));
        label.setColour (juce::Label::textColourId, Theme::foreground);
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void styleSlider (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    }

    //==============================================================================
    // Auxiliares de layout vertical.
    //
    // A JUCE clampa removeFromTop/removeFromBottom em silencio: pedir 26px a uma
    // area de 5px devolve 5px, e o componente seguinte acaba posicionado por cima
    // do anterior. Era esse o "ruido" na base do painel 05 - a nota e o botao de
    // exportar, ambos com 0px, desenhados no mesmo pixel.
    //
    // takeTop/takeBottom devolvem um retangulo vazio quando nao ha altura, e quem
    // chama esconde o componente; space() so consome se sobrar. Assim nenhum
    // tamanho de janela consegue produzir sobreposicao: no pior caso, o texto
    // opcional some em vez de virar borrao.
    juce::Rectangle<int> takeTop (juce::Rectangle<int>& area, int height)
    {
        if (area.getHeight() < height)
            return {};

        return area.removeFromTop (height);
    }

    juce::Rectangle<int> takeBottom (juce::Rectangle<int>& area, int height)
    {
        if (area.getHeight() < height)
            return {};

        return area.removeFromBottom (height);
    }

    void space (juce::Rectangle<int>& area, int height)
    {
        if (area.getHeight() > height)
            area.removeFromTop (height);
    }

    // Mostra e posiciona, ou esconde quando o retangulo veio vazio. Um retangulo
    // vazio significa que o espaco acabou: esconder e melhor do que sobrepor.
    void place (juce::Component& component, juce::Rectangle<int> bounds)
    {
        component.setVisible (! bounds.isEmpty());
        component.setBounds (bounds);
    }

    juce::String signedValue (int value)
    {
        return (value > 0 ? Text::from ("+") : Text::from ("")) + juce::String (value);
    }

    //==============================================================================
    // Visualizador: envelope min/max da fonte, trecho de loop e cursor do transport.
    class WaveformView : public juce::Component
    {
    public:
        void setPeaks (WaveformPtr newPeaks)
        {
            if (peaks == newPeaks)
                return;

            peaks = std::move (newPeaks);
            repaint();
        }

        void setPlayhead (double fraction)
        {
            const auto next = static_cast<float> (juce::jlimit (0.0, 1.0, fraction));
            if (juce::approximatelyEqual (playhead, next))
                return;

            playhead = next;
            repaint();
        }

        void setLoopRegion (bool active, double startFraction, double endFraction)
        {
            const auto nextStart = static_cast<float> (startFraction);
            const auto nextEnd   = static_cast<float> (endFraction);

            if (loopActive == active
                && juce::approximatelyEqual (loopStart, nextStart)
                && juce::approximatelyEqual (loopEnd, nextEnd))
                return;

            loopActive = active;
            loopStart  = nextStart;
            loopEnd    = nextEnd;
            repaint();
        }

        void setPlaceholder (const juce::String& text)
        {
            placeholder = text;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            const auto area = getLocalBounds().toFloat();
            Theme::paintWaveBackdrop (g, area);

            if (peaks == nullptr || peaks->isEmpty())
            {
                drawPlaceholder (g, area);
                return;
            }

            const auto envelope = buildEnvelope (area);

            if (loopActive && loopEnd > loopStart)
            {
                const juce::Rectangle<float> region (area.getX() + area.getWidth() * loopStart, area.getY(),
                                                     area.getWidth() * (loopEnd - loopStart), area.getHeight());

                g.setColour (Theme::signal.withAlpha (0.12f));
                g.fillRect (region.reduced (1.0f, 2.0f));
                g.setColour (Theme::signal.withAlpha (0.75f));
                g.drawVerticalLine (juce::jmax ((int) area.getX(), (int) region.getX()),
                                    area.getY() + 2.0f, area.getBottom() - 2.0f);
                g.drawVerticalLine (juce::jmin ((int) area.getRight() - 1, (int) region.getRight()),
                                    area.getY() + 2.0f, area.getBottom() - 2.0f);
            }

            // Trecho ainda não reproduzido.
            g.setColour (Theme::azure.withAlpha (0.38f));
            g.fillPath (envelope);

            // Trecho já reproduzido, em cor cheia (recorte pelo cursor).
            g.saveState();
            g.reduceClipRegion (area.toNearestInt()
                                    .withWidth (juce::jmax (0, (int) (area.getWidth() * playhead)))
                                    .withTrimmedRight (juce::jmax (0, (int) (area.getWidth() * (1.0f - playhead)))));
            g.setColour (Theme::azure);
            g.fillPath (envelope);
            g.restoreState();

            if (playhead > 0.0f && playhead < 1.0f)
            {
                g.setColour (Theme::signal);
                g.drawVerticalLine ((int) (area.getX() + area.getWidth() * playhead),
                                    area.getY() + 1.0f, area.getBottom() - 1.0f);
            }
        }

    private:
        void drawPlaceholder (juce::Graphics& g, const juce::Rectangle<float>& area)
        {
            if (placeholder.isEmpty())
                return;

            const auto font = Theme::font (Theme::Type::body);
            const float textWidth = juce::jmin (area.getWidth() - 32.0f,
                                                juce::GlyphArrangement::getStringWidth (font, placeholder) + 32.0f);
            const juce::Rectangle<float> chip (area.getCentreX() - textWidth * 0.5f,
                                               area.getCentreY() - 15.0f, textWidth, 30.0f);

            g.setColour (Theme::panel.withAlpha (0.88f));
            g.fillRoundedRectangle (chip, Theme::radiusSmall);
            g.setColour (Theme::line);
            g.drawRoundedRectangle (chip.reduced (0.5f), Theme::radiusSmall, 1.0f);

            g.setColour (Theme::muted);
            g.setFont (font);
            g.drawText (placeholder, chip, juce::Justification::centred);
        }

        // Uma coluna de pixels por amostra exibida: custo independente do arquivo.
        juce::Path buildEnvelope (const juce::Rectangle<float>& area) const
        {
            juce::Path path;
            const int columns = juce::jmax (2, static_cast<int> (area.getWidth()));
            const int buckets = peaks->numBuckets();
            const float midY  = area.getCentreY();
            const float half  = area.getHeight() * 0.44f;

            for (int column = 0; column < columns; ++column)
            {
                const int first = static_cast<int> (static_cast<double> (column) * buckets / columns);
                const int last  = juce::jmax (first + 1,
                                              static_cast<int> (static_cast<double> (column + 1) * buckets / columns));

                float low = 0.0f;
                float high = 0.0f;

                for (int b = first; b < juce::jmin (last, buckets); ++b)
                {
                    low  = juce::jmin (low, peaks->min[static_cast<size_t> (b)]);
                    high = juce::jmax (high, peaks->max[static_cast<size_t> (b)]);
                }

                const float x = area.getX() + static_cast<float> (column) + 0.5f;
                path.addLineSegment (juce::Line<float> (x, midY - high * half, x, midY - low * half), 1.0f);
            }

            return path;
        }

        WaveformPtr peaks;
        juce::String placeholder;
        float playhead = 0.0f;
        bool loopActive = false;
        float loopStart = 0.0f;
        float loopEnd = 0.0f;
    };

    //==============================================================================
    // Caixa de leitura: micro-rótulo + cifra isolada (contraste da escala).
    class StatTile : public juce::Component
    {
    public:
        void setContent (juce::String captionText, juce::String valueText,
                         juce::Colour accentColour = {})
        {
            const auto resolved = (accentColour == juce::Colour()) ? Theme::foreground : accentColour;

            if (caption == captionText && value == valueText && accent == resolved)
                return;

            caption = std::move (captionText);
            value   = std::move (valueText);
            accent  = resolved;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat().reduced (0.5f);

            g.setColour (Theme::panel);
            g.fillRoundedRectangle (area, Theme::radiusSmall);
            g.setColour (Theme::line);
            g.drawRoundedRectangle (area, Theme::radiusSmall, 1.0f);

            auto inner = area.reduced (10.0f, 7.0f);

            g.setColour (Theme::muted);
            g.setFont (Theme::font (Theme::Type::section, true));
            g.drawText (caption.toUpperCase(), inner.removeFromTop (12.0f), juce::Justification::centredLeft);

            g.setColour (accent);
            g.setFont (Theme::monoFont (static_cast<float> (juce::jmax (12.0f, inner.getHeight() * 0.72f)), true));
            g.drawText (value, inner, juce::Justification::centredLeft);
        }

    private:
        juce::String caption;
        juce::String value;
        juce::Colour accent { Theme::foreground };
    };

    //==============================================================================
    // Anel do seletor (símbolo da tonalidade ou da referência).
    class RingView : public juce::Component
    {
    public:
        void setSymbol (juce::String newSymbol)
        {
            if (symbol == newSymbol)
                return;

            symbol = std::move (newSymbol);
            repaint();
        }

        // Anel sem dado não deve competir com anel que tem. Apagar o destaque
        // (não o anel) é o que comunica "ainda não sei" sem parecer quebrado.
        void setDimmed (bool shouldBeDimmed)
        {
            if (dimmed == shouldBeDimmed)
                return;

            dimmed = shouldBeDimmed;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            const auto area = getLocalBounds().toFloat();
            Theme::paintRing (g, area);

            const float side = juce::jmin (area.getWidth(), area.getHeight());
            g.setColour (dimmed ? Theme::muted : Theme::azure);
            g.setFont (Theme::monoFont (side * 0.36f, true));
            g.drawText (symbol, area, juce::Justification::centred);
        }

    private:
        juce::String symbol;
        bool dimmed = false;
    };

    // Texto do estagio de carga. O identificador vem do nucleo (FilePlayer); a
    // traducao mora aqui, porque a tabela de strings e coisa da UI.
    juce::String loadingStageText (int stage)
    {
        switch (static_cast<FilePlayer::LoadStage> (stage))
        {
            case FilePlayer::LoadStage::preparing:  return Text::t ("Preparando o carregamento");
            case FilePlayer::LoadStage::decoding:   return Text::t ("Lendo o áudio");
            case FilePlayer::LoadStage::waveform:   return Text::t ("Desenhando a forma de onda");
            case FilePlayer::LoadStage::tempo:      return Text::t ("Estimando o andamento");
            case FilePlayer::LoadStage::tuning:     return Text::t ("Estimando a afinação");
            case FilePlayer::LoadStage::publishing: return Text::t ("Publicando o resultado");
        }

        return {};
    }

} // anonymous namespace

//==============================================================================
class TrackPanel : public EditorPanel
{
public:
    TrackPanel (PlayScoreEditor& owner, PlayScoreProcessor& p)
        : EditorPanel (p), editor (owner)
    {
        styleBody (fileNameLabel);
        fileNameLabel.setFont (Theme::font (Theme::Type::body, true));
        addAndMakeVisible (fileNameLabel);

        styleCaption (metaLabel);
        addAndMakeVisible (metaLabel);

        loadButton.onClick = [this] { editor.requestLoadAudio(); };
        addAndMakeVisible (loadButton);

        addAndMakeVisible (waveform);

        styleMono (positionLabel);
        positionLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (positionLabel);

        styleCaption (hintLabel);
        hintLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (hintLabel);

        styleMono (durationLabel);
        durationLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (durationLabel);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("01 / Faixa");
        hintLabel.setText (Text::t ("Sincronizado ao transporte do hospedeiro"), juce::dontSendNotification);
        waveform.setPlaceholder (Text::t ("Carregue um áudio para ver a forma de onda"));
        repaint();
    }

    void refreshState() override
    {
        const auto name = processor.getLoadedFileName();
        const bool hasFile = name.isNotEmpty();

        loadButton.setButtonText (Text::t (hasFile ? "Trocar áudio" : "Carregar áudio"));
        fileNameLabel.setText (hasFile ? name : Text::t ("Nenhum áudio carregado"), juce::dontSendNotification);
        fileNameLabel.setTooltip (hasFile ? name : juce::String());
        fileNameLabel.setColour (juce::Label::textColourId, hasFile ? Theme::foreground : Theme::muted);

        if (hasFile)
        {
            const auto extension = juce::File (name).getFileExtension().toUpperCase()
                                                              .removeCharacters (Text::from ("."));
            const auto megabytes = Text::number (static_cast<double> (processor.getFileSizeBytes())
                                                 / (1024.0 * 1024.0), 1);

            metaLabel.setText (Text::format (Text::t ("{0} · {1} MB"), { extension, megabytes }),
                             juce::dontSendNotification);
        }
        else
        {
            metaLabel.setText (Text::t ("WAV, FLAC ou MP3"), juce::dontSendNotification);
        }

        waveform.setPeaks (processor.getWaveformPeaks());

        const double duration = processor.getPlaybackDurationSeconds();
        const double rate     = processor.getTransportSampleRate();
        const double position = (rate > 0.0)
            ? static_cast<double> (processor.getTransportSample()) / rate
            : 0.0;

        positionLabel.setText (Text::timePrecise (position), juce::dontSendNotification);
        durationLabel.setText (Text::time (duration), juce::dontSendNotification);
        waveform.setPlayhead (duration > 0.0 ? position / duration : 0.0);

        double loopStart = 0.0;
        double loopEnd = 0.0;
        const bool loopActive = processor.getLoopFractions (loopStart, loopEnd);
        waveform.setLoopRegion (loopActive, loopStart, loopEnd);
    }

    void paint (juce::Graphics& g) override
    {
        paintChrome (g, title);
    }

    void resized() override
    {
        auto area = contentArea();

        auto header = takeTop (area, 42);
        loadButton.setBounds (header.removeFromRight (132).withHeight (30).translated (0, 6));
        header.removeFromRight (12);
        fileNameLabel.setBounds (header.removeFromTop (20));
        metaLabel.setBounds (header.removeFromTop (16));

        auto times = takeBottom (area, 20);
        positionLabel.setBounds (times.removeFromLeft (96));
        durationLabel.setBounds (times.removeFromRight (96));
        hintLabel.setBounds (times);

        space (area, 10);

        // A forma de onda e o elemento flexivel: absorve o que sobrar. Abaixo do
        // minimo ela some, em vez de virar uma tarja de poucos pixels.
        if (area.getHeight() >= 40)
            place (waveform, area);
        else
            waveform.setVisible (false);
    }

private:
    PlayScoreEditor& editor;

    juce::String title;
    juce::Label fileNameLabel;
    juce::Label metaLabel;
    juce::TextButton loadButton;
    WaveformView waveform;
    juce::Label positionLabel;
    juce::Label hintLabel;
    juce::Label durationLabel;
};

//==============================================================================
class TransportPanel : public EditorPanel
{
public:
    TransportPanel (PlayScoreEditor&, PlayScoreProcessor& p)
        : EditorPanel (p)
    {
        addAndMakeVisible (stateTile);
        addAndMakeVisible (positionTile);

        muteToggle.setToggleState (p.isOutputMuted(), juce::dontSendNotification);
        addAndMakeVisible (muteToggle);
        muteToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            p.parameters, Parameter::muted, muteToggle);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("02 / Transporte");
        muteToggle.setButtonText (Text::t ("Silenciar saída"));
        repaint();
    }

    void refreshState() override
    {
        const bool playing = processor.isTransportPlaying();
        const bool hasAudio = processor.getDurationSeconds() > 0.0;

        juce::String state;
        juce::Colour accent = Theme::muted;

        if (playing && processor.hasReachedEndOfFile())
        {
            state  = Text::t ("Fim do arquivo");
            accent = Theme::signal;
        }
        else if (playing)
        {
            state  = Text::t ("Tocando");
            accent = Theme::teal;
        }
        else if (hasAudio)
        {
            state  = Text::t ("Pausado");
            accent = Theme::ice;
        }
        else
        {
            state = Text::t ("Aguardando o hospedeiro");
        }

        stateTile.setContent (Text::t ("Estado"), state, accent);

        const double rate = processor.getTransportSampleRate();
        const double position = (rate > 0.0)
            ? static_cast<double> (processor.getTransportSample()) / rate
            : 0.0;

        positionTile.setContent (Text::t ("Posição"), Text::timePrecise (position), Theme::foreground);
    }

    void paint (juce::Graphics& g) override
    {
        paintChrome (g, title);
    }

    void resized() override
    {
        auto area = contentArea();

        auto tiles = takeTop (area, 56);
        juce::FlexBox tileRow;
        tileRow.flexDirection = juce::FlexBox::Direction::row;
        tileRow.items.add (juce::FlexItem (stateTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 0, 0)));
        tileRow.items.add (juce::FlexItem (positionTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 0, 0, 4)));
        tileRow.performLayout (tiles);

        space (area, 12);

        auto toggles = takeTop (area, 28);
        juce::FlexBox toggleRow;
        toggleRow.flexDirection = juce::FlexBox::Direction::row;
        toggleRow.items.add (juce::FlexItem (muteToggle).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 0, 0, 4)));
        toggleRow.performLayout (toggles);
    }

private:
    juce::String title;
    StatTile stateTile;
    StatTile positionTile;
    juce::ToggleButton muteToggle;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteToggleAttachment;
};

//==============================================================================
// Painel 03 — ficha da canção. Substituiu a tabela de instrumentos: a v1.x não
// escolhe mais instrumento porque a reprodução já vem no tom do piano, e o que
// o músico precisa confronting uma gravação é a identidade dela (título, ISRC,
// ano) e o andamento original.
//
// Cada campo exibe também DE ONDE veio. Isso não é enfeite: o BPM é estimativa
// do áudio, enquanto título/ISRC/ano vêm da tag do arquivo e, mais adiante, da
// consulta ao MusicBrainz. Sem marcar a origem, um número estimado e um dado
// cadastral pareceriam igualmente confiáveis — e não são.
class SongSheetPanel : public EditorPanel
{
public:
    SongSheetPanel (PlayScoreEditor&, PlayScoreProcessor& p)
        : EditorPanel (p)
    {
        addAndMakeVisible (ring);

        titleLabel.setFont (Theme::font (17.0f, true));
        titleLabel.setColour (juce::Label::textColourId, Theme::foreground);
        addAndMakeVisible (titleLabel);

        styleCaption (originLabel);
        addAndMakeVisible (originLabel);

        styleCaption (isrcCaption);
        addAndMakeVisible (isrcCaption);
        styleMono (isrcValue);
        isrcValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (isrcValue);

        styleCaption (yearCaption);
        addAndMakeVisible (yearCaption);
        styleMono (yearValue);
        yearValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (yearValue);

        styleCaption (bpmCaption);
        addAndMakeVisible (bpmCaption);
        styleMono (bpmValue);
        bpmValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (bpmValue);

        styleCaption (idCaption);
        addAndMakeVisible (idCaption);
        idValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (idValue);

        styleCaption (noticeLabel);
        noticeLabel.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (noticeLabel);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("03 / Ficha da canção");

        isrcCaption.setText (Text::t ("ISRC do fonograma"), juce::dontSendNotification);
        yearCaption.setText (Text::t ("Ano de publicação"), juce::dontSendNotification);
        bpmCaption.setText (Text::t ("BPM original"), juce::dontSendNotification);
        idCaption.setText (Text::t ("Impressão digital"), juce::dontSendNotification);

        // O sufixo do BPM vai pela tabela como o resto. Montado com Text::from
        // ele sairia em pt-BR dentro de um painel em inglês — e o usuário leria
        // "128.0 · estimado" sem entender por que a única palavra está errada.
        bpmEstimatedSuffix = Text::t (" \xC2\xB7 estimado");
        repaint();
    }

    void refreshState() override
    {
        const auto songTitle = processor.getSongTitle();
        const auto isrc      = processor.getSongIsrc();
        const auto year      = processor.getSongYear();
        const double bpm     = processor.getAudioBpm();

        // O anel carrega o BPM porque e o dado que se procura de relance. Este
        // painel mostra em PONTO FIXO (inteiro): a ficha da cancao registra o
        // valor redondo. O valor fino, em PONTO FLUTUANTE, e o painel 05.
        const bool hasBpm = bpm > 0.0;
        ring.setSymbol (hasBpm ? compactNumber (bpm) : juce::String ("--"));
        ring.setDimmed (! hasBpm);

        titleLabel.setText (songTitle.isNotEmpty() ? songTitle
                                                   : Text::t ("Sem título nas tags"),
                           juce::dontSendNotification);

        originLabel.setText (Text::t ("Lido das tags do arquivo"),
                             juce::dontSendNotification);

        // Campo ausente não vira invenção: mostra o traço e diz por quê. A
        // mensagem é diferente para "o arquivo não diz" e "não há áudio", porque
        // o usuário age de forma diferente em cada caso.
        isrcValue.setText (isrc.isNotEmpty() ? formatIsrc (isrc) : missingValue(),
                           juce::dontSendNotification);
        yearValue.setText (year.isNotEmpty() ? year : missingValue(),
                           juce::dontSendNotification);
        bpmValue.setText (hasBpm ? (compactNumber (bpm) + bpmEstimatedSuffix)
                                 : missingValue(),
                           juce::dontSendNotification);

        idValue.setText (describeIdentification (processor.getIdentificationState(),
                                                processor.getIdentificationFailure()),
                         juce::dontSendNotification);

        noticeLabel.setText (buildNotice (songTitle.isNotEmpty(), isrc.isNotEmpty(),
                                          year.isNotEmpty(), hasBpm,
                                          processor.getIdentificationState()),
                             juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        paintChrome (g, title);
    }

    void resized() override
    {
        auto area = contentArea();

        auto header = takeTop (area, 56);
        ring.setBounds (header.removeFromLeft (56));
        header.removeFromLeft (12);
        titleLabel.setBounds (header.removeFromTop (24));
        originLabel.setBounds (header.removeFromTop (18));

        space (area, 10);

        // Linhas rótulo/valor: a coluna de rótulos tem largura fixa para os
        // três alinharem, e o valor ocupa o resto à direita.
        constexpr int rowHeight = 24;
        constexpr int rowGap    = 4;
        constexpr int captionWidth = 150;

        auto addRow = [&] (juce::Label& caption, juce::Label& value)
        {
            auto row = takeTop (area, rowHeight);
            caption.setVisible (! row.isEmpty());
            value.setVisible (! row.isEmpty());
            caption.setBounds (row.removeFromLeft (captionWidth));
            value.setBounds (row);
            space (area, rowGap);
        };

        addRow (isrcCaption, isrcValue);
        addRow (yearCaption, yearValue);
        addRow (bpmCaption, bpmValue);
        addRow (idCaption, idValue);

        // O aviso e o ultimo: absorve o que sobrar e some quando nao ha altura
        // para duas linhas de texto.
        if (area.getHeight() >= 28)
            place (noticeLabel, area);
        else
            noticeLabel.setVisible (false);
    }

private:
    // Campo ausente mostra um travessão. Construído por Text::from com escapes
    // porque juce::String (const char*) lê os bytes como Latin-1: o travessão
    // são 3 bytes e virariam 3 caracteres que não formam nada. Um hífen ASCII
    // evitaria o problema, mas o travessão é o que distingue "não sei" de
    // "valor zero" para quem lê a ficha.
    static juce::String missingValue() { return Text::from ("\xE2\x80\x94"); }

    static juce::String compactNumber (double value)
    {
        const auto rounded = std::round (value);
        return juce::String ((int) rounded);
    }

    // BRABC1234567 -> BR-ABC-12-34567. O formato com hífen é o de exibição
    // (ISO 3901:2001); o canônico, sem separador, é o que a API devolve.
    static juce::String formatIsrc (const juce::String& raw)
    {
        if (raw.length() != 12)
            return raw;

        return raw.substring (0, 2) + "-" + raw.substring (2, 5) + "-"
             + raw.substring (5, 7) + "-" + raw.substring (7, 12);
    }

    // Traduz o estado do worker para o que o usuário lê.
    //
    // Fica curto de propósito: a coluna de valor tem pouco mais de 100 px na
    // largura mínima da janela, e esta linha divide espaço com três números. A
    // explicação inteira vive no aviso embaixo, que ocupa a largura toda e
    // quebra linha.
    //
    // O ponto que não pode ser atingido: `ready` quer dizer "a impressão digital foi
    // calculada localmente", e NÃO "a faixa foi identificada". Nada saiu daqui
    // para o AcoustID ainda — a consulta online não existe no código. Um texto
    // que dissesse "identificada" seria mentira na tela, e é exatamente o tipo
    // de erro que a ficha não pode cometer.
    static juce::String describeIdentification (Fingerprint::State state,
                                               Fingerprint::Failure failure)
    {
        switch (state)
        {
            case Fingerprint::State::idle:
                return missingValue();

            case Fingerprint::State::running:
                return Text::t ("analisando…");

            case Fingerprint::State::ready:
                return Text::t ("pronta");

            case Fingerprint::State::cancelled:
                return Text::t ("cancelada");

            case Fingerprint::State::failed:
                break;
        }

        switch (failure)
        {
            case Fingerprint::Failure::fileNotFound:
                return Text::t ("arquivo ausente");
            case Fingerprint::Failure::unsupportedFormat:
                return Text::t ("formato inválido");
            case Fingerprint::Failure::noChannels:
                return Text::t ("sem canais");
            case Fingerprint::Failure::sampleRateTooLow:
                return Text::t ("amostragem baixa");
            case Fingerprint::Failure::tooShort:
                return Text::t ("áudio curto");
            case Fingerprint::Failure::readFailed:
                return Text::t ("falha na leitura");
            case Fingerprint::Failure::emptyFingerprint:
                return Text::t ("impressão vazia");
            case Fingerprint::Failure::algorithmFailed:
            case Fingerprint::Failure::none:
            default:
                return Text::t ("falha");
        }
    }

    static juce::String buildNotice (bool hasTitle, bool hasIsrc, bool hasYear, bool hasBpm,
                                     Fingerprint::State idState)
    {
        if (! hasTitle && ! hasIsrc && ! hasYear && ! hasBpm
            && idState == Fingerprint::State::idle)
            return Text::t ("Carregue um arquivo de áudio para ler a ficha.");

        // Impressão digital pronta é informação que o usuário precisa, e o
        // aviso é o único lugar com espaço para dizer com precisão o que ela
        // significa — e o que ainda não significa.
        if (idState == Fingerprint::State::ready)
            return Text::t ("Impressão digital calculada localmente. Cruzá-la com o "
                            "AcoustID exige rede e ainda não está habilitado.");

        if (hasIsrc && hasTitle && hasYear)
            return {};

        return Text::t ("Campos sem valor não constam no arquivo. A impressão digital "
                        "é calculada localmente; cruzá-la com o AcoustID exige rede "
                        "e ainda não está habilitado.");
    }

    juce::String title;
    juce::String bpmEstimatedSuffix;
    RingView ring;
    juce::Label titleLabel;
    juce::Label originLabel;
    juce::Label isrcCaption, isrcValue;
    juce::Label yearCaption, yearValue;
    juce::Label bpmCaption, bpmValue;
    juce::Label idCaption, idValue;
    juce::Label noticeLabel;
};

//==============================================================================
class TuningPanel : public EditorPanel
{
public:
    TuningPanel (PlayScoreEditor&, PlayScoreProcessor& p)
        : EditorPanel (p)
    {
        ring.setSymbol (Text::from ("A4"));
        addAndMakeVisible (ring);

        styleCaption (referenceCaption);
        addAndMakeVisible (referenceCaption);

        referenceValue.setFont (Theme::monoFont (Theme::Type::value, true));
        referenceValue.setColour (juce::Label::textColourId, Theme::foreground);
        referenceValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (referenceValue);

        addAndMakeVisible (fileTuningTile);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("04 / Afinação");
        referenceCaption.setText (Text::t ("Referência · A4"), juce::dontSendNotification);
        repaint();
    }

    void refreshState() override
    {
        const double reference = processor.getReferencePitchHz();

        referenceValue.setText (juce::String (reference, 0) + Text::from (" Hz"), juce::dontSendNotification);

        const double detectedHz = processor.getDetectedTuningHz();
        const auto detectedCents = static_cast<int> (std::lround (processor.getDetectedTuningCents()));

        fileTuningTile.setContent (Text::t ("Arquivo"),
                                   Text::format (Text::t ("A = {0} Hz ({1} cents)"),
                                                 { Text::number (detectedHz, 1), signedValue (detectedCents) }),
                                   Theme::foreground);
    }

    void paint (juce::Graphics& g) override
    {
        paintChrome (g, title);
    }

    void resized() override
    {
        auto area = contentArea();

        auto header = takeTop (area, 48);
        ring.setBounds (header.removeFromLeft (48));
        header.removeFromLeft (12);
        referenceCaption.setBounds (header.removeFromTop (16));
        referenceValue.setBounds (header.removeFromTop (26));

        space (area, 8);

        // O tile de afinacao do arquivo e informativo: e o que cede em janela
        // menor, antes de qualquer controle.
        if (area.getHeight() >= 48)
            place (fileTuningTile, takeTop (area, juce::jmin (56, area.getHeight())));
        else
            fileTuningTile.setVisible (false);
    }

private:
    juce::String title;
    RingView ring;
    juce::Label referenceCaption;
    juce::Label referenceValue;
    StatTile fileTuningTile;
};

//==============================================================================
class MeterPanel : public EditorPanel
{
public:
    MeterPanel (PlayScoreEditor& owner, PlayScoreProcessor& p)
        : EditorPanel (p), editor (owner)
    {
        addAndMakeVisible (bpmTile);
        addAndMakeVisible (barsTile);
        addAndMakeVisible (durationTile);

        // Botao de fonte do BPM: alterna entre o valor real medido pelo
        // SoundStretch e o padrao corrigido (x2). Nao e automacao: e escolha de
        // leitura, entao nao entra no APVTS.
        bpmSourceButton.onClick = [this]
        {
            processor.setBpmDoubled (! processor.isBpmDoubled());
            refreshState();
            repaint();
        };
        addAndMakeVisible (bpmSourceButton);

        exportButton.onClick = [this] { editor.requestExportMap(); };
        addAndMakeVisible (exportButton);

        styleCaption (noteLabel);
        noteLabel.setColour (juce::Label::textColourId, Theme::muted.withAlpha (0.85f));
        addAndMakeVisible (noteLabel);

        styleCaption (trimLabel);
        trimLabel.setColour (juce::Label::textColourId, Theme::signal);
        trimLabel.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (trimLabel);

        styleCaption (messageLabel);
        messageLabel.setColour (juce::Label::textColourId, Theme::teal);
        addAndMakeVisible (messageLabel);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("05 / BPM & Compassos");
        exportButton.setButtonText (Text::t ("Exportar Mapa (.mid)"));
        noteLabel.setText (Text::t ("Estimado do áudio — sugestão; a métrica do hospedeiro é a definitiva."),
                           juce::dontSendNotification);
        repaint();
    }

    void refreshState() override
    {
        const double bpm = processor.getAudioBpm();
        const bool hasAnalysis = bpm > 0.0 && processor.getDurationSeconds() > 0.0;
        const auto empty = Text::t ("--");

        bpmTile.setContent (Text::t ("BPM"),
                            hasAnalysis ? Text::number (bpm, 3) : empty,
                            hasAnalysis ? Theme::ice : Theme::muted);

        barsTile.setContent (Text::t ("Compassos"),
                             hasAnalysis ? juce::String (processor.getMeasureCount()) : empty,
                             hasAnalysis ? Theme::foreground : Theme::muted);

        durationTile.setContent (Text::t ("Duração"),
                                 hasAnalysis ? Text::time (processor.getDurationSeconds()) : empty,
                                 hasAnalysis ? Theme::foreground : Theme::muted);

        bpmSourceButton.setButtonText (processor.isBpmDoubled()
                                           ? Text::t ("Fonte: padrão (×2)")
                                           : Text::t ("Fonte: real (SoundStretch)"));

        // Aviso de adaptacao: o audio foi cortado no inicio para casar com o
        // compasso 1 do editor. Sem isso o usuario nao saberia por que a onda
        // comeca diferente do arquivo original.
        if (processor.wasAudioTrimmed())
        {
            trimLabel.setText (Text::t ("Áudio ajustado") + ": "
                                   + Text::time (processor.getLeadingSilenceSeconds())
                                   + Text::t (" de silêncio inicial removido para alinhar ao início do editor."),
                               juce::dontSendNotification);
            trimLabel.setVisible (true);
        }
        else
        {
            trimLabel.setVisible (false);
        }

        exportButton.setEnabled (hasAnalysis);
        messageLabel.setText (editor.exportMessage(), juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        paintChrome (g, title);
    }

    void resized() override
    {
        auto area = contentArea();

        auto tilesRow = takeTop (area, 56);

        juce::FlexBox row;
        row.flexDirection = juce::FlexBox::Direction::row;
        row.items.add (juce::FlexItem (bpmTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 0, 0)));
        row.items.add (juce::FlexItem (barsTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 0, 4)));
        row.items.add (juce::FlexItem (durationTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 0, 0, 4)));
        row.performLayout (tilesRow);

        space (area, 6);

        place (bpmSourceButton, takeTop (area, 26));
        space (area, 6);

        place (exportButton, takeTop (area, 30));
        space (area, 6);

        // Os textos vem de baixo para cima, nesta ordem: a mensagem do exportar
        // fica colada no rodape, o aviso de audio ajustado acima dela e a nota de
        // estimativa por ultimo. Como takeBottom devolve vazio quando nao cabem,
        // em janela baixa eles somem um a um, a nota primeiro.
        place (messageLabel, takeBottom (area, 18));
        place (trimLabel, takeBottom (area, 18));
        place (noteLabel, takeBottom (area, 28));
    }

private:
    PlayScoreEditor& editor;
    juce::String title;
    StatTile bpmTile, barsTile, durationTile;
    juce::TextButton bpmSourceButton;
    juce::TextButton exportButton;
    juce::Label noteLabel;
    juce::Label trimLabel;
    juce::Label messageLabel;
};

//==============================================================================
EditorPanel::~EditorPanel() = default;

EditorPanel::EditorPanel (PlayScoreProcessor& owner)
    : processor (owner)
{
}

juce::Rectangle<int> EditorPanel::contentArea() const
{
    return getLocalBounds().reduced (padding, padding).withTrimmedTop (titleHeight);
}

juce::Rectangle<int> EditorPanel::paintChrome (juce::Graphics& g, const juce::String& sectionTitle)
{
    Theme::paintPanel (g, getLocalBounds().toFloat().reduced (0.5f));

    if (sectionTitle.isNotEmpty())
    {
        const auto titleArea = getLocalBounds().reduced (padding, padding)
                                               .removeFromTop (titleHeight)
                                               .toFloat();
        Theme::paintSectionTitle (g, titleArea, sectionTitle);
    }

    return contentArea();
}

//==============================================================================
PlayScoreEditor::PlayScoreEditor (PlayScoreProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    lookAndFeel = std::make_unique<StudioLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());
    static_cast<StudioLookAndFeel*> (lookAndFeel.get())->setFlagCulture (Text::getCulture());

    // keyPressed so chega ao editor se ele tiver o foco de teclado. Sem isto o
    // Ctrl+D e ignorado quando a janela do plugin abre dentro do hospedeiro.
    setWantsKeyboardFocus (true);

    trackPanel      = std::make_unique<TrackPanel> (*this, p);
    transportPanel  = std::make_unique<TransportPanel> (*this, p);
    songSheetPanel  = std::make_unique<SongSheetPanel> (*this, p);
    tuningPanel     = std::make_unique<TuningPanel> (*this, p);
    meterPanel      = std::make_unique<MeterPanel> (*this, p);

    panels = { trackPanel.get(), transportPanel.get(), songSheetPanel.get(),
               tuningPanel.get(), meterPanel.get() };

    for (auto* panel : panels)
        addAndMakeVisible (panel);

    styleCaption (cultureLabel);
    // O rotulo fica no token de controle para acompanhar o combo, e nao no de
    // secao que styleCaption aplica por padrao: "Idioma:" e o par do seletor, e
    // a 1,5px de distancia vertical as duas palavras pareciam de campos diferentes.
    cultureLabel.setFont (Theme::font (Theme::Type::control, true));
    addAndMakeVisible (cultureLabel);

    // Cultura ativa == índice da enumeração (PortugueseBR=0 ... SpanishES=3),
    // então SelectedId = 1 + int(Culture). Os nomes vêm do Text, que é a fonte
    // única dos rótulos; aqui não se escreve nome de idioma.
    for (const auto culture :
         { Text::Culture::PortugueseBR, Text::Culture::EnglishUK,
           Text::Culture::EnglishUS, Text::Culture::SpanishES })
        cultureSelector.addItem (Text::cultureName (culture), 1 + static_cast<int> (culture));
    cultureSelector.setSelectedId (1 + static_cast<int> (Text::getCulture()), juce::dontSendNotification);
    cultureSelector.onChange = [this]
    {
        Text::setCulture (static_cast<Text::Culture> (cultureSelector.getSelectedId() - 1));
        languageChanged();
    };
    addAndMakeVisible (cultureSelector);

    // Botao de informacoes a direita do seletor, ao lado da bandeira do idioma.
    creditsButton = std::make_unique<InfoButton>();
    creditsButton->setTooltip (Text::t ("Créditos e tecnologias"));
    creditsButton->onClick = [this]
    {
        creditsOverlay->refreshTexts();
        creditsOverlay->setVisible (true);
        creditsOverlay->toFront (true);
    };
    addAndMakeVisible (creditsButton.get());

    // Overlay por ultimo: como ultima crianca, desenha acima de todos os paineis.
    // Invisivel ate uma carga comecar.
    loadingOverlay = std::make_unique<LoadingOverlay>();
    addAndMakeVisible (loadingOverlay.get());
    loadingOverlay->setVisible (false);

    // O overlay de creditos fica acima ate do de carga: abrir as informacoes nao
    // pode ser encoberto por uma barra de progresso.
    creditsOverlay = std::make_unique<CreditsOverlay>();
    addAndMakeVisible (creditsOverlay.get());
    creditsOverlay->setVisible (false);

    // Proporção 5:4 (referência visual 2000x1600), redimensionável pelo hospedeiro.
    // O padrão é o menor tamanho em que os cinco painéis mostram todo o conteúdo,
    // inclusive as notas de rodapé. O mínimo fica abaixo dele de propósito: em
    // janela menor o essencial continua visível e só as notas cedem, porque cada
    // painel posiciona o opcional com takeBottom e esconde o que não couber. Não
    // subo o mínimo para os 896 de altura porque telas de 1366x768 (notebook
    // comum) não comportam a janela; a contrapartida é que nelas as notas somem.
    setSize (1120, 896);
    setResizable (true, false);
    setResizeLimits (1000, 800, 1600, 1280);

    // "boundsConstrainer" e nao "constrainer" por causa de C4458: juce::Component
    // ja tem um membro chamado constrainer.
    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (5.0 / 4.0);

    refreshAllTexts();
    refreshAllState();

    startTimerHz (20);
}

PlayScoreEditor::~PlayScoreEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::Rectangle<int> PlayScoreEditor::headerArea() const
{
    return getLocalBounds().removeFromTop (margin + headerHeight)
                            .withTrimmedTop (margin)
                            .reduced (margin, 0);
}

juce::Rectangle<int> PlayScoreEditor::footerArea() const
{
    return getLocalBounds().removeFromBottom (margin + footerHeight)
                            .withTrimmedBottom (margin)
                            .reduced (margin, 0);
}

juce::Rectangle<int> PlayScoreEditor::bodyArea() const
{
    auto area = getLocalBounds();
    area.removeFromTop (margin + headerHeight);
    area.removeFromBottom (margin + footerHeight);
    return area.reduced (margin, 0);
}

void PlayScoreEditor::resized()
{
    // O bloco do idioma vive na direita do cabecalho. reduced(), e nao
    // removeFromRight(): removeFromRight devolve a faixa removida - os 20px da
    // margem - entao cultureBounds ficava com 20px de largura. O combo era
    // comprimido contra a borda direita e o popup da JUCE, que se alinha pela
    // borda esquerda do combo, abria para fora da janela: era assim que
    // "Espanol (Espana)" aparecia cortado. O rotulo, no segundo removeFromRight,
    // encolhia a largura zero e sumia.
    auto cultureBounds = getLocalBounds().removeFromTop (margin + headerHeight)
                                            .withTrimmedTop (margin)
                                            .reduced (margin, 0);

    const int cultureTop = 12;

    // A altura do combo e a altura da linha do popup, por acaso da JUCE e nao por
    // escolha: positionComboBoxText pune o label interno com alturaDoCombo - 2, e
    // getOptionsForComboBoxPopupMenu usa essa mesma altura como standardItemHeight.
    // Com 24px de combo a linha do popup saia com 22px e um texto espremido - foi
    // o que o usuario reportou como menu "minusculo". Subir o combo para 38px
    // corrige os dois de uma vez, sem precisar mexer em ItemHeight.
    // O botao de creditos fica no canto, a direita do seletor: assim a bandeira
    // do combo e o "i" ficam lado a lado, como pedido.
    creditsButton->setBounds (cultureBounds.removeFromRight (creditsButtonWidth)
                                           .withTrimmedTop (cultureTop)
                                           .withHeight (cultureSelectorHeight));
    cultureBounds.removeFromRight (creditsButtonGap);

    cultureSelector.setBounds (cultureBounds.removeFromRight (cultureSelectorWidth)
                                              .withTrimmedTop (cultureTop)
                                              .withHeight (cultureSelectorHeight));
    cultureLabel.setBounds (cultureBounds.removeFromRight (cultureLabelWidth)
                                           .withTrimmedTop (cultureTop)
                                           .withHeight (cultureSelectorHeight));

    auto body = bodyArea();

    const int leftWidth = static_cast<int> (static_cast<float> (body.getWidth() - gap) * 0.63f);
    auto leftColumn  = body.removeFromLeft (leftWidth);
    body.removeFromLeft (gap);
    auto rightColumn = body;

    {
        const int trackHeight = static_cast<int> (static_cast<float> (leftColumn.getHeight() - gap) * 0.60f);
        trackPanel->setBounds (leftColumn.removeFromTop (trackHeight));
        leftColumn.removeFromTop (gap);
        transportPanel->setBounds (leftColumn);
    }

    {
        const int total = rightColumn.getHeight() - gap * 2;
        // As proporcoes seguem a altura que cada painel precisa, e nao numeros
        // redondos: com 0.38/0.36/resto o painel 05 ficava 26px abaixo do proprio
        // conteudo, e era o unico dos tres a cortar no tamanho padrao.
        const int songSheetHeight = static_cast<int> (static_cast<float> (total) * 0.365f);
        const int tuningHeight    = static_cast<int> (static_cast<float> (total) * 0.345f);

        songSheetPanel->setBounds (rightColumn.removeFromTop (songSheetHeight));
        rightColumn.removeFromTop (gap);
        tuningPanel->setBounds (rightColumn.removeFromTop (tuningHeight));
        rightColumn.removeFromTop (gap);
        meterPanel->setBounds (rightColumn);
    }

    // Os overlays cobrem a janela inteira; reafirmam-se no topo a cada layout. Os
    // creditos ficam acima do de carga quando visiveis.
    loadingOverlay->setBounds (getLocalBounds());
    loadingOverlay->toFront (false);

    creditsOverlay->setBounds (getLocalBounds());

    if (creditsOverlay->isVisible())
        creditsOverlay->toFront (false);
}

void PlayScoreEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (Theme::panel.brighter (0.05f), bounds.getX(), bounds.getY(),
                                             Theme::background, bounds.getCentreX(), bounds.getBottom(),
                                             false));
    g.fillAll();

    // Cabeçalho
    const auto header = headerArea();
    const int logoSide = 42;

    // "headerCursor" e nao "cursor" por causa de C4458: juce::Component ja tem um
    // membro chamado cursor.
    juce::Rectangle<int> headerCursor = header;

#if JUCE_TARGET_HAS_BINARY_DATA
    const juce::Image logo = juce::ImageCache::getFromMemory (BinaryData::PartePlayIcon_png,
                                                              BinaryData::PartePlayIcon_pngSize);

    if (logo.isValid())
    {
        const auto logoArea = headerCursor.removeFromLeft (logoSide).toFloat();
        g.drawImageWithin (logo, static_cast<int> (logoArea.getX()), static_cast<int> (logoArea.getY()),
                           logoSide, logoSide, juce::RectanglePlacement::centred, false);
        headerCursor.removeFromLeft (12);
    }
#endif

    const int badgeWidthState = 108;
    const int badgeWidthMode  = 60;
    const int reservedRight   = cultureSelectorWidth + cultureLabelWidth + 20
                                + creditsButtonWidth + creditsButtonGap
                                + badgeWidthState + badgeWidthMode + 20;

    auto textArea = headerCursor.withWidth (juce::jmax (120, headerCursor.getWidth() - reservedRight));

    g.setColour (Theme::foreground);
    g.setFont (Theme::font (Theme::Type::title, true));
    g.drawText (Text::t ("PartePlay"), textArea.removeFromTop (24), juce::Justification::centredLeft);

    g.setColour (Theme::muted);
    g.setFont (Theme::font (Theme::Type::caption, true));
    g.drawText (Text::t ("ESPAÇO DE TRABALHO DO ÁUDIO DE REFERÊNCIA"),
                textArea.removeFromTop (16), juce::Justification::centredLeft);

    // Selos à direita do título, à esquerda do bloco de idioma. O sinal e de
    // subtracao: com "+" o selo comecava em header.getRight() + 322, ou seja,
    // 300px fora da janela - os selos "Tocando"/"VST3" nao apareciam.
    const float badgeY = static_cast<float> (header.getY()) + 18.0f;
    float badgeRight = static_cast<float> (header.getRight())
                         - static_cast<float> (cultureSelectorWidth + cultureLabelWidth + 20
                                               + creditsButtonWidth + creditsButtonGap);

    badgeRight -= static_cast<float> (badgeWidthState);
    const bool playing = processor.isTransportPlaying();
    Theme::paintBadge (g, juce::Rectangle<float> (badgeRight, badgeY, static_cast<float> (badgeWidthState), 22.0f),
                       playing ? Text::t ("Tocando") : Text::t ("Pausado"),
                       playing ? Theme::teal : Theme::muted);

    badgeRight -= static_cast<float> (badgeWidthMode) + 8.0f;
    Theme::paintBadge (g, juce::Rectangle<float> (badgeRight, badgeY, static_cast<float> (badgeWidthMode), 22.0f),
                       Text::t ("VST3"), Theme::azure);

    // Rodapé
    const auto footer = footerArea();
    // withHeight recebe int: o 1.0f era C4244 e a linha produzia um float de 1
    // truncado para a mesma coisa.
    Theme::paintRule (g, footer.withHeight (1).toFloat().translated (0.0f, -10.0f));

// Corpo, e nao caption: o rodape era o texto mais pequeno da tela e foi lido
    // errado ("SOFWARE" em vez de "SOFTWARE") por ser 11px. Legibilidade aqui
    // vale mais do que hierarquia sutil.
    const auto footerRow = footer.withTrimmedBottom (10);

    g.setColour (Theme::muted);
    g.setFont (Theme::font (Theme::Type::body));
    g.drawText (Text::t ("PartePlay · Feito para músicos e arranjadores"),
                footerRow, juce::Justification::centredLeft);
    g.drawText (footerCredit(), footerRow, juce::Justification::centredRight, true);

#if PARTEPLAY_DEBUG_OVERLAY
    if (debugOverlayVisible)
        paintDebugOverlay (g);
#endif
}

void PlayScoreEditor::timerCallback()
{
    refreshAllState();
    refreshLoadingOverlay();
    repaint (headerArea());
}

void PlayScoreEditor::refreshLoadingOverlay()
{
    if (! processor.isLoadingAudio())
    {
        if (loadingOverlay->isVisible())
            loadingOverlay->setVisible (false);

        return;
    }

    loadingOverlay->setState (processor.getLoadProgress(),
                              loadingStageText (processor.getLoadStage()));

    if (! loadingOverlay->isVisible())
    {
        loadingOverlay->setVisible (true);
        loadingOverlay->toFront (false);
    }
}

#if PARTEPLAY_DEBUG_OVERLAY
void PlayScoreEditor::paintDebugOverlay (juce::Graphics& g)
{
    // Os numeros que decidem se a interface esta legivel. A altura da linha do
    // popup aparece porque ela nao e um token: a JUCE deriva da altura do combo,
    // entao muda sozinha quando o combo muda e ninguem ve no Theme.
    const int popupRowHeight = cultureSelector.getHeight() - 2;

    // Escala global do Desktop, e nao do componente: Component nao expoe a escala
    // de DPI, e num plugin quem aplica o DPI e o host. Esta e a unica escala que o
    // JUCE 9 expoe, e e ela que explica um texto pequeno: se o host nao estiver
    // aplicando DPI, 1000 px logicos sao 1000 px fisicos e o 15 px continua 15 px.
    const float scale = juce::Desktop::getInstance().getGlobalScaleFactor();
    const auto* primary = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

    juce::StringArray lines;

    lines.add ("PARTEPLAY DEBUG   Ctrl+D fecha");
    lines.add ("janela    " + juce::String (getWidth()) + " x " + juce::String (getHeight())
                     + " logico   min " + juce::String (getConstrainer()->getMinimumWidth())
                     + " x " + juce::String (getConstrainer()->getMinimumHeight()));
    lines.add ("pixels    " + juce::String (juce::roundToInt (getWidth() * scale))
                     + " x " + juce::String (juce::roundToInt (getHeight() * scale))
                     + "   escala " + juce::String (scale, 2));
    lines.add ("tela      " + (primary != nullptr
                     ? juce::String (primary->logicalBounds.getWidth(), 0) + " x "
                           + juce::String (primary->logicalBounds.getHeight(), 0)
                           + "   escala " + juce::String (primary->scale, 2)
                           + "   " + juce::String (juce::roundToInt (primary->dpi)) + " dpi"
                     : juce::String ("?")));
    lines.add ("versao    " + juce::String (PARTEPLAY_VERSION));
    lines.add ("idioma    " + juce::String (cultureSelector.getWidth()) + " x "
                     + juce::String (cultureSelector.getHeight())
                     + "   popup " + juce::String (popupRowHeight)
                     + "   cultura " + Text::cultureCode().toStdString());
    lines.add ("fonte     controle " + juce::String (Theme::Type::control)
                     + "   corpo " + juce::String (Theme::Type::body)
                     + "   legenda " + juce::String (Theme::Type::caption)
                     + "   secao " + juce::String (Theme::Type::section));

    // Diagnostico da divergencia de tempo: o que o host declara, o que o audio
    // revela e o que o transporte entrega. "nao informado" e diferente de zero.
    const double hostBpm = processor.getHostBpm();
    const int hostNum = processor.getHostTimeSignatureNumerator();
    const int hostDen = processor.getHostTimeSignatureDenominator();
    const bool hostPlaying = processor.isTransportPlaying();

    lines.add ("host      bpm " + (hostBpm > 0.0 ? juce::String (hostBpm, 2)
                                                 : juce::String ("nao informado"))
                     + "   compasso " + (hostNum > 0
                            ? juce::String (hostNum) + "/" + juce::String (hostDen)
                            : juce::String ("nao informado")));
    lines.add ("transporte " + juce::String (hostPlaying ? "tocando" : "parado")
                     + "   amostra " + juce::String (processor.getTransportSample())
                     + "   ppq " + juce::String (processor.getHostPpqPosition(), 2));
    lines.add ("analise   bpm " + juce::String (processor.getAudioBpm(), 2)
                     + "   compasso " + juce::String (processor.getBeatsPerBar()) + "/4"
                     + "   compassos " + juce::String (processor.getMeasureCount()));

    const auto font = Theme::monoFont (12.0f);

    int widest = 0;
    for (const auto& line : lines)
        widest = juce::jmax (widest, juce::GlyphArrangement::getStringWidthInt (font, line));

    const auto box = juce::Rectangle<float> (static_cast<float> (margin) + 6.0f,
                                             static_cast<float> (margin) + 52.0f,
                                             static_cast<float> (widest) + 20.0f,
                                             static_cast<float> (lines.size()) * 16.0f + 12.0f)
                         .toNearestInt();

    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.fillRect (box.toFloat());
    g.setColour (Theme::azure);
    g.drawRect (box.toFloat(), 1.0f);

    auto textArea = box.toFloat().reduced (10.0f, 6.0f);
    g.setFont (font);
    g.setColour (Theme::foreground);

    for (const auto& line : lines)
    {
        g.drawText (line, textArea.removeFromTop (16.0f),
                    juce::Justification::topLeft, false);
    }
}
#endif

void PlayScoreEditor::visibilityChanged()
{
    // Ao abrir, puxa o foco de teclado para o editor: e o unico jeito de o
    // keyPressed do Ctrl+D ser chamado quando o host monta a janela do plugin.
    if (isShowing())
        grabKeyboardFocus();
}

bool PlayScoreEditor::keyPressed (const juce::KeyPress& key)
{
    // Esc fecha os creditos mesmo se o foco nao tiver chegado ao overlay.
    if (key.getKeyCode() == juce::KeyPress::escapeKey
        && creditsOverlay != nullptr && creditsOverlay->isVisible())
    {
        creditsOverlay->setVisible (false);
        return true;
    }

#if PARTEPLAY_DEBUG_OVERLAY
    if (key.getKeyCode() == 'd' && key.getModifiers().isCtrlDown())
    {
        debugOverlayVisible = ! debugOverlayVisible;
        repaint();
        return true;
    }
#endif

    return AudioProcessorEditor::keyPressed (key);
}

void PlayScoreEditor::refreshAllTexts()
{
    cultureLabel.setText (Text::t ("Idioma:"), juce::dontSendNotification);

    // O botao e o overlay de creditos tambem seguem o idioma ativo.
    if (creditsButton != nullptr)
        creditsButton->setTooltip (Text::t ("Créditos e tecnologias"));

    if (creditsOverlay != nullptr)
        creditsOverlay->refreshTexts();

    for (auto* panel : panels)
        panel->refreshTexts();
}

void PlayScoreEditor::refreshAllState()
{
    for (auto* panel : panels)
        panel->refreshState();
}

void PlayScoreEditor::languageChanged()
{
    refreshAllTexts();
    refreshAllState();

    // A bandeira do seletor acompanha o idioma ativo.
    static_cast<StudioLookAndFeel*> (lookAndFeel.get())->setFlagCulture (Text::getCulture());
    cultureSelector.repaint();

    if (loadingOverlay != nullptr)
        loadingOverlay->repaint();

    if (creditsOverlay != nullptr)
        creditsOverlay->repaint();

    repaint();
}

void PlayScoreEditor::requestLoadAudio()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        Text::t ("Selecionar o áudio de referência"),
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        Text::from ("*.wav;*.flac;*.ogg;*.mp3"));

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode,
        [this] (const juce::FileChooser& chooser)
        {
            const auto result = chooser.getResult();

            lastExportMessage.clear();

            if (result != juce::File())
            {
                processor.beginLoadAudioFile (result);

                // Mostra o overlay ja no clique: a primeira passada do timer viria
                // so 50 ms depois e o arquivo pareceria ignorado.
                loadingOverlay->setState (0.0f, loadingStageText (0));
                loadingOverlay->setVisible (true);
                loadingOverlay->toFront (false);
            }

            refreshAllState();
        });
}

void PlayScoreEditor::requestExportMap()
{
    const juce::File current (processor.getLoadedFileName());
    const juce::String baseName = current.getFileNameWithoutExtension();
    const juce::String qualified = baseName.isEmpty() ? Text::from ("mapa-de-tempo") : baseName;

    const auto suggested = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                               .getChildFile (juce::File::createLegalFileName (
                                   qualified + Text::from (" - mapa de tempo.mid")));

    exportChooser = std::make_unique<juce::FileChooser> (Text::t ("Exportar mapa de tempo (MIDI)"),
                                                         suggested, Text::from ("*.mid"));

    exportChooser->launchAsync (juce::FileBrowserComponent::saveMode,
        [this] (const juce::FileChooser& chooser)
        {
            const auto result = chooser.getResult();

            lastExportMessage = (result != juce::File() && processor.exportTempoMap (result))
                ? Text::format (Text::t ("Mapa de tempo exportado: {0}"), { result.getFileName() })
                : Text::t ("Falha ao exportar o mapa de tempo.");

            refreshAllState();
        });
}
