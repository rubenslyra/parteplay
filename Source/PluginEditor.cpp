#include "PluginEditor.h"
#include "PluginProcessor.h"

PlayScoreEditor::PlayScoreEditor (PlayScoreProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (480, 340);

    loadButton.setButtonText ("Carregar Áudio");
    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Selecionar o áudio de referência",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.wav;*.flac;*.ogg;*.mp3");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode,
            [this] (const juce::FileChooser& chooser)
            {
                const auto result = chooser.getResult();
                if (result != juce::File())
                    processor.loadAudioFile (result);
                updateFileLabel();
                updateAnalysisLabel();
            });
    };
    addAndMakeVisible (loadButton);

    fileLabel.setText ("Nenhum áudio carregado", juce::dontSendNotification);
    addAndMakeVisible (fileLabel);

    instrumentLabel.setText ("Instrumento:", juce::dontSendNotification);
    addAndMakeVisible (instrumentLabel);

    instrumentSelector.addItemList ({ "Piano / Trombone (C)",
                                       "Trompete / Sax Tenor (Bb)",
                                       "Sax Alto (Eb)",
                                       "Trompa (F)",
                                       "Ajuste Manual" }, 1);
    instrumentSelector.setSelectedItemIndex (0);
    addAndMakeVisible (instrumentSelector);

    instrumentAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.parameters, "instrument", instrumentSelector);

    pitchLabel.setText ("Afinação (Hz):", juce::dontSendNotification);
    addAndMakeVisible (pitchLabel);

    pitchSlider.setRange (432.0, 445.0, 0.1);
    pitchSlider.setValue (440.0);
    addAndMakeVisible (pitchSlider);

    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "referencePitch", pitchSlider);

    transposeLabel.setText ("Transposição (semitons):", juce::dontSendNotification);
    addAndMakeVisible (transposeLabel);

    transposeSlider.setRange (-12.0, 12.0, 1.0);
    transposeSlider.setValue (0.0);
    addAndMakeVisible (transposeSlider);

    transposeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "transpose", transposeSlider);

    analysisLabel.setText ("BPM: -- | Compassos (4/4): --", juce::dontSendNotification);
    addAndMakeVisible (analysisLabel);

    statusLabel.setText ("", juce::dontSendNotification);
    addAndMakeVisible (statusLabel);

    updateFileLabel();
    updateAnalysisLabel();
    updateStatusLabel();

    startTimerHz (20);
}

PlayScoreEditor::~PlayScoreEditor() = default;

void PlayScoreEditor::timerCallback()
{
    updateStatusLabel();
}

void PlayScoreEditor::updateFileLabel()
{
    const auto name = processor.getLoadedFileName();
    fileLabel.setText (name.isEmpty() ? "Nenhum áudio carregado" : name, juce::dontSendNotification);
}

void PlayScoreEditor::updateAnalysisLabel()
{
    const double bpm = processor.getAudioBpm();
    const int measures = processor.getMeasureCount();
    const double duration = processor.getDurationSeconds();

    juce::String text;

    if (bpm <= 0.0 || duration <= 0.0)
    {
        text = "BPM: -- | Compassos (4/4): -- | Duração: --:--";
    }
    else
    {
        const int minutes = (int) (duration / 60.0);
        const int seconds = (int) duration - minutes * 60;

        text = juce::String::formatted ("BPM: %s | Compassos (4/4): %d | Duração: %02d:%02d",
                                        juce::String (bpm, 1), measures, minutes, seconds);
    }

    analysisLabel.setText (text, juce::dontSendNotification);
}

void PlayScoreEditor::updateStatusLabel()
{
    juce::String text;

    if (! processor.isTransportPlaying())
    {
        text = "Pausado";
    }
    else if (processor.hasReachedEndOfFile())
    {
        text = "Fim do arquivo";
    }
    else
    {
        const auto sample = processor.getTransportSample();
        const auto rate = processor.getTransportSampleRate();
        const double seconds = rate > 0.0 ? static_cast<double> (sample) / rate : 0.0;

        text = juce::String::formatted ("Tocando | %02d:%05.2f | Transposição %+d st",
                                        static_cast<int> (seconds) / 60,
                                        seconds - static_cast<int> (seconds / 60.0) * 60.0,
                                        processor.getActiveSemitones());
    }

    statusLabel.setText (text, juce::dontSendNotification);
}

void PlayScoreEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff222226));

#if JUCE_TARGET_HAS_BINARY_DATA
    const juce::Image logo = juce::ImageCache::getFromMemory (BinaryData::PartePlayIcon_png,
                                                              BinaryData::PartePlayIcon_pngSize);

    if (logo.isValid())
    {
        const float side = 30.0f;
        g.drawImageWithin (logo, 14, 12, (int) side, (int) side,
                           juce::RectanglePlacement::centred, false);
    }
#endif

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (18.0f).boldened());
    g.drawText ("PartePlay", 52, 10, getWidth() - 60, 24, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff9a9aa4));
    g.setFont (juce::Font (12.0f));
    g.drawText ("Reprodução de referência sincronizada ao transport",
                52, 34, getWidth() - 64, 18, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff8a8a94));
    g.setFont (juce::Font (11.0f));
    g.drawText ("Rubinho Lyra Software Eng",
                0, getHeight() - 22, getWidth(), 16, juce::Justification::centred);
}

void PlayScoreEditor::resized()
{
    const int margin = 24;

    loadButton.setBounds (margin, 72, 180, 26);
    fileLabel.setBounds (margin + 190, 76, getWidth() - margin * 2 - 190, 20);

    instrumentLabel.setBounds (margin, 108, 120, 20);
    instrumentSelector.setBounds (margin + 130, 108, getWidth() - margin * 2 - 130, 22);

    pitchLabel.setBounds (margin, 140, 120, 20);
    pitchSlider.setBounds (margin + 130, 140, getWidth() - margin * 2 - 130, 22);

    transposeLabel.setBounds (margin, 172, 170, 20);
    transposeSlider.setBounds (margin + 180, 172, getWidth() - margin * 2 - 180, 22);

    analysisLabel.setBounds (margin, 208, getWidth() - margin * 2, 22);
    statusLabel.setBounds (margin, 240, getWidth() - margin * 2, 22);
}