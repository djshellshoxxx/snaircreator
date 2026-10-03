cmake_minimum_required(VERSION 3.22)
project(SnairCreator VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(FetchContent)

FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
set(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(clap_juce_extensions GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git GIT_TAG main GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(clap_juce_extensions)

set(SNAIR_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND SNAIR_FORMATS AU)
endif()

juce_add_plugin(SnairCreator
  COMPANY_NAME "Circuit Drift Labs"
  BUNDLE_ID "com.circuitdriftlabs.snaircreator"
  PLUGIN_MANUFACTURER_CODE Cdlb
  PLUGIN_CODE SnCr
  FORMATS ${SNAIR_FORMATS}
  PRODUCT_NAME "SnairCreator"
  DESCRIPTION "Source-driven snare and clap generator"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(SnairCreator)
target_sources(SnairCreator PRIVATE
  Source/SnairEngine.cpp Source/SnairEngine.h
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/PluginEditor.cpp Source/PluginEditor.h)
target_compile_definitions(SnairCreator PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SnairCreator PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

clap_juce_extensions_plugin(TARGET SnairCreator
  CLAP_ID "com.circuitdriftlabs.snaircreator"
  CLAP_FEATURES instrument drum)

enable_testing()
juce_add_console_app(SnairCreatorTests PRODUCT_NAME "SnairCreatorTests")
juce_generate_juce_header(SnairCreatorTests)
target_sources(SnairCreatorTests PRIVATE tests/SnairEngineTests.cpp Source/SnairEngine.cpp Source/SnairEngine.h)
target_include_directories(SnairCreatorTests PRIVATE Source)
target_compile_definitions(SnairCreatorTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SnairCreatorTests PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_test(NAME SnairEngine COMMAND SnairCreatorTests)
