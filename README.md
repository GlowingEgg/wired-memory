# Wired Memory

A sampler plugin that captures audio from the browser via macOS's SCStream API. Built with JUCE + React/Vite. Uses the "Wired" design system — glass UI floating over blooming Japanese suburban landscapes.

## Quick start

### 1. Install UI dependencies

```bash
cd ui && npm install && cd ..
```

### 2. Configure and build

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Debug
```

JUCE is downloaded into `Libs/` on the first configure run.

### 3. Dev workflow

```bash
# terminal 1 — UI dev server
cd ui && npm run dev

# terminal 2 — build and run standalone
cmake --build build --target WiredMemory_Standalone
./build/WiredMemory_artefacts/Debug/Standalone/Wired\ Memory.app/Contents/MacOS/Wired\ Memory
```

In DEBUG mode the plugin loads UI from `http://localhost:5173` with Vite HMR.

## Built plugin output

```
build/WiredMemory_artefacts/<Debug|Release>/
├── VST3/       Wired Memory.vst3
├── AU/         Wired Memory.component
└── Standalone/ Wired Memory.app
```

## Ableton symlinks

```bash
ln -s "$(pwd)/build/WiredMemory_artefacts/Debug/VST3/Wired Memory.vst3" \
      ~/Library/Audio/Plug-Ins/VST3/

ln -s "$(pwd)/build/WiredMemory_artefacts/Debug/AU/Wired Memory.component" \
      ~/Library/Audio/Plug-Ins/Components/
```

## Directory layout

```
wired-memory/
├── CMakeLists.txt
├── docs/
│   └── UI_STRATEGY.md
├── ui/
│   ├── src/
│   │   ├── main.tsx
│   │   ├── App.tsx          ← root component
│   │   ├── App.css          ← Wired design system styles
│   │   ├── design.css       ← Wired CSS tokens
│   │   └── plugin-bridge.ts ← JUCE ↔ React parameter bridge
│   ├── index.html
│   ├── package.json
│   ├── tsconfig.json
│   └── vite.config.ts
├── Source/
│   ├── PluginProcessor.h / .cpp  ← audio processing + parameter tree
│   └── PluginEditor.h / .cpp     ← mounts WebBrowserComponent
└── README.md
```
