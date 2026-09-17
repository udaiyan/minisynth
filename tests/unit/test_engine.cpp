#include <catch2/catch_test_macros.hpp>

#include "dsp/SynthEngine.h"

using namespace minisynth;

namespace
{
    constexpr double sr = 44100.0;

    SynthEngine makeEngine()
    {
        SynthEngine engine;
        engine.setSampleRate(sr);
        engine.setFilterParams(20000.0, 0.707, FilterMode::lowPass);
        return engine;
    }
}

TEST_CASE("SynthEngine: starts with no active voices", "[engine]")
{
    auto engine = makeEngine();
    REQUIRE(engine.getActiveVoiceCount() == 0);
}

TEST_CASE("SynthEngine: allocates one voice per note", "[engine]")
{
    auto engine = makeEngine();

    engine.noteOn(60, 1.0f);
    engine.noteOn(64, 1.0f);
    engine.noteOn(67, 1.0f);

    REQUIRE(engine.getActiveVoiceCount() == 3);
}

TEST_CASE("SynthEngine: polyphony is capped at maxVoices", "[engine]")
{
    auto engine = makeEngine();

    for (int i = 0; i < SynthEngine::maxVoices + 4; ++i)
        engine.noteOn(60 + i, 1.0f);

    REQUIRE(engine.getActiveVoiceCount() == SynthEngine::maxVoices);
}

TEST_CASE("SynthEngine: voice stealing is round-robin and deterministic", "[engine]")
{
    auto engine = makeEngine();

    for (int i = 0; i < SynthEngine::maxVoices; ++i)
        engine.noteOn(60 + i, 1.0f);

    // Voice 0 was allocated first, so it is the first to be stolen.
    engine.noteOn(100, 1.0f);

    REQUIRE(engine.getVoice(0).getCurrentNote() == 100);

    // The next steal should take voice 1.
    engine.noteOn(101, 1.0f);

    REQUIRE(engine.getVoice(1).getCurrentNote() == 101);
}

TEST_CASE("SynthEngine: noteOff releases the matching voice, not others", "[engine]")
{
    auto engine = makeEngine();

    engine.noteOn(60, 1.0f);
    engine.noteOn(64, 1.0f);

    REQUIRE(engine.getActiveVoiceCount() == 2);

    engine.noteOff(60);

    // The voice is still active — it's in the release phase.
    REQUIRE(engine.getActiveVoiceCount() == 2);

    // Run long enough for the release to finish.
    for (int i = 0; i < 44100 * 2; ++i)
        engine.getNextSample();

    REQUIRE(engine.getActiveVoiceCount() == 1);
}

TEST_CASE("SynthEngine: a stray noteOff does not affect active voices", "[engine]")
{
    auto engine = makeEngine();

    engine.noteOn(60, 1.0f);

    engine.noteOff(72);   // never played

    REQUIRE(engine.getActiveVoiceCount() == 1);
}

TEST_CASE("SynthEngine: allNotesOff releases every active voice", "[engine]")
{
    auto engine = makeEngine();

    for (int i = 0; i < 5; ++i)
        engine.noteOn(60 + i, 1.0f);

    engine.allNotesOff();

    for (int i = 0; i < 44100 * 2; ++i)
        engine.getNextSample();

    REQUIRE(engine.getActiveVoiceCount() == 0);
}

TEST_CASE("SynthEngine: polyphonic output is the sum of the voices", "[engine]")
{
    auto engine = makeEngine();

    engine.noteOn(60, 1.0f);

    float singlePeak = 0.0f;
    for (int i = 0; i < 4410; ++i)
        singlePeak = std::max(singlePeak, std::abs(engine.getNextSample()));

    auto engine2 = makeEngine();
    engine2.noteOn(60, 1.0f);
    engine2.noteOn(64, 1.0f);
    engine2.noteOn(67, 1.0f);

    float chordPeak = 0.0f;
    for (int i = 0; i < 4410; ++i)
        chordPeak = std::max(chordPeak, std::abs(engine2.getNextSample()));

    REQUIRE(chordPeak > singlePeak);
}