#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Instrument.h"
#include "ParameterIds.h"
#include "Text.h"

PlayScoreEditor::PlayScoreEditor (PlayScoreProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (480, 452);

    loadButton.setButtonText (Text::from ("Carregar Áudio"));
    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            Text::from ("Selecionar o áudio de referência"),
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

    fileLabel.setText (Text::from ("Nenhum áudio carregado"), juce::dontSendNotification);
    addAndMakeVisible (fileLabel);

    instrumentLabel.setText (Text::from ("Instrumento:"), juce::dontSendNotification);
    addAndMakeVisible (instrumentLabel);

    instrumentSelector.addItemList (InstrumentTable::getDisplayNames(), 1);
    instrumentSelector.setSelectedItemIndex (0);
    addAndMakeVisible (instrumentSelector);

    instrumentAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.parameters, Parameter::instrument, instrumentSelector);

    pitchLabel.setText (Text::from ("Afinação (Hz):"), juce::dontSendNotification);
    addAndMakeVisible (pitchLabel);

    pitchSlider.setRange (Tuning::minReferenceHz, Tuning::maxReferenceHz, 0.1);
    pitchSlider.setValue (Tuning::defaultReferenceHz);
    addAndMakeVisible (pitchSlider);

    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, Parameter::referencePitch, pitchSlider);

    transposeLabel.setText (Text::from ("Transposição (semitons):"), juce::dontSendNotification);
    addAndMakeVisible (transposeLabel);

    transposeSlider.setRange (-12.0, 12.0, 1.0);
    transposeSlider.setValue (0.0);
    addAndMakeVisible (transposeSlider);

    transposeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, Parameter::transpose, transposeSlider);

    speedLabel.setText (Text::from ("Velocidade (treino %):"), juce::dontSendNotification);
    addAndMakeVisible (speedLabel);

    speedSlider.setRange (50.0, 150.0, 1.0);
    speedSlider.setValue (100.0);
    speedSlider.setTextValueSuffix (Text::from (" %"));
    addAndMakeVisible (speedSlider);

    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, Parameter::trainingSpeed, speedSlider);

    loopStartLabel.setText (Text::from ("Loop início (compasso):"), juce::dontSendNotification);
    addAndMakeVisible (loopStartLabel);

    loopStartSlider.setRange (1.0, 8.0, 1.0);
    loopStartSlider.setValue (1.0);
    loopStartSlider.setSkewFactor (1.0);
    addAndMakeVisible (loopStartSlider);

    loopStartAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, Parameter::loopStart, loopStartSlider);

    loopEndLabel.setText (Text::from ("Loop fim (compasso):"), juce::dontSendNotification);
    addAndMakeVisible (loopEndLabel);

    loopEndSlider.setRange (1.0, 8.0, 1.0);
    loopEndSlider.setValue (1.0);
    loopEndSlider.setSkewFactor (1.0);
    addAndMakeVisible (loopEndSlider);

    loopEndAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, Parameter::loopEnd, loopEndSlider);

    loopToggle.setButtonText (Text::from ("Repetir trecho"));
    addAndMakeVisible (loopToggle);

    loopToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.parameters, Parameter::loopEnabled, loopToggle);

    exportButton.setButtonText (Text::from ("Exportar Mapa (.mid)"));
    exportButton.setEnabled (false);
    exportButton.onClick = [this] { exportTempoMap(); };
    addAndMakeVisible (exportButton);

    analysisLabel.setText (Text::from ("BPM: -- | Assinatura: -- | Afinação: --Hz | Compassos: -- | Duração: --:--"), juce::dontSendNotification);
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
    exportButton.setEnabled (processor.getAudioBpm() > 0.0);
    updateStatusLabel();
}

void PlayScoreEditor::updateFileLabel()
{
    const auto name = processor.getLoadedFileName();
    fileLabel.setText (name.isEmpty() ? Text::from ("Nenhum áudio carregado") : name, juce::dontSendNotification);

    if (! name.isEmpty())
        exportMessage.clear();
}

void PlayScoreEditor::updateAnalysisLabel()
{
    const double bpm = processor.getAudioBpm();
    const int meter = processor.getBeatsPerBar();
    const double tuning = processor.getDetectedTuningHz();
    const int measures = processor.getMeasureCount();
    const double duration = processor.getDurationSeconds();

    juce::String text;

    if (bpm <= 0.0 || duration <= 0.0)
    {
        text = Text::from ("BPM: -- | Assinatura: -- | Afinação: --Hz | Compassos: -- | Duração: --:--");
    }
    else
    {
        const int minutes = (int) (duration / 60.0);
        const int seconds = (int) duration - minutes * 60;

        text = juce::String::formatted (Text::from ("BPM: %s | Assinatura: %d/4 | Afinação: %.1fHz | Compassos: %d | Duração: %02d:%02d"),
                                        juce::String (bpm, 1), meter, tuning, measures, minutes, seconds);
    }

    analysisLabel.setText (text, juce::dontSendNotification);

    const int loopMax = juce::jmax (2, measures);
    if ((int) loopStartSlider.getMaximum() != loopMax)
        loopStartSlider.setRange (1.0, (double) loopMax, 1.0);
    if ((int) loopEndSlider.getMaximum() != loopMax)
        loopEndSlider.setRange (1.0, (double) loopMax, 1.0);
}

void PlayScoreEditor::updateStatusLabel()
{
    if (exportMessage.isNotEmpty())
    {
        statusLabel.setText (exportMessage, juce::dontSendNotification);
        return;
    }

    juce::String text;

    const double speed = processor.getTrainingSpeed();
    const bool loopActive = processor.isLoopEnabled();

    if (speed != 1.0 || loopActive)
    {
        juce::String state = Text::from ("Treino |");

        if (speed != 1.0)
            state += Text::from (" ") + juce::String (speed * 100.0, 0) + Text::from ("%");

        if (loopActive)
            state += Text::from (" | Loop ") + juce::String (processor.getLoopStartMeasure())
                   + Text::from ("-") + juce::String (processor.getLoopEndMeasure());

        if (! processor.isTransportPlaying())
        {
            text = state + Text::from (" | Pausado");
        }
        else
        {
            const auto sample = processor.getTransportSample();
            const auto rate = processor.getTransportSampleRate();
            const double seconds = rate > 0.0 ? static_cast<double> (sample) / rate : 0.0;

            text = juce::String::formatted (Text::from ("%s | %02d:%05.2f"),
                                            state, static_cast<int> (seconds) / 60,
                                            seconds - static_cast<int> (seconds / 60.0) * 60.0);
        }
    }
    else if (! processor.isTransportPlaying())
    {
        text = Text::from ("Pausado");
    }
    else if (processor.hasReachedEndOfFile())
    {
        text = Text::from ("Fim do arquivo");
    }
    else
    {
        const auto sample = processor.getTransportSample();
        const auto rate = processor.getTransportSampleRate();
        const double seconds = rate > 0.0 ? static_cast<double> (sample) / rate : 0.0;

        text = juce::String::formatted (Text::from ("Tocando | %02d:%05.2f | Transposição %+d st"),
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
    g.drawText (Text::from ("PartePlay"), 52, 10, getWidth() - 60, 24, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff9a9aa4));
    g.setFont (juce::Font (12.0f));
    g.drawText (Text::from ("Reprodução de referência sincronizada ao transport"),
                52, 34, getWidth() - 64, 18, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff8a8a94));
    g.setFont (juce::Font (11.0f));
    g.drawText (Text::from ("Rubinho Lyra Software Eng"),
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

    speedLabel.setBounds (margin, 204, 150, 20);
    speedSlider.setBounds (margin + 160, 204, getWidth() - margin * 2 - 160, 22);

    loopStartLabel.setBounds (margin, 236, 150, 20);
    loopStartSlider.setBounds (margin + 160, 236, getWidth() - margin * 2 - 160, 22);

    loopEndLabel.setBounds (margin, 268, 150, 20);
    loopEndSlider.setBounds (margin + 160, 268, getWidth() - margin * 2 - 160, 22);

    analysisLabel.setBounds (margin, 300, getWidth() - margin * 2, 22);
    statusLabel.setBounds (margin, 332, getWidth() - margin * 2, 22);
    exportButton.setBounds (margin, 364, 220, 26);
}

void PlayScoreEditor::exportTempoMap()
{
    const juce::String baseName = juce::File (processor.getLoadedFileName()).getFileNameWithoutExtension();
    const juce::String qualified = baseName.isEmpty() ? "mapa-de-tempo" : baseName;

    const auto suggested = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                               .getChildFile (juce::File::createLegalFileName (qualified + " - mapa de tempo.mid"));

    exportChooser = std::make_unique<juce::FileChooser> (Text::from ("Exportar mapa de tempo (MIDI)"), suggested, "*.mid");

    exportChooser->launchAsync (juce::FileBrowserComponent::saveMode,
        [this] (const juce::FileChooser& chooser)
        {
            const auto result = chooser.getResult();
            exportMessage = (result != juce::File() && processor.exportTempoMap (result))
                ? Text::from ("Mapa de tempo exportado: ") + result.getFileName()
                : Text::from ("Falha ao exportar o mapa de tempo.");

            updateStatusLabel();
        });
}