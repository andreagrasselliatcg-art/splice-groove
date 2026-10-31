cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) on macOS
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

project(SpliceGroove VERSION 1.0.0 LANGUAGES C CXX)

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

juce_add_plugin(SpliceGroove
    COMPANY_NAME "Andrea Grasselli"
    BUNDLE_ID "com.andreagrasselli.splicegroove"
    PLUGIN_MANUFACTURER_CODE Agrs
    PLUGIN_CODE SpGv
    FORMATS ${SG_FORMATS}
    PRODUCT_NAME "Splice Groove"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Drum
    DESCRIPTION "Drum pattern generator using your Splice samples")

target_sources(SpliceGroove PRIVATE
    Source/GenreEngine.cpp
    Source/SampleLibrary.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(SpliceGroove PRIVATE Source)

target_compile_definitions(SpliceGroove PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# Optional: lets the Linux CI/dev build skip X11 extension headers
if(SG_MINIMAL_LINUX)
    target_compile_definitions(SpliceGroove PUBLIC
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JUCE_USE_XRENDER=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_link_libraries(SpliceGroove
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Developer test harness (not part of the plugin): cmake -DSG_BUILD_TESTS=ON
if(SG_BUILD_TESTS)
    juce_add_console_app(SpliceGrooveTests PRODUCT_NAME "SpliceGrooveTests")
    target_sources(SpliceGrooveTests PRIVATE
        Tests/TestMain.cpp
        Source/GenreEngine.cpp Source/SampleLibrary.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(SpliceGrooveTests PRIVATE Source)
    target_compile_definitions(SpliceGrooveTests PRIVATE
        JucePlugin_Name="Splice Groove" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)
    if(SG_MINIMAL_LINUX)
        target_compile_definitions(SpliceGrooveTests PRIVATE
            JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
            JUCE_USE_XRENDER=0 JUCE_ALSA=0 JUCE_JACK=0)
    endif()
    target_link_libraries(SpliceGrooveTests PRIVATE juce::juce_audio_utils juce::juce_audio_formats
        PUBLIC juce::juce_recommended_config_flags)
endif()
