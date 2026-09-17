#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include "dsp/Oscillator.h"

using namespace minisynth;
using Catch::Matchers::WithinAbs;

namespace
{
    // The oscillator outputs the current phase and then advances, so
    // call N uses phase (N-1) * phaseIncrement. This helper makes that
    // explicit: sampleAt(osc, 26) is the 26th sample the oscillator produces.
    float sampleAt(Oscillator& osc, int n)
    {
        float s = 0.0f;
        for (int i = 0; i < n; ++i)
            s = osc.getNextSample();
        return s;
    }
}

//==============================================================================
// Defaults and invariants
//==============================================================================

TEST_CASE("Oscillator: default state produces valid samples", "[oscillator]")
{
    Oscillator osc;
    osc.setSampleRate(44100.0);

    for (int i = 0; i < 1000; ++i)
    {
        const auto s = osc.getNextSample();

        REQUIRE(std::isfinite(s));
        REQUIRE(s >= -1.0f);
        REQUIRE(s <= 1.0f);
    }
}

//==============================================================================
// Waveform correctness
//==============================================================================

TEST_CASE("Oscillator: sine at 441 Hz completes a cycle in 100 samples", "[oscillator]")
{
    Oscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);
    osc.setWaveform(Waveform::sine);

    // 441 Hz at 44100 Hz means 100 samples per cycle, so each sample is
    // 1% of a cycle. Phase 0.25 is the peak; phase 0.50 is back at zero.
    osc.reset();
    REQUIRE_THAT(sampleAt(osc, 1), WithinAbs(0.0f, 0.01f));   // phase 0.00

    osc.reset();
    REQUIRE_THAT(sampleAt(osc, 26), WithinAbs(1.0f, 0.01f));   // phase 0.25

    osc.reset();
    REQUIRE_THAT(sampleAt(osc, 51), WithinAbs(0.0f, 0.01f));   // phase 0.50
}

TEST_CASE("Oscillator: saw ramps up and wraps back to -1", "[oscillator]")
{
    Oscillator osc;
    osc.setSampleRate(100.0);
    osc.setFrequency(1.0);
    osc.setWaveform(Waveform::saw);
    osc.reset();

    // Start of cycle: phase 0.00 → -1
    REQUIRE_THAT(osc.getNextSample(), WithinAbs(-1.0f, 0.02f));

    // Half way: phase 0.50 → 0
    for (int i = 0; i < 49; ++i)
        osc.getNextSample();

    REQUIRE_THAT(osc.getNextSample(), WithinAbs(0.0f, 0.02f));

    // Near the end: phase 0.99 → 0.98
    for (int i = 0; i < 48; ++i)
        osc.getNextSample();

    REQUIRE_THAT(osc.getNextSample(), WithinAbs(0.98f, 0.02f));

    // And the next sample wraps: phase 0.00 → -1
    REQUIRE_THAT(osc.getNextSample(), WithinAbs(-1.0f, 0.02f));
}
TEST_CASE("Oscillator: square outputs only +1 or -1", "[oscillator]")
{
    Oscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(440.0);
    osc.setWaveform(Waveform::square);
    osc.reset();

    for (int i = 0; i < 1000; ++i)
    {
        const auto s = osc.getNextSample();
        REQUIRE((s == 1.0f || s == -1.0f));
    }
}

//==============================================================================
// Boundary conditions
//==============================================================================

TEST_CASE("Oscillator: guards against invalid input", "[oscillator]")
{
    Oscillator osc;
    osc.setSampleRate(44100.0);
    osc.setWaveform(Waveform::sine);

    SECTION("negative frequency is clamped to zero")
    {
        osc.setFrequency(-100.0);
        osc.reset();

        for (int i = 0; i < 10; ++i)
            REQUIRE(osc.getNextSample() == 0.0f);
    }

    SECTION("zero sample rate falls back to a safe default")
    {
        osc.setSampleRate(0.0);
        osc.setFrequency(440.0);
        osc.reset();

        for (int i = 0; i < 100; ++i)
            REQUIRE(std::isfinite(osc.getNextSample()));
    }

    SECTION("extremely high frequency does not produce NaN")
    {
        osc.setFrequency(1.0e9);
        osc.reset();

        for (int i = 0; i < 100; ++i)
            REQUIRE(std::isfinite(osc.getNextSample()));
    }
}

//==============================================================================
// State management
//==============================================================================

TEST_CASE("Oscillator: reset returns the phase to the start", "[oscillator]")
{
    Oscillator osc;
    osc.setSampleRate(100.0);
    osc.setFrequency(1.0);
    osc.setWaveform(Waveform::saw);
    osc.reset();

    const auto firstSample = osc.getNextSample();

    for (int i = 0; i < 30; ++i)
        osc.getNextSample();

    osc.reset();

    REQUIRE(osc.getNextSample() == firstSample);
}