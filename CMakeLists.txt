cmake_minimum_required(VERSION 3.22)

project(KissOfShame VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# Cross-DAW formats. AU + VST3 + Standalone build everywhere out of the box.
# AAX (Pro Tools) additionally requires Avid's SDK (NDA, not redistributable)
# and PACE signing: configure with -DKOS_AAX_SDK_PATH=/path/to/aax-sdk to
# enable the AAX target.
set(KOS_FORMATS AU VST3 Standalone)
if(DEFINED KOS_AAX_SDK_PATH)
    juce_set_aax_sdk_path("${KOS_AAX_SDK_PATH}")
    list(APPEND KOS_FORMATS AAX)
endif()

juce_add_plugin(KissOfShame
    PRODUCT_NAME                "The Kiss Of Shame"
    COMPANY_NAME                "Infernal Love"
    BUNDLE_ID                   com.InfernalLove.KissOfShame
    PLUGIN_MANUFACTURER_CODE    ILov
    PLUGIN_CODE                 Plug
    FORMATS                     ${KOS_FORMATS}
    AAX_CATEGORY                AAX_ePlugInCategory_Harmonic
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Distortion
    AU_MAIN_TYPE                kAudioUnitType_Effect)

juce_generate_juce_header(KissOfShame)

target_sources(KissOfShame PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/AudioProcessing/Granulate.cpp
    Source/GUIUtilities/CustomKnob.cpp
    Source/GUIUtilities/ImageInteractor.cpp)

juce_add_binary_data(KissOfShameAssets
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES
        GUI_Resources/KOS_Graphics/00.png
        GUI_Resources/KOS_Graphics/01.png
        GUI_Resources/KOS_Graphics/02.png
        GUI_Resources/KOS_Graphics/03_alpha.png
        GUI_Resources/KOS_Graphics/04_alpha.png
        GUI_Resources/KOS_Graphics/05_alpha.png
        GUI_Resources/KOS_Graphics/06_alpha.png
        GUI_Resources/KOS_Graphics/07.png
        GUI_Resources/KOS_Graphics/08.png
        GUI_Resources/KOS_Graphics/09_alpha.png
        GUI_Resources/KOS_Graphics/09_v2.png
        GUI_Resources/KOS_Graphics/10.png
        GUI_Resources/KOS_Graphics/11.png
        GUI_Resources/KOS_Graphics/12_alpha.png
        GUI_Resources/KOS_Graphics/fond_alpha.png
        GUI_Resources/KOS_Graphics/fond_alone2.png
        GUI_Resources/KOS_Graphics/link.png
        GUI_Resources/KOS_Graphics/wheels.png
        Audio_Resources/Hiss.wav
        Audio_Resources/PinkNoise.wav
        Audio_Resources/LowLevelGrainNoise.wav)

target_compile_definitions(KissOfShame PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(KissOfShame
    PRIVATE
        KissOfShameAssets
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags)

#===============================================================================
# Offline smoke test: boots the processor and proves audio passes through
# every environment without going silent or non-finite.
juce_add_console_app(KissOfShameSmokeTest PRODUCT_NAME "KissOfShameSmokeTest")

target_sources(KissOfShameSmokeTest PRIVATE Tests/SmokeTest.cpp)

# The GUI regression tests pump the message loop to flush async parameter
# attachments.
target_compile_definitions(KissOfShameSmokeTest PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)

target_include_directories(KissOfShameSmokeTest PRIVATE
    Source
    $<TARGET_PROPERTY:KissOfShame,JUCE_GENERATED_SOURCES_DIRECTORY>)

target_link_libraries(KissOfShameSmokeTest PRIVATE
    KissOfShame
    KissOfShameAssets
    juce::juce_audio_utils
    juce::juce_gui_extra)

enable_testing()
add_test(NAME SmokeTest COMMAND KissOfShameSmokeTest)

#===============================================================================
# Headless editor screenshot tool (docs / visual review):
#   KissOfShameSnapshot <outputDir> [--extreme]
juce_add_console_app(KissOfShameSnapshot PRODUCT_NAME "KissOfShameSnapshot")

target_sources(KissOfShameSnapshot PRIVATE Tests/SnapshotTool.cpp)

target_compile_definitions(KissOfShameSnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)

target_include_directories(KissOfShameSnapshot PRIVATE
    Source
    $<TARGET_PROPERTY:KissOfShame,JUCE_GENERATED_SOURCES_DIRECTORY>)

target_link_libraries(KissOfShameSnapshot PRIVATE
    KissOfShame
    KissOfShameAssets
    juce::juce_audio_utils
    juce::juce_gui_extra)
