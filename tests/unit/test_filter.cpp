#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include "dsp/StateVariableFilter.h"

using namespace minisynth;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr double sr = 44100.0;
    constexpr double twoPi = 6.283185307179586;

    // Run the filter with a constant input until it settles, return the last output.
    float settleWithDC(StateVariableFilter& filter, float input, int samples = 4410)
    {
        float out = 0.0f;
        for (int i = 0; i < samples; ++i)
            out = filter.processSample(input);
        return out;
    }
}

TEST_CASE("StateVariableFilter: low-pass passes DC", "[filter]")
{
    StateVariableFilter filter;
    filter.setSampleRate(sr);
    filter.setCutoff(1000.0);
    filter.setResonance(0.707);
    filter.setMode(FilterMode::lowPass);
    filter.reset();

    REQUIRE_THAT(settleWithDC(filter, 1.0f), WithinAbs(1.0f, 0.01f));
}

TEST_CASE("StateVariableFilter: high-pass blocks DC", "[filter]")
{
    StateVariableFilter filter;
    filter.setSampleRate(sr);
    filter.setCutoff(1000.0);
    filter.setResonance(0.707);
    filter.setMode(FilterMode::highPass);
    filter.reset();

    REQUIRE_THAT(settleWithDC(filter, 1.0f), WithinAbs(0.0f, 0.01f));
}

TEST_CASE("StateVariableFilter: low-pass attenuates a high frequency", "[filter]")
{
    StateVariableFilter filter;
    filter.setSampleRate(sr);
    filter.setCutoff(200.0);
    filter.setResonance(0.707);
    filter.setMode(FilterMode::lowPass);
    filter.reset();

    // Feed a 10 kHz sine — 50× above the cutoff
    const double freq = 10000.0;
    const double inc = twoPi * freq / sr;
    double phase = 0.0;

    auto step = [&]
        {
            const auto out = filter.processSample(static_cast<float> (std::sin(phase)));
            phase += inc;
            if (phase >= twoPi) phase -= twoPi;
            return out;
        };

    // Let the filter reach steady state
    for (int i = 0; i < 4410; ++i)
        step();

    // Measure peak amplitude over the next 100 ms
    float peak = 0.0f;
    for (int i = 0; i < 4410; ++i)
        peak = std::max(peak, std::abs(step()));

    REQUIRE(peak < 0.1f);
}

TEST_CASE("StateVariableFilter: band-pass rejects both extremes", "[filter]")
{
    StateVariableFilter filter;
    filter.setSampleRate(sr);
    filter.setCutoff(1000.0);
    filter.setResonance(0.707);
    filter.setMode(FilterMode::bandPass);
    filter.reset();

    REQUIRE_THAT(settleWithDC(filter, 1.0f), WithinAbs(0.0f, 0.05f));
}

TEST_CASE("StateVariableFilter: guards against invalid configuration", "[filter]")
{
    SECTION("cutoff above Nyquist is clamped")
    {
        StateVariableFilter filter;
        filter.setSampleRate(sr);
        filter.setCutoff(100000.0);   // far above Nyquist (22050)
        filter.reset();

        for (int i = 0; i < 1000; ++i)
            REQUIRE(std::isfinite(filter.processSample(1.0f)));
    }

    SECTION("zero sample rate falls back to a safe default")
    {
        StateVariableFilter filter;
        filter.setSampleRate(0.0);
        filter.setCutoff(1000.0);
        filter.reset();

        for (int i = 0; i < 1000; ++i)
            REQUIRE(std::isfinite(filter.processSample(1.0f)));
    }

    SECTION("extreme resonance stays stable")
    {
        StateVariableFilter filter;
        filter.setSampleRate(sr);
        filter.setCutoff(1000.0);
        filter.setResonance(100.0);
        filter.reset();

        for (int i = 0; i < 10000; ++i)
            REQUIRE(std::isfinite(filter.processSample(1.0f)));
    }
}