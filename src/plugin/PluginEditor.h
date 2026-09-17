#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h> 
#include "PluginProcessor.h"

class MiniSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit MiniSynthEditor(MiniSynthProcessor&);
    ~MiniSynthEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MiniSynthProcessor& processor;

    juce::Slider   gainSlider, cutoffSlider, resonanceSlider;
    juce::Label    gainLabel, cutoffLabel, resonanceLabel, modeLabel;
    juce::ComboBox modeBox;

    juce::MidiKeyboardComponent keyboard;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment;
    std::unique_ptr<SliderAttachment> resonanceAttachment;
    std::unique_ptr<ComboAttachment>  modeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniSynthEditor)
};