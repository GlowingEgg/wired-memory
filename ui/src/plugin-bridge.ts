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

const mockSliderDefaults: Record<string, number> = {
  speed: Math.pow(0.9 / 9.9, 0.3),   // 1.0x — matches JUCE skew=0.3, range 0.1–10.0
  start: 0.0,
  length: 1.0,
};

function getMockSlider(name: string) {
  if (!mockSliders.has(name)) {
    // Seed from URL param for shareable previews: ?speed=0.5
    const urlParam = new URLSearchParams(window.location.search).get(name);
    const defaultVal = mockSliderDefaults[name] ?? 0.5;
    mockSliders.set(name, {
      normValue: urlParam != null ? parseFloat(urlParam) : defaultVal,
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

// -- SCK Audio Capture --------------------------------------------------------

export interface AudioSourceInfo {
  bundleId: string;
  displayName: string;
}

export interface SCKStatus {
  permissionDenied: boolean;
}

/**
 * Listen for source list updates pushed from C++.
 * Returns an unsubscribe function.
 */
export function addSourcesListener(
  cb: (sources: AudioSourceInfo[]) => void
): () => void {
  return addBackendListener("sck:sources", (data) => {
    try {
      const sources = typeof data === "string" ? JSON.parse(data) : data;
      cb(sources as AudioSourceInfo[]);
    } catch {
      cb([]);
    }
  });
}

/**
 * Listen for SCK status updates (permission, streaming state).
 * Returns an unsubscribe function.
 */
export function addStatusListener(
  cb: (status: SCKStatus) => void
): () => void {
  return addBackendListener("sck:status", (data) => {
    try {
      const status = typeof data === "string" ? JSON.parse(data) : data;
      cb(status as SCKStatus);
    } catch {
      cb({ permissionDenied: false });
    }
  });
}

/**
 * Tell the C++ backend to start capturing from the given app.
 */
export function setSource(bundleId: string): void {
  if (Juce) {
    Juce.getNativeFunction("sck_setSource")(bundleId);
  } else {
    console.log("[mock] setSource:", bundleId);
  }
}

/**
 * Listen for waveform snapshot data pushed from C++ (~30fps).
 * Each snapshot is a Float32-style array of 128 sample values.
 * Returns an unsubscribe function.
 */
export function addWaveformListener(
  cb: (samples: number[]) => void
): () => void {
  if (isInsidePlugin()) {
    return addBackendListener("sck:waveform", (data) => {
      try {
        const samples = typeof data === "string" ? JSON.parse(data) : data;
        cb(samples as number[]);
      } catch {
        // ignore malformed data
      }
    });
  }

  // Dev mode: generate mock waveform data at ~30fps
  let running = true;
  let phase = 0;
  const interval = setInterval(() => {
    if (!running) return;
    const samples: number[] = [];
    for (let i = 0; i < 128; i++) {
      const t = i / 128;
      samples.push(
        Math.sin(phase + t * Math.PI * 6) * 0.3
        + Math.sin(phase * 1.7 + t * Math.PI * 14) * 0.15
        + Math.sin(phase * 0.3 + t * Math.PI * 23) * 0.08
        + (Math.random() - 0.5) * 0.04
      );
    }
    phase += 0.15;
    cb(samples);
  }, 33);

  return () => {
    running = false;
    clearInterval(interval);
  };
}

/**
 * Listen for the captured sample snapshot pushed from C++ when recording stops.
 * The snapshot is a 512-point peak-envelope array.
 * Returns an unsubscribe function.
 */
export function addSampleListener(
  cb: (samples: number[]) => void
): () => void {
  return addBackendListener("sck:sample", (data) => {
    try {
      const samples = typeof data === "string" ? JSON.parse(data) : data;
      cb(samples as number[]);
    } catch {
      // ignore malformed data
    }
  });
}

/**
 * Start playing the captured sample.
 */
export function startPlayback(): void {
  if (Juce) {
    Juce.getNativeFunction("sck_play")();
  } else {
    console.log("[mock] startPlayback");
  }
}

/**
 * Stop playing the captured sample.
 */
export function stopPlayback(): void {
  if (Juce) {
    Juce.getNativeFunction("sck_stop")();
  } else {
    console.log("[mock] stopPlayback");
  }
}

export interface PlaybackState {
  playing: boolean;
  progress: number;
  duration: number;
}

/**
 * Listen for grain position snapshots pushed from C++ (~30fps).
 * Each snapshot is an array of normalised sample positions [0, 1] for active grains.
 * Returns an unsubscribe function.
 */
export function addGrainListener(
  cb: (positions: number[]) => void
): () => void {
  return addBackendListener("sck:grains", (data) => {
    try {
      const positions = typeof data === "string" ? JSON.parse(data) : data;
      cb(positions as number[]);
    } catch {
      cb([]);
    }
  });
}

/**
 * Listen for playback state updates pushed from C++ (~30fps).
 * Returns an unsubscribe function.
 */
export function addPlaybackListener(
  cb: (state: PlaybackState) => void
): () => void {
  return addBackendListener("sck:playback", (data) => {
    try {
      const state = typeof data === "string" ? JSON.parse(data) : data;
      cb(state as PlaybackState);
    } catch {
      // ignore malformed data
    }
  });
}

/**
 * Ask the C++ backend to refresh the available sources list.
 */
export function refreshSources(): void {
  if (Juce) {
    Juce.getNativeFunction("sck_refreshSources")();
  } else {
    // In dev mode, fire a mock sources event
    const mockSources: AudioSourceInfo[] = [
      { bundleId: "com.google.Chrome", displayName: "Google Chrome" },
      { bundleId: "com.apple.Safari", displayName: "Safari" },
      { bundleId: "com.spotify.client", displayName: "Spotify" },
    ];
    setTimeout(() => {
      const listeners = mockEventListeners.get("sck:sources");
      if (listeners) {
        listeners.forEach((cb) => cb(JSON.stringify(mockSources)));
      }
    }, 100);
  }
}
