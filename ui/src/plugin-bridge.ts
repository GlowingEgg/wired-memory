/**
 * plugin-bridge.ts
 *
 * Single seam between React components and JUCE's window.__JUCE__ bridge.
 *
 * - Inside the plugin:  window.__JUCE__ exists -> delegates to juce-framework-frontend
 * - In the browser:     window.__JUCE__ is absent -> uses in-memory mock state
 *
 * Components should never call window.__JUCE__ directly -- always go through
 * the functions exported from this module.
 */

// -- Types --------------------------------------------------------------------

export type SliderListener = (value: number) => void;
export type ToggleListener = (value: boolean) => void;

export interface SliderState {
  getValue(): number;
  getNormalisedValue(): number;
  setNormalisedValue(v: number): void;
  /** Returns an unsubscribe function. */
  addListener(cb: SliderListener): () => void;
}

export interface ToggleState {
  getValue(): boolean;
  setValue(v: boolean): void;
  /** Returns an unsubscribe function. */
  addListener(cb: ToggleListener): () => void;
}

// -- Mock implementation (browser dev) ----------------------------------------

const mockSliders = new Map<string, { normValue: number; listeners: Set<SliderListener> }>();

function getMockSlider(name: string) {
  if (!mockSliders.has(name)) {
    // Seed from URL param for shareable previews: ?gain=0.75
    const urlParam = new URLSearchParams(window.location.search).get(name);
    mockSliders.set(name, {
      normValue: urlParam != null ? parseFloat(urlParam) : 0.5,
      listeners: new Set(),
    });
  }
  return mockSliders.get(name)!;
}

const mockToggles = new Map<string, { value: boolean; listeners: Set<ToggleListener> }>();

function getMockToggle(name: string) {
  if (!mockToggles.has(name)) {
    mockToggles.set(name, { value: false, listeners: new Set() });
  }
  return mockToggles.get(name)!;
}

const mockEventListeners = new Map<string, Set<(data: unknown) => void>>();

// -- JUCE frontend (bundled from juce-framework-frontend) ---------------------

import * as JuceFrontend from "juce-framework-frontend";

// Only use the real JUCE frontend when running inside the plugin
const Juce = (typeof window !== "undefined" && (window as any).__JUCE__ !== undefined)
  ? JuceFrontend
  : null;

// -- Public API ---------------------------------------------------------------

export function getSliderState(name: string): SliderState {
  if (Juce) {
    const relay = Juce.getSliderState(name);
    return {
      getValue: () => relay.getNormalisedValue(),
      getNormalisedValue: () => relay.getNormalisedValue(),
      setNormalisedValue: (v) => relay.setNormalisedValue(v),
      addListener: (cb) => {
        const id = relay.valueChangedEvent.addListener(() => cb(relay.getNormalisedValue()));
        return () => relay.valueChangedEvent.removeListener(id);
      },
    };
  }

  // Mock path
  const mock = getMockSlider(name);
  return {
    getValue: () => mock.normValue,
    getNormalisedValue: () => mock.normValue,
    setNormalisedValue: (v) => {
      mock.normValue = Math.max(0, Math.min(1, v));
      mock.listeners.forEach((cb) => cb(mock.normValue));
    },
    addListener: (cb) => {
      mock.listeners.add(cb);
      return () => mock.listeners.delete(cb);
    },
  };
}

export function getToggleState(name: string): ToggleState {
  if (Juce) {
    const relay = Juce.getToggleState(name);
    return {
      getValue: () => relay.getValue(),
      setValue: (v) => relay.setValue(v),
      addListener: (cb) => {
        const id = relay.valueChangedEvent.addListener(() => cb(relay.getValue()));
        return () => relay.valueChangedEvent.removeListener(id);
      },
    };
  }

  const mock = getMockToggle(name);
  return {
    getValue: () => mock.value,
    setValue: (v) => {
      mock.value = v;
      mock.listeners.forEach((cb) => cb(v));
    },
    addListener: (cb) => {
      mock.listeners.add(cb);
      return () => mock.listeners.delete(cb);
    },
  };
}

/**
 * Listen to events emitted by the plugin via
 *   webBrowser->emitEventIfBrowserIsVisible("eventId", data).
 * Returns an unsubscribe function.
 */
export function addBackendListener(
  eventId: string,
  cb: (data: unknown) => void
): () => void {
  if (typeof window !== "undefined" && (window as any).__JUCE__?.backend) {
    const backend = (window as any).__JUCE__.backend;
    const handle = backend.addEventListener(eventId, cb);
    return () => backend.removeEventListener(handle);
  }

  // Mock path: register in-memory; fire manually in dev via window.__mockEvent(id, data)
  if (!mockEventListeners.has(eventId)) {
    mockEventListeners.set(eventId, new Set());
  }
  mockEventListeners.get(eventId)!.add(cb);
  return () => mockEventListeners.get(eventId)?.delete(cb);
}

/** True when running inside the plugin, false in standalone browser dev. */
export const isInsidePlugin = (): boolean =>
  typeof window !== "undefined" && (window as any).__JUCE__ !== undefined;
