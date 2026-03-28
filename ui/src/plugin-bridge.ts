/**
 * plugin-bridge.ts
 *
 * Single seam between React components and JUCE's window.__JUCE__ bridge.
 *
 * - Inside the plugin:  window.__JUCE__ exists → delegates to real relay objects
 * - In the browser:     window.__JUCE__ is absent → uses in-memory mock state
 *
 * Components should never call window.__JUCE__ directly — always go through
 * the functions exported from this module.
 */

// ── Types ─────────────────────────────────────────────────────────────────────

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

// ── JUCE window type (partial) ────────────────────────────────────────────────

interface JuceBackend {
  getSliderState(name: string): {
    getNormalisedValue(): number;
    setNormalisedValue(v: number): void;
    addParameterChangeListener(cb: () => void): void;
    removeParameterChangeListener(cb: () => void): void;
  };
  getToggleState(name: string): {
    getValue(): boolean;
    setValue(v: boolean): void;
    addParameterChangeListener(cb: () => void): void;
    removeParameterChangeListener(cb: () => void): void;
  };
  addEventListener(eventId: string, cb: (data: unknown) => void): void;
  removeEventListener(eventId: string, cb: (data: unknown) => void): void;
}

declare global {
  interface Window {
    __JUCE__?: { backend: JuceBackend };
  }
}

// ── Mock implementation (browser dev) ────────────────────────────────────────

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

// ── Public API ────────────────────────────────────────────────────────────────

export function getSliderState(name: string): SliderState {
  if (window.__JUCE__) {
    const relay = window.__JUCE__.backend.getSliderState(name);
    return {
      getValue: () => relay.getNormalisedValue(),
      getNormalisedValue: () => relay.getNormalisedValue(),
      setNormalisedValue: (v) => relay.setNormalisedValue(v),
      addListener: (cb) => {
        const wrapped = () => cb(relay.getNormalisedValue());
        relay.addParameterChangeListener(wrapped);
        return () => relay.removeParameterChangeListener(wrapped);
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
  if (window.__JUCE__) {
    const relay = window.__JUCE__.backend.getToggleState(name);
    return {
      getValue: () => relay.getValue(),
      setValue: (v) => relay.setValue(v),
      addListener: (cb) => {
        const wrapped = () => cb(relay.getValue());
        relay.addParameterChangeListener(wrapped);
        return () => relay.removeParameterChangeListener(wrapped);
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
 * Listen to arbitrary events emitted from C++ via:
 *   webBrowser->emitEventIfBrowserIsVisible("eventId", data)
 *
 * Returns an unsubscribe function — call it in a useEffect cleanup.
 *
 * Example:
 *   useEffect(() => addBackendListener("chordChanged", (data) => {
 *     setChord(data as ChordData);
 *   }), []);
 */
export function addBackendListener(
  eventId: string,
  cb: (data: unknown) => void
): () => void {
  if (window.__JUCE__) {
    window.__JUCE__.backend.addEventListener(eventId, cb);
    return () => window.__JUCE__!.backend.removeEventListener(eventId, cb);
  }

  // Mock path: register in-memory; fire manually in dev via window.__mockEvent(id, data)
  if (!mockEventListeners.has(eventId)) {
    mockEventListeners.set(eventId, new Set());
  }
  mockEventListeners.get(eventId)!.add(cb);
  return () => mockEventListeners.get(eventId)?.delete(cb);
}

/** True when running inside the plugin, false in standalone browser dev. */
export const isInsidePlugin = (): boolean => window.__JUCE__ !== undefined;
