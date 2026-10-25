# SunVox VST Architecture & Core Design

## 1. Project Overview
This project wraps the SunVox modular sound engine (`sunvox_lib`) into a modern VST3 and CLAP plugin using JUCE. It features an interactive, clean vector-based modular node graph UI, removing the need for a tracker interface while preserving full modular synthesis capabilities.

## 2. Technical Details
- **Framework:** Modern C++20 with JUCE (Audio plugin target: VST3, CLAP, Standalone).
- **Audio Engine:** SunVox dynamic library (`sunvox_lib`) running in offline stream callback mode (`SV_INIT_FLAG_USER_AUDIO_CALLBACK`).
- **UI Architecture:** Resizable, vector-rendered modular graph (`juce::Component`) with zoom/pan capabilities, draggable modules, and Bézier patch cables.

### Specifications & Documentation
- SunVox C API details and reference documentation are located in `docs/`.
- Official API reference: [SunVox library for developers](https://www.warmplace.ru/soft/sunvox/sunvox_lib.php)

## 3. Coding Conventions
- **Standard:** Modern C++20.
- **Naming:**
  * Classes and Structs: `PascalCase` (e.g., `SunVoxAudioProcessor`, `NodeGraphEditor`).
  * Member Variables: `camelCase` with trailing underscore (e.g., `sampleRate_`, `isEngineLoaded_`).
  * Functions and Methods: `camelCase` (e.g., `sendMidiEvent()`, `loadSunVoxFile()`).
  * Constants / Enums: `kPascalCase` or `UPPER_SNAKE_CASE` (e.g., `kDefaultSampleRate = 44100`).
- **Structure:** Clean separation into `src/audio/`, `src/gui/`, and `src/common/`.
- **Indentation:** Tabs for indentation, spaces for alignment.

## 4. Build System & Toolchain (Linux)
The project builds using **CMake** and Ninja.

### Build Commands:
```bash
# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build -j$(nproc)

# Release build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build-release -j$(nproc)

# Run Standalone for testing
./build/SunVoxPlugin_Artefacts/Debug/Standalone/SunVoxPlugin

5. Project Folder Structure

src/: Plugin source code (JUCE processor, editor, and GUI components).
src/audio/: AudioProcessor, SunVox wrapper, MIDI event translation.
src/gui/: Modular node graph canvas, vector cables, and control panels.
src/common/: Shared structures and utility functions.
sunvox_lib/: Platform-specific shared libraries (.so, .dll, .dylib) and header sunvox.h.
docs/: SunVox developer documentation and manuals.

6. Testing and verification
Make module tests for non-UI components. Also verify the UI functionality with UI tests if possible.
