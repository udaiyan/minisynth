#include "PluginEditor.h"

MiniSynthEditor::MiniSynthEditor(MiniSynthProcessor& p)
    : AudioProcessorEditor(&p),
    processor(p),
    keyboard(p.keyboardState,
        juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(36, 96);   // C2 to C7
    keyboard.setKeyWidth(22.0f);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,
        juce::Colours::white);
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,
        juce::Colours::black);
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,
        juce::Colour(0xff3a3a45));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
        juce::Colour(0xff5ac8fa).withAlpha(0.4f));

    setWantsKeyboardFocus(true);

    // ---- Knobs ----
    auto setUpKnob = [this](juce::Slider& slider, juce::Label& label,
        const juce::String& labelText)
        {
            addAndMakeVisible(slider);
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
            slider.setColour(juce::Slider::rotarySliderFillColourId,
                juce::Colour(0xff5ac8fa));

            addAndMakeVisible(label);
            label.setText(labelText, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId,
                juce::Colours::white.withAlpha(0.7f));
        };

    setUpKnob(gainSlider, gainLabel, "GAIN");
    setUpKnob(cutoffSlider, cutoffLabel, "CUTOFF");
    setUpKnob(resonanceSlider, resonanceLabel, "RESONANCE");

    // ---- Mode dropdown ----
    addAndMakeVisible(modeLabel);
    modeLabel.setText("FILTER MODE", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId,
        juce::Colours::white.withAlpha(0.7f));

    addAndMakeVisible(modeBox);
    modeBox.setColour(juce::ComboBox::backgroundColourId,
        juce::Colour(0xff2a2a35));
    modeBox.setColour(juce::ComboBox::textColourId,
        juce::Colours::white);
    modeBox.setColour(juce::ComboBox::outlineColourId,
        juce::Colour(0xff5ac8fa));
    modeBox.addItemList({ "Low Pass", "Band Pass", "High Pass" }, 1);

    // ---- Attachments ----
    gainAttachment = std::make_unique<SliderAttachment>(processor.apvts, "gain", gainSlider);
    cutoffAttachment = std::make_unique<SliderAttachment>(processor.apvts, "cutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<SliderAttachment>(processor.apvts, "resonance", resonanceSlider);
    modeAttachment = std::make_unique<ComboAttachment>(processor.apvts, "filterMode", modeBox);

    // IDs let tests (and accessibility tooling) locate these components.
    gainSlider.setComponentID("gainSlider");
    cutoffSlider.setComponentID("cutoffSlider");
    resonanceSlider.setComponentID("resonanceSlider");
    modeBox.setComponentID("filterMode");

    setSize(620, 480);
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

    auto knobRow = area.removeFromTop(200);
    const int knobWidth = knobRow.getWidth() / 3;

    auto placeKnob = [](juce::Rectangle<int> cell, juce::Label& label, juce::Slider& slider)
        {
            label.setBounds(cell.removeFromTop(24));
            slider.setBounds(cell.withSizeKeepingCentre(130, 130));
        };

    placeKnob(knobRow.removeFromLeft(knobWidth), gainLabel, gainSlider);
    placeKnob(knobRow.removeFromLeft(knobWidth), cutoffLabel, cutoffSlider);
    placeKnob(knobRow, resonanceLabel, resonanceSlider);

    auto modeRow = area.removeFromTop(70).reduced(60, 0);
    modeLabel.setBounds(modeRow.removeFromTop(22));
    modeBox.setBounds(modeRow.removeFromTop(32));

    // Keyboard fills whatever's left at the bottom
    keyboard.setBounds(area.removeFromTop(juce::jmin(area.getHeight(), 80)));
}