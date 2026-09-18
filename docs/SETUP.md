# Setup

How to get this repository building and testing on a fresh machine.

Every problem documented here is one that actually came up during
development. Where a fix is a workaround rather than a solution, that's
stated.

---

## 1. Requirements

| Requirement | Minimum | Notes |
|---|---|---|
| OS | Windows 10/11, or Linux | macOS should work but is untested |
| Compiler | MSVC 19.4x (VS 2022+), or GCC 11+ / Clang 14+ | C++17 required |
| CMake | 3.22 | Needed for `FetchContent` features |
| Git | Any recent version | For `FetchContent` cloning |
| Python | 3.8+ | Only for the coverage job (gcovr) |

JUCE and Catch2 are downloaded automatically by CMake at configure time.
Nothing is installed system-wide.

---

## 2. Windows setup

### 2.1 Visual Studio workload

Open the **Visual Studio Installer**, find your installation, click
**Modify**, and go to the **Individual components** tab. Verify these are
ticked:

- **MSVC v145 - VS 2026 C++ x64/x86 build tools (Latest)**
- **Windows 11 SDK** (any version — 10.0.26100 or newer)
- **C++ CMake tools for Windows**

The **Windows SDK is not optional**. It provides `rc.exe` and `mt.exe`,
which CMake's linker invocation needs. If it's missing, the build fails
with `CMAKE_MT-NOTFOUND` or "cannot open file manifest.rc" during the
compiler test — an error that looks like a broken compiler but isn't.

### 2.2 Verify the toolchain

Open a **Developer PowerShell for VS** (Start menu → search "Developer
PowerShell"). Run:

    cmake --version
    where.exe rc
    where.exe mt

The CMake version must be **3.22 or higher**. The `where.exe` calls
should return paths under `C:\Program Files (x86)\Windows Kits\10\bin\`.

If `where.exe rc` reports "Could not find files", the Windows SDK is
either not installed or not on the PATH. Reinstall the SDK component
from §2.1.

### 2.3 Build

From a Developer PowerShell:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel

The first configure downloads JUCE and Catch2 (about 1 GB of git clone)
and takes 5–15 minutes. Subsequent configures are instant.

### 2.4 Test

    ctest --test-dir build -C Release --output-on-failure

The `-C Release` flag selects the configuration on multi-config
generators (Visual Studio). On single-config generators (Ninja, Unix
Makefiles) it's ignored.

### 2.5 Run the standalone app

The Standalone build lands at:

    build/MiniSynth_artefacts/Release/Standalone/MiniSynth.exe

Or in VS: select **MiniSynth_Standalone.exe** in the toolbar's startup
item dropdown and press F5.

---

## 3. Linux setup

Install the build dependencies:

    sudo apt-get update
    sudo apt-get install -y \
      libasound2-dev \
      libjack-jackd2-dev \
      libxi-dev \
      libx11-dev libxcomposite-dev libxcursor-dev \
      libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
      libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev

**`libxi-dev` is the one that's usually missing.** It provides
`X11/extensions/XInput2.h`, which JUCE's `juce_gui_basics` includes. The
error is `fatal error: X11/extensions/XInput2.h: No such file or
directory`, and it appears partway through the build — after the DSP
library has already compiled successfully, which makes it look like a
JUCE problem rather than a missing package.

For headless testing (CI or a server without a display), also install:

    sudo apt-get install -y xvfb

Then run tests via `xvfb-run`:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    xvfb-run --auto-servernum ctest --test-dir build --output-on-failure

    ### Why xvfb is needed

The editor component tests construct a `juce::AudioProcessorEditor`,
which on Linux creates a real X11 window. There's no display on a CI
runner, so the tests would fail or hang. `xvfb-run` starts a virtual
framebuffer — an X server that exists only in memory. Window creation
succeeds, nothing is drawn anywhere, and the tests run normally.

`--auto-servernum` picks a free display number so parallel CI jobs
don't collide.

**If you're on a desktop Linux machine**, you don't need xvfb — just
run `ctest` directly.

---

## 4. CMake presets

The repository doesn't currently ship a `CMakePresets.json`. Visual
Studio uses its own default preset when opening the folder (the
configuration is `x64-Debug` by default and can be changed in the
toolbar dropdown).

The command-line examples in this document use `cmake -B build`
directly, which works without a preset.

### Why the Visual Studio generator, not Ninja

VS's CMake integration defaults to Ninja, which relies entirely on the
environment PATH to find `rc.exe` and `mt.exe`. In some VS 2026
installations this PATH setup doesn't happen, and the compiler test
fails with a misleading "compiler is not able to compile a simple test
program" error.

The Visual Studio generator drives MSBuild, which locates the Windows
SDK via its own configuration rather than the PATH. Using it removes
the entire class of problem.

The trade-off is that Ninja builds are faster. If your PATH is set up
correctly and you prefer Ninja, add a preset for it — the build itself
doesn't care.

---

## 5. Visual Studio specifics

### Opening the folder

Use **File → Open → Folder**, not **Open Project**. VS reads the
`CMakeLists.txt` and generates its own project structure. There is no
`.sln` file to open.

### First configure

The Output window (View → Output, dropdown set to "CMake") shows the
configure progress. You'll see JUCE and Catch2 being downloaded on the
first run. Wait for "CMake generation finished".

### Test Explorer

VS shows the tests under Test Explorer. If tests that should exist are
missing:

1. **Rebuild.** `catch_discover_tests` registers tests with CTest at
   build time. If the binary hasn't been rebuilt, discovery is stale.
2. **Refresh Test Explorer.** There's a refresh icon in its toolbar.
3. **Close and reopen Visual Studio.** VS caches the test list in `.vs/`,
   and a full close/reopen forces a rediscovery.
4. **Delete the cache:**

       rmdir /s /q .vs

   VS regenerates it on next open. This is safe — `.vs/` contains only
   per-user state.

   ### A test that has been renamed

If you rename a `TEST_CASE`, Test Explorer may still be filtering for
the old name, and running that test produces:

    No test cases matched '"old test name"'
    No tests ran

The binary is correct; VS is passing a stale filter. Close and reopen
Visual Studio, or run from the command line:

    ctest --test-dir build -C Release --output-on-failure

Which reads the test list from the freshly-built binary every time.

---

## 6. Common build failures

### `CMAKE_MT-NOTFOUND` or `rc.exe not found`

The Windows SDK isn't installed or isn't on the PATH. See §2.1.

### `unresolved external symbol createPluginFilter`

JUCE's plugin wrappers call `createPluginFilter()` to instantiate your
processor. It must be defined in the global namespace in
`PluginProcessor.cpp`:

    juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
    {
        return new MiniSynthProcessor();
    }

Not inside the class, not inside a namespace. It's easy to move it into
a namespace by accident during refactoring, and the error only appears
at link time.

### Test discovery hangs with no output

A static initialiser is running before `main()`. In this codebase the
usual culprit is `juce::ScopedJuceInitialiser_GUI` at namespace scope in
`tests/plugin/`.

**Fix:** put it in a fixture struct and use `TEST_CASE_METHOD`:

    struct JuceFixture
    {
        juce::ScopedJuceInitialiser_GUI juceInit;
    };

    TEST_CASE_METHOD (JuceFixture, "name", "[tag]") { ... }

Static initialisers run before Catch2 processes `--list-tests`, so
discovery silently fails. This is why `catch_discover_tests` can appear
to do nothing for a JUCE test target.

### CMake error: "Cannot find source file"

`add_library` and `add_executable` validate that their source files
exist at configure time. Create the `.cpp` file first, then add it to
`CMakeLists.txt`, then save. Doing it the other way round fails with a
clear error, but the error appears in the CMake output window rather
than on the file you just edited.

### `X11/extensions/XInput2.h: No such file or directory`

Missing `libxi-dev` on Linux. See §3.

### Build terminated with no error (Linux CI)

Out of memory. JUCE translation units are large, and a Debug build with
coverage instrumentation can peak above 1 GB each. Building with
`--parallel 2` and splitting the build into separate target invocations
keeps the peak within the 7 GB available on a GitHub runner.

If it still fails, add swap:

    sudo fallocate -l 4G /swapfile
    sudo chmod 600 /swapfile
    sudo mkswap /swapfile
    sudo swapon /swapfile

---

## 7. Coverage build

Coverage is measured on Linux only. It's a separate CMake configuration
so the main build stays fast.

    cmake -B build -DCMAKE_BUILD_TYPE=Debug -DMINISYNTH_ENABLE_COVERAGE=ON
    cmake --build build --parallel 2 --target minisynth_tests
    cmake --build build --parallel 2 --target minisynth_plugin_tests
    xvfb-run --auto-servernum ctest --test-dir build --output-on-failure

Then generate the report:

    gcovr --root . \
          --xml-pretty \
          --output coverage.xml \
          --filter 'src/' \
          --exclude '.*_deps.*' \
          --exclude '.*tests.*' \
          --exclude '.*artefacts.*' \
          --gcov-ignore-errors=all \
          --print-summary

Install gcovr if it's missing:

    pip install gcovr

`--gcov-ignore-errors=all` suppresses fatal errors when gcov is run
against JUCE and Catch2 object files whose source paths can't be
resolved. It's noise — the `--filter` and `--exclude` patterns decide
what actually appears in the report.

---

## 8. IDE-independent build

Everything above works from the command line on any platform. The
repository is editor-agnostic — no `.sln`, no `.vcxproj`, no
`.idea/`. Any tool that reads `CMakeLists.txt` will work.

If you're using VS Code, the **CMake Tools** extension is the usual
choice. It reads `CMakePresets.json` and provides build/test commands
in the command palette.

The only IDE-specific guidance in this document is §5, and that's
because Visual Studio's CMake integration has a few behaviours worth
knowing about. Nothing in the build depends on them.

---

## 9. Continuous integration

CI runs on every push and pull request to `main`. Three jobs:

| Job | OS | What it does |
|---|---|---|
| `build-and-test` | Windows | Release build, runs the test suite |
| `build-and-test` | Linux | Release build, runs the test suite under xvfb |
| `coverage` | Linux | Debug + coverage build, uploads to Codecov |

Test results are attached to every run as JUnit XML artifacts. The
coverage report is attached as `coverage.xml`.

If a job fails on Linux but passes locally on Windows, the usual cause
is a missing apt package. See §3.

### Checking CI before pushing

You can run the exact same commands CI runs:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

If those pass locally, the Windows CI job will almost certainly pass.

---

## 10. Known environment quirks

Documented because they cost time to diagnose.

**Visual Studio Test Explorer caches the test list.** Renaming a test
or adding a new test file may not show up until VS is restarted or
`.vs/` is deleted. The command-line `ctest` never has this problem.

**The Windows SDK is a separate component from the C++ workload.**
Installing "Desktop development with C++" doesn't always install it.

**`FetchContent` caches JUCE in `build/_deps/`.** Deleting the build
directory forces a re-download. There's a global CMake cache as well,
but it's not always used.

**First configure is slow.** 5–15 minutes, depending on network and
disk. This is normal and only happens once per build directory.

**Linux link errors usually mean a missing `-dev` package.** The header
name gives you the package: search for it at
https://packages.ubuntu.com and the result names the package.

