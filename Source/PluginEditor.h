#pragma once

#include <JuceHeader.h>

class PlayScoreProcessor;

class PlayScoreEditor : public juce::AudioProcessorEditor,
                        public juce::Timer
{
public:
    explicit PlayScoreEditor (PlayScoreProcessor& processor);
    ~PlayScoreEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateFileLabel();
    void updateAnalysisLabel();
    void updateStatusLabel();
    void exportTempoMap();

    PlayScoreProcessor& processor;

    juce::TextButton loadButton;
    juce::Label fileLabel;

    juce::Label instrumentLabel;
    juce::ComboBox instrumentSelector;

    juce::Label pitchLabel;
    juce::Slider pitchSlider;

    juce::Label transposeLabel;
    juce::Slider transposeSlider;

    juce::ToggleButton trainingToggle;
    juce::Label speedLabel;
    juce::Slider speedSlider;
    juce::ToggleButton loopToggle;
    juce::Label loopStartLabel;
    juce::Slider loopStartSlider;
    juce::Label loopEndLabel;
    juce::Slider loopEndSlider;

    juce::TextButton exportButton;

    juce::Label analysisLabel;
    juce::Label statusLabel;

    juce::Label cultureLabel;
    juce::ComboBox cultureSelector;

    juce::String exportMessage;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> exportChooser;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> instrumentAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transposeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> trainingToggleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopToggleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> loopStartAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> loopEndAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayScoreEditor)
};