cmake_minimum_required(VERSION 3.22)
project(SunVoxPlugin VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_WINE_FIXES "Enable Wine/Bottles specific rendering and focus workarounds" OFF)

if (WIN32)
    if (ENABLE_WINE_FIXES)
        message(STATUS "Build Target: Linux WINE / Bottles Compatibility Mode")
        add_compile_definitions(
            JUCE_DIRECT2D=0
            JUCE_USE_DIRECTWRITE=0
            JUCE_USE_OPENGL=0
            ENABLE_WINE_BUILD=1
        )
    else()
        message(STATUS "Build Target: Windows Native (Direct2D / Standard Windows Messaging)")
        add_compile_definitions(
            JUCE_DIRECT2D=1
            JUCE_USE_DIRECTWRITE=1
            JUCE_USE_OPENGL=0
            ENABLE_WINE_BUILD=0
        )
    endif()
endif()

# JUCE Framework
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(JUCE)
if (NOT juce_POPULATED)
    FetchContent_Populate(JUCE)
    if (WIN32 AND ENABLE_WINE_FIXES)
        message(STATUS "SunVoxPlugin: Applying Wine compatibility patches to JUCE...")
        set(_JUCE_WIN_FILE "${juce_SOURCE_DIR}/modules/juce_gui_basics/native/juce_Windowing_windows.cpp")
        if (EXISTS "${_JUCE_WIN_FILE}")
            file(READ "${_JUCE_WIN_FILE}" _JUCE_WIN_CONTENT)
            string(REPLACE "const auto renderer = juce_isRunningInWine() ? 0 : 1;"
                           "const auto renderer = 0;"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT}")
            string(REPLACE "contextDescriptorList<GDIRenderContext, D2DRenderContext>"
                           "contextDescriptorList<GDIRenderContext, GDIRenderContext>"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "void updateCurrentMonitorAndRefreshVBlankDispatcher (ForceRefreshDispatcher force = ForceRefreshDispatcher::no)"
                           "void updateCurrentMonitorAndRefreshVBlankDispatcher (ForceRefreshDispatcher force = ForceRefreshDispatcher::no) { ignoreUnused (force); return; } void unused_updateCurrentMonitorAndRefreshVBlankDispatcher (ForceRefreshDispatcher force = ForceRefreshDispatcher::no)"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "VBlankDispatcher::getInstance()->removeListener (*this);"
                           "/* VBlankDispatcher removeListener disabled */"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "auto* dispatcher = VBlankDispatcher::getInstance();"
                           "/* auto* dispatcher = nullptr; */"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "dispatcher->reconfigureDisplays();"
                           "/* dispatcher->reconfigureDisplays(); */"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "std::optional<TimedCallback> monitorUpdateTimer;"
                           "std::optional<TimedCallback> monitorUpdateTimer; std::optional<TimedCallback> vblankFallbackTimer;"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "setCurrentRenderingEngine (engine);"
                           "setCurrentRenderingEngine (engine); vblankFallbackTimer.emplace ([this] { onVBlank (Time::getMillisecondCounterHiRes() * 0.001); }); vblankFallbackTimer->startTimerHz (60);"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "suspendResumeRegistration = {};"
                           "vblankFallbackTimer.reset(); suspendResumeRegistration = {};"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "void blitToDC (HDC dc, int x, int y) const noexcept"
                           "void blitToDC (HDC dc, int x, int y) const noexcept { SetMapMode (dc, MM_TEXT); GdiFlush(); if (! BitBlt (dc, x, y, width, height, hdc, 0, 0, SRCCOPY)) StretchDIBits (dc, x, y, width, height, 0, 0, width, height, bitmapData, (const BITMAPINFO*) &bitmapInfo, DIB_RGB_COLORS, SRCCOPY); } void unused_blitToDC (HDC dc, int x, int y) const noexcept"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            string(REPLACE "void handleShowWindow() override {}"
                           "void handleShowWindow() override { peer.getComponent().repaint(); }"
                           _JUCE_WIN_CONTENT_PATCHED "${_JUCE_WIN_CONTENT_PATCHED}")
            file(WRITE "${_JUCE_WIN_FILE}" "${_JUCE_WIN_CONTENT_PATCHED}")
            message(STATUS "SunVoxPlugin: Patched juce_Windowing_windows.cpp (forced GDIRenderContext, 60Hz fallback timer, BitBlt rendering)")
        endif()
        set(_JUCE_VBLANK_FILE "${juce_SOURCE_DIR}/modules/juce_gui_basics/native/juce_VBlank_windows.cpp")
        if (EXISTS "${_JUCE_VBLANK_FILE}")
            file(READ "${_JUCE_VBLANK_FILE}" _JUCE_VBLANK_CONTENT)
            string(REPLACE "void updateDisplay (ComponentPeer::VBlankListener& listener, HMONITOR monitor)"
                           "void updateDisplay (ComponentPeer::VBlankListener& listener, HMONITOR monitor) { ignoreUnused (listener, monitor); return; } void unused_updateDisplay (ComponentPeer::VBlankListener& listener, HMONITOR monitor)"
                           _JUCE_VBLANK_CONTENT_PATCHED "${_JUCE_VBLANK_CONTENT}")
            string(REPLACE "void reconfigureDisplays()"
                           "void reconfigureDisplays() { return; } void unused_reconfigureDisplays()"
                           _JUCE_VBLANK_CONTENT_PATCHED "${_JUCE_VBLANK_CONTENT_PATCHED}")
            file(WRITE "${_JUCE_VBLANK_FILE}" "${_JUCE_VBLANK_CONTENT_PATCHED}")
            message(STATUS "SunVoxPlugin: Patched juce_VBlank_windows.cpp (disabled VBlankDispatcher)")
        endif()
        set(_JUCE_DWRITE_FILE "${juce_SOURCE_DIR}/modules/juce_graphics/native/juce_DirectWriteTypeface_windows.cpp")
        if (EXISTS "${_JUCE_DWRITE_FILE}")
            file(READ "${_JUCE_DWRITE_FILE}" _JUCE_DWRITE_CONTENT)
            string(REPLACE "auto factory = factories->getDWriteFactory().getInterface<IDWriteFactory2>();"
                           "ignoreUnused (c, language); return {}; auto factory = factories->getDWriteFactory().getInterface<IDWriteFactory2>();"
                           _JUCE_DWRITE_CONTENT_PATCHED "${_JUCE_DWRITE_CONTENT}")
            file(WRITE "${_JUCE_DWRITE_FILE}" "${_JUCE_DWRITE_CONTENT_PATCHED}")
            message(STATUS "SunVoxPlugin: Patched juce_DirectWriteTypeface_windows.cpp (disabled DirectWrite system font fallback for Wine)")
        endif()
    else()
        find_package(Git QUIET)
        if (Git_FOUND AND EXISTS "${juce_SOURCE_DIR}/.git")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} checkout -f -- modules/juce_gui_basics modules/juce_graphics
                WORKING_DIRECTORY "${juce_SOURCE_DIR}"
                OUTPUT_QUIET
                ERROR_QUIET
            )
        endif()
    endif()
    add_subdirectory(${juce_SOURCE_DIR} ${juce_BINARY_DIR})
endif()

# CLAP support via clap-juce-extensions
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-juce-extensions/CMakeLists.txt")
    add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)
endif()

juce_add_plugin(SunVoxPlugin
    COMPANY_NAME "WarmPlace"
    VERSION "0.1.0"
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
        JUCE_VST3_CAN_REPLACE_VST2=0
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

if(WIN32)
    set(SUNVOX_LIB_FILE "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/sunvox_lib/windows/lib_x86_64/sunvox.dll")
    set(SUNVOX_LIB_NAME "sunvox.dll")
elseif(APPLE)
    set(SUNVOX_LIB_FILE "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/sunvox_lib/macos/lib_x86_64/sunvox.dylib")
    set(SUNVOX_LIB_NAME "sunvox.dylib")
else()
    set(SUNVOX_LIB_FILE "${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib/linux/x86_64/sunvox.so")
    set(SUNVOX_LIB_NAME "sunvox.so")
endif()

# Copy sunvox library next to the Standalone executable
if(TARGET SunVoxPlugin_Standalone)
    add_custom_command(TARGET SunVoxPlugin_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SUNVOX_LIB_FILE}"
            "$<TARGET_FILE_DIR:SunVoxPlugin_Standalone>/${SUNVOX_LIB_NAME}"
        COMMENT "Copying ${SUNVOX_LIB_NAME} next to Standalone executable"
    )
endif()

# Copy sunvox library and assets into VST3 bundle directory
if(TARGET SunVoxPlugin_VST3)
    add_custom_command(TARGET SunVoxPlugin_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SUNVOX_LIB_FILE}"
            "$<TARGET_FILE_DIR:SunVoxPlugin_VST3>/${SUNVOX_LIB_NAME}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:SunVoxPlugin_VST3>/../Resources/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:SunVoxPlugin_VST3>/../Resources/assets"
        COMMENT "Copying ${SUNVOX_LIB_NAME} and assets into VST3 bundle"
    )
endif()

# Copy sunvox library next to CLAP plugin
if(TARGET SunVoxPlugin_CLAP)
    add_custom_command(TARGET SunVoxPlugin_CLAP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SUNVOX_LIB_FILE}"
            "$<TARGET_FILE_DIR:SunVoxPlugin_CLAP>/${SUNVOX_LIB_NAME}"
        COMMENT "Copying ${SUNVOX_LIB_NAME} next to CLAP plugin"
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
