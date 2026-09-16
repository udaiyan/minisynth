cmake_minimum_required(VERSION 3.22)
project(MiniSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- JUCE (downloaded automatically at configure time) ----
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.2          
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(JUCE)

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
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)