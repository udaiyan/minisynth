#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include "dsp/AdsrEnvelope.h"

using namespace minisynth;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr double sr = 44100.0;
    constexpr int    oneSec = static_cast<int> (sr);
}

TEST_CASE("AdsrEnvelope: silent before any note", "[envelope]")
{
    AdsrEnvelope env;
    env.setSampleRate(sr);

    REQUIRE(env.getStage() == AdsrEnvelope::Stage::idle);
    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 100; ++i)
        REQUIRE(env.getNextSample() == 0.0f);
}

TEST_CASE("AdsrEnvelope: attack reaches full level in the configured time", "[envelope]")
{
    AdsrEnvelope env;
    env.setSampleRate(sr);
    env.setParameters(0.1, 0.1, 0.5, 0.1);   // attack, decay, sustain, release

    env.noteOn();
    REQUIRE(env.getStage() == AdsrEnvelope::Stage::attack);

    const int attackSamples = static_cast<int> (0.1 * sr);   // 4410

    float level = 0.0f;
    for (int i = 0; i < attackSamples; ++i)
        level = env.getNextSample();

    REQUIRE_THAT(level, WithinAbs(1.0f, 0.01f));
}

TEST_CASE("AdsrEnvelope: decay settles at the sustain level", "[envelope]")
{
    AdsrEnvelope env;
    env.setSampleRate(sr);
    env.setParameters(0.01, 0.1, 0.6, 0.1);

    env.noteOn();

    for (int i = 0; i < oneSec; ++i)
        env.getNextSample();

    REQUIRE(env.getStage() == AdsrEnvelope::Stage::sustain);
    REQUIRE_THAT(env.getNextSample(), WithinAbs(0.6f, 0.001f));
}

TEST_CASE("AdsrEnvelope: release reaches zero in the configured time", "[envelope]")
{
    AdsrEnvelope env;
    env.setSampleRate(sr);
    env.setParameters(0.01, 0.01, 0.5, 0.2);

    env.noteOn();
    for (int i = 0; i < oneSec; ++i)
        env.getNextSample();

    env.noteOff();
    REQUIRE(env.getStage() == AdsrEnvelope::Stage::release);

    const int releaseSamples = static_cast<int>(0.2 * sr);   // 8820

    // ---- The release should not finish early ----
    for (int i = 0; i < releaseSamples - 100; ++i)
        env.getNextSample();

    REQUIRE(env.getStage() == AdsrEnvelope::Stage::release);

    // ---- ...and it should be done shortly after the nominal time ----
    // A small tolerance absorbs floating-point accumulation; without it
    // we would be asserting that 0.5 / 8820 is exactly representable,
    // which it isn't.
    for (int i = 0; i < 200; ++i)
        env.getNextSample();

    REQUIRE(env.getStage() == AdsrEnvelope::Stage::idle);
    REQUIRE(env.getNextSample() == 0.0f);
}

TEST_CASE("AdsrEnvelope: release from mid-attack still takes the release time", "[envelope]")
{
    AdsrEnvelope env;
    env.setSampleRate(sr);
    env.setParameters(1.0, 0.01, 0.5, 0.2);   // very long attack

    env.noteOn();

    // Halfway through the attack — level should be around 0.5
    for (int i = 0; i < static_cast<int>(0.5 * sr); ++i)
        env.getNextSample();

    REQUIRE(env.getStage() == AdsrEnvelope::Stage::attack);

    env.noteOff();
    REQUIRE(env.getStage() == AdsrEnvelope::Stage::release);

    // Despite releasing from ~0.5 rather than 1.0, it should still
    // take the full 0.2 seconds to reach zero.
    const int releaseSamples = static_cast<int>(0.2 * sr);

    for (int i = 0; i < releaseSamples; ++i)
        env.getNextSample();

    REQUIRE(env.getStage() == AdsrEnvelope::Stage::idle);
}

TEST_CASE("AdsrEnvelope: guards against invalid parameters", "[envelope]")
{
    AdsrEnvelope env;
    env.setSampleRate(sr);

    SECTION("zero attack and release do not produce NaN")
    {
        env.setParameters(0.0, 0.0, 0.5, 0.0);
        env.noteOn();

        for (int i = 0; i < 1000; ++i)
            REQUIRE(std::isfinite(env.getNextSample()));
    }

    SECTION("sustain level is clamped to 0..1")
    {
        env.setParameters(0.01, 0.01, 5.0, 0.1);
        env.noteOn();

        for (int i = 0; i < oneSec; ++i)
        {
            const auto level = env.getNextSample();
            REQUIRE(level >= 0.0f);
            REQUIRE(level <= 1.0f);
        }
    }

    SECTION("a stray noteOff while idle is ignored")
    {
        env.noteOff();
        REQUIRE(env.getStage() == AdsrEnvelope::Stage::idle);
        REQUIRE(env.getNextSample() == 0.0f);
    }
}