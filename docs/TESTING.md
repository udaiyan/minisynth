# Testing Strategy

How the test suite is structured, why it's structured that way, and the
conventions that keep it useful as it grows.

---

## 1. Test layers

Three test suites, each answering a different question.

| Suite | Location | Tests | Runtime | Question it answers |
|---|---|---|---|---|
| Unit | `tests/unit/` | 30 | <1 s | Does the DSP behave correctly? |
| Integration | `tests/plugin/test_plugin.cpp` | 8 | ~1 s | Does the plugin fulfil its host contract? |
| Component | `tests/plugin/test_editor.cpp` | 7 | ~1 s | Is the UI wired to the model? |

The unit suite links `minisynth_dsp` only. The other two link JUCE and
compile `PluginProcessor.cpp` and `PluginEditor.cpp` directly.

### Unit (`tests/unit/`)

Covers `src/dsp/` - `Oscillator`, `AdsrEnvelope`, `StateVariableFilter`,
`SynthVoice`, `SynthEngine`. No JUCE. No plugin host. No message thread.
Plain objects with plain state.

These are the tests you run every time you save a file. They're fast
enough to be part of the edit loop.

### Integration (`tests/plugin/test_plugin.cpp`)

Covers `MiniSynthProcessor` as a JUCE `AudioProcessor`. Parameter
layout, defaults, state save/restore, bus-layout negotiation, buffer
sanity, and MIDI → sound → silence.

This suite exists to catch the bugs unit tests structurally can't: the
wiring between the framework and the engine.

### Component (`tests/plugin/test_editor.cpp`)

Covers `MiniSynthEditor` - layout, parameter bindings, the filter-mode
dropdown, and the on-screen keyboard.

These test the *binding*, not the pixels. See §6 for why.

---

## 2. Framework choice: Catch2

Selected over GoogleTest and doctest.

| Criterion | Catch2 | GoogleTest | doctest |
|---|---|---|---|
| CMake integration | `catch_discover_tests` - first-class | First-class | Manual |
| Assertion readability | Expression decomposition | Standard macros | Standard macros |
| Setup reuse | `SECTION` | Fixtures | `SUBCASE` |
| Header-only option | Yes (v3 has a compiled form) | No | Yes |
| Per-test CI reporting | Built-in | Built-in | Manual |

**What decided it:** `catch_discover_tests` registers each `TEST_CASE`
individually with CTest at build time. That means CI reports "test 47 of
63 failed", not "the test binary exited with code 1". Per-test
granularity is what makes a CI report actionable, and it's the reason
Catch2 edges out doctest for this project.

GoogleTest is equally capable and would have been a defensible choice.
Catch2's `SECTION` blocks and expression decomposition tip it.

**What we don't use:** Catch2's BDD macros (`SCENARIO` / `GIVEN` /
`WHEN` / `THEN`). They read nicely but add indirection, and plain
`TEST_CASE` with a descriptive name covers the same ground.

---

## 3. Architecture: why the DSP has no JUCE

`src/dsp/` contains zero JUCE includes. `src/plugin/` links it, and so
do the tests. This is the single most consequential decision in the
codebase.

It's an application of the **Humble Object** pattern: extract the logic
into a plain class, leave the framework class as a thin shell.

`MiniSynthProcessor::processBlock` does four things:

1. Read cached parameter pointers
2. Loop over samples
3. Call `engine.getNextSample()`
4. Write the result to the buffer

Everything it *does* is delegated to `minisynth::SynthEngine` and the
classes beneath it. That's around 40 lines of glue. The 500+ lines of
behaviour live in `src/dsp/`.

### What this buys

**Speed.** The DSP suite runs in well under a second. A plugin-host
test of equivalent coverage would take minutes.

**Diagnosis.** A failed envelope test is an envelope bug. A failed
plugin test could be any of a dozen things. Separating the layers means
a red test names the layer it broke in.

**A working edit loop.** Changing the filter doesn't recompile JUCE.
Test suites don't die of being wrong - they die of being slow enough
that people stop running them.

### What it costs

**The glue is under-tested.** If `prepareToPlay` forgets to forward the
sample rate, no unit test will notice. The integration suite exists
specifically to cover that seam, and it's deliberately small because
the surface area is small.

---

## 4. Conventions

### Naming

Test names read as sentences describing behaviour:

    TEST_CASE ("AdsrEnvelope: release reaches zero in the configured time", "[envelope]")

Format: `<Class>: <what it does>`. This makes the test list itself a
specification - reading the names top to bottom tells you what the
class promises.

### Tags

Every test carries a tag: `[oscillator]`, `[envelope]`, `[filter]`,
`[voice]`, `[engine]`, `[plugin]`, `[editor]`.

Tags let you run a subset in a tight loop:

    minisynth_tests.exe "[filter]"

Tag from the start. It costs nothing and it's the difference between a
two-second and a ten-second iteration.

### `CHECK` vs `REQUIRE`

- **`REQUIRE`** - stops the test on failure. Use for preconditions and
  short tests.
- **`CHECK`** - continues on failure. Use inside long loops, so you see
  every failure rather than just the first.

Rule of thumb: `CHECK` in loops over 100 iterations, `REQUIRE` elsewhere.

### Fixtures

`tests/plugin/` uses a `JuceFixture` struct, applied via
`TEST_CASE_METHOD`, to hold a `ScopedJuceInitialiser_GUI`.

**It must be a fixture, not a namespace-scope global.** A static
initialiser runs before `main()`, which breaks Catch2's `--list-tests`
pass and causes test discovery to hang. This cost us a debugging
session; it's documented here so it doesn't happen again.

### Helpers

Repeated setup goes in an anonymous namespace at the top of the file:

- `sampleAt (Oscillator& osc, int n)`
- `settleWithDC (StateVariableFilter& filter, float input)`
- `makeReadyVoice()`

Anonymous namespaces give internal linkage - the helper won't collide
with a same-named function in another test file.

**Push counting into helpers.** Three separate test failures in this
repo were caused by off-by-one errors in loop counts. `sampleAt(osc, 26)`
is unambiguous; "advance 24 samples then check" is not.

### Floating-point assertions

| Situation | Assertion style |
|---|---|
| Exact values, no arithmetic | `==` |
| A range with meaningful tolerance | `WithinAbs(x, tol)` |
| After accumulation over many samples | Bounds, never `==` |

**Never assert exact equality after a chain of floating-point
arithmetic.** The envelope's release phase accumulates rounding error
across thousands of subtractions; asserting it lands exactly on zero is
asserting that `0.5 / 8820` is representable in binary, which it isn't.

Tests that depend on exact timing use a tolerance band: run for
`nominal − 100` samples and assert *not* finished, then run 200 more and
assert finished.

### Assertions on discrete signals

Test what the sampler produces, not the continuous ideal it approximates.

A saw wave is defined mathematically as ramping from −1 to +1 over one
cycle. Sampled at N points, it produces values from −1 up to `1 − 2/N`,
then wraps. It never outputs exactly +1. The test asserts on `0.98` for
a 100-sample cycle, not `1.0`.

---

## 5. Coverage

Current: **~91% line coverage on `src/`**, reported by gcovr and published
to [Codecov](https://codecov.io/gh/udaiyan/minisynth).

Coverage runs only on Linux, in a separate CI job with a Debug build and
`--coverage` instrumentation. The main Windows/Linux jobs build Release
without instrumentation - they're about correctness, not measurement.

`tests/` and `build/_deps/` are excluded. Coverage of test code is not a
useful metric, and JUCE/Catch2 coverage would swamp the signal.

### What isn't covered

The uncovered lines fall into three categories:

1. **Rendering** - `paint()` methods. Verified manually. See §6.
2. **Host notification paths** - code triggered by a running DAW.
   Exercised by the integration suite's state round-trip, but not every
   branch.
3. **Defensive branches** - guards for states that are hard to reach
   from tests, such as the `allNotesOff` path when no voices are active.

**Do not chase 100%.** The last few percent of coverage usually costs
more than it's worth and encourages assertions on implementation details
that break when the code is refactored.

---

## 6. What we deliberately don't test

Every testing strategy has exclusions. These are conscious decisions,
not oversights.

### Golden-image / visual regression tests

**Not used.** Rendering is platform-dependent - font hinting, DPI
scaling, and subpixel antialiasing all differ between Windows, Linux,
and macOS, and between JUCE versions. Reference images would need to be
maintained per-platform and regenerated on every JUCE upgrade.

The bugs this would catch are visual (spacing, colour, alignment), and
they're caught by a human looking at the screen in under a second.
The cost/benefit doesn't work at this scale.

### External UI automation (pywinauto, FlaUI, SikuliX)

**Not used.** Two reasons:

1. **It can only see the accessibility tree.** JUCE reports `Slider`
   and `ComboBox` as UI Automation elements, but anything drawn
   manually in `paint()` is invisible to automation tools.
2. **It only works for the Standalone app.** Inside a DAW, the window
   belongs to the host, and automation becomes host-specific.

Combined with the CI cost - Linux jobs need `xvfb`, tests become
timing-dependent and flaky - the payoff doesn't justify the complexity.

**If we added it**, the candidate uses would be: launching the
Standalone, loading a preset, and asserting audio is produced. That's
an end-to-end smoke test, not a UI test.

### Audio output golden files

**Not used.** Comparing rendered audio against a reference file is
sensitive to floating-point differences between compilers and
optimisation levels, and the reference goes stale whenever the DSP
changes intentionally.

Instead we assert on *properties* of the audio: finite, bounded,
non-zero when active, zero when idle. These survive a change from a
naive oscillator to a band-limited one.

### Threading and real-time safety

**Not tested.** Verifying that `processBlock` doesn't allocate, lock,
or block requires runtime instrumentation (address sanitiser, thread
sanitiser, allocation hooks) that's out of scope for this project.

The discipline is enforced by convention: no `new`, no `std::vector`
resize, no mutex acquisition in `processBlock`. A future improvement is
a CI job running the tests under ASan and TSan. Noted in `ROADMAP.md`.

---

## 7. Writing a new test

A worked example. Suppose you're adding a `triangle` waveform.

**Step 1 - decide which layer.** Triangle generation is DSP logic, so
the test goes in `tests/unit/test_oscillator.cpp`.

**Step 2 - write the test in terms of the contract.** A triangle ramps
up and down. At sample rate 100 Hz and frequency 1 Hz, one cycle is 100
samples. Phase 0.00 → −1, phase 0.25 → 0, phase 0.50 → +1, phase 0.75 → 0.

    TEST_CASE ("Oscillator: triangle ramps up and back down", "[oscillator]")
    {
        Oscillator osc;
        osc.setSampleRate (100.0);
        osc.setFrequency (1.0);
        osc.setWaveform (Waveform::triangle);

        osc.reset();
        REQUIRE_THAT (sampleAt (osc, 1),  WithinAbs (-1.0f, 0.02f));

        osc.reset();
        REQUIRE_THAT (sampleAt (osc, 26), WithinAbs ( 0.0f, 0.02f));

        osc.reset();
        REQUIRE_THAT (sampleAt (osc, 51), WithinAbs ( 1.0f, 0.02f));

        osc.reset();
        REQUIRE_THAT (sampleAt (osc, 76), WithinAbs ( 0.0f, 0.02f));
    }

**Step 3 - use the existing helpers.** `sampleAt` already exists.
`WithinAbs` is already imported. No new machinery.

**Step 4 - run it.** `minisynth_tests.exe "[oscillator]"`. If it fails,
the bug is in the oscillator, not the test - unless the test itself is
wrong about the discrete behaviour (see §4).

**Step 5 - check coverage.** Run the coverage job locally or wait for
CI. The new branch in the `switch` statement should now be covered.

**What good looks like:**

- One behaviour per test
- A name that describes the behaviour, not the implementation
- Simple numbers (100 Hz sample rate, 1 Hz frequency) so the maths is
  obvious
- Assertions on properties, not on internal state
- Uses existing helpers rather than reimplementing setup

---

## 8. Running the suite

**Locally, from the command line:**

    ctest --test-dir build --output-on-failure

**A single suite:**

    ./build/minisynth_tests.exe
    ./build/minisynth_plugin_tests.exe

**A subset by tag:**

    ./build/minisynth_tests.exe "[filter]"

**In CI:** every push and PR runs the full suite on Windows and Linux.
Test results are attached to every run as JUnit XML artifacts. Coverage
is uploaded to Codecov.

---

## 9. Known improvements

Documented but not implemented. In priority order.

1. **Per-target coverage instrumentation.** The current CMake applies
   `--coverage` globally, which instruments JUCE and Catch2 as well as
   our code. Moving to per-target instrumentation would speed up the
   coverage build and remove the need for `--gcov-ignore-errors=all`.

2. **ASan and TSan CI jobs.** Would catch use-after-free and data races
   in the plugin tests, which the current suite can't detect.

3. **Fuzz testing the state deserialiser.** `setStateInformation` parses
   arbitrary bytes from a project file. A fuzzer would find crashes that
   the current "feed it valid data and check it round-trips" test can't.

4. **Benchmark suite.** Catch2 has built-in benchmarking. Useful for
   catching performance regressions in the voice loop, which currently
   has no automated performance check.




