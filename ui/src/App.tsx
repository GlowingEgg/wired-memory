import { useEffect, useRef, useState } from "react";
import { getSliderState, isInsidePlugin } from "./plugin-bridge";
import "./App.css";

/**
 * Root plugin UI component.
 *
 * Replace the contents of this file (and App.css) with your actual plugin UI.
 * The `getSliderState` / `getToggleState` calls in plugin-bridge.ts work both
 * in the browser (mock) and inside the plugin (real JUCE relays).
 *
 * Example parameter names ("gain", "cutoff") must match the IDs you declare in
 * PluginProcessor's AudioProcessorValueTreeState and the relay names in
 * PluginEditor.cpp.
 *
 * ─── 3D UI WITH REACT THREE FIBER ────────────────────────────────────────────
 * This template ships with @react-three/fiber, @react-three/drei, and three.js
 * pre-installed. The default UI is plain HTML/CSS (what you see below), but 3D
 * is available whenever you need it — no extra setup required.
 *
 * To add 3D elements:
 *   1. Import ThreeCanvas:
 *        import ThreeCanvas from "./r3f/ThreeCanvas";
 *   2. Wrap 3D content in <ThreeCanvas> anywhere in your JSX:
 *        <ThreeCanvas height="300px">
 *          <ambientLight />
 *          <mesh><boxGeometry /><meshStandardMaterial color="hotpink" /></mesh>
 *        </ThreeCanvas>
 *   3. For complex scenes, create components in ui/src/r3f/components/ and
 *      compose them inside a ThreeCanvas.
 *
 * See ui/src/r3f/ExampleScene.tsx for a complete working reference (spinning
 * cube with lights). See ../after-hours/ui/src/r3f/ for a production example.
 * ─────────────────────────────────────────────────────────────────────────────
 */
export default function App() {
  // Example: a single gain knob
  const [gain, setGain] = useState(0.5);
  const sliderRef = useRef(getSliderState("gain"));

  useEffect(() => {
    const slider = sliderRef.current;
    setGain(slider.getNormalisedValue());
    const unsub = slider.addListener(setGain);
    return unsub;
  }, []);

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const v = parseFloat(e.target.value);
    sliderRef.current.setNormalisedValue(v);
  };

  return (
    <div className="plugin-shell">
      {!isInsidePlugin() && (
        <div className="dev-badge">DEV</div>
      )}

      <h1 className="plugin-name">My Plugin Name</h1>

      <div className="knob-row">
        <label className="knob-label">
          <span>Gain</span>
          <input
            type="range"
            min={0}
            max={1}
            step={0.001}
            value={gain}
            onChange={handleChange}
          />
          <span className="knob-value">{(gain * 100).toFixed(1)}%</span>
        </label>
      </div>
    </div>
  );
}
