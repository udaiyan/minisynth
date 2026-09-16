#include "PluginEditor.h"

MiniSynthEditor::MiniSynthEditor(MiniSynthProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    addAndMakeVisible(gainSlider);
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    gainSlider.setColour(juce::Slider::rotarySliderFillColourId,
        juce::Colour(0xff5ac8fa));

    addAndMakeVisible(gainLabel);
    gainLabel.setText("GAIN", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setColour(juce::Label::textColourId,
        juce::Colours::white.withAlpha(0.7f));

    gainAttachment = std::make_unique<SliderAttachment>(processor.apvts,
        "gain",
        gainSlider);

    setSize(420, 320);
}

void MiniSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1b1b22));

    auto header = getLocalBounds().removeFromTop(52);
    g.setColour(juce::Colour(0xff2a2a35));
    g.fillRect(header);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    g.drawText("MiniSynth", header.reduced(18, 0),
        juce::Justification::centredLeft, false);
}

void MiniSynthEditor::resized()
{
    auto area = getLocalBounds().reduced(18);
    area.removeFromTop(52);

    auto panel = area.removeFromTop(juce::jmin(area.getHeight(), 210));
    gainLabel.setBounds(panel.removeFromTop(24));
    gainSlider.setBounds(panel.withSizeKeepingCentre(150, 150));
}