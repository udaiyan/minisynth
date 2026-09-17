cmake_minimum_required(VERSION 3.22)
project(MiniSynth VERSION 0.1.0 LANGUAGES C CXX)

# ---- Optional coverage instrumentation ----
option(MINISYNTH_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(MINISYNTH_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -g -O0)
    add_link_options(--coverage)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- JUCE (downloaded automatically at configure time) ----
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.2          
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(JUCE)

# ---- Test framework ----
include(CTest)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# ---- DSP library ----
# Pure C++, no JUCE. This is what makes it unit-testable.
add_library(minisynth_dsp STATIC
    src/dsp/AdsrEnvelope.cpp
    src/dsp/Oscillator.cpp
    src/dsp/StateVariableFilter.cpp
    src/dsp/SynthEngine.cpp
    src/dsp/SynthVoice.cpp)

target_include_directories(minisynth_dsp PUBLIC src)

target_compile_features(minisynth_dsp PUBLIC cxx_std_17)

# ---- The plugin ----
juce_add_plugin(MiniSynth
    COMPANY_NAME              "Darren U"
    PLUGIN_MANUFACTURER_CODE  Dazz      # 4 chars, first must be uppercase
    PLUGIN_CODE               Mnsy      # 4 chars, must be unique
    FORMATS                   Standalone VST3
    PRODUCT_NAME              "MiniSynth"
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD   FALSE)

target_sources(MiniSynth PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp)

target_include_directories(MiniSynth PRIVATE src)

target_compile_definitions(MiniSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MiniSynth
    PRIVATE
        minisynth_dsp  
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# ---- Tests ----
add_executable(minisynth_tests
    tests/unit/test_engine.cpp
    tests/unit/test_envelope.cpp
    tests/unit/test_filter.cpp
    tests/unit/test_oscillator.cpp
    tests/unit/test_voice.cpp)

target_link_libraries(minisynth_tests
    PRIVATE
        minisynth_dsp
        Catch2::Catch2WithMain)

catch_discover_tests(minisynth_tests)

# ---- Plugin-level tests (these use JUCE) -----
juce_add_console_app(minisynth_plugin_tests
    PRODUCT_NAME "minisynth_plugin_tests")

target_sources(minisynth_plugin_tests PRIVATE
    tests/plugin/test_editor.cpp
    tests/plugin/test_plugin.cpp
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp)

target_compile_definitions(minisynth_plugin_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(minisynth_plugin_tests PRIVATE
    minisynth_dsp
    juce::juce_audio_utils
    juce::juce_dsp
    Catch2::Catch2WithMain
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

catch_discover_tests(minisynth_plugin_tests)