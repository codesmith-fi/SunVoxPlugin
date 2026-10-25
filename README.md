### SunVoxPlugin
A modern, vector-based modular synth and effect plugin (VST3 / CLAP / Standalone) powered by Alexander Zolotov's powerful sunvox_lib audio engine, built with JUCE.

### Overview
SunVox has long been celebrated for its lightweight footprint, versatile sound architecture, and expansive modular sonic palette—ranging from raw 8-bit SID tones and FM bells to lush analog generators, wavetables, and complex MetaModule structures.

SunVoxPlugin brings this legendary sound engine directly into modern DAWs (FL Studio, Reaper, Bitwig, etc.) with a clean, hardware-accelerated, vector-based node graph UI. It bridges the gap between SunVox's deep synthesis engine and contemporary DAW workflows, replacing tracker-style constraints with an intuitive modular patching environment.

### Key Features
- SunVox Engine Core: Direct integration with sunvox_lib, running natively in memory for zero-latency, high-efficiency synthesis and DSP.
- Vector-Based Node Canvas: Smooth zoom and pan navigation (0.35x–2.0x), customizable module placement, and dynamic Bézier patch cables.
- Intuitive Patching: Real-time drag-to-connect audio/event routing, cable hover feedback, and quick disconnection workflows.
- Real-Time Module Inspector: Instant access to all native SunVox parameters (frequencies, resonance, envelopes, waveforms, LFOs) via a dedicated side panel.
- Full Preset & Project Recall: Seamless DAW state management (getStateInformation / setStateInformation) ensuring your modular patches reload exactly as saved.
- Integrated Preset & Instrument Library: Out-of-the-box support for loading native .sunsynth instruments, .sunvox project files, and FastTracker II .xi extended instruments.
- Cross-Platform Architecture: Built using modern C++20 and CMake/Ninja. Runs natively on Linux (ALSA/JACK) with support for Windows environments via WINE/Bottles.

### Architecture

┌─────────────────────────────────────────────────────────────┐
│                       JUCE Frontend                         │
│  ┌──────────────────────┐        ┌───────────────────────┐  │
│  │   GraphCanvas (UI)   │        │   InspectorPanel      │  │
│  │  (Bézier Cables/Pan) │        │ (Parameter Sliders)   │  │
│  └──────────┬───────────┘        └───────────┬───────────┘  │
│             │                                │              │
│             └───────────────┬────────────────┘              │
│                             │                               │
│              ┌──────────────┴──────────────┐                │
│              │      SunVoxEngine API       │                │
│              └──────────────┬──────────────┘                │
└─────────────────────────────┼───────────────────────────────┘
                              │
┌─────────────────────────────┴───────────────────────────────┐
│                     sunvox_lib (C Engine)                   │
│  [ Generators ]  [ Filters ]  [ MetaModules ]  [ Samplers ] │
└─────────────────────────────────────────────────────────────┘

### Building from Source

## Prerequisites
C++20 Compiler (GCC 12+, Clang 14+)
CMake 3.22+
Ninja
Audio development libraries (ALSA, FreeType, libX11 / Xrandr / Xinerama on Linux)

## Build Steps

```Bash
# Clone the repository with submodules
git clone --recursive https://github.com/codesmith-fi/SunVoxPlugin.git
cd SunVoxPlugin

# Configure the build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build Standalone and VST3 / CLAP targets
cmake --build build -j$(nproc)
```

The resulting binaries will be located under build/SunVoxPlugin_artefacts/Release/.

# Included Instruments
The repository includes a curated selection of instruments located in assets/instruments/:

- Basses: Deep FM subs, analog leads, punchy chiptune basses.
- Pads & Ambience: Ethereal layered textures, resonant MetaModule atmospheric patches (e.g. Underwater.sunsynth).
- Leads & Synths: Polyphonic analog emulations, multi-wave leads, classic tracker tones.

### Credits & Acknowledgments
Alexander Zolotov (NightRadio / WarmPlace): Creator of the incredible SunVox and sunvox_lib.
JUCE Framework: GUI rendering, audio buffer pipelines, and cross-platform plugin wrappers.

### License
SunVoxPlugin source code is licensed under the MIT License.
sunvox_lib is copyright © Alexander Zolotov and is distributed under its respective license terms.
