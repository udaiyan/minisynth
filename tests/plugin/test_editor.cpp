#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <juce_audio_processors/juce_audio_processors.h>
#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

using Catch::Matchers::WithinAbs;

// Same fixture pattern as test_plugin.cpp — the JUCE message manager must
// exist while the editor is alive, but must NOT be constructed before main().
struct JuceFixture
{
    juce::ScopedJuceInitialiser_GUI juceInit;
};

namespace
{
    constexpr double sr = 44100.0;
    constexpr int    blockSize = 512;

    // Bundles a processor with its editor. The editor must be destroyed
    // BEFORE the processor, so member order matters: 'editor' is declared
    // after 'processor' and is therefore destroyed first.
    struct EditorHarness
    {
        std::unique_ptr<MiniSynthProcessor> processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor;

        EditorHarness()
        {
            processor = std::make_unique<MiniSynthProcessor>();
            processor->prepareToPlay(sr, blockSize);
            editor.reset(processor->createEditor());
            editor->setSize(620, 480);   // trigger a real resized()
        }
    };

    juce::Slider* findSlider(juce::Component& parent, const juce::String& id)
    {
        return dynamic_cast<juce::Slider*> (parent.findChildWithID(id));
    }

    juce::ComboBox* findCombo(juce::Component& parent, const juce::String& id)
    {
        return dynamic_cast<juce::ComboBox*> (parent.findChildWithID(id));
    }
}

//==============================================================================
// Construction
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Editor: constructs and reports a sensible size", "[editor]")
{
    EditorHarness h;

    REQUIRE(h.editor != nullptr);
    REQUIRE(h.editor->getWidth() == 620);
    REQUIRE(h.editor->getHeight() == 480);
}

TEST_CASE_METHOD(JuceFixture, "Editor: contains the expected controls", "[editor]")
{
    EditorHarness h;

    REQUIRE(findSlider(*h.editor, "gainSlider") != nullptr);
    REQUIRE(findSlider(*h.editor, "cutoffSlider") != nullptr);
    REQUIRE(findSlider(*h.editor, "resonanceSlider") != nullptr);
    REQUIRE(findCombo(*h.editor, "filterMode") != nullptr);
}

//==============================================================================
// Layout
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Editor: every control has non-zero bounds after resized()", "[editor]")
{
    EditorHarness h;

    for (auto* child : h.editor->getChildren())
    {
        if (!child->isVisible())
            continue;

        INFO("Component ID: " << child->getComponentID());

        REQUIRE(child->getWidth() > 0);
        REQUIRE(child->getHeight() > 0);
    }
}

//==============================================================================
// Parameter binding — the highest-value editor tests
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Editor: moving the gain slider updates the parameter", "[editor]")
{
    EditorHarness h;

    auto* slider = findSlider(*h.editor, "gainSlider");
    REQUIRE(slider != nullptr);

    auto* param = h.processor->apvts.getParameter("gain");
    REQUIRE(param != nullptr);

    // Move the slider to a specific real-world value.
    const auto targetDb = -36.0f;
    slider->setValue(targetDb, juce::sendNotificationSync);

    REQUIRE_THAT(param->getValue(),
        WithinAbs(param->getNormalisableRange().convertTo0to1(targetDb), 0.001f));
}

TEST_CASE_METHOD(JuceFixture, "Editor: changing the parameter moves the gain slider", "[editor]")
{
    EditorHarness h;

    auto* slider = findSlider(*h.editor, "gainSlider");
    REQUIRE(slider != nullptr);

    auto* param = dynamic_cast<juce::AudioParameterFloat*> (
        h.processor->apvts.getParameter("gain"));
    REQUIRE(param != nullptr);

    const auto targetDb = -48.0f;
    param->setValueNotifyingHost(
        param->getNormalisableRange().convertTo0to1(targetDb));

    REQUIRE_THAT(static_cast<float> (slider->getValue()),
        WithinAbs(targetDb, 0.5f));
}

TEST_CASE_METHOD(JuceFixture, "Editor: the filter mode dropdown is populated and bound", "[editor]")
{
    EditorHarness h;

    auto* combo = findCombo(*h.editor, "filterMode");
    REQUIRE(combo != nullptr);

    // Populated with the three modes.
    REQUIRE(combo->getNumItems() == 3);
    REQUIRE(combo->getItemText(0) == "Low Pass");
    REQUIRE(combo->getItemText(1) == "Band Pass");
    REQUIRE(combo->getItemText(2) == "High Pass");

    // Selecting an item updates the parameter.
    combo->setSelectedItemIndex(2, juce::sendNotificationSync);

    auto* param = dynamic_cast<juce::AudioParameterChoice*> (
        h.processor->apvts.getParameter("filterMode"));
    REQUIRE(param != nullptr);
    REQUIRE(param->getIndex() == 2);
}

//==============================================================================
// Keyboard
//==============================================================================

TEST_CASE_METHOD(JuceFixture, "Editor: has an on-screen keyboard wired to the processor", "[editor]")
{
    EditorHarness h;

    // The keyboard is a MidiKeyboardComponent; find it by type.
    juce::MidiKeyboardComponent* keyboard = nullptr;

    for (auto* child : h.editor->getChildren())
        if (auto* k = dynamic_cast<juce::MidiKeyboardComponent*> (child))
            keyboard = k;

    REQUIRE(keyboard != nullptr);

    // Pressing a key on the component should register in the processor's state.
    const int testNote = 60;   // middle C
    REQUIRE_FALSE(h.processor->keyboardState.isNoteOn(1, testNote));

    h.processor->keyboardState.noteOn(1, testNote, 0.8f);

    REQUIRE(h.processor->keyboardState.isNoteOn(1, testNote));
}

