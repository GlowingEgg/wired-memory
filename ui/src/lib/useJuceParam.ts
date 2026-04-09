/**
 * Hooks for binding React state to JUCE parameters via plugin-bridge.
 *
 * These are used internally by design system components (Knob, Fader, Button).
 * You don't need to call them directly unless you're building a custom component
 * that isn't part of a design system.
 *
 * Works in both browser dev mode (mock state) and inside the plugin (real JUCE).
 */

import { useEffect, useRef, useState } from "react";
import { getSliderState, getToggleState } from "../plugin-bridge";

/** Bind a float parameter (0–1 normalised). */
export function useJuceSlider(name: string) {
  const ref = useRef(getSliderState(name));
  const [value, setValue] = useState(() => ref.current.getNormalisedValue());

  useEffect(() => {
    const slider = ref.current;
    setValue(slider.getNormalisedValue());
    return slider.addListener(setValue);
  }, [name]);

  return {
    value,
    set: (v: number) => ref.current.setNormalisedValue(v),
  };
}

/** Bind a boolean parameter. */
export function useJuceToggle(name: string) {
  const ref = useRef(getToggleState(name));
  const [value, setValue] = useState(() => ref.current.getValue());

  useEffect(() => {
    const toggle = ref.current;
    setValue(toggle.getValue());
    return toggle.addListener(setValue);
  }, [name]);

  return {
    value,
    set: (v: boolean) => ref.current.setValue(v),
  };
}
