#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
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

    juce::Slider gainSlider;
    juce::Label  gainLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniSynthEditor)
};