#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <juce_audio_processors/juce_audio_processors.h>
#include "plugin/PluginProcessor.h"

#include <cmath>

using Catch::Matchers::WithinAbs;

// JUCE needs a message manager alive for AudioProcessor construction. Scoping
// it to the test body (via this fixture) keeps it out of the way during
// Catch2's --list-tests pass, which is how catch_discover_tests works.
struct JuceFixture
{
    juce::ScopedJuceInitialiser_GUI juceInit;
};

namespace
{
    constexpr double sr = 44100.0;
    constexpr int    blockSize = 512;

    std::unique_ptr<MiniSynthProcessor> makeProcessor()
    {
        auto p = std::make_unique<MiniSynthProcessor>();
        p->prepareToPlay(sr, blockSize);
        return p;
    }
}

//==============================================================================
// Identity and metadata
//==============================================================================

TEST_CASE_METHOD(JuceFixture,"Plugin: reports the expected metadata to the host", "[plugin]")
{
    auto processor = makeProcessor();

    REQUIRE(processor->getName() == "MiniSynth");
    REQUIRE(processor->acceptsMidi());
    REQUIRE_FALSE(processor->producesMidi());
    REQUIRE_FALSE(processor->isMidiEffect());
    REQUIRE(processor->hasEditor());
}

//==============================================================================
// Parameter layout
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Plugin: exposes every expected parameter", "[plugin]")
{
    auto processor = makeProcessor();

    REQUIRE(processor->apvts.getParameter("gain") != nullptr);
    REQUIRE(processor->apvts.getParameter("cutoff") != nullptr);
    REQUIRE(processor->apvts.getParameter("resonance") != nullptr);
    REQUIRE(processor->apvts.getParameter("filterMode") != nullptr);
}

TEST_CASE_METHOD(JuceFixture, "Plugin: parameters have the expected defaults", "[plugin]")
{
    auto processor = makeProcessor();

    SECTION("gain defaults to -12 dB")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (
            processor->apvts.getParameter("gain"));

        REQUIRE(param != nullptr);
        REQUIRE_THAT(param->get(), WithinAbs(-12.0f, 0.01f));
    }

    SECTION("cutoff defaults to 800 Hz")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (
            processor->apvts.getParameter("cutoff"));

        REQUIRE(param != nullptr);
        REQUIRE_THAT(param->get(), WithinAbs(800.0f, 0.5f));
    }

    SECTION("filterMode defaults to low-pass (index 0)")
    {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (
            processor->apvts.getParameter("filterMode"));

        REQUIRE(param != nullptr);
        REQUIRE(param->getIndex() == 0);
        REQUIRE(param->choices.size() == 3);
    }
}

//==============================================================================
// State round-trip
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Plugin: state survives a save and restore", "[plugin]")
{
    auto original = makeProcessor();

    // Change two parameters from their defaults.
    auto* gain = dynamic_cast<juce::AudioParameterFloat*> (
        original->apvts.getParameter("gain"));
    REQUIRE(gain != nullptr);
    gain->setValueNotifyingHost(gain->getNormalisableRange().convertTo0to1(-30.0f));

    auto* cutoff = dynamic_cast<juce::AudioParameterFloat*> (
        original->apvts.getParameter("cutoff"));
    REQUIRE(cutoff != nullptr);
    cutoff->setValueNotifyingHost(cutoff->getNormalisableRange().convertTo0to1(2500.0f));

    // Serialise.
    juce::MemoryBlock saved;
    original->getStateInformation(saved);
    REQUIRE(saved.getSize() > 0);

    // Load into a fresh, defaulted processor.
    auto restored = makeProcessor();
    restored->setStateInformation(saved.getData(),
        static_cast<int> (saved.getSize()));

    // Both parameters should have come back.
    auto* restoredGain = dynamic_cast<juce::AudioParameterFloat*> (
        restored->apvts.getParameter("gain"));
    REQUIRE(restoredGain != nullptr);
    REQUIRE_THAT(restoredGain->get(), WithinAbs(-30.0f, 0.01f));

    auto* restoredCutoff = dynamic_cast<juce::AudioParameterFloat*> (
        restored->apvts.getParameter("cutoff"));
    REQUIRE(restoredCutoff != nullptr);
    REQUIRE_THAT(restoredCutoff->get(), WithinAbs(2500.0f, 0.5f));
}

//==============================================================================
// Bus layout
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Plugin: accepts mono and stereo, rejects surround", "[plugin]")
{
    auto processor = makeProcessor();

    SECTION("stereo output is supported")
    {
        juce::AudioProcessor::BusesLayout layout;
        layout.outputBuses.add(juce::AudioChannelSet::stereo());
        REQUIRE(processor->isBusesLayoutSupported(layout));
    }

    SECTION("mono output is supported")
    {
        juce::AudioProcessor::BusesLayout layout;
        layout.outputBuses.add(juce::AudioChannelSet::mono());
        REQUIRE(processor->isBusesLayoutSupported(layout));
    }

    SECTION("5.1 surround output is rejected")
    {
        juce::AudioProcessor::BusesLayout layout;
        layout.outputBuses.add(juce::AudioChannelSet::create5point1());
        REQUIRE_FALSE(processor->isBusesLayoutSupported(layout));
    }
}

//==============================================================================
// Audio output behaviour
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Plugin: produces silence when no notes are playing", "[plugin]")
{
    auto processor = makeProcessor();

    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // The engine has no active voices, so every sample should be zero.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            REQUIRE(buffer.getSample(ch, i) == 0.0f);
}

TEST_CASE_METHOD(JuceFixture, "Plugin: a note-on produces finite, bounded audio", "[plugin]")
{
    auto processor = makeProcessor();

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    processor->processBlock(buffer, midi);

    bool anyNonZero = false;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto s = buffer.getSample(ch, i);

            REQUIRE(std::isfinite(s));
            REQUIRE(std::abs(s) <= 1.0f);

            if (std::abs(s) > 0.001f)
                anyNonZero = true;
        }
    }

    REQUIRE(anyNonZero);
}

TEST_CASE_METHOD(JuceFixture, "Plugin: a note-off eventually leads back to silence", "[plugin]")
{
    auto processor = makeProcessor();

    juce::AudioBuffer<float> buffer(2, blockSize);

    // Play the note.
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        buffer.clear();
        processor->processBlock(buffer, midi);
    }

    // Release it.
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        buffer.clear();
        processor->processBlock(buffer, midi);
    }

    // Run long enough for the envelope release to finish.
    // Release is 200 ms; 40 blocks × 512 samples / 44100 ≈ 465 ms.
    for (int block = 0; block < 40; ++block)
    {
        juce::MidiBuffer midi;
        buffer.clear();
        processor->processBlock(buffer, midi);
    }

    // Should now be silent.
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = std::max(peak, std::abs(buffer.getSample(ch, i)));

    REQUIRE(peak < 0.001f);
}