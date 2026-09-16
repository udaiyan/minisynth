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

    return layout;
}

MiniSynthProcessor::MiniSynthProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output",
        juce::AudioChannelSet::stereo(),
        true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{}

void MiniSynthProcessor::prepareToPlay(double sampleRate, int)
{
    phase = 0.0;
    phaseDelta = juce::MathConstants<double>::twoPi * 440.0 / sampleRate;
}

bool MiniSynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void MiniSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto gainDb = apvts.getRawParameterValue("gain")->load();
    const auto gain = juce::Decibels::decibelsToGain(gainDb);

    const auto numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        const auto sample = (float)std::sin(phase) * gain;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, i, sample);

        phase += phaseDelta;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
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