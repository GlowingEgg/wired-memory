# juce-template

Minimal JUCE + CMake audio plugin starter. Fetches JUCE automatically via `FetchContent` — no submodules. Targets VST3, AU, and Standalone on macOS.

## Quick start

### 1. Copy the template

```bash
cp -r juce-template my-plugin-name
cd my-plugin-name
```

Use lowercase-with-hyphens for the directory name (e.g. `grain-delay`, `spectral-gate`).

### 2. Update `CMakeLists.txt`

Open `CMakeLists.txt` and update the following:

| What | Where |
|---|---|
| `project(MyPluginName ...)` | Project name — PascalCase, no spaces |
| `LIB_JUCE_TAG` | Latest JUCE release tag from [juce-framework/JUCE](https://github.com/juce-framework/JUCE/releases) |
| `PRODUCT_NAME` | Display name shown in the DAW |
| `COMPANY_NAME` | Your name or studio |
| `BUNDLE_ID` | Reverse-DNS, globally unique (e.g. `com.yourname.mypluginname`) |
| `PLUGIN_MANUFACTURER_CODE` | 4-char code identifying you as the maker |
| `PLUGIN_CODE` | 4-char code, unique per plugin you ship |

Also rename the class `MyPluginNameAudioProcessor` / `MyPluginNameAudioProcessorEditor` in `Source/` to match your project name, and update `target_sources` / `target_link_libraries` if the target name changes.

### 3. Initialize git

```bash
git init
git add .
git commit -m "Initial commit from juce-template"
```

Each plugin is its own standalone repo.

### 4. Configure and build

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

JUCE is downloaded into `Libs/` on the first configure run. Subsequent builds skip the download.

To generate an Xcode project:

```bash
cmake -Bbuild -GXcode
open build/MyPluginName.xcodeproj
```

## Updating JUCE

1. Change `LIB_JUCE_TAG` in `CMakeLists.txt` to the new release tag.
2. Delete `build/` so CMake re-fetches: `rm -rf build/`
3. Reconfigure: `cmake -Bbuild`

## Directory layout

```
juce-template/
├── CMakeLists.txt      ← primary config — edit this first
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   └── PluginEditor.cpp
└── README.md
```
