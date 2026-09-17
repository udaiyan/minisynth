#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include "dsp/SynthVoice.h"

using namespace minisynth;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr double sr = 44100.0;

    SynthVoice makeReadyVoice()
    {
        SynthVoice voice;
        voice.setSampleRate(sr);
        voice.getEnvelope().setParameters(0.001, 10.0, 1.0, 0.1);
        voice.getFilter().setCutoff(20000.0);   // effectively bypass
        return voice;
    }
}

TEST_CASE("SynthVoice: inactive and silent until noteOn", "[voice]")
{
    auto voice = makeReadyVoice();

    REQUIRE_FALSE(voice.isActive());
    REQUIRE(voice.getNextSample() == 0.0f);
}

TEST_CASE("SynthVoice: noteOn makes the voice active and records the note", "[voice]")
{
    auto voice = makeReadyVoice();

    voice.noteOn(69, 1.0f);   // A4

    REQUIRE(voice.isActive());
    REQUIRE(voice.getCurrentNote() == 69);
}

TEST_CASE("SynthVoice: produces non-silent output while active", "[voice]")
{
    auto voice = makeReadyVoice();
    voice.noteOn(60, 1.0f);

    float peak = 0.0f;
    for (int i = 0; i < 4410; ++i)
        peak = std::max(peak, std::abs(voice.getNextSample()));

    REQUIRE(peak > 0.1f);
}

TEST_CASE("SynthVoice: becomes inactive after release completes", "[voice]")
{
    auto voice = makeReadyVoice();
    voice.noteOn(60, 1.0f);

    for (int i = 0; i < 4410; ++i)
        voice.getNextSample();

    voice.noteOff();

    // The release is 0.1 s — one second is comfortably enough
    for (int i = 0; i < 44100; ++i)
        voice.getNextSample();

    REQUIRE_FALSE(voice.isActive());
    REQUIRE(voice.getNextSample() == 0.0f);
}

TEST_CASE("SynthVoice: velocity scales the output amplitude", "[voice]")
{
    auto quiet = makeReadyVoice();
    auto loud = makeReadyVoice();

    quiet.noteOn(60, 0.25f);
    loud.noteOn(60, 1.0f);

    float quietPeak = 0.0f;
    float loudPeak = 0.0f;

    for (int i = 0; i < 4410; ++i)
    {
        quietPeak = std::max(quietPeak, std::abs(quiet.getNextSample()));
        loudPeak = std::max(loudPeak, std::abs(loud.getNextSample()));
    }

    REQUIRE(quietPeak < loudPeak);
    REQUIRE(quietPeak > 0.0f);
}