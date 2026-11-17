cmake_minimum_required(VERSION 3.22)
project(BOOM VERSION 0.5.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BOOM_DSP_ONLY "Build DSP validation tests without JUCE" OFF)

if(NOT BOOM_DSP_ONLY)

# If JUCE is installed locally: cmake -B build -DJUCE_SOURCE_DIR=/path/to/JUCE
if(DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
 add_subdirectory("${JUCE_SOURCE_DIR}" juce)
else()
 include(FetchContent)
 FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
 FetchContent_MakeAvailable(juce)
endif()
if(APPLE)
 set(BOOM_FORMATS AU VST3 Standalone)
elseif(WIN32)
 set(BOOM_FORMATS VST3 Standalone)
else()
 set(BOOM_FORMATS VST3 Standalone)
endif()

juce_add_binary_data(BOOMArtwork
 SOURCES
  Assets/boom-console-reference.jpg
  Assets/boom-browser-reference.jpg)

juce_add_plugin(BOOM
 COMPANY_NAME "Liberty Music Software"
 BUNDLE_ID "com.libertymusicsoftware.boom"
 PLUGIN_MANUFACTURER_CODE LibM
 PLUGIN_CODE Boom
 PRODUCT_NAME "BOOM"
 DESCRIPTION "Professional Analog Mastering Console"
 IS_SYNTH FALSE
 NEEDS_MIDI_INPUT FALSE
 NEEDS_MIDI_OUTPUT FALSE
 IS_MIDI_EFFECT FALSE
 EDITOR_WANTS_KEYBOARD_FOCUS TRUE
 COPY_PLUGIN_AFTER_BUILD FALSE
 FORMATS ${BOOM_FORMATS})

juce_generate_juce_header(BOOM)
target_sources(BOOM PRIVATE
 Source/PluginProcessor.h Source/PluginProcessor.cpp
 Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginEditorPaint.cpp
 Source/PluginEditorPanels.cpp Source/PluginEditorMenus.cpp Source/BOOMUIMap.h
 Source/BoomDSP.h Source/LoudnessMeter.h Source/OversampledTube.h Source/FactoryLibrary.h)
target_compile_definitions(BOOM PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BOOM PRIVATE
 BOOMArtwork
 juce::juce_audio_utils
 juce::juce_gui_extra
 juce::juce_dsp
 juce::juce_recommended_config_flags
 juce::juce_recommended_warning_flags
 juce::juce_recommended_lto_flags)

endif() # BOOM_DSP_ONLY

include(CTest)
if(BUILD_TESTING)
 add_executable(boom_ui_map_validation Tests/ui_map_validation.cpp)
 add_test(NAME BOOM_UI_MAP_VALIDATION COMMAND boom_ui_map_validation)
 add_executable(boom_ui_dsp_contract Tests/ui_dsp_contract.cpp)
 add_test(NAME BOOM_UI_DSP_CONTRACT COMMAND boom_ui_dsp_contract)
 add_executable(boom_dsp_smoke Tests/dsp_smoke.cpp)
 add_executable(boom_library_smoke Tests/library_smoke.cpp)
 add_test(NAME BOOM_DSP COMMAND boom_dsp_smoke)
 add_test(NAME BOOM_FACTORY_LIBRARY COMMAND boom_library_smoke)
 add_executable(boom_mastering_validation Tests/mastering_validation.cpp)
 add_test(NAME BOOM_MASTERING_VALIDATION COMMAND boom_mastering_validation)
 add_executable(boom_processor_regression Tests/processor_regression.cpp)
 add_test(NAME BOOM_PROCESSOR_REGRESSION COMMAND boom_processor_regression)
 add_executable(boom_v04_dsp_regression Tests/v04_dsp_regression.cpp)
 add_test(NAME BOOM_V04_DSP_REGRESSION COMMAND boom_v04_dsp_regression)
 add_executable(boom_v05_oversampling Tests/v05_oversampling.cpp)
 add_test(NAME BOOM_V05_OVERSAMPLING COMMAND boom_v05_oversampling)
 add_executable(boom_v05_loudness_reference Tests/v05_loudness_reference.cpp)
 add_test(NAME BOOM_V05_LOUDNESS_REFERENCE COMMAND boom_v05_loudness_reference)
 add_executable(boom_v05_true_peak_probe Tests/v05_true_peak_probe.cpp)
endif()
