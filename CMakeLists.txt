cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) on macOS
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

project(GrooveX VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: use a local checkout if provided (-DJUCE_DIR=...), otherwise download it.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(SG_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND SG_FORMATS AU)
endif()

juce_add_plugin(GrooveX
    COMPANY_NAME "Tune Talks"
    BUNDLE_ID "com.tunetalks.groovex"
    PLUGIN_MANUFACTURER_CODE Tntk
    PLUGIN_CODE GrvX
    FORMATS ${SG_FORMATS}
    PRODUCT_NAME "Groove-X"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Drum
    DESCRIPTION "Drum pattern generator using your Splice samples")

target_sources(GrooveX PRIVATE
    Source/GenreEngine.cpp
    Source/SampleLibrary.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(GrooveX PRIVATE Source)

target_compile_definitions(GrooveX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# Optional: lets the Linux CI/dev build skip X11 extension headers
if(SG_MINIMAL_LINUX)
    target_compile_definitions(GrooveX PUBLIC
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JUCE_USE_XRENDER=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_link_libraries(GrooveX
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Developer test harness (not part of the plugin): cmake -DSG_BUILD_TESTS=ON
if(SG_BUILD_TESTS)
    juce_add_console_app(GrooveXTests PRODUCT_NAME "GrooveXTests")
    target_sources(GrooveXTests PRIVATE
        Tests/TestMain.cpp
        Source/GenreEngine.cpp Source/SampleLibrary.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(GrooveXTests PRIVATE Source)
    target_compile_definitions(GrooveXTests PRIVATE
        JucePlugin_Name="Groove-X" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)
    if(SG_MINIMAL_LINUX)
        target_compile_definitions(GrooveXTests PRIVATE
            JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
            JUCE_USE_XRENDER=0 JUCE_ALSA=0 JUCE_JACK=0)
    endif()
    target_link_libraries(GrooveXTests PRIVATE juce::juce_audio_utils juce::juce_audio_formats
        PUBLIC juce::juce_recommended_config_flags)
endif()
