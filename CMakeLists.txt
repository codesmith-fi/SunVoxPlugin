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

# CLAP support via clap-juce-extensions
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-juce-extensions/CMakeLists.txt")
    add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)
endif()

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

if(COMMAND clap_juce_extensions_plugin)
    clap_juce_extensions_plugin(TARGET SunVoxPlugin
        CLAP_ID "com.WarmPlace.SunVoxPlugin"
        CLAP_FEATURES instrument synthesizer)
endif()

target_sources(SunVoxPlugin
    PRIVATE
        src/audio/SunVoxEngine.hpp
        src/audio/SunVoxEngine.cpp
        src/audio/PresetManager.hpp
        src/audio/PresetManager.cpp
        src/audio/PluginProcessor.hpp
        src/audio/PluginProcessor.cpp
        src/gui/PluginEditor.hpp
        src/gui/PluginEditor.cpp
        src/gui/PresetBrowserComponent.hpp
        src/gui/PresetBrowserComponent.cpp
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

# RPATH configuration for VST3 and CLAP so they can resolve sunvox.so via $ORIGIN
if(TARGET SunVoxPlugin_VST3)
    set_target_properties(SunVoxPlugin_VST3 PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

if(TARGET SunVoxPlugin_CLAP)
    set_target_properties(SunVoxPlugin_CLAP PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Copy sunvox.so next to the Standalone executable
if(TARGET SunVoxPlugin_Standalone)
    add_custom_command(TARGET SunVoxPlugin_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so"
            "$<TARGET_FILE_DIR:SunVoxPlugin_Standalone>/sunvox.so"
        COMMENT "Copying sunvox.so next to Standalone executable"
    )
endif()

# Copy sunvox.so and assets into VST3 bundle directory
if(TARGET SunVoxPlugin_VST3)
    add_custom_command(TARGET SunVoxPlugin_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so"
            "$<TARGET_FILE_DIR:SunVoxPlugin_VST3>/sunvox.so"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:SunVoxPlugin_VST3>/../Resources/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:SunVoxPlugin_VST3>/../Resources/assets"
        COMMENT "Copying sunvox.so and assets into VST3 bundle"
    )
endif()

# Copy sunvox.so next to CLAP plugin
if(TARGET SunVoxPlugin_CLAP)
    add_custom_command(TARGET SunVoxPlugin_CLAP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so"
            "$<TARGET_FILE_DIR:SunVoxPlugin_CLAP>/sunvox.so"
        COMMENT "Copying sunvox.so next to CLAP plugin"
    )
endif()

# Local installation target to ~/.vst3, ~/.clap, and ~/.local/share/SunVoxPlugin
set(LOCAL_INSTALL_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.vst3"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/SunVoxPlugin_artefacts/$<CONFIG>/VST3/SunVoxPlugin.vst3"
        "$ENV{HOME}/.vst3/SunVoxPlugin.vst3"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.local/share/SunVoxPlugin/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$ENV{HOME}/.local/share/SunVoxPlugin/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so"
        "$ENV{HOME}/.local/share/SunVoxPlugin/sunvox.so"
)
set(LOCAL_INSTALL_DEPS SunVoxPlugin_VST3)

if(TARGET SunVoxPlugin_CLAP)
    list(APPEND LOCAL_INSTALL_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.clap"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:SunVoxPlugin_CLAP>"
            "$ENV{HOME}/.clap/SunVoxPlugin.clap"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so"
            "$ENV{HOME}/.clap/sunvox.so"
    )
    list(APPEND LOCAL_INSTALL_DEPS SunVoxPlugin_CLAP)
endif()

add_custom_target(install_local
    ${LOCAL_INSTALL_COMMANDS}
    DEPENDS ${LOCAL_INSTALL_DEPS}
    COMMENT "Installing VST3, CLAP, and assets to ~/.vst3, ~/.clap, and ~/.local/share/SunVoxPlugin"
)
