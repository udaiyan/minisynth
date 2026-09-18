# Roadmap

Planned work for MiniSynth, tracked in Jira-style tickets.

This repository is a solo project, so the board is documented here rather
than in a live tool. The structure mirrors how the work would be tracked
if a squad were working on it: small tickets, explicit priorities, and
a clear split between what's shipped, what's in flight, and what's
deliberately deferred.

**Ticket prefixes:**

- `MS-nnn` — MiniSynth project
- `TEST-nnn` — test infrastructure
- `CI-nnn` — build and pipeline

**Status:** `Done` · `In Progress` · `Next` · `Backlog` · `Won't Do`

---

## 1. Completed

### Engine

| ID | Title | Status |
|---|---|---|
| MS-001 | Polyphonic voice engine, 8 voices, round-robin stealing | Done |
| MS-002 | Sine, saw, and square oscillators | Done |
| MS-003 | ADSR amplitude envelope | Done |
| MS-004 | State-variable filter with LP/BP/HP modes | Done |
| MS-005 | MIDI note on/off with sample-accurate timing | Done |
| MS-006 | Separation of `src/dsp` from JUCE | Done |

### Plugin

| ID | Title | Status |
|---|---|---|
| MS-010 | APVTS parameter layout: gain, cutoff, resonance, filter mode | Done |
| MS-011 | Parameter caching in `processBlock` (avoid string lookups) | Done |
| MS-012 | State save/restore via XML | Done |
| MS-013 | Editor: three knobs, filter-mode dropdown, on-screen keyboard | Done |
| MS-014 | Keyboard input via `MidiKeyboardState` | Done |

### Test infrastructure

| ID | Title | Status |
|---|---|---|
| TEST-001 | Catch2 + CTest integration via `catch_discover_tests` | Done |
| TEST-002 | Unit suite for `src/dsp` (5 classes) | Done |
| TEST-003 | Plugin-level suite: parameters, state, buses, buffers | Done |
| TEST-004 | Editor component suite: layout, bindings, dropdown, keyboard | Done |
| TEST-005 | `JuceFixture` pattern to keep discovery from hanging | Done |

### CI

| ID | Title | Status |
|---|---|---|
| CI-001 | GitHub Actions matrix: Windows + Linux | Done |
| CI-002 | JUCE and Catch2 dependency caching | Done |
| CI-003 | JUnit XML test results as run artifacts | Done |
| CI-004 | Coverage job with gcovr, published to Codecov | Done |
| CI-005 | Headless Linux test execution via `xvfb` | Done |

---

## 2. In flight

Work started but not finished. Each has a note explaining what's
remaining.

| ID | Title | Priority | Remaining |
|---|---|---|---|
| MS-020 | Parameter smoothing on cutoff and resonance | High | Per-block interpolation; ~50 lines in `SynthVoice` + a test that asserts no discontinuity on a fast sweep |
| TEST-010 | Per-target coverage instrumentation | Medium | Move `--coverage` from global CMake flags to `target_compile_options` on `minisynth_dsp`, `minisynth_tests`, `minisynth_plugin_tests` |

**Why parameter smoothing is High.** It's the one limitation a user
notices immediately. Zipper noise on a fast cutoff sweep is audible,
and it's the kind of detail that separates a demo from something you'd
ship.

---

## 3. Next

Committed to, not started. These are the tickets a squad would pick up
in the next sprint.

### Engine

| ID | Title | Estimate |
|---|---|---|
| MS-030 | Band-limited oscillators (PolyBLEP) | M |
| MS-031 | Voice-stealing crossfade | S |
| MS-032 | Per-voice filter envelope | M |
| MS-033 | Pitch bend and mod wheel | S |

### Test

| ID | Title | Estimate |
|---|---|---|
| TEST-020 | ASan CI job | S |
| TEST-021 | TSan CI job | M |
| TEST-022 | Fuzz test for `setStateInformation` | M |

### CI

| ID | Title | Estimate |
|---|---|---|
| CI-010 | Build-time caching for CI | S |
| CI-011 | PR comment with coverage delta | S |

**Estimates:** S = half a day, M = 1–2 days.

**Why band-limiting is next.** The saw and square oscillators alias at
high pitches. It's a known limitation, documented in the README, and
it's the largest remaining correctness gap. PolyBLEP is well understood
and testable — you can assert that high harmonics don't fold back into
the audible range.

---

## 4. Backlog

Not scheduled. Listed so they're not forgotten.

### Engine

- MS-040 — Second oscillator + detune
- MS-041 — LFO with assignable destination
- MS-042 — Sub-oscillator
- MS-043 — Arpeggiator
- MS-044 — Pitch envelope

### Plugin

- MS-050 — Preset browser
- MS-051 — A/B compare
- MS-052 — MIDI learn for parameter mapping
- MS-053 — Standalone audio device settings panel
- MS-054 — Undo/redo for editor changes

### Test

- TEST-030 — Benchmark suite for the voice loop
- TEST-031 — Golden-audio regression on a small, stable render
- TEST-032 — Property-based tests for the envelope state machine

### CI

- CI-020 — CodeQL static analysis
- CI-021 — clang-format check on PRs
- CI-022 — Dependency update PRs via Dependabot

---

## 5. Deliberately not planned

Explicit non-goals. Documented so they're not relitigated.

### macOS CI

**Not planned.** Adding a macOS runner is a one-line matrix change, but
the build has never been validated on macOS and debugging a platform
we don't use isn't a good use of time. If macOS support becomes a
requirement, the first ticket would be a manual build verification
before touching CI.

### External UI automation (pywinauto, FlaUI)

**Not planned.** See `docs/TESTING.md` §6 for the reasoning. The
short version: it can only see the accessibility tree, only works for
the Standalone app, and adds a flakiness surface that isn't justified
by the bugs it would catch.

### Golden-image visual regression

**Not planned.** Platform-dependent rendering makes this a maintenance
burden. See `docs/TESTING.md` §6.

### VST2 support

**Won't do.** Steinberg has deprecated VST2 and no longer issues
licences. VST3 only.

### 32-bit builds

**Won't do.** No user need, and it doubles the CI matrix.

---

## 6. How this maps to a squad

If this were a team rather than a solo project, the tickets above would
break into sprints as follows.

### Sprint 1 — Foundations

The work that's already done. In a squad context this would be:

- **One engineer** on the DSP library and the JUCE/JUCE-free split
- **One engineer** on the plugin shell and editor
- **One engineer** on test infrastructure and CI, working in parallel

The dependency is that DSP interfaces need to be agreed before the
plugin work starts. That's a two-day spike, not a sprint.

### Sprint 2 — Hardening

- **Two engineers** on the test suites (unit and integration/component)
- **One engineer** on CI: matrix, caching, coverage
- **One engineer** on documentation

### Sprint 3 — Quality gaps

The "Next" section above. Band-limiting is the largest piece and could
absorb two engineers if split into "implement PolyBLEP" and "write
tests for aliasing behaviour".

### Ongoing

- CI maintenance
- Dependency updates
- Bug triage

### Where the squad boundaries are

Three natural ownership areas:

**Engine** (`src/dsp/`) — DSP correctness, voice allocation, envelope
behaviour.

**Plugin** (`src/plugin/`) — host integration, parameters, editor,
state.

**Infrastructure** (`tests/`, `.github/`, `docs/`) — framework,
CI, coverage, onboarding.

The split is deliberate: the engine can be developed and tested
independently of the plugin, so the two can move in parallel without
blocking each other. The infrastructure area is a shared service — it
serves the other two rather than building features of its own.

---

## 7. How tickets are written

For consistency, tickets in this project follow a fixed shape.

**Title:** imperative, specific. "Add parameter smoothing" not
"Parameter smoothing improvements".

**Description:** what changes, and why. One paragraph.

**Acceptance criteria:** a list of checkable statements. Each one
becomes a test if it's testable.

**Estimate:** S / M / L for anything in "Next". Backlog items aren't
estimated — estimating them would imply a commitment.

**Definition of done:**

- Merged to `main`
- Tests pass on Windows and Linux CI
- Coverage doesn't decrease
- If it changes a documented behaviour, the docs are updated
- If it's AI-generated test code, the commit says so (see
  `docs/AI_ASSISTED_TESTS.md` §9)

