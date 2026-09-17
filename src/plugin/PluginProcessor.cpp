#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout
MiniSynthProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "gain", 1 },
        "Gain",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f },
        -12.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "cutoff", 1 },
        "Cutoff",
        juce::NormalisableRange<float> { 20.0f, 20000.0f, 1.0f, 0.23f },
        800.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "resonance", 1 },
        "Resonance",
        juce::NormalisableRange<float> { 0.5f, 10.0f, 0.01f },
        0.707f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "filterMode", 1 },
        "Filter Mode",
        juce::StringArray{ "Low Pass", "Band Pass", "High Pass" },
        0));

    return layout;
}

MiniSynthProcessor::MiniSynthProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output",
        juce::AudioChannelSet::stereo(),
        true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    gainParam = apvts.getRawParameterValue("gain");
    cutoffParam = apvts.getRawParameterValue("cutoff");
    resonanceParam = apvts.getRawParameterValue("resonance");
    filterModeParam = apvts.getRawParameterValue("filterMode");
}

void MiniSynthProcessor::prepareToPlay(double sampleRate, int)
{
    engine.setSampleRate(sampleRate);
    engine.reset();

    // Sensible defaults so there's sound before any knob is touched.
    engine.setFilterParams(800.0, 0.707, minisynth::FilterMode::lowPass);
}

bool MiniSynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void MiniSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Merge on-screen keyboard events into the incoming MIDI.
    keyboardState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);

    const auto gain = juce::Decibels::decibelsToGain(gainParam->load());

    engine.setFilterParams(cutoffParam->load(),
        resonanceParam->load(),
        static_cast<minisynth::FilterMode> (
            static_cast<int> (filterModeParam->load())));

    const auto numSamples = buffer.getNumSamples();

    auto it = midi.begin();
    const auto end = midi.end();

    for (int i = 0; i < numSamples; ++i)
    {
        // Handle any MIDI events scheduled at or before this sample.
        while (it != end && (*it).samplePosition <= i)
        {
            const auto message = (*it).getMessage();

            if (message.isNoteOn())
                engine.noteOn(message.getNoteNumber(),
                    message.getFloatVelocity());
            else if (message.isNoteOff())
                engine.noteOff(message.getNoteNumber());
            else if (message.isAllNotesOff())
                engine.allNotesOff();

            ++it;
        }

        const auto sample = engine.getNextSample() * gain;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, i, sample);
    }
}

juce::AudioProcessorEditor* MiniSynthProcessor::createEditor()
{
    return new MiniSynthEditor(*this);
}

void MiniSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void MiniSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
// This is how JUCE's plugin wrappers instantiate your processor.
// It must be in the global namespace (not inside a class or namespace).
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MiniSynthProcessor();
}