cmake_minimum_required(VERSION 3.24)

project(SnairCreator VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(SnairCreator
    PRODUCT_NAME "SnairCreator"
    COMPANY_NAME "Circuit Drift Labs"
    VERSION "${PROJECT_VERSION}"
)

target_sources(SnairCreator PRIVATE
    native/Source/Main.cpp
    native/Source/MainComponent.cpp
)

target_include_directories(SnairCreator PRIVATE native/Source)

target_compile_features(SnairCreator PRIVATE cxx_std_20)

target_compile_definitions(SnairCreator PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(SnairCreator PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
