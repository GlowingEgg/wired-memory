# Plugin UI Strategy: React + JUCE WebBrowserComponent

## Core Idea

Write plugin UI as a **React/Vite app**. JUCE 8's `WebBrowserComponent` hosts it natively
inside the plugin window (WKWebView on macOS, WebView2 on Windows). The same UI you iterate
on in a browser at `localhost:5173` is what runs in the DAW — no translation, no image
hand-off, no re-implementation.

```
Browser (hot reload) ──► same React app ──► embedded in plugin via BinaryData
      ↕                                              ↕
  Mock bridge                                  window.__JUCE__ bridge
(localStorage/state)                       (real parameter sync via WebSliderRelay)
```

## Dev Workflow

### 1. Fast visual iteration (browser)

```bash
cd ui && npm run dev
# → http://localhost:5173
```

`window.__JUCE__` doesn't exist in the browser, so `plugin-bridge.ts` falls back to
in-memory mock state. Design freely — tweak CSS, animate, try layouts, no rebuild needed.

### 2. Live in the plugin (debug build)

```bash
# In one terminal:
cd ui && npm run dev

# In another: build the plugin in DEBUG mode
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug && cmake --build build --target MyPluginName_Standalone
```

In DEBUG mode, `PluginEditor.cpp` points `WebBrowserComponent` at `http://localhost:5173`.
The plugin window IS the browser — Vite HMR works, parameter changes reflect in real time.

### 3. Embed for release

```bash
cd ui && npm run build   # outputs to ui/dist/
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

In RELEASE mode, the editor loads from an embedded resource provider backed by BinaryData.
No external server needed, ships as a self-contained plugin.

## Parameter Binding

JUCE 8 provides relay objects that bridge `AudioProcessorValueTreeState` parameters to JS.
Each relay is named and attached to the `WebBrowserComponent::Options`. In JS, you get a
matching object via `window.__JUCE__`.

**C++ (PluginEditor.cpp):**
```cpp
// Declare relays as members:
juce::WebSliderRelay gainRelay { "gain" };
juce::WebToggleButtonRelay bypassRelay { "bypass" };

// Wire them into options:
auto options = juce::WebBrowserComponent::Options{}
    .withNativeIntegrationEnabled()
    .withOptionsFrom(gainRelay)
    .withOptionsFrom(bypassRelay)
    .withResourceProvider(...);
```

**TypeScript (plugin-bridge.ts mock-compatible):**
```ts
const gain = getSliderState("gain");
gain.addListener(({ value }) => setGainDisplay(value));
gain.setNormalisedValue(0.75);
```

In the browser (no `window.__JUCE__`), `getSliderState` returns an in-memory mock so the
component renders and responds to interaction without needing a running plugin.

## Project Layout

```
juce-template/
├── Source/
│   ├── PluginProcessor.h / .cpp   — audio processing, parameter tree
│   └── PluginEditor.h / .cpp      — mounts WebBrowserComponent
├── ui/
│   ├── src/
│   │   ├── main.tsx               — React entry
│   │   ├── App.tsx                — root component, receives mock/live params
│   │   └── plugin-bridge.ts       — window.__JUCE__ abstraction + mock
│   ├── index.html
│   ├── package.json
│   ├── tsconfig.json
│   └── vite.config.ts
└── CMakeLists.txt                 — USE_WEB_UI option wires npm build → BinaryData
```

## CMake Integration (USE_WEB_UI)

`CMakeLists.txt` has a `USE_WEB_UI` option (default ON).

When ON:
- `JUCE_WEB_BROWSER=1`
- `juce_gui_extra` is linked (provides `WebBrowserComponent`)
- A custom cmake target runs `npm run build` in `ui/`
- `juce_add_binary_data(PluginUI ...)` embeds `ui/dist/` into the binary
- `PluginUI` BinaryData is linked into the plugin target

When OFF: falls back to the default JUCE `paint()`-based editor.

## Tips

- Use CSS custom properties for all colors/sizes — easy to theme per-plugin
- Keep `plugin-bridge.ts` as the single seam between React and JUCE; never call
  `window.__JUCE__` directly from components
- The mock bridge can be backed by URL params (`?gain=0.5`) for shareable previews
- Resize the plugin window by calling `setSize()` from C++ after the web view loads;
  alternatively let the web view dictate size via a message to the native side
- For complex animations, prefer CSS transitions over JS — they run off the main thread
  and won't compete with the audio callback
