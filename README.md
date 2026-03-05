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

`CMakeLists.txt` defaults to building for your machine's native architecture (arm64 on Apple Silicon, x86_64 on Intel). To build a universal binary instead:

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

To generate an Xcode project:

```bash
cmake -Bbuild -GXcode
open build/MyPluginName.xcodeproj
```

## Built plugin output

After building, plugin artefacts land in:

```
build/MyPluginName_artefacts/<Debug|Release>/
├── VST3/       My Plugin Name.vst3
├── AU/         My Plugin Name.component
└── Standalone/ My Plugin Name.app
```

(Replace `MyPluginName` with your `project()` name and `My Plugin Name` with your `PRODUCT_NAME`.)

## Dev workflow with Ableton Live

The fastest loop is to symlink the build output directly into the system plugin folder so every rebuild is immediately visible — no manual copying.

### One-time setup

```bash
# VST3 (Ableton 11+ on macOS defaults to VST3)
ln -s "$(pwd)/build/MyPluginName_artefacts/Debug/VST3/My Plugin Name.vst3" \
      ~/Library/Audio/Plug-Ins/VST3/

# AU (optional)
ln -s "$(pwd)/build/MyPluginName_artefacts/Debug/AU/My Plugin Name.component" \
      ~/Library/Audio/Plug-Ins/Components/
```

Run from the project root. Adjust `MyPluginName` / `My Plugin Name` to match your project.

### First-time scan in Ableton

1. Open **Ableton Live → Preferences → Plug-Ins**.
2. Enable **VST3 Plug-In Custom Folder** (if using VST3) or **Use Audio Units**.
3. Click **Rescan**. Your plugin will appear in the browser.

### Iterating

```bash
cmake --build build --config Debug
```

Rebuild, switch back to Ableton, and reload the device — no rescan needed because the symlink already points to the updated binary. If Ableton freezes or the plugin crashes, use **Option + drag** the device off the track to force-unload it, then reload after the fix.

> **Note:** AU changes sometimes require killing and restarting `coreaudiod` (`sudo killall coreaudiod`) for the host to pick up the new binary.

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
