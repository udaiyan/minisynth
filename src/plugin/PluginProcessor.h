#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Oscillator.h"
#include "dsp/StateVariableFilter.h"
#include "dsp/SynthEngine.h"
#include <atomic> 

class MiniSynthProcessor : public juce::AudioProcessor
{
public:
    MiniSynthProcessor();
    ~MiniSynthProcessor() override = default;

    // --- Audio lifecycle ---
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // --- Editor ---
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // --- Identity / metadata ---
    const juce::String getName() const override { return "MiniSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // --- Presets (unused for now) ---
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // --- State save/restore ---
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    minisynth::SynthEngine engine;

    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* resonanceParam = nullptr;
    std::atomic<float>* filterModeParam = nullptr;

public:
    // The on-screen keyboard feeds notes into here.
    juce::MidiKeyboardState keyboardState;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniSynthProcessor)
};