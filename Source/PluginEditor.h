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

    PlayScoreProcessor& processor;

    juce::TextButton loadButton;
    juce::Label fileLabel;

    juce::Label instrumentLabel;
    juce::ComboBox instrumentSelector;

    juce::Label pitchLabel;
    juce::Slider pitchSlider;

    juce::Label transposeLabel;
    juce::Slider transposeSlider;

    juce::Label analysisLabel;
    juce::Label statusLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> instrumentAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transposeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayScoreEditor)
};