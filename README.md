# juce-template

Minimal JUCE + CMake audio plugin starter. Fetches JUCE automatically via `FetchContent` — no submodules. Targets VST3, AU, and Standalone on macOS. UI is built as a React/Vite app and hosted natively inside the plugin window via JUCE 8's `WebBrowserComponent`.

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

### 4. Install UI dependencies

```bash
cd ui && npm install && cd ..
```

### 5. Configure and build

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Debug
```

JUCE is downloaded into `Libs/` on the first configure run. Subsequent builds skip the download.

> **Apple Silicon — always pass `-DCMAKE_OSX_ARCHITECTURES=arm64` explicitly.**
> CMake's `CMAKE_HOST_SYSTEM_PROCESSOR` can resolve to `x86_64` in some environments (Rosetta terminal, certain shell configs) even on an M-series Mac, producing an Intel binary that Ableton and other native-arm64 hosts silently reject. Passing the flag explicitly prevents this.

To build a universal binary instead:

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

To generate an Xcode project:

```bash
cmake -Bbuild -GXcode
open build/MyPluginName.xcodeproj
```

---

## UI workflow

The plugin UI is a **React/Vite app** hosted inside the plugin window via JUCE 8's `WebBrowserComponent` (WKWebView on macOS). The same code that runs at `localhost:5173` in your browser runs inside the DAW — no translation, no re-implementation.

### Design in the browser (fastest iteration)

```bash
cd ui && npm run dev
# open http://localhost:5173
```

`window.__JUCE__` doesn't exist in the browser, so `ui/src/plugin-bridge.ts` falls back to in-memory mock state. Edit React components and CSS — changes appear instantly via Vite HMR. No plugin rebuild needed.

You can seed mock parameter values from URL params for shareable previews:

```
http://localhost:5173?gain=0.8&cutoff=0.3
```

### See the UI live inside the plugin

Run the Vite dev server and a debug plugin build at the same time:

```bash
# terminal 1
cd ui && npm run dev

# terminal 2 — build and run the Standalone target
cmake --build build --target MyPluginName_Standalone
./build/MyPluginName_artefacts/Debug/Standalone/My\ Plugin\ Name.app/Contents/MacOS/My\ Plugin\ Name
```

In `DEBUG` mode, `PluginEditor.cpp` points `WebBrowserComponent` at `http://localhost:5173`. Vite HMR works inside the plugin window — save a CSS file and the plugin updates without a C++ rebuild.

### Embed for release

```bash
cd ui && npm run build    # outputs to ui/dist/
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release
```

On the first build after `npm run build`, cmake sees files in `ui/dist/` and embeds them as BinaryData via `juce_add_binary_data`. In `RELEASE` mode the editor loads from that embedded data — no external server, fully self-contained.

> **Note on the two-step cmake:** cmake globs `ui/dist/` at configure time. If you add new asset files (fonts, images, etc.) after the initial configure you'll need to rerun `cmake -Bbuild ...` so the glob picks them up.

### Disabling the web UI

If you want to skip React entirely and use the traditional JUCE `paint()`-based editor:

```bash
cmake -Bbuild -DUSE_WEB_UI=OFF ...
```

The editor falls back to a plain centered text label. Useful for headless builds or platforms where WKWebView is unavailable.

### Wiring up real parameters

1. Declare parameters in `PluginProcessor` using `AudioProcessorValueTreeState`.

2. Add relay members to `PluginEditor.h` — one per parameter:

   ```cpp
   juce::WebSliderRelay gainRelay { "gain" };
   juce::WebToggleButtonRelay bypassRelay { "bypass" };
   ```

3. Wire them into `WebBrowserComponent::Options` in `PluginEditor.cpp`:

   ```cpp
   auto options = juce::WebBrowserComponent::Options{}
       .withNativeIntegrationEnabled()
       .withOptionsFrom(gainRelay)
       .withOptionsFrom(bypassRelay)
       .withResourceProvider(...);
   ```

4. In React, use `getSliderState` / `getToggleState` from `plugin-bridge.ts`:

   ```ts
   import { getSliderState } from "./plugin-bridge";

   const gain = getSliderState("gain");   // works in browser (mock) and plugin (real)
   gain.addListener(({ value }) => setDisplay(value));
   gain.setNormalisedValue(0.75);
   ```

   The parameter names must match between the relay constructor and the bridge call.

### Customising the look

`ui/src/App.css` defines CSS custom properties at `:root`. Override them per-plugin without touching component code:

```css
:root {
  --color-bg: #0d0d0d;
  --color-accent: #ff6b35;
  --plugin-width: 600px;
  --plugin-height: 400px;
}
```

See `docs/UI_STRATEGY.md` for the full strategy writeup.

---

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
3. Reconfigure: `cmake -Bbuild -DCMAKE_OSX_ARCHITECTURES=arm64`

## Directory layout

```
juce-template/
├── CMakeLists.txt          ← primary config — edit this first
├── docs/
│   └── UI_STRATEGY.md      ← full UI strategy writeup
├── ui/                     ← React/Vite plugin UI
│   ├── src/
│   │   ├── main.tsx
│   │   ├── App.tsx         ← root component — start editing here
│   │   ├── App.css         ← CSS custom properties for theming
│   │   └── plugin-bridge.ts ← window.__JUCE__ abstraction + mock
│   ├── index.html
│   ├── package.json
│   ├── tsconfig.json
│   └── vite.config.ts
├── Source/
│   ├── PluginProcessor.h / .cpp   ← audio processing + parameter tree
│   └── PluginEditor.h / .cpp      ← mounts WebBrowserComponent
└── README.md
```
