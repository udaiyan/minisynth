# MiniSynth

A polyphonic subtractive synthesiser built with JUCE, with a
three-layer automated test suite and CI on Windows and Linux.

![CI](https://github.com/udaiyan/minisynth/actions/workflows/ci.yml/badge.svg)
[![codecov](https://codecov.io/gh/udaiyan/minisynth/branch/main/graph/badge.svg)](https://codecov.io/gh/udaiyan/minisynth)

## What's here
[the synth features, briefly]

## Testing
- 45 tests across three layers
- ~90% coverage on src/dsp, ~85% on src/plugin
- CI runs on Windows and Linux on every push
- Test results and coverage attached to every CI run

## Architecture
- `src/dsp/` — pure C++, no JUCE, fully unit-testable
- `src/plugin/` — JUCE shell: parameters, editor, host integration
- `tests/unit/` — fast DSP tests
- `tests/plugin/` — JUCE integration and component tests

## Why this structure

The synthesis engine in `src/dsp/` has no JUCE dependency. The plugin
links it; the tests link it; nothing else does.

This is a deliberate application of the **Humble Object** pattern — push
the logic into a plain class, and leave the framework class as a thin
shell. `MiniSynthProcessor` reads parameters, loops over samples, and
writes a buffer. Everything it *does* is delegated to `minisynth::SynthEngine`
and the classes beneath it.

That split buys three things:

1. **Fast tests.** The DSP suite runs 45 tests in under a second. No
   plugin host, no audio device, no message thread — just plain objects
   with plain state.

2. **Good failure messages.** An envelope test that fails is an envelope
   bug. A plugin test that fails could be any of ten things. Testing the
   layers separately means a red test names the layer it broke in.

3. **A fast feedback loop.** Changing the filter doesn't recompile JUCE.
   That matters more than it sounds — test suites don't die of being
   wrong, they die of being slow enough that people stop running them.

The cost is real: the glue is under-tested, and the plugin-level suite
exists specifically to cover it.

## Known limitations
- Naive saw/square oscillators alias at high frequencies
- No parameter smoothing (zipper noise on fast moves)
- Voice stealing clicks rather than crossfading
- No velocity on the on-screen keyboard

## Docs
- [Testing strategy](docs/TESTING.md)
- [AI-assisted test policy](docs/AI_ASSISTED_TESTS.md)
- [Setup](docs/SETUP.md)
- [Roadmap](ROADMAP.md)