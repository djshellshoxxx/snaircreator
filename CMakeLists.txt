cmake_minimum_required(VERSION 3.24)
project(SnairCreator VERSION 0.0.1 LANGUAGES C CXX)

option(SNAIRCREATOR_BUILD_TESTS "Build SnairCreator unit tests" ON)
option(SNAIRCREATOR_BUILD_CLAP "Build the CLAP plug-in (clap-juce-extensions)" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS")
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.3
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(SNAIRCREATOR_BUILD_CLAP)
    FetchContent_Declare(clap_juce_extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        # main @ 2026-10-06: first revision with JUCE 9 support (0.26.0 predates it)
        GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d
        GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_MakeAvailable(clap_juce_extensions)
endif()

set(SNAIR_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND SNAIR_FORMATS AU)
endif()

juce_add_plugin(SnairCreator
    PRODUCT_NAME "SnairCreator"
    COMPANY_NAME "Circuit Drift Labs"
    COMPANY_WEBSITE "https://djshellshoxxx.github.io/circuitdriftlabs/"
    BUNDLE_ID "com.circuitdriftlabs.snaircreator"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_MANUFACTURER_CODE Cdlb
    PLUGIN_CODE Snr1
    FORMATS ${SNAIR_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(SnairCreator)

set(SNAIR_SOURCES
    native/Source/ClapRenderer.cpp
    native/Source/FactoryPresets.cpp
    native/Source/HitPlayer.cpp
    native/Source/LayerExtraction.cpp
    native/Source/PluginEditor.cpp
    native/Source/PluginProcessor.cpp
    native/Source/PresetManager.cpp
    native/Source/SnairEngine.cpp
    native/Source/SnareRenderer.cpp
    native/Source/SourceAnalyzer.cpp
    native/Source/SourceLoader.cpp
    native/Source/WavExporter.cpp
    native/Source/WaveformView.cpp)
target_sources(SnairCreator PRIVATE ${SNAIR_SOURCES})
set(SNAIR_DEFINITIONS
    SNAIR_VERSION_STRING="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_include_directories(SnairCreator PRIVATE native/Source)
target_compile_definitions(SnairCreator PUBLIC ${SNAIR_DEFINITIONS})
target_link_libraries(SnairCreator
    PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(SNAIRCREATOR_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET SnairCreator
        CLAP_ID "com.circuitdriftlabs.snaircreator"
        CLAP_FEATURES instrument drum-machine sampler stereo)
endif()

if(SNAIRCREATOR_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(SnairCreatorTests PRODUCT_NAME "SnairCreatorTests")
    juce_generate_juce_header(SnairCreatorTests)
    target_sources(SnairCreatorTests PRIVATE native/tests/TestMain.cpp native/tests/EngineTests.cpp ${SNAIR_SOURCES})
    target_include_directories(SnairCreatorTests PRIVATE native/Source)
    target_compile_definitions(SnairCreatorTests PRIVATE ${SNAIR_DEFINITIONS} JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(SnairCreatorTests PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME snaircreator_tests COMMAND SnairCreatorTests)
endif()
