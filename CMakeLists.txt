cmake_minimum_required(VERSION 3.22)
project(SunVoxPlugin VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE Framework
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SunVoxPlugin
    COMPANY_NAME "WarmPlace"
    PLUGIN_MANUFACTURER_CODE WrPl
    PLUGIN_CODE SvPl
    FORMATS Standalone VST3
    PRODUCT_NAME "SunVoxPlugin"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
)

target_sources(SunVoxPlugin
    PRIVATE
        src/audio/SunVoxEngine.hpp
        src/audio/SunVoxEngine.cpp
        src/audio/PluginProcessor.hpp
        src/audio/PluginProcessor.cpp
        src/gui/PluginEditor.hpp
        src/gui/PluginEditor.cpp
        src/gui/ModuleNodeComponent.hpp
        src/gui/ModuleNodeComponent.cpp
        src/gui/GraphCanvas.hpp
        src/gui/GraphCanvas.cpp
        src/gui/InspectorPanel.hpp
        src/gui/InspectorPanel.cpp
)

target_include_directories(SunVoxPlugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib
)

target_link_libraries(SunVoxPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags

        ${CMAKE_DL_LIBS}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(SunVoxPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

juce_generate_juce_header(SunVoxPlugin)

# Copy sunvox.so next to the Standalone executable
if(TARGET SunVoxPlugin_Standalone)
    add_custom_command(TARGET SunVoxPlugin_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so"
            "$<TARGET_FILE_DIR:SunVoxPlugin_Standalone>/sunvox.so"
        COMMENT "Copying sunvox.so next to Standalone executable"
    )
endif()

# Copy sunvox.so into VST3 binary directory
if(TARGET SunVoxPlugin_VST3)
    add_custom_command(TARGET SunVoxPlugin_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so"
            "$<TARGET_FILE_DIR:SunVoxPlugin_VST3>/sunvox.so"
        COMMENT "Copying sunvox.so into VST3 bundle"
    )
endif()
