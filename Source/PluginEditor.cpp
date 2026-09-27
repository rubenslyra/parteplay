#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Instrument.h"
#include "ParameterIds.h"
#include "Text.h"
#include "Theme.h"
#include "Waveform.h"

#include <cmath>

namespace
{
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

        void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                           juce::ComboBox& box) override
        {
            auto area = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width),
                                                static_cast<float> (height)).reduced (0.5f);

            g.setColour (Theme::panel);
            g.fillRoundedRectangle (area, Theme::radiusSmall);
            g.setColour (box.isEnabled() ? Theme::line : Theme::line.withAlpha (0.5f));
            g.drawRoundedRectangle (area, Theme::radiusSmall, 1.0f);

            const float boxSide = static_cast<float> (height) * 0.28f;
            juce::Path arrow;
            arrow.addTriangle (area.getRight() - height * 0.62f, area.getCentreY() - boxSide * 0.5f,
                               area.getRight() - height * 0.38f, area.getCentreY() - boxSide * 0.5f,
                               area.getRight() - height * 0.50f, area.getCentreY() + boxSide * 0.7f);

            g.setColour (box.isEnabled() ? Theme::azure : Theme::muted);
            g.fillPath (arrow);
        }
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

        void paint (juce::Graphics& g) override
        {
            const auto area = getLocalBounds().toFloat();
            Theme::paintRing (g, area);

            const float side = juce::jmin (area.getWidth(), area.getHeight());
            g.setColour (Theme::azure);
            g.setFont (Theme::monoFont (side * 0.36f, true));
            g.drawText (symbol, area, juce::Justification::centred);
        }

    private:
        juce::String symbol;
    };

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
            const auto megabytes = juce::String (static_cast<double> (processor.getFileSizeBytes())
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

        auto header = area.removeFromTop (42);
        loadButton.setBounds (header.removeFromRight (132).withHeight (30).translated (0, 6));
        header.removeFromRight (12);
        fileNameLabel.setBounds (header.removeFromTop (20));
        metaLabel.setBounds (header.removeFromTop (16));

        auto times = area.removeFromBottom (20);
        positionLabel.setBounds (times.removeFromLeft (96));
        durationLabel.setBounds (times.removeFromRight (96));
        hintLabel.setBounds (times);

        area.removeFromBottom (10);
        waveform.setBounds (area);
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

        loopToggle.setToggleState (p.isLoopEnabled(), juce::dontSendNotification);
        addAndMakeVisible (loopToggle);
        loopToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            p.parameters, Parameter::loopEnabled, loopToggle);

        muteToggle.setToggleState (p.isOutputMuted(), juce::dontSendNotification);
        addAndMakeVisible (muteToggle);
        muteToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            p.parameters, Parameter::muted, muteToggle);

        styleCaption (loopStartLabel);
        addAndMakeVisible (loopStartLabel);
        styleSlider (loopStartSlider);
        loopStartSlider.setRange (1.0, 8.0, 1.0);
        addAndMakeVisible (loopStartSlider);
        styleMono (loopStartValue);
        loopStartValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (loopStartValue);
        loopStartAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.parameters, Parameter::loopStart, loopStartSlider);

        styleCaption (loopEndLabel);
        addAndMakeVisible (loopEndLabel);
        styleSlider (loopEndSlider);
        loopEndSlider.setRange (1.0, 8.0, 1.0);
        addAndMakeVisible (loopEndSlider);
        styleMono (loopEndValue);
        loopEndValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (loopEndValue);
        loopEndAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.parameters, Parameter::loopEnd, loopEndSlider);

        styleCaption (speedLabel);
        addAndMakeVisible (speedLabel);
        styleSlider (speedSlider);
        speedSlider.setRange (50.0, 150.0, 1.0);
        addAndMakeVisible (speedSlider);
        styleMono (speedValue);
        speedValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (speedValue);
        speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.parameters, Parameter::trainingSpeed, speedSlider);

        styleCaption (noteLabel);
        noteLabel.setColour (juce::Label::textColourId, Theme::muted.withAlpha (0.85f));
        addAndMakeVisible (noteLabel);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("02 / Transporte");
        loopToggle.setButtonText (Text::t ("Repetir trecho"));
        muteToggle.setButtonText (Text::t ("Silenciar saída"));
        loopStartLabel.setText (Text::t ("Início (compasso)"), juce::dontSendNotification);
        loopEndLabel.setText (Text::t ("Fim (compasso)"), juce::dontSendNotification);
        speedLabel.setText (Text::t ("Velocidade (treino)"), juce::dontSendNotification);
        noteLabel.setText (Text::t ("Play, pause e busca são controlados pelo hospedeiro."),
                           juce::dontSendNotification);
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

        const auto measures = juce::jmax (2, processor.getMeasureCount());

        if (static_cast<int> (loopStartSlider.getMaximum()) != measures)
            loopStartSlider.setRange (1.0, static_cast<double> (measures), 1.0);
        if (static_cast<int> (loopEndSlider.getMaximum()) != measures)
            loopEndSlider.setRange (1.0, static_cast<double> (measures), 1.0);

        loopStartValue.setText (juce::String (processor.getLoopStartMeasure()), juce::dontSendNotification);
        loopEndValue.setText (juce::String (processor.getLoopEndMeasure()), juce::dontSendNotification);
        speedValue.setText (juce::String (processor.getTrainingSpeed() * 100.0, 0) + Text::t (" %"),
                            juce::dontSendNotification);

        const bool loopControls = processor.isLoopEnabled();
        loopStartSlider.setEnabled (loopControls);
        loopEndSlider.setEnabled (loopControls);
        loopStartLabel.setEnabled (loopControls);
        loopEndLabel.setEnabled (loopControls);
    }

    void paint (juce::Graphics& g) override
    {
        auto content = paintChrome (g, title);
        Theme::paintRule (g, content.removeFromBottom (34).toFloat().removeFromTop (1.0f));
    }

    void resized() override
    {
        auto area = contentArea();

        auto tiles = area.removeFromTop (56);
        juce::FlexBox tileRow;
        tileRow.flexDirection = juce::FlexBox::Direction::row;
        tileRow.items.add (juce::FlexItem (stateTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 0, 0)));
        tileRow.items.add (juce::FlexItem (positionTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 0, 0, 4)));
        tileRow.performLayout (tiles);

        area.removeFromTop (12);

        auto toggles = area.removeFromTop (28);
        juce::FlexBox toggleRow;
        toggleRow.flexDirection = juce::FlexBox::Direction::row;
        toggleRow.items.add (juce::FlexItem (loopToggle).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 0, 0)));
        toggleRow.items.add (juce::FlexItem (muteToggle).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 0, 0, 4)));
        toggleRow.performLayout (toggles);

        area.removeFromTop (10);
        area.removeFromBottom (34);

        layoutRow (speedLabel, speedSlider, speedValue, area.removeFromTop (26));
        area.removeFromTop (6);
        layoutRow (loopStartLabel, loopStartSlider, loopStartValue, area.removeFromTop (26));
        area.removeFromTop (6);
        layoutRow (loopEndLabel, loopEndSlider, loopEndValue, area.removeFromTop (26));

        noteLabel.setBounds (area.removeFromBottom (24));
    }

private:
    void layoutRow (juce::Component& caption, juce::Component& slider, juce::Component& value,
                    juce::Rectangle<int> row)
    {
        const int captionWidth = 132;
        const int valueWidth   = 52;

        caption.setBounds (row.removeFromLeft (captionWidth));
        value.setBounds (row.removeFromRight (valueWidth));
        row.removeFromRight (10);
        slider.setBounds (row);
    }

    juce::String title;
    StatTile stateTile;
    StatTile positionTile;
    juce::ToggleButton loopToggle;
    juce::ToggleButton muteToggle;
    juce::Label loopStartLabel, loopEndLabel, speedLabel;
    juce::Slider loopStartSlider, loopEndSlider, speedSlider;
    juce::Label loopStartValue, loopEndValue, speedValue;
    juce::Label noteLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopToggleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteToggleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> loopStartAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> loopEndAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;
};

//==============================================================================
class InstrumentPanel : public EditorPanel
{
public:
    InstrumentPanel (PlayScoreEditor&, PlayScoreProcessor& p)
        : EditorPanel (p)
    {
        ring.setSymbol (Text::from ("C"));
        addAndMakeVisible (ring);

        nameLabel.setFont (Theme::font (17.0f, true));
        nameLabel.setColour (juce::Label::textColourId, Theme::foreground);
        addAndMakeVisible (nameLabel);

        styleCaption (keyLabel);
        addAndMakeVisible (keyLabel);

        for (int i = 0; i < InstrumentTable::numEntries; ++i)
        {
            auto button = std::make_unique<juce::TextButton>();
            button->setClickingTogglesState (true);
            button->setRadioGroupId (radioGroup);
            button->setTriggeredOnMouseDown (true);

            const int index = i;
            button->onClick = [this, index] { selectIndex (index); };

            buttons.push_back (button.get());
            buttonOwners.push_back (std::move (button));
            addAndMakeVisible (buttons.back());
        }

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("03 / Instrumento");

        for (int i = 0; i < static_cast<int> (buttons.size()); ++i)
            buttons[static_cast<size_t> (i)]->setButtonText (
                InstrumentTable::displayName (InstrumentTable::fromIndex (i)));

        repaint();
    }

    void refreshState() override
    {
        const auto instrument = processor.getCurrentInstrument();
        const int index = processor.getCurrentInstrumentIndex();

        for (int i = 0; i < static_cast<int> (buttons.size()); ++i)
            buttons[static_cast<size_t> (i)]->setToggleState (i == index, juce::dontSendNotification);

        ring.setSymbol (InstrumentTable::symbolFor (instrument));
        nameLabel.setText (InstrumentTable::displayName (instrument), juce::dontSendNotification);
        keyLabel.setText (Text::format (Text::t ("Afinação em {0} · {1} semitons"),
                                        { InstrumentTable::keyNameFor (instrument),
                                          InstrumentTable::signedSemitonesFor (instrument) }),
                          juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        paintChrome (g, title);
    }

    void resized() override
    {
        auto area = contentArea();

        auto header = area.removeFromTop (64);
        ring.setBounds (header.removeFromLeft (64).reduced (0, 0));
        header.removeFromLeft (12);
        nameLabel.setBounds (header.removeFromTop (24));
        keyLabel.setBounds (header.removeFromTop (18));

        area.removeFromTop (14);

        juce::Grid grid;
        grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                                 juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
        grid.templateRows    = { juce::Grid::TrackInfo (juce::Grid::Px (32)),
                                 juce::Grid::TrackInfo (juce::Grid::Px (32)),
                                 juce::Grid::TrackInfo (juce::Grid::Px (32)),
                                 juce::Grid::TrackInfo (juce::Grid::Px (32)) };
        grid.setGap (juce::Grid::Px (8));

        for (auto* button : buttons)
            grid.items.add (juce::GridItem (*button));

        grid.performLayout (area);
    }

private:
    void selectIndex (int index)
    {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (
            processor.parameters.getParameter (Parameter::instrument));

        if (param == nullptr)
            return;

        const auto count = param->choices.size();

        if (count > 1)
            param->setValueNotifyingHost (static_cast<float> (index) / static_cast<float> (count - 1));
    }

    static constexpr int radioGroup = 7101;

    juce::String title;
    RingView ring;
    juce::Label nameLabel;
    juce::Label keyLabel;
    std::vector<juce::TextButton*> buttons;
    std::vector<std::unique_ptr<juce::TextButton>> buttonOwners;
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

        styleSlider (pitchSlider);
        pitchSlider.setRange (Tuning::minReferenceHz, Tuning::maxReferenceHz, 0.1);
        addAndMakeVisible (pitchSlider);
        pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.parameters, Parameter::referencePitch, pitchSlider);

        styleMono (pitchMinLabel);
        pitchMinLabel.setText (juce::String ((int) Tuning::minReferenceHz), juce::dontSendNotification);
        addAndMakeVisible (pitchMinLabel);
        styleMono (pitchMaxLabel);
        pitchMaxLabel.setText (juce::String ((int) Tuning::maxReferenceHz), juce::dontSendNotification);
        pitchMaxLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (pitchMaxLabel);

        resetButton.onClick = [this]
        {
            pitchSlider.setValue (Tuning::defaultReferenceHz);
        };
        addAndMakeVisible (resetButton);

        addAndMakeVisible (transposeTile);
        addAndMakeVisible (fileTuningTile);

        styleCaption (manualLabel);
        addAndMakeVisible (manualLabel);
        styleSlider (manualSlider);
        manualSlider.setRange (-12.0, 12.0, 1.0);
        addAndMakeVisible (manualSlider);
        styleMono (manualValue);
        manualValue.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (manualValue);
        manualAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.parameters, Parameter::transpose, manualSlider);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("04 / Afinação & Transposição");
        referenceCaption.setText (Text::t ("Referência · A4"), juce::dontSendNotification);
        resetButton.setButtonText (Text::t ("Restaurar 440 Hz"));
        manualLabel.setText (Text::t ("Transposição manual (semitons)"), juce::dontSendNotification);
        repaint();
    }

    void refreshState() override
    {
        const double reference = processor.getReferencePitchHz();

        referenceValue.setText (juce::String (reference, 0) + Text::from (" Hz"), juce::dontSendNotification);

        const int semitones = processor.getActiveSemitones();
        transposeTile.setContent (Text::t ("Transposição"),
                                  Text::format (Text::t ("{0} st"), { signedValue (semitones) }),
                                  semitones != 0 ? Theme::signal : Theme::foreground);

        const double detectedHz = processor.getDetectedTuningHz();
        const auto detectedCents = static_cast<int> (std::lround (processor.getDetectedTuningCents()));

        fileTuningTile.setContent (Text::t ("Arquivo"),
                                   Text::format (Text::t ("A = {0} Hz ({1} cents)"),
                                                 { juce::String (detectedHz, 1), signedValue (detectedCents) }),
                                   Theme::foreground);

        const bool manual = processor.getCurrentInstrument() == Instrument::Manual;
        manualSlider.setEnabled (manual);
        manualLabel.setEnabled (manual);
        manualValue.setEnabled (manual);
        manualValue.setText (signedValue (processor.getManualSemitones()), juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        paintChrome (g, title);
    }

    void resized() override
    {
        auto area = contentArea();

        auto header = area.removeFromTop (52);
        ring.setBounds (header.removeFromLeft (52));
        header.removeFromLeft (12);
        referenceCaption.setBounds (header.removeFromTop (16));
        referenceValue.setBounds (header.removeFromTop (26));

        area.removeFromTop (12);

        auto sliderRow = area.removeFromTop (22);
        pitchMinLabel.setBounds (sliderRow.removeFromLeft (30));
        pitchMaxLabel.setBounds (sliderRow.removeFromRight (30));
        sliderRow.removeFromLeft (8);
        sliderRow.removeFromRight (8);
        pitchSlider.setBounds (sliderRow);

        area.removeFromTop (8);
        resetButton.setBounds (area.removeFromTop (28).removeFromLeft (150));

        area.removeFromTop (12);

        auto tiles = area.removeFromTop (56);
        juce::FlexBox tileRow;
        tileRow.flexDirection = juce::FlexBox::Direction::row;
        tileRow.items.add (juce::FlexItem (transposeTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 0, 0)));
        tileRow.items.add (juce::FlexItem (fileTuningTile).withFlex (1.35f).withMargin (juce::FlexItem::Margin (0, 0, 0, 4)));
        tileRow.performLayout (tiles);

        area.removeFromTop (12);

        auto manualRow = area.removeFromTop (26);
        manualLabel.setBounds (manualRow.removeFromLeft (170));
        manualValue.setBounds (manualRow.removeFromRight (44));
        manualRow.removeFromRight (10);
        manualSlider.setBounds (manualRow);
    }

private:
    juce::String title;
    RingView ring;
    juce::Label referenceCaption;
    juce::Label referenceValue;
    juce::Slider pitchSlider;
    juce::Label pitchMinLabel, pitchMaxLabel;
    juce::TextButton resetButton;
    StatTile transposeTile;
    StatTile fileTuningTile;
    juce::Label manualLabel;
    juce::Slider manualSlider;
    juce::Label manualValue;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> manualAttachment;
};

//==============================================================================
class MeterPanel : public EditorPanel
{
public:
    MeterPanel (PlayScoreEditor& owner, PlayScoreProcessor& p)
        : EditorPanel (p), editor (owner)
    {
        addAndMakeVisible (bpmTile);
        addAndMakeVisible (meterTile);
        addAndMakeVisible (barsTile);
        addAndMakeVisible (durationTile);

        exportButton.onClick = [this] { editor.requestExportMap(); };
        addAndMakeVisible (exportButton);

        styleCaption (noteLabel);
        noteLabel.setColour (juce::Label::textColourId, Theme::muted.withAlpha (0.85f));
        addAndMakeVisible (noteLabel);

        styleCaption (messageLabel);
        messageLabel.setColour (juce::Label::textColourId, Theme::teal);
        addAndMakeVisible (messageLabel);

        refreshTexts();
        refreshState();
    }

    void refreshTexts() override
    {
        title = Text::t ("05 / BPM & Métrica");
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
                            hasAnalysis ? juce::String (bpm, 1) : empty,
                            hasAnalysis ? Theme::ice : Theme::muted);

        meterTile.setContent (Text::t ("Assinatura"),
                              hasAnalysis ? Text::format (Text::t ("{0}/4"),
                                                          { juce::String (processor.getBeatsPerBar()) })
                                          : empty,
                              hasAnalysis ? Theme::foreground : Theme::muted);

        barsTile.setContent (Text::t ("Compassos"),
                             hasAnalysis ? juce::String (processor.getMeasureCount()) : empty,
                             hasAnalysis ? Theme::foreground : Theme::muted);

        durationTile.setContent (Text::t ("Duração"),
                                 hasAnalysis ? Text::time (processor.getDurationSeconds()) : empty,
                                 hasAnalysis ? Theme::foreground : Theme::muted);

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

        auto firstRow = area.removeFromTop (56);
        auto secondRow = area.removeFromTop (56);
        area.removeFromTop (12);

        juce::FlexBox row1;
        row1.flexDirection = juce::FlexBox::Direction::row;
        row1.items.add (juce::FlexItem (bpmTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 6, 0)));
        row1.items.add (juce::FlexItem (meterTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 0, 6, 4)));
        row1.performLayout (firstRow);

        juce::FlexBox row2;
        row2.flexDirection = juce::FlexBox::Direction::row;
        row2.items.add (juce::FlexItem (barsTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 4, 0, 0)));
        row2.items.add (juce::FlexItem (durationTile).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0, 0, 0, 4)));
        row2.performLayout (secondRow);

        area.removeFromBottom (24);
        messageLabel.setBounds (area.removeFromBottom (20));
        noteLabel.setBounds (area.removeFromBottom (30));
        area.removeFromBottom (10);
        exportButton.setBounds (area.removeFromTop (32));
    }

private:
    PlayScoreEditor& editor;
    juce::String title;
    StatTile bpmTile, meterTile, barsTile, durationTile;
    juce::TextButton exportButton;
    juce::Label noteLabel;
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

    trackPanel      = std::make_unique<TrackPanel> (*this, p);
    transportPanel  = std::make_unique<TransportPanel> (*this, p);
    instrumentPanel = std::make_unique<InstrumentPanel> (*this, p);
    tuningPanel     = std::make_unique<TuningPanel> (*this, p);
    meterPanel      = std::make_unique<MeterPanel> (*this, p);

    panels = { trackPanel.get(), transportPanel.get(), instrumentPanel.get(),
               tuningPanel.get(), meterPanel.get() };

    for (auto* panel : panels)
        addAndMakeVisible (panel);

    styleCaption (cultureLabel);
    addAndMakeVisible (cultureLabel);

    cultureSelector.addItem (Text::from ("Português"), 1);
    cultureSelector.addItem (Text::from ("English"), 2);
    cultureSelector.addItem (Text::from ("Español"), 3);
    cultureSelector.setSelectedId (1 + static_cast<int> (Text::getCulture()), juce::dontSendNotification);
    cultureSelector.onChange = [this]
    {
        Text::setCulture (static_cast<Text::Culture> (cultureSelector.getSelectedId() - 1));
        languageChanged();
    };
    addAndMakeVisible (cultureSelector);

    // Proporção 5:4 (referência visual 2000x1600), redimensionável pelo hospedeiro.
    setSize (1000, 800);
    setResizable (true, false);
    setResizeLimits (800, 640, 1600, 1280);

    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (5.0 / 4.0);

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
    auto cultureBounds = getLocalBounds().removeFromTop (margin + headerHeight)
                                                .withTrimmedTop (margin)
                                                .removeFromRight (margin);
    cultureSelector.setBounds (cultureBounds.removeFromRight (132).withHeight (24).withTrimmedTop (18));
    cultureLabel.setBounds (cultureBounds.removeFromRight (72).withHeight (20).withTrimmedTop (20));

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
        const int instrumentHeight = static_cast<int> (static_cast<float> (total) * 0.38f);
        const int tuningHeight     = static_cast<int> (static_cast<float> (total) * 0.36f);

        instrumentPanel->setBounds (rightColumn.removeFromTop (instrumentHeight));
        rightColumn.removeFromTop (gap);
        tuningPanel->setBounds (rightColumn.removeFromTop (tuningHeight));
        rightColumn.removeFromTop (gap);
        meterPanel->setBounds (rightColumn);
    }
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

    juce::Rectangle<int> cursor = header;

#if JUCE_TARGET_HAS_BINARY_DATA
    const juce::Image logo = juce::ImageCache::getFromMemory (BinaryData::PartePlayIcon_png,
                                                              BinaryData::PartePlayIcon_pngSize);

    if (logo.isValid())
    {
        const auto logoArea = cursor.removeFromLeft (logoSide).toFloat();
        g.drawImageWithin (logo, static_cast<int> (logoArea.getX()), static_cast<int> (logoArea.getY()),
                           logoSide, logoSide, juce::RectanglePlacement::centred, false);
        cursor.removeFromLeft (12);
    }
#endif

    const int badgeWidthState = 108;
    const int badgeWidthMode  = 172;
    const int reservedRight   = 132 + 72 + 20 + badgeWidthState + badgeWidthMode + 20;

    auto textArea = cursor.withWidth (juce::jmax (120, cursor.getWidth() - reservedRight));

    g.setColour (Theme::foreground);
    g.setFont (Theme::font (Theme::Type::title, true));
    g.drawText (Text::t ("PartePlay"), textArea.removeFromTop (24), juce::Justification::centredLeft);

    g.setColour (Theme::muted);
    g.setFont (Theme::font (Theme::Type::caption, true));
    g.drawText (Text::t ("Espaço de trabalho do áudio de referência").toUpperCase(),
                textArea.removeFromTop (16), juce::Justification::centredLeft);

    // Selos à direita do título.
    const float badgeY = static_cast<float> (header.getY()) + 18.0f;
    float badgeRight = static_cast<float> (header.getRight()) + 132.0f + 72.0f + 20.0f;

    badgeRight -= static_cast<float> (badgeWidthState);
    const bool playing = processor.isTransportPlaying();
    Theme::paintBadge (g, juce::Rectangle<float> (badgeRight, badgeY, static_cast<float> (badgeWidthState), 22.0f),
                       playing ? Text::t ("Tocando") : Text::t ("Pausado"),
                       playing ? Theme::teal : Theme::muted);

    badgeRight -= static_cast<float> (badgeWidthMode) + 8.0f;
    Theme::paintBadge (g, juce::Rectangle<float> (badgeRight, badgeY, static_cast<float> (badgeWidthMode), 22.0f),
                       Text::t ("VST3 · Transporte escravo"), Theme::azure);

    // Rodapé
    const auto footer = footerArea();
    Theme::paintRule (g, footer.withHeight (1.0f).toFloat().translated (0.0f, -10.0f));

    g.setColour (Theme::muted);
    g.setFont (Theme::font (Theme::Type::caption));
    g.drawText (Text::t ("PartePlay · Feito para músicos e arranjadores"),
                footer.withTrimmedBottom (10), juce::Justification::centredLeft);
    g.drawText (Text::t ("Produção Rubinho Lyra / Software Eng").toUpperCase(),
                footer.withTrimmedBottom (10), juce::Justification::centredRight);
}

void PlayScoreEditor::timerCallback()
{
    refreshAllState();
    repaint (headerArea());
}

void PlayScoreEditor::refreshAllTexts()
{
    cultureLabel.setText (Text::t ("Idioma:"), juce::dontSendNotification);

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

            if (result != juce::File())
                processor.loadAudioFile (result);

            lastExportMessage.clear();
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
