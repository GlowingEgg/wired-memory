# JUCE Plugin Template — Agent Guide

This is a JUCE audio plugin template with a React-based UI. The C++ DSP layer lives in
`Source/` and the UI lives in `ui/`. They communicate over `window.__JUCE__` via
`ui/src/plugin-bridge.ts`.

---

## UI: Default is plain React (HTML/CSS)

`ui/src/App.tsx` is the UI entry point. The default style is plain React with HTML and CSS.
Replace the example gain knob with the actual plugin controls, using `getSliderState` and
`getToggleState` from `plugin-bridge.ts` to bind parameters.

Parameter names in the bridge must match the IDs declared in:
- `PluginProcessor`'s `AudioProcessorValueTreeState`
- The relay names in `PluginEditor.cpp`

---

## UI: Use Three.js / React Three Fiber for 3D

**Use r3f whenever 3D visuals are requested or would clearly improve the UI** — spinning
objects, particle effects, 3D knobs, animated geometry, visualizers, glowing orbs, etc.
If in doubt, 3D tends to look great for audio plugins; lean toward using it.

The packages are already installed: `@react-three/fiber`, `@react-three/drei`, `three`.
No additional setup is needed.

### How to add a 3D element

1. Import the canvas wrapper:
   ```tsx
   import ThreeCanvas from "./r3f/ThreeCanvas";
   ```

2. Wrap your scene in `<ThreeCanvas>` anywhere in your JSX:
   ```tsx
   <ThreeCanvas height="300px">
     <ambientLight intensity={0.5} />
     <directionalLight position={[5, 5, 5]} />
     <mesh>
       <sphereGeometry args={[1, 32, 32]} />
       <meshStandardMaterial color="#7c3aed" />
     </mesh>
   </ThreeCanvas>
   ```

3. For non-trivial scenes, create components in `ui/src/r3f/components/` and compose
   them inside a `<ThreeCanvas>`.

4. Animate with `useFrame` from `@react-three/fiber`:
   ```tsx
   import { useFrame } from "@react-three/fiber";
   useFrame((_state, delta) => { meshRef.current.rotation.y += delta; });
   ```

5. Use `Html` from `@react-three/drei` to overlay DOM elements (labels, buttons) on top
   of the 3D scene.

### Reference files

- `ui/src/r3f/ThreeCanvas.tsx` — the Canvas wrapper (camera, tone mapping, sizing)
- `ui/src/r3f/ExampleScene.tsx` — complete working example: spinning cube with lights
- `../after-hours/ui/src/r3f/` — production example with nebula, orbs, keyboard, etc.

### Plain + 3D can coexist

You can mix plain HTML sections and `<ThreeCanvas>` blocks in the same `App.tsx`. 3D
does not have to be the entire UI.

---

## plugin-bridge.ts

| Function | Purpose |
|---|---|
| `getSliderState(name)` | Bind a float parameter (0–1 normalised) |
| `getToggleState(name)` | Bind a boolean parameter |
| `addBackendListener(eventId, cb)` | Receive events pushed from C++ |
| `isInsidePlugin()` | True inside JUCE, false in browser dev |

All functions work in browser dev mode with mock state. Sliders can be seeded via URL
params: `?gain=0.75`. `addBackendListener` mocks can be fired manually in the console
via `window.__mockEvent(id, data)` (wire up as needed during dev).

---

## Dev workflow

```bash
cd ui
npm run dev      # Vite dev server at localhost:5173
npm run build    # Production build → ui/dist/ (embedded by JUCE)
```

The C++ build uses CMake. The UI build output is inlined into the plugin binary.

---

## Design Systems

Pre-built visual design systems live in `../design-systems/` (sibling to this directory).
When creating a plugin with a specific design style, read `../design-systems/README.md` first.

**Adoption workflow (brief):**
1. Read `../design-systems/<name>/manifest.json` to understand what the system includes.
2. Copy `tokens.css` → `ui/src/design.css`; add `import "./design.css"` at the top of `main.tsx`.
3. Copy `components/` → `ui/src/components/`
4. If `uses3D: true`, copy `scene/` → `ui/src/r3f/components/` and `theme.ts` → `ui/src/lib/theme.ts`
5. Use components from `ui/src/components/` in `App.tsx` — they handle JUCE binding internally.

Available design systems: `minimal`, `crystal`, `analog`, `neon`, `cosmic`, `modular`

---

## File layout

```
Source/               C++ JUCE plugin (DSP + PluginEditor WebBrowser component)
ui/
  src/
    App.tsx           UI entry point — edit this
    App.css           UI styles (layout only; tokens come from design.css when using a design system)
    plugin-bridge.ts  JUCE ↔ React parameter bridge
    lib/
      useJuceParam.ts   Hooks for binding React state to JUCE params (used by design system components)
    components/         Design system components copied here at plugin creation time
    r3f/
      ThreeCanvas.tsx     Canvas wrapper — import this when adding 3D
      ExampleScene.tsx    Reference: spinning cube
      components/         Put scene-specific 3D components here
CMakeLists.txt
```
