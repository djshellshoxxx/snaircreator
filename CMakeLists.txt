cmake_minimum_required(VERSION 3.24)
project(SnairCreator VERSION 1.0.0 LANGUAGES C CXX)
option(SNAIRCREATOR_BUILD_TESTS "Build SnairCreator unit tests" ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 9.0.3 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(SNAIR_CORE
    native/Source/AdvancedPanel.cpp
    native/Source/FactoryPresets.cpp
    native/Source/LayerExtraction.cpp
    native/Source/SnareRenderer.cpp
    native/Source/ClapRenderer.cpp
    native/Source/SourceLoader.cpp
    native/Source/SourceAnalyzer.cpp
    native/Source/LayerExtraction.cpp
    native/Source/SnareRenderer.cpp
    native/Source/ClapRenderer.cpp
    native/Source/SnairEngine.cpp
    native/Source/WavExporter.cpp
    native/Source/PresetManager.cpp
    native/Source/SessionStore.cpp
    native/Source/WaveformView.cpp
)
juce_add_gui_app(SnairCreator PRODUCT_NAME "SnairCreator" COMPANY_NAME "Circuit Drift Labs" BUNDLE_ID "com.circuitdriftlabs.snaircreator" VERSION "${PROJECT_VERSION}")
juce_generate_juce_header(SnairCreator)
target_sources(SnairCreator PRIVATE native/Source/Main.cpp native/Source/MainComponent.cpp ${SNAIR_CORE})
target_include_directories(SnairCreator PRIVATE native/Source)
target_compile_features(SnairCreator PRIVATE cxx_std_20)
target_compile_definitions(SnairCreator PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SnairCreator PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

if(SNAIRCREATOR_BUILD_TESTS)
  enable_testing()
  juce_add_console_app(SnairCreatorTests PRODUCT_NAME "SnairCreatorTests")
  juce_generate_juce_header(SnairCreatorTests)
  target_sources(SnairCreatorTests PRIVATE
    native/tests/TestMain.cpp
    native/tests/EngineTests.cpp
    native/Source/SourceAnalyzer.cpp
    native/Source/SnairEngine.cpp
    native/Source/WavExporter.cpp
    native/Source/PresetManager.cpp
    native/Source/SessionStore.cpp
  )
  target_include_directories(SnairCreatorTests PRIVATE native/Source)
  target_compile_features(SnairCreatorTests PRIVATE cxx_std_20)
  target_compile_definitions(SnairCreatorTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(SnairCreatorTests PRIVATE juce::juce_audio_formats juce::juce_dsp juce::juce_core juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
  add_test(NAME snaircreator_engine COMMAND SnairCreatorTests)
endif()
