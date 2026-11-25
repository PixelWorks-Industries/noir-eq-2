cmake_minimum_required(VERSION 3.22)
project(NOIR_EQ VERSION 0.2.1)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NOIR_EQ
    COMPANY_NAME "Zain Audio"
    BUNDLE_ID com.zainaudio.noireq
    PLUGIN_MANUFACTURER_CODE Zain
    PLUGIN_CODE N0EQ
    FORMATS VST3 Standalone
    PRODUCT_NAME "NOIR EQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(NOIR_EQ)

juce_add_binary_data(NOIR_EQ_Assets
    SOURCES
        Assets/noir_surface.svg
        Assets/noir_mark.svg
        Assets/noir_plate.svg
        Assets/noir_wordmark.svg
        Assets/noir_meter_ticks.svg)
target_sources(NOIR_EQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)
target_compile_features(NOIR_EQ PRIVATE cxx_std_17)
target_compile_definitions(NOIR_EQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1)
target_link_libraries(NOIR_EQ PRIVATE
    NOIR_EQ_Assets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
