cmake_minimum_required(VERSION 3.22)

project(PartePlay VERSION 0.1.0 LANGUAGES C CXX)

set(JUCE_ROOT "D:/Program Files/JUCE" CACHE PATH
    "Path to the JUCE framework source")

add_subdirectory("${JUCE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)

juce_add_plugin(PartePlay
    VERSION "0.1.0"
    COMPANY_NAME "PartePlay"
    PRODUCT_NAME "PartePlay"
    PLUGIN_NAME "PartePlay"
    PLUGIN_MANUFACTURER_CODE "PRPL"
    PLUGIN_CODE "PTPY"
    DESCRIPTION "Reproducao sincronizada ao transport + transposicao para instrumentos transpositores"
    FORMATS VST3
    VST3_CATEGORIES "Fx|PitchShift"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_COPY_DIR "C:/Program Files/Common Files/VST3"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png"
)

target_sources(PartePlay PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FilePlayer.cpp
    Source/TempoAnalyser.cpp
)

target_link_libraries(PartePlay PRIVATE juce::juce_audio_formats)

juce_add_binary_data(BinaryData SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png")
target_link_libraries(PartePlay PRIVATE BinaryData)

target_include_directories(PartePlay PRIVATE Source)

target_compile_definitions(PartePlay PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    target_compile_options(PartePlay PRIVATE /utf-8)
endif()

juce_generate_juce_header(PartePlay)