# Wired Memory — Agent Guide

Sampler plugin that captures browser audio via macOS SCStream API. JUCE C++ backend
in `Source/`, React/Vite UI in `ui/`. Uses the "Wired" design system (glass panels
over Japanese suburban landscape, CRT/phosphor aesthetic).

---

## Design System: Wired

The UI uses the Wired design system from `../design-systems/wired/`. Key files:
- `ui/src/design.css` — CSS tokens (copied from `../design-systems/wired/tokens.css`)
- `ui/src/App.css` — Full Wired component styles (glass panels, knobs, scopes, etc.)
- `ui/src/App.tsx` — Main UI with landscape background canvas, glass panels, controls

Visual language: frosted white glass panels, VT323 + Share Tech Mono fonts, light blue /
amber / cyan accent colors, CRT scanlines, bloom overlays, viewfinder corners.

---

## Plugin Architecture

This is a sampler plugin. The eventual workflow:
1. Capture audio from browser tabs via macOS SCStream API
2. Store captured audio as samples
3. Play back samples with tape-machine-style controls (speed, start, length, loop, reverse)

Current state: basic I/O boilerplate with gain parameter. SCStream capture not yet implemented.

---

## Parameters

| ID | Type | Range | Description |
|---|---|---|---|
| `gain` | float | 0–1 | Output gain |

Parameters are declared in `PluginProcessor::createParameterLayout()`, relayed in
`PluginEditor.h`, and consumed via `useJuceSlider`/`useJuceToggle` hooks in React.

---

## UI: React + Wired Design System

`ui/src/App.tsx` is the UI entry point. Components are inline in App.tsx (Knob, Toggle,
Readout, Scope, BackgroundCanvas). The landscape background is painted procedurally
on a canvas element using simplex noise.

---

## plugin-bridge.ts

| Function | Purpose |
|---|---|
| `getSliderState(name)` | Bind a float parameter (0–1 normalised) |
| `getToggleState(name)` | Bind a boolean parameter |
| `addBackendListener(eventId, cb)` | Receive events pushed from C++ |
| `isInsidePlugin()` | True inside JUCE, false in browser dev |

---

## Dev workflow

```bash
cd ui
npm run dev      # Vite dev server at localhost:5173
npm run build    # Production build → ui/dist/ (embedded by JUCE)
```

The C++ build uses CMake. The UI build output is inlined into the plugin binary.

---

## File layout

```
Source/               C++ JUCE plugin (DSP + PluginEditor WebBrowser component)
ui/
  src/
    App.tsx           UI entry point — Wired sampler interface
    App.css           Wired design system styles
    design.css        Wired CSS tokens
    plugin-bridge.ts  JUCE ↔ React parameter bridge
    lib/
      useJuceParam.ts   Hooks for binding React state to JUCE params
    r3f/
      ThreeCanvas.tsx     Canvas wrapper (available if 3D needed later)
      ExampleScene.tsx    Reference scene
CMakeLists.txt
```
