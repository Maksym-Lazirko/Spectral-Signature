cmake_minimum_required(VERSION 3.24)
project(SpectralSignature VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(JUCE_SOURCE_DIR "" CACHE PATH "Local JUCE 8.0.12 source directory (no network fetch)")
if(NOT EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Set -DJUCE_SOURCE_DIR=<local JUCE 8.0.12 source directory>. See BUILD.md.")
endif()
add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
get_directory_property(JUCE_VERSION DIRECTORY "${JUCE_SOURCE_DIR}" DEFINITION JUCE_VERSION)
if(NOT JUCE_VERSION VERSION_EQUAL "8.0.12")
    message(FATAL_ERROR "This release is validated with JUCE 8.0.12 only; found ${JUCE_VERSION}")
endif()

juce_add_plugin(SpectralSignature
    COMPANY_NAME "Lazirko Records"
    BUNDLE_ID "com.yourcompany.SpectralCarver"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Dc8f
    FORMATS VST3
    PRODUCT_NAME "Spectral Signature"
    DESCRIPTION "Subtle spectral engraving for stereo masters"
    VST3_CATEGORIES Fx Mastering
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_AUTO_MANIFEST TRUE)
juce_generate_juce_header(SpectralSignature)
target_sources(SpectralSignature PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h
    Source/SpectralEngine.cpp Source/SpectralEngine.h Source/RealtimeFFT.h)
target_compile_definitions(SpectralSignature PUBLIC
    _CRT_SECURE_NO_WARNINGS
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
target_link_libraries(SpectralSignature PRIVATE
    juce::juce_audio_processors juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags)
if(MSVC)
    target_compile_options(SpectralSignature PRIVATE /W4 /permissive- /sdl /guard:cf)
    target_link_options(SpectralSignature_VST3 PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA)
else()
    target_compile_options(SpectralSignature PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(SPECTRAL_BUILD_TESTS "Build offline processor and DSP regression tests" ON)
if(SPECTRAL_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(SpectralTests PRODUCT_NAME "Spectral Signature Tests")
    target_sources(SpectralTests PRIVATE Tests/ProcessorTests.cpp)
    target_link_libraries(SpectralTests PRIVATE SpectralSignature)
    target_compile_definitions(SpectralTests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
    target_include_directories(SpectralTests PRIVATE Source "${JUCE_SOURCE_DIR}/modules"
        "$<TARGET_PROPERTY:SpectralSignature,JUCE_GENERATED_SOURCES_DIRECTORY>")
    add_test(NAME SpectralRegression COMMAND SpectralTests)
endif()
