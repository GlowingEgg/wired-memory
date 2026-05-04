import { useCallback, useEffect, useRef, useState } from "react";
import { createNoise2D } from "simplex-noise";
import "./App.css";
import { useJuceSlider, useJuceToggle } from "./lib/useJuceParam";
import {
  addGrainListener,
  addPlaybackListener,
  addSampleListener,
  addSourcesListener,
  addStatusListener,
  addWaveformListener,
  stopPlayback as bridgeStopPlayback,
  isInsidePlugin,
  loadSampleFromFile,
  refreshSources,
  requestSampleSnapshot,
  setSource,
  startPlayback,
  trimSample,
  type AudioSourceInfo,
} from "./plugin-bridge";

/* ── Seeded PRNG ── */
function mulberry32(seed: number) {
  return () => {
    let t = (seed += 0x6d2b79f5);
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

const rng = mulberry32(1998);
const noise2D = createNoise2D(rng);

function fbm(
  x: number,
  y: number,
  octaves: number,
  lac = 2.0,
  gain = 0.5,
): number {
  let value = 0,
    amp = 1,
    freq = 1,
    max = 0;
  for (let i = 0; i < octaves; i++) {
    value += amp * noise2D(x * freq, y * freq);
    max += amp;
    amp *= gain;
    freq *= lac;
  }
  return value / max;
}

/* ── Paint the full-frame landscape ── */
function paintLandscape(ctx: CanvasRenderingContext2D, w: number, h: number) {
  const skyGrad = ctx.createLinearGradient(0, 0, 0, h * 0.6);
  skyGrad.addColorStop(0, "#8eaec4");
  skyGrad.addColorStop(0.25, "#a4bfcf");
  skyGrad.addColorStop(0.5, "#c4cbba");
  skyGrad.addColorStop(0.75, "#d4c8a0");
  skyGrad.addColorStop(1, "#d0bf8e");
  ctx.fillStyle = skyGrad;
  ctx.fillRect(0, 0, w, h);

  for (let y = 0; y < h * 0.45; y += 2) {
    for (let x = 0; x < w; x += 2) {
      const cloud = fbm(x * 0.006 + 2, y * 0.012 + 2, 5, 2.0, 0.48);
      if (cloud > 0.08) {
        const a = Math.min((cloud - 0.08) * 2.5, 0.75);
        ctx.fillStyle = `rgba(255,255,248,${a})`;
        ctx.fillRect(x, y, 2, 2);
      }
    }
  }

  const horizonY = h * 0.5;
  ctx.fillStyle = "#7a8a8e";
  ctx.beginPath();
  ctx.moveTo(0, horizonY + 12);
  for (let x = 0; x <= w; x += 2) {
    const mtn = fbm(x * 0.006 + 7, 2.0, 3, 2.0, 0.5);
    const peak = noise2D(x * 0.03 + 10, 2.8) * 8;
    ctx.lineTo(x, horizonY - 18 + mtn * 25 + peak);
  }
  ctx.lineTo(w, horizonY + 12);
  ctx.closePath();
  ctx.fill();

  ctx.fillStyle = "#4a6040";
  ctx.beginPath();
  ctx.moveTo(0, horizonY + 12);
  for (let x = 0; x <= w; x += 2) {
    const trees = fbm(x * 0.01 + 20, 1.0, 4, 2.0, 0.5);
    ctx.lineTo(x, horizonY - 2 + trees * 20);
  }
  ctx.lineTo(w, horizonY + 12);
  ctx.closePath();
  ctx.fill();

  const groundY = horizonY + 10;
  const groundGrad = ctx.createLinearGradient(0, groundY, 0, h);
  groundGrad.addColorStop(0, "#6a8048");
  groundGrad.addColorStop(0.15, "#5c7240");
  groundGrad.addColorStop(0.4, "#506838");
  groundGrad.addColorStop(0.7, "#485e32");
  groundGrad.addColorStop(1, "#3e5228");
  ctx.fillStyle = groundGrad;
  ctx.fillRect(0, groundY, w, h - groundY);

  for (let y = groundY; y < h; y += 2) {
    for (let x = 0; x < w; x += 2) {
      const g =
        noise2D(x * 0.05, y * 0.05) * 0.3 + noise2D(x * 0.14, y * 0.14) * 0.12;
      if (g > 0.1) {
        ctx.fillStyle = `rgba(70,100,35,${g * 0.6})`;
        ctx.fillRect(x, y, 2, 2);
      }
    }
  }

  const roadY = groundY + 5;
  ctx.fillStyle = "#9a9688";
  ctx.fillRect(0, roadY, w, 10);
  ctx.fillStyle = "rgba(210,190,60,0.3)";
  ctx.fillRect(0, roadY + 4.5, w, 1);

  drawPole(ctx, w * 0.72, horizonY - 95, horizonY + 18);
  drawPole(ctx, w * 0.18, horizonY - 50, horizonY + 10);
  drawWires(ctx, w, horizonY);

  const bloom1 = ctx.createRadialGradient(
    w * 0.45,
    h * 0.2,
    0,
    w * 0.45,
    h * 0.2,
    w * 0.6,
  );
  bloom1.addColorStop(0, "rgba(255,252,235,0.4)");
  bloom1.addColorStop(0.3, "rgba(255,248,220,0.25)");
  bloom1.addColorStop(0.6, "rgba(255,245,210,0.1)");
  bloom1.addColorStop(1, "transparent");
  ctx.fillStyle = bloom1;
  ctx.fillRect(0, 0, w, h);

  const bloom2 = ctx.createLinearGradient(0, horizonY - 30, 0, horizonY + 30);
  bloom2.addColorStop(0, "transparent");
  bloom2.addColorStop(0.4, "rgba(255,240,200,0.15)");
  bloom2.addColorStop(0.5, "rgba(255,235,190,0.2)");
  bloom2.addColorStop(0.6, "rgba(255,240,200,0.15)");
  bloom2.addColorStop(1, "transparent");
  ctx.fillStyle = bloom2;
  ctx.fillRect(0, 0, w, h);

  const bloom3 = ctx.createRadialGradient(
    w * 0.2,
    h * 0.15,
    0,
    w * 0.2,
    h * 0.15,
    w * 0.45,
  );
  bloom3.addColorStop(0, "rgba(255,250,240,0.2)");
  bloom3.addColorStop(0.5, "rgba(255,248,230,0.08)");
  bloom3.addColorStop(1, "transparent");
  ctx.fillStyle = bloom3;
  ctx.fillRect(0, 0, w, h);

  applyDither(ctx, w, h);
}

function drawPole(
  ctx: CanvasRenderingContext2D,
  x: number,
  topY: number,
  bottomY: number,
) {
  ctx.fillStyle = "#3a3832";
  ctx.fillRect(x - 1.5, topY, 3, bottomY - topY);
  ctx.fillRect(x - 24, topY + 6, 48, 2);
  ctx.fillRect(x - 18, topY + 18, 36, 1.5);
  ctx.fillStyle = "#555550";
  for (const o of [-22, -14, -6, 6, 14, 22]) {
    ctx.fillRect(x + o - 0.5, topY + 3, 1.5, 5);
  }
}

function drawWires(ctx: CanvasRenderingContext2D, w: number, horizonY: number) {
  ctx.strokeStyle = "rgba(40,38,34,0.5)";
  ctx.lineWidth = 0.7;
  const p1 = { x: w * 0.18, y: horizonY - 44 };
  const p2 = { x: w * 0.72, y: horizonY - 89 };
  for (const off of [-22, -14, -6, 6, 14, 22]) {
    ctx.beginPath();
    const x1 = p1.x + off * 0.5;
    const y1 = p1.y + 7;
    const x2 = p2.x + off;
    const y2 = p2.y + 7;
    const sag = 20 + Math.abs(off) * 0.4;
    const cpY = Math.max(y1, y2) + sag;
    ctx.moveTo(Math.max(0, x1 - 60), y1 + 4);
    ctx.lineTo(x1, y1);
    ctx.quadraticCurveTo((x1 + x2) / 2, cpY, x2, y2);
    ctx.lineTo(Math.min(w, x2 + 80), y2 - 5);
    ctx.stroke();
  }
}

function applyDither(ctx: CanvasRenderingContext2D, w: number, h: number) {
  const imgData = ctx.getImageData(0, 0, w, h);
  const d = imgData.data;
  const bayer = [0, 2, 3, 1];
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const i = (y * w + x) * 4;
      const t = (bayer[(y % 2) * 2 + (x % 2)] / 4) * 12 - 6;
      d[i] = Math.max(0, Math.min(255, d[i] + t));
      d[i + 1] = Math.max(0, Math.min(255, d[i + 1] + t));
      d[i + 2] = Math.max(0, Math.min(255, d[i + 2] + t));
    }
  }
  ctx.putImageData(imgData, 0, 0);
}

/* ── Background canvas component ── */
function BackgroundCanvas() {
  const ref = useRef<HTMLCanvasElement>(null);
  useEffect(() => {
    const c = ref.current;
    if (!c) return;
    const ctx = c.getContext("2d");
    if (!ctx) return;
    c.width = 560;
    c.height = 420;
    paintLandscape(ctx, 560, 420);
  }, []);
  return <canvas ref={ref} className="wrd-bg-canvas" />;
}

/* ── SVG knob (interactive) ── */
function Knob({
  label,
  normalizedValue,
  displayValue,
  unit,
  color = "light",
  onChange,
  defaultValue,
  bipolar,
}: {
  label: string;
  normalizedValue: number;
  displayValue: string;
  unit?: string;
  color?: "light" | "amber" | "cyan";
  onChange?: (normalized: number) => void;
  defaultValue?: number;
  bipolar?: boolean;
}) {
  const dragRef = useRef<{ startY: number; startValue: number } | null>(null);

  const handlePointerDown = useCallback(
    (e: React.PointerEvent) => {
      if (!onChange) return;
      (e.target as HTMLElement).setPointerCapture(e.pointerId);
      dragRef.current = { startY: e.clientY, startValue: normalizedValue };
      document.body.style.userSelect = "none";
      document.body.style.webkitUserSelect = "none";
    },
    [onChange, normalizedValue],
  );

  const handlePointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (!dragRef.current || !onChange) return;
      const delta = (dragRef.current.startY - e.clientY) / 150;
      onChange(Math.max(0, Math.min(1, dragRef.current.startValue + delta)));
    },
    [onChange],
  );

  const handlePointerUp = useCallback(() => {
    dragRef.current = null;
    document.body.style.userSelect = "";
    document.body.style.webkitUserSelect = "";
  }, []);

  const handleDoubleClick = useCallback(() => {
    if (!onChange || defaultValue === undefined) return;
    onChange(defaultValue);
  }, [onChange, defaultValue]);

  const norm = Math.max(0, Math.min(1, normalizedValue));
  const startA = -225;
  const sweep = norm * 270;
  const endA = startA + sweep;
  const trackEndA = startA + 270;

  const r = 15,
    cx = 18,
    cy = 18;
  const rad = (d: number) => (d * Math.PI) / 180;
  const px = (a: number) => cx + r * Math.cos(rad(a));
  const py = (a: number) => cy + r * Math.sin(rad(a));

  return (
    <div
      className="wrd-knob"
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
      onDoubleClick={handleDoubleClick}
      style={
        onChange ? { cursor: "ns-resize", touchAction: "none" } : undefined
      }
    >
      <svg width="36" height="36" viewBox="0 0 36 36" className="wrd-knob-svg">
        <path
          d={`M ${px(startA)} ${py(startA)} A ${r} ${r} 0 1 1 ${px(trackEndA)} ${py(trackEndA)}`}
          fill="none"
          stroke="rgba(0,0,0,0.1)"
          strokeWidth="1.5"
          strokeLinecap="round"
        />
        {bipolar ? (
          (() => {
            const midA = startA + 135;
            const arcSweep = Math.abs(endA - midA);
            if (arcSweep < 0.5) return null;
            const fromA = norm >= 0.5 ? midA : endA;
            const toA = norm >= 0.5 ? endA : midA;
            return (
              <path
                d={`M ${px(fromA)} ${py(fromA)} A ${r} ${r} 0 ${arcSweep > 180 ? 1 : 0} 1 ${px(toA)} ${py(toA)}`}
                fill="none"
                className={`wrd-arc--${color}`}
                strokeWidth="1.5"
                strokeLinecap="round"
              />
            );
          })()
        ) : (
          sweep > 0.5 && (
            <path
              d={`M ${px(startA)} ${py(startA)} A ${r} ${r} 0 ${sweep > 180 ? 1 : 0} 1 ${px(endA)} ${py(endA)}`}
              fill="none"
              className={`wrd-arc--${color}`}
              strokeWidth="1.5"
              strokeLinecap="round"
            />
          )
        )}
        <circle cx={cx} cy={cy} r="2.5" className={`wrd-dot--${color}`} />
        <line
          x1={cx + 6 * Math.cos(rad(endA))}
          y1={cy + 6 * Math.sin(rad(endA))}
          x2={cx + 11 * Math.cos(rad(endA))}
          y2={cy + 11 * Math.sin(rad(endA))}
          className={`wrd-tick--${color}`}
          strokeWidth="1.5"
          strokeLinecap="round"
        />
      </svg>
      <div className="wrd-knob-readout">
        <span className={`wrd-val--${color}`}>{displayValue}</span>
        {unit && <span className="wrd-unit">{unit}</span>}
      </div>
      <div className="wrd-knob-label">{label}</div>
    </div>
  );
}

/* ── Toggle ── */
function Toggle({
  label,
  on,
  onClick,
  disabled,
}: {
  label: string;
  on: boolean;
  onClick?: () => void;
  disabled?: boolean;
}) {
  return (
    <div
      className={`wrd-toggle ${disabled ? "wrd-toggle-disabled" : ""}`}
      onClick={disabled ? undefined : onClick}
      style={
        disabled
          ? { opacity: 0.3, pointerEvents: "none" }
          : onClick
            ? { cursor: "pointer" }
            : undefined
      }
    >
      <div className={`wrd-toggle-track ${on ? "wrd-toggle--on" : ""}`}>
        <div className="wrd-toggle-thumb" />
      </div>
      <div className="wrd-toggle-label">{label}</div>
    </div>
  );
}

/* ── Note picker (drag to change MIDI note) ── */
const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"] as const;

function midiNoteName(note: number): string {
  const n = Math.max(0, Math.min(127, Math.round(note)));
  const name = NOTE_NAMES[n % 12];
  const octave = Math.floor(n / 12) - 1;
  return `${name}${octave}`;
}

function NotePicker({
  label,
  value,
  onChange,
}: {
  label: string;
  value: number;
  onChange: (normalized: number) => void;
}) {
  const dragRef = useRef<{ startY: number; startNote: number } | null>(null);

  const handlePointerDown = useCallback(
    (e: React.PointerEvent) => {
      (e.target as HTMLElement).setPointerCapture(e.pointerId);
      dragRef.current = { startY: e.clientY, startNote: value };
      document.body.style.userSelect = "none";
      document.body.style.webkitUserSelect = "none";
    },
    [value],
  );

  const handlePointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (!dragRef.current) return;
      const delta = (dragRef.current.startY - e.clientY) / 6;
      const newNote = Math.max(
        0,
        Math.min(127, Math.round(dragRef.current.startNote + delta)),
      );
      onChange(newNote / 127);
    },
    [onChange],
  );

  const handlePointerUp = useCallback(() => {
    dragRef.current = null;
    document.body.style.userSelect = "";
    document.body.style.webkitUserSelect = "";
  }, []);

  const handleDoubleClick = useCallback(() => {
    onChange(60 / 127);
  }, [onChange]);

  return (
    <div
      className="wrd-note-picker"
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
      onDoubleClick={handleDoubleClick}
    >
      <div className="wrd-note-picker-box">{midiNoteName(value)}</div>
      <div className="wrd-knob-label">{label}</div>
    </div>
  );
}

/* ── Live waveform visualiser (incoming signal) ── */
function LiveWaveform({ sourceSelector }: { sourceSelector: React.ReactNode }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const bufferRef = useRef<number[]>(new Array(128).fill(0));

  useEffect(() => {
    const unsub = addWaveformListener((samples) => {
      bufferRef.current = samples;
    });
    return unsub;
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    let raf: number;
    const draw = () => {
      const w = canvas.width / dpr;
      const h = canvas.height / dpr;
      const samples = bufferRef.current;
      const midY = h / 2;

      ctx.clearRect(0, 0, w, h);

      // Grid lines
      ctx.strokeStyle = "rgba(60, 100, 160, 0.08)";
      ctx.lineWidth = 1;
      for (let y = 0; y < h; y += h / 6) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
      }
      for (let x = 0; x < w; x += w / 8) {
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, h);
        ctx.stroke();
      }

      // Center line
      ctx.strokeStyle = "rgba(60, 100, 160, 0.15)";
      ctx.beginPath();
      ctx.moveTo(0, midY);
      ctx.lineTo(w, midY);
      ctx.stroke();

      // Waveform fill
      ctx.beginPath();
      ctx.moveTo(0, midY);
      for (let i = 0; i < samples.length; i++) {
        const x = (i / (samples.length - 1)) * w;
        const y = midY - samples[i] * midY * 0.9;
        ctx.lineTo(x, y);
      }
      ctx.lineTo(w, midY);
      ctx.closePath();
      ctx.fillStyle = "rgba(60, 100, 160, 0.1)";
      ctx.fill();

      // Waveform glow
      ctx.beginPath();
      for (let i = 0; i < samples.length; i++) {
        const x = (i / (samples.length - 1)) * w;
        const y = midY - samples[i] * midY * 0.9;
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      }
      ctx.strokeStyle = "rgba(60, 100, 160, 0.15)";
      ctx.lineWidth = 3;
      ctx.stroke();

      // Waveform line
      ctx.beginPath();
      for (let i = 0; i < samples.length; i++) {
        const x = (i / (samples.length - 1)) * w;
        const y = midY - samples[i] * midY * 0.9;
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      }
      ctx.strokeStyle = "rgba(60, 100, 160, 0.7)";
      ctx.lineWidth = 1;
      ctx.stroke();

      raf = requestAnimationFrame(draw);
    };

    const dpr = window.devicePixelRatio || 2;
    const resizeCanvas = () => {
      const rect = canvas.getBoundingClientRect();
      canvas.width = rect.width * dpr;
      canvas.height = rect.height * dpr;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    };

    resizeCanvas();
    raf = requestAnimationFrame(draw);

    const observer = new ResizeObserver(resizeCanvas);
    observer.observe(canvas);

    return () => {
      cancelAnimationFrame(raf);
      observer.disconnect();
    };
  }, []);

  return (
    <div className="wrd-monitor-live">
      <div className="wrd-monitor-header">{sourceSelector}</div>
      <canvas ref={canvasRef} className="wrd-monitor-canvas" />
    </div>
  );
}

/* ── Audio file detection (used for picker + drag-drop) ── */
const AUDIO_EXTENSIONS = [
  ".wav",
  ".aif",
  ".aiff",
  ".flac",
  ".ogg",
  ".oga",
  ".mp3",
  ".m4a",
  ".aac",
  ".caf",
  ".wma",
  ".opus",
];
const AUDIO_FILE_ACCEPT = `audio/*,${AUDIO_EXTENSIONS.join(",")}`;

function isAudioFile(file: { name: string; type?: string }): boolean {
  if (file.type && file.type.startsWith("audio/")) return true;
  const lower = file.name.toLowerCase();
  return AUDIO_EXTENSIONS.some((ext) => lower.endsWith(ext));
}

/* ── Capture button ── */
type CaptureState = "idle" | "recording" | "processing" | "done";

function CaptureButton({
  state,
  onClick,
  onLoadFile,
  recordDisabled,
  recordDisabledReason,
}: {
  state: CaptureState;
  onClick: () => void;
  onLoadFile: (file: File) => void;
  recordDisabled?: boolean;
  recordDisabledReason?: string;
}) {
  const isRecording = state === "recording";
  const fileInputRef = useRef<HTMLInputElement>(null);

  const handleLoadClick = useCallback(() => {
    fileInputRef.current?.click();
  }, []);

  const handleFileChange = useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) => {
      const file = e.target.files?.[0];
      if (file && isAudioFile(file)) onLoadFile(file);
      e.target.value = "";
    },
    [onLoadFile],
  );

  // Allow STOP while recording even if a source has since become unavailable.
  const recordButtonDisabled =
    state === "processing" || (!isRecording && !!recordDisabled);

  return (
    <div className="wrd-capture-row">
      <div className="wrd-capture-line" />
      <button
        className={`wrd-capture-btn ${isRecording ? "wrd-capture-btn--recording" : ""}`}
        onClick={onClick}
        disabled={recordButtonDisabled}
        title={
          recordButtonDisabled && recordDisabledReason
            ? recordDisabledReason
            : undefined
        }
      >
        <div
          className={`wrd-capture-btn-ring ${isRecording ? "wrd-capture-btn-ring--active" : ""}`}
        >
          <div
            className={
              isRecording ? "wrd-capture-btn-stop" : "wrd-capture-btn-circle"
            }
          />
        </div>
        <span className="wrd-capture-btn-label">
          {state === "idle" && "REC"}
          {state === "recording" && "STOP"}
          {state === "processing" && "..."}
          {state === "done" && "REC"}
        </span>
      </button>
      <button
        className="wrd-load-btn"
        onClick={handleLoadClick}
        disabled={state === "recording" || state === "processing"}
        title="Load audio file (or drop one onto the panel)"
      >
        LOAD
      </button>
      <input
        ref={fileInputRef}
        type="file"
        accept={AUDIO_FILE_ACCEPT}
        style={{ display: "none" }}
        onChange={handleFileChange}
      />
      <div className="wrd-capture-line" />
    </div>
  );
}

/* ── Sample waveform viewer ── */
function SampleWaveform({
  state,
  sampleData,
  playing,
  playProgress,
  startNorm,
  lengthNorm,
  grainSizeNorm,
  grainPositions,
  onStartChange,
  onLengthChange,
  onTrim,
  children,
}: {
  state: CaptureState;
  sampleData: number[];
  playing: boolean;
  playProgress: number;
  startNorm: number;
  lengthNorm: number;
  grainSizeNorm: number;
  grainPositions: number[];
  onStartChange?: (v: number) => void;
  onLengthChange?: (v: number) => void;
  onTrim?: () => void;
  children?: React.ReactNode;
}) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);
  const dragRef = useRef<{
    mode: "left" | "right" | "body";
    startX: number;
    origStart: number;
    origLength: number;
  } | null>(null);

  // Region drag handlers
  const handleRegionPointerDown = useCallback(
    (e: React.PointerEvent, mode: "left" | "right" | "body") => {
      if (!onStartChange || !onLengthChange) return;
      e.stopPropagation();
      (e.target as HTMLElement).setPointerCapture(e.pointerId);
      dragRef.current = {
        mode,
        startX: e.clientX,
        origStart: startNorm,
        origLength: lengthNorm,
      };
      document.body.style.userSelect = "none";
      document.body.style.webkitUserSelect = "none";
    },
    [onStartChange, onLengthChange, startNorm, lengthNorm],
  );

  const handleRegionPointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (
        !dragRef.current ||
        !onStartChange ||
        !onLengthChange ||
        !wrapRef.current
      )
        return;
      const wrapWidth = wrapRef.current.getBoundingClientRect().width;
      const deltaNorm = (e.clientX - dragRef.current.startX) / wrapWidth;
      const { mode, origStart, origLength } = dragRef.current;

      if (mode === "left") {
        const newStart = Math.max(
          0,
          Math.min(origStart + origLength - 0.01, origStart + deltaNorm),
        );
        const newLength = origLength - (newStart - origStart);
        onStartChange(newStart);
        onLengthChange(Math.max(0.01, newLength));
      } else if (mode === "right") {
        const newLength = Math.max(
          0.01,
          Math.min(1 - origStart, origLength + deltaNorm),
        );
        onLengthChange(newLength);
      } else {
        // body drag — move both start and end together
        const maxStart = 1 - origLength;
        const newStart = Math.max(0, Math.min(maxStart, origStart + deltaNorm));
        onStartChange(newStart);
      }
    },
    [onStartChange, onLengthChange],
  );

  const handleRegionPointerUp = useCallback(() => {
    dragRef.current = null;
    document.body.style.userSelect = "";
    document.body.style.webkitUserSelect = "";
  }, []);

  useEffect(() => {
    if (state !== "done" || sampleData.length === 0) return;
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const dpr = window.devicePixelRatio || 2;
    const rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

    const w = rect.width;
    const h = rect.height;
    const midY = h / 2;

    ctx.clearRect(0, 0, w, h);

    // Grid
    ctx.strokeStyle = "rgba(60, 100, 160, 0.08)";
    ctx.lineWidth = 1;
    for (let y = 0; y < h; y += h / 4) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(w, y);
      ctx.stroke();
    }

    // Center line
    ctx.strokeStyle = "rgba(60, 100, 160, 0.15)";
    ctx.beginPath();
    ctx.moveTo(0, midY);
    ctx.lineTo(w, midY);
    ctx.stroke();

    // Waveform fill
    ctx.beginPath();
    ctx.moveTo(0, midY);
    for (let i = 0; i < sampleData.length; i++) {
      const x = (i / (sampleData.length - 1)) * w;
      const y = midY - sampleData[i] * midY * 0.85;
      ctx.lineTo(x, y);
    }
    ctx.lineTo(w, midY);
    ctx.closePath();
    ctx.fillStyle = "rgba(40, 140, 170, 0.12)";
    ctx.fill();

    // Waveform glow
    ctx.beginPath();
    for (let i = 0; i < sampleData.length; i++) {
      const x = (i / (sampleData.length - 1)) * w;
      const y = midY - sampleData[i] * midY * 0.85;
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    }
    ctx.strokeStyle = "rgba(40, 140, 170, 0.2)";
    ctx.lineWidth = 3;
    ctx.stroke();

    // Waveform line
    ctx.beginPath();
    for (let i = 0; i < sampleData.length; i++) {
      const x = (i / (sampleData.length - 1)) * w;
      const y = midY - sampleData[i] * midY * 0.85;
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    }
    ctx.strokeStyle = "rgba(40, 140, 170, 0.75)";
    ctx.lineWidth = 1;
    ctx.stroke();

    // Playhead — mapped within the selected region
    if (playing && playProgress > 0) {
      const effectiveLen = Math.min(lengthNorm, 1 - startNorm);
      const px = (startNorm + playProgress * effectiveLen) * w;
      ctx.strokeStyle = "rgba(60, 100, 160, 0.8)";
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(px, 0);
      ctx.lineTo(px, h);
      ctx.stroke();
    }

    // Grain size window — orange rectangle from left edge of selected region
    if (grainSizeNorm > 0 && playing) {
      const gsW = grainSizeNorm * w;
      const regionLeft = startNorm * w;
      ctx.fillStyle = "rgba(220, 160, 40, 0.18)";
      ctx.fillRect(regionLeft, 0, gsW, h);
      ctx.strokeStyle = "rgba(220, 160, 40, 0.5)";
      ctx.lineWidth = 1;
      ctx.strokeRect(regionLeft, 0, gsW, h);
    }

    // Per-grain cursors — lightweight lines for each active grain
    if (grainPositions.length > 0) {
      ctx.strokeStyle = "rgba(60, 100, 160, 0.3)";
      ctx.lineWidth = 1;
      for (const pos of grainPositions) {
        const gx = pos * w;
        ctx.beginPath();
        ctx.moveTo(gx, 0);
        ctx.lineTo(gx, h);
        ctx.stroke();
      }
    }
  }, [state, sampleData, playing, playProgress, startNorm, lengthNorm, grainSizeNorm, grainPositions]);

  const tagLabel =
    state === "recording"
      ? "RECORDING"
      : state === "processing"
        ? "PROCESSING"
        : state === "done"
          ? playing
            ? "PLAYING"
            : "LOADED"
          : "IDLE";
  const tagClass =
    state === "recording"
      ? "wrd-monitor-tag--rec"
      : state === "processing"
        ? "wrd-monitor-tag--proc"
        : playing
          ? "wrd-monitor-tag--play"
          : "";

  const effectiveLen = Math.min(lengthNorm, 1 - startNorm);

  return (
    <div className="wrd-glass wrd-monitor-sample">
      <div className="wrd-monitor-header">
        <span className="wrd-monitor-label">SAMPLE</span>
        <span className={`wrd-monitor-tag ${tagClass}`}>{tagLabel}</span>
        {state === "done" && onTrim && (
          <button
            className="wrd-trim-btn"
            onClick={onTrim}
            title="Trim sample to selected window"
            disabled={lengthNorm >= 0.999 && startNorm <= 0.001}
          >
            KEEP SELECTED
          </button>
        )}
      </div>
      {state === "idle" && (
        <div className="wrd-monitor-empty">
          <span>NO SAMPLE LOADED</span>
        </div>
      )}
      {state === "recording" && (
        <div className="wrd-monitor-empty wrd-monitor-recording">
          <div className="wrd-recording-bars">
            {Array.from({ length: 24 }, (_, i) => (
              <div
                key={i}
                className="wrd-recording-bar"
                style={{ animationDelay: `${i * 0.08}s` }}
              />
            ))}
          </div>
          <span className="wrd-recording-text">CAPTURING AUDIO...</span>
        </div>
      )}
      {state === "processing" && (
        <div className="wrd-monitor-empty wrd-monitor-processing">
          <span className="wrd-processing-text">PROCESSING SAMPLE...</span>
        </div>
      )}
      {state === "done" && (
        <div
          ref={wrapRef}
          className="wrd-sample-canvas-wrap"
          onPointerMove={handleRegionPointerMove}
          onPointerUp={handleRegionPointerUp}
        >
          <canvas
            ref={canvasRef}
            className="wrd-monitor-canvas wrd-sample-canvas"
          />
          <div
            className="wrd-region-overlay"
            style={{
              left: `${startNorm * 100}%`,
              width: `${effectiveLen * 100}%`,
            }}
          >
            <div
              className="wrd-region-line wrd-region-line--left wrd-region-handle"
              onPointerDown={(e) => handleRegionPointerDown(e, "left")}
            />
            <div
              className="wrd-region-body"
              onPointerDown={(e) => handleRegionPointerDown(e, "body")}
            />
            <div
              className="wrd-region-line wrd-region-line--right wrd-region-handle"
              onPointerDown={(e) => handleRegionPointerDown(e, "right")}
            />
          </div>
        </div>
      )}
      {children}
    </div>
  );
}

/* ── Source selector dropdown ── */
function SourceSelector({
  sources,
  selected,
  onSelect,
  permissionDenied,
}: {
  sources: AudioSourceInfo[];
  selected: string;
  onSelect: (bundleId: string) => void;
  permissionDenied: boolean;
}) {
  if (permissionDenied) {
    return (
      <div className="wrd-source-selector wrd-source-selector--error">
        <span className="wrd-source-icon">⚠</span>
        <span>Screen Recording permission required</span>
      </div>
    );
  }

  return (
    <div
      className={`wrd-source-selector ${!selected ? "wrd-source-selector--empty" : ""}`}
    >
      <span className="wrd-source-icon">◉</span>
      <select
        className="wrd-source-select"
        value={selected}
        onChange={(e) => onSelect(e.target.value)}
      >
        <option value="">Select source...</option>
        {sources.map((s) => (
          <option key={s.bundleId} value={s.bundleId}>
            {s.displayName}
          </option>
        ))}
      </select>
      <button
        className="wrd-source-refresh"
        onClick={() => refreshSources()}
        title="Refresh sources"
      >
        ↻
      </button>
    </div>
  );
}

/* ── Main app ── */
export default function App() {
  const speedParam = useJuceSlider("speed");
  const startParam = useJuceSlider("start");
  const lengthParam = useJuceSlider("length");
  const grainSizeParam = useJuceSlider("grain_size");
  const densityParam = useJuceSlider("density");
  const scatterParam = useJuceSlider("scatter");
  const pitchScatterParam = useJuceSlider("pitch_scatter");
  const envAttackParam = useJuceSlider("env_attack");
  const envDecayParam = useJuceSlider("env_decay");
  const envSustainParam = useJuceSlider("env_sustain");
  const envReleaseParam = useJuceSlider("env_release");
  const loopParam = useJuceToggle("loop");
  const reverseParam = useJuceToggle("reverse");
  const freezeParam = useJuceToggle("freeze");
  const driftParam = useJuceSlider("drift");
  const smearParam = useJuceSlider("smear");
  const speedLockPitchParam = useJuceToggle("speed_lock_pitch");

  // Hybrid sampler/synth mode (MIDI 1/4)
  const synthModeParam = useJuceToggle("synth_mode");
  const triggerModeParam = useJuceToggle("trigger_mode");
  const rootNoteParam = useJuceSlider("root_note");
  const densityTrackParam = useJuceSlider("density_track");
  const velocitySensParam = useJuceSlider("velocity_sens");
  const glideParam = useJuceSlider("glide");
  const fineTuneParam = useJuceSlider("fine_tune");
  const sampleGainParam = useJuceSlider("sample_gain");

  // Speed knob: non-linear UI curve for higher resolution near center (1x).
  // We map a "curved" 0-1 knob position through a power curve so that
  // movement near 12 o'clock (center) produces smaller parameter changes.
  const speedCurveExp = 3.0;
  // Normalised parameter value that corresponds to 1.0x speed
  // JUCE: norm = pow((1.0 - 0.1) / (10.0 - 0.1), skew) where skew=0.3
  const speedDefaultNorm = Math.pow(0.9 / 9.9, 0.3); // ≈ 0.4547
  // Parameter → knob position (expand center)
  const speedToKnob = (paramNorm: number): number => {
    const centered = (paramNorm - 0.5) * 2; // -1 to 1
    return (
      (Math.sign(centered) * Math.pow(Math.abs(centered), 1 / speedCurveExp)) /
        2 +
      0.5
    );
  };
  // Knob position → parameter (compress center)
  const knobToSpeed = (knobNorm: number): number => {
    const centered = (knobNorm - 0.5) * 2; // -1 to 1
    return (
      (Math.sign(centered) * Math.pow(Math.abs(centered), speedCurveExp)) / 2 +
      0.5
    );
  };

  const speedKnobPos = speedToKnob(speedParam.value);
  const speedDefaultKnob = speedToKnob(speedDefaultNorm);
  const handleSpeedChange = useCallback(
    (knobNorm: number) => {
      speedParam.set(knobToSpeed(knobNorm));
    },
    [speedParam],
  );

  // Convert normalised speed to actual value (matches JUCE NormalisableRange skew=0.3)
  // JUCE: actual = 0.1 + 9.9 * pow(normalised, 1/0.3)
  const speedActual = 0.1 + 9.9 * Math.pow(speedParam.value, 1.0 / 0.3);
  const speedDisplay = (speedActual * 100).toFixed(0);
  const startDisplay = (startParam.value * 100).toFixed(1);
  const lengthDisplay = (lengthParam.value * 100).toFixed(1);

  // Grain Size: JUCE range 0.01–0.5s with skew 0.5
  // actual = 0.01 + 0.49 * pow(norm, 1/0.5) = 0.01 + 0.49 * norm^2
  const grainSizeActual = 0.01 + 0.49 * Math.pow(grainSizeParam.value, 2.0);
  const grainSizeDisplay = (grainSizeActual * 1000).toFixed(0);

  // Density: JUCE range 1–32, integer steps
  const densityActual = 1 + 31 * densityParam.value;
  const densityDisplay = Math.round(densityActual).toString();

  // Scatter: 0–100%
  const scatterDisplay = (scatterParam.value * 100).toFixed(0);

  // Pitch: bipolar -12 to +12 semitones (normalised 0–1 maps to actual -1 to +1 octaves)
  const pitchActual = pitchScatterParam.value * 2 - 1;
  const pitchSemitones = pitchActual * 12;
  const pitchScatterDisplay = (pitchSemitones >= 0 ? "+" : "") + pitchSemitones.toFixed(0);

  // Drift: 0–100%
  const driftDisplay = (driftParam.value * 100).toFixed(0);

  // Smear: 0–100%
  const smearDisplay = (smearParam.value * 100).toFixed(0);

  // Voice ADSR: A/D/R as ms (range 1ms–5000ms, skew 0.5 → actual = 0.001 + 4.999 * norm²),
  // S as % level
  const envAttackActual  = 0.001 + 4.999 * Math.pow(envAttackParam.value,  2.0);
  const envDecayActual   = 0.001 + 4.999 * Math.pow(envDecayParam.value,   2.0);
  const envReleaseActual = 0.001 + 4.999 * Math.pow(envReleaseParam.value, 2.0);
  const envAttackDisplay  = (envAttackActual  * 1000).toFixed(0);
  const envDecayDisplay   = (envDecayActual   * 1000).toFixed(0);
  const envReleaseDisplay = (envReleaseActual * 1000).toFixed(0);
  const envSustainDisplay = (envSustainParam.value * 100).toFixed(0);

  // Synth params (MIDI 1/4)
  const rootNoteValue = Math.round(rootNoteParam.value * 127);
  const densityTrackDisplay = (densityTrackParam.value * 100).toFixed(0);
  const velocitySensDisplay = (velocitySensParam.value * 100).toFixed(0);
  const glideDisplay = (glideParam.value * 100).toFixed(0);
  // fine_tune: -100 to +100 cents (norm 0..1)
  const fineTuneCents = -100 + 200 * fineTuneParam.value;
  const fineTuneDisplay =
    (fineTuneCents >= 0 ? "+" : "") + fineTuneCents.toFixed(0);

  // sample_gain: 0–4 with skew 0.5 → actual = 4 * norm². Display as %.
  const sampleGainActual = 4 * Math.pow(sampleGainParam.value, 2.0);
  const sampleGainDisplay = (sampleGainActual * 100).toFixed(0);

  const synthOn = synthModeParam.value;

  const captureParam = useJuceToggle("capture");

  // Capture state
  const [captureState, setCaptureState] = useState<CaptureState>("idle");
  const [sampleData, setSampleData] = useState<number[]>([]);
  const captureTimerRef = useRef<number | null>(null);

  // Playback state — driven by C++ backend in plugin, local animation in dev
  const [playing, setPlaying] = useState(false);
  const [playProgress, setPlayProgress] = useState(0);
  const [sampleDuration, setSampleDuration] = useState(0);
  const playRafRef = useRef<number | null>(null);
  const playStartRef = useRef<number>(0);
  const playDurationMs = 2000; // sample duration in dev mode

  const stopPlayback = useCallback(() => {
    if (isInsidePlugin()) {
      bridgeStopPlayback();
    } else {
      setPlaying(false);
      setPlayProgress(0);
      setGrainPositions([]);
      if (playRafRef.current) {
        cancelAnimationFrame(playRafRef.current);
        playRafRef.current = null;
      }
    }
  }, []);

  const handlePlayStop = useCallback(() => {
    if (playing) {
      stopPlayback();
    } else if (captureState === "done" && sampleData.length > 0) {
      if (isInsidePlugin()) {
        startPlayback();
      } else {
        // Dev mode: animate locally with loop/reverse support
        setPlaying(true);
        playStartRef.current = performance.now();
        const tick = () => {
          const elapsed = performance.now() - playStartRef.current;
          let progress = elapsed / playDurationMs;
          if (reverseParam.value) {
            progress = 1 - progress;
          }
          if (progress >= 1 || progress <= 0) {
            if (loopParam.value) {
              playStartRef.current = performance.now();
              setPlayProgress(reverseParam.value ? 1 : 0);
            } else {
              setPlaying(false);
              setPlayProgress(0);
              playRafRef.current = null;
              return;
            }
          } else {
            setPlayProgress(Math.max(0, Math.min(1, progress)));
          }
          // Mock grain positions for dev mode
          const effectiveLen = Math.min(lengthParam.value, 1 - startParam.value);
          const headPos = startParam.value + Math.max(0, Math.min(1, progress)) * effectiveLen;
          const density = Math.round(1 + 31 * densityParam.value);
          const scatter = scatterParam.value;
          const mockGrains: number[] = [];
          for (let g = 0; g < density; g++) {
            const jitter = (Math.random() - 0.5) * scatter * effectiveLen;
            mockGrains.push(Math.max(0, Math.min(1, headPos + jitter)));
          }
          setGrainPositions(mockGrains);
          playRafRef.current = requestAnimationFrame(tick);
        };
        playRafRef.current = requestAnimationFrame(tick);
      }
    }
  }, [
    playing,
    captureState,
    sampleData,
    stopPlayback,
    loopParam.value,
    reverseParam.value,
  ]);

  // Listen for playback state from C++ backend
  useEffect(() => {
    const unsub = addPlaybackListener((state) => {
      setPlaying(state.playing);
      setPlayProgress(state.progress);
      if (state.duration > 0) setSampleDuration(state.duration);
    });
    return unsub;
  }, []);

  // Grain positions for visualization
  const [grainPositions, setGrainPositions] = useState<number[]>([]);

  useEffect(() => {
    const unsub = addGrainListener((positions) => {
      setGrainPositions(positions);
    });
    return unsub;
  }, []);

  // Load a file from disk (drag-drop or file picker). Same neutral-control
  // reset as recording so a fresh sample lands on a clean slate.
  const resetSampleControls = useCallback(() => {
    speedParam.set(speedDefaultNorm);
    startParam.set(0);
    lengthParam.set(1);
    grainSizeParam.set(0.4286);
    densityParam.set(0);
    scatterParam.set(0);
    pitchScatterParam.set(0.5);
    envAttackParam.set(Math.sqrt(0.004 / 4.999));
    envDecayParam.set(0);
    envSustainParam.set(1);
    envReleaseParam.set(Math.sqrt(0.049 / 4.999));
    freezeParam.set(false);
    driftParam.set(0);
    smearParam.set(0);
    speedLockPitchParam.set(true);
    loopParam.set(false);
    reverseParam.set(false);
    sampleGainParam.set(0.5);
  }, [
    speedParam,
    startParam,
    lengthParam,
    grainSizeParam,
    densityParam,
    scatterParam,
    pitchScatterParam,
    envAttackParam,
    envDecayParam,
    envSustainParam,
    envReleaseParam,
    freezeParam,
    driftParam,
    smearParam,
    speedLockPitchParam,
    loopParam,
    reverseParam,
    sampleGainParam,
    speedDefaultNorm,
  ]);

  const handleLoadFile = useCallback(
    async (file: File) => {
      if (!isAudioFile(file)) return;
      stopPlayback();
      if (captureParam.value) captureParam.set(false);
      resetSampleControls();
      setSampleData([]);
      setCaptureState("processing");
      try {
        const ok = await loadSampleFromFile(file);
        if (!ok) {
          // Decode failed — drop back to a usable state
          setCaptureState((prev) =>
            prev === "processing" ? "idle" : prev,
          );
        }
      } catch {
        setCaptureState((prev) => (prev === "processing" ? "idle" : prev));
      }
    },
    [stopPlayback, captureParam, resetSampleControls],
  );

  // Drag-and-drop on the workstation
  const [dragActive, setDragActive] = useState(false);
  const dragDepthRef = useRef(0);

  // During drag, browsers expose item MIME types but not filenames. Accept the
  // drag if any item looks like audio, OR if the type is unknown/empty (some
  // formats like .aif don't get a MIME from the OS) — final filtering happens
  // on drop, where we have access to the filename.
  const dragHasAudioCandidate = (dt: DataTransfer | null) => {
    if (!dt) return false;
    if (!Array.from(dt.types ?? []).includes("Files")) return false;
    const items = Array.from(dt.items ?? []);
    if (items.length === 0) return true; // no item info available — be permissive
    return items.some(
      (it) => it.kind === "file" && (it.type === "" || it.type.startsWith("audio/")),
    );
  };

  const handleDragEnter = useCallback((e: React.DragEvent) => {
    if (!dragHasAudioCandidate(e.dataTransfer)) return;
    e.preventDefault();
    dragDepthRef.current += 1;
    setDragActive(true);
  }, []);

  const handleDragLeave = useCallback((e: React.DragEvent) => {
    if (!Array.from(e.dataTransfer?.types ?? []).includes("Files")) return;
    e.preventDefault();
    dragDepthRef.current = Math.max(0, dragDepthRef.current - 1);
    if (dragDepthRef.current === 0) setDragActive(false);
  }, []);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    if (!dragHasAudioCandidate(e.dataTransfer)) return;
    e.preventDefault();
    e.dataTransfer.dropEffect = "copy";
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      if (!Array.from(e.dataTransfer?.types ?? []).includes("Files")) return;
      e.preventDefault();
      dragDepthRef.current = 0;
      setDragActive(false);
      const file = Array.from(e.dataTransfer.files ?? []).find(isAudioFile);
      if (file) handleLoadFile(file);
    },
    [handleLoadFile],
  );

  // Stop any active playback when a new recording starts, reset controls to neutral
  const handleCapture = useCallback(() => {
    if (captureState === "idle" || captureState === "done") {
      stopPlayback();
      speedParam.set(speedDefaultNorm);
      startParam.set(0);
      lengthParam.set(1);
      grainSizeParam.set(0.4286);
      densityParam.set(0);
      scatterParam.set(0);
      pitchScatterParam.set(0.5);
      envAttackParam.set(Math.sqrt(0.004 / 4.999));
      envDecayParam.set(0);
      envSustainParam.set(1);
      envReleaseParam.set(Math.sqrt(0.049 / 4.999));
      freezeParam.set(false);
      driftParam.set(0);
      smearParam.set(0);
      speedLockPitchParam.set(true);
      loopParam.set(false);
      reverseParam.set(false);
      sampleGainParam.set(0.5);
      setCaptureState("recording");
      setSampleData([]);
      captureParam.set(true);
    } else if (captureState === "recording") {
      captureParam.set(false);
      setCaptureState("processing");

      if (!isInsidePlugin()) {
        captureTimerRef.current = window.setTimeout(() => {
          const fakeSample: number[] = [];
          for (let i = 0; i < 512; i++) {
            const t = i / 512;
            fakeSample.push(
              Math.sin(t * Math.PI * 12) * 0.6 * (1 - t * 0.3) +
                noise2D(i * 0.04, 1.5) * 0.3 +
                noise2D(i * 0.12, 3.2) * 0.15,
            );
          }
          setSampleData(fakeSample);
          setCaptureState("done");
        }, 800);
      }
    }
  }, [
    captureState,
    captureParam,
    stopPlayback,
    speedParam,
    startParam,
    lengthParam,
    grainSizeParam,
    densityParam,
    scatterParam,
    pitchScatterParam,
    envAttackParam,
    envDecayParam,
    envSustainParam,
    envReleaseParam,
    freezeParam,
    driftParam,
    smearParam,
    loopParam,
    reverseParam,
    speedLockPitchParam,
    sampleGainParam,
    speedDefaultNorm,
  ]);

  // Listen for real sample data from the C++ backend
  useEffect(() => {
    const unsub = addSampleListener((samples) => {
      setSampleData(samples);
      setCaptureState("done");
    });
    // Ask C++ to resend any sample restored from saved plugin state.
    requestSampleSnapshot();
    return unsub;
  }, []);

  useEffect(() => {
    return () => {
      if (captureTimerRef.current) clearTimeout(captureTimerRef.current);
      if (playRafRef.current) cancelAnimationFrame(playRafRef.current);
    };
  }, []);

  // SCK source state
  const [sources, setSources] = useState<AudioSourceInfo[]>([]);
  const [selectedSource, setSelectedSource] = useState("");
  const [permissionDenied, setPermissionDenied] = useState(false);

  useEffect(() => {
    const unsub1 = addSourcesListener(setSources);
    const unsub2 = addStatusListener((status) => {
      setPermissionDenied(status.permissionDenied);
    });

    refreshSources();

    return () => {
      unsub1();
      unsub2();
    };
  }, []);

  const handleSourceSelect = useCallback((bundleId: string) => {
    setSelectedSource(bundleId);
    setSource(bundleId);
  }, []);

  const handleTrim = useCallback(() => {
    const s = startParam.value;
    const l = Math.min(lengthParam.value, 1 - s);
    if (l <= 0) return;

    if (isInsidePlugin()) {
      stopPlayback();
      trimSample(s, l);
    } else {
      // Dev mode: trim the local sample buffer
      stopPlayback();
      const total = sampleData.length;
      const startIdx = Math.floor(s * total);
      const endIdx = Math.min(total, Math.floor((s + l) * total));
      const trimmed = sampleData.slice(startIdx, endIdx);
      setSampleData(trimmed);
      startParam.set(0);
      lengthParam.set(1);
    }
  }, [
    startParam,
    lengthParam,
    sampleData,
    stopPlayback,
  ]);

  return (
    <div className="wrd-frame">
      {/* Full-frame background */}
      <BackgroundCanvas />
      <div className="wrd-bloom-overlay" />

      {/* CRT overlay effects */}
      <div className="wrd-scanlines" />
      <div className="wrd-vignette" />

      {/* UI layer — glass panels over landscape */}
      <div className="wrd-ui">
        {/* Top bar */}
        <div className="wrd-glass wrd-topbar">
          <span className="wrd-topbar-title">wired memory</span>
          <span className="wrd-topbar-sub">SAMPLER</span>
          <span className="wrd-topbar-spacer" />
          {!isInsidePlugin() && <span className="wrd-topbar-sub">DEV</span>}
          <span className="wrd-topbar-status">待機中</span>
          <span className="wrd-topbar-time">--:--</span>
        </div>

        {/* Centered workstation */}
        <div
          className={`wrd-workstation ${dragActive ? "wrd-workstation--drag" : ""}`}
          onDragEnter={handleDragEnter}
          onDragLeave={handleDragLeave}
          onDragOver={handleDragOver}
          onDrop={handleDrop}
        >
          <div className="wrd-monitor-section">
            <div className="wrd-viewfinder">
              <div className="wrd-corner wrd-corner--tl" />
              <div className="wrd-corner wrd-corner--tr" />
              <div className="wrd-corner wrd-corner--bl" />
              <div className="wrd-corner wrd-corner--br" />
            </div>
            <LiveWaveform
              sourceSelector={
                <SourceSelector
                  sources={sources}
                  selected={selectedSource}
                  onSelect={handleSourceSelect}
                  permissionDenied={permissionDenied}
                />
              }
            />
            <CaptureButton
              state={captureState}
              onClick={handleCapture}
              onLoadFile={handleLoadFile}
              recordDisabled={isInsidePlugin() && !selectedSource}
              recordDisabledReason={
                permissionDenied
                  ? "Screen Recording permission required to record"
                  : "Select an audio source to record"
              }
            />
            {dragActive && (
              <div className="wrd-drop-overlay">
                <div className="wrd-drop-overlay-text">DROP AUDIO FILE</div>
              </div>
            )}
            <SampleWaveform
              state={captureState}
              sampleData={sampleData}
              playing={playing}
              playProgress={playProgress}
              startNorm={startParam.value}
              lengthNorm={lengthParam.value}
              grainSizeNorm={
                (sampleDuration > 0 || !isInsidePlugin())
                  ? grainSizeActual / (sampleDuration > 0 ? sampleDuration : playDurationMs / 1000)
                  : 0
              }
              grainPositions={grainPositions}
              onStartChange={startParam.set}
              onLengthChange={lengthParam.set}
              onTrim={handleTrim}
            >
              <div className="wrd-sample-controls">
                <button
                  className={`wrd-play-btn ${playing ? "wrd-play-btn--active" : ""}`}
                  onClick={handlePlayStop}
                  disabled={captureState !== "done" || synthOn}
                  style={
                    synthOn
                      ? { opacity: 0.35, pointerEvents: "none" }
                      : undefined
                  }
                >
                  {playing ? "⏹" : "▶"}
                </button>
                <div className="wrd-sample-knobs">
                  <Knob
                    label="SPEED"
                    normalizedValue={speedKnobPos}
                    displayValue={speedDisplay}
                    unit="%"
                    color="amber"
                    onChange={handleSpeedChange}
                    defaultValue={speedDefaultKnob}
                  />
                  <Toggle
                    label="VARI"
                    on={speedLockPitchParam.value}
                    onClick={() => speedLockPitchParam.set(!speedLockPitchParam.value)}
                  />
                  <Knob
                    label="START"
                    normalizedValue={startParam.value}
                    displayValue={startDisplay}
                    unit="%"
                    color="cyan"
                    onChange={startParam.set}
                    defaultValue={0}
                  />
                  <Knob
                    label="LENGTH"
                    normalizedValue={lengthParam.value}
                    displayValue={lengthDisplay}
                    unit="%"
                    color="light"
                    onChange={lengthParam.set}
                    defaultValue={1}
                  />
                  <Knob
                    label="VOL"
                    normalizedValue={sampleGainParam.value}
                    displayValue={sampleGainDisplay}
                    unit="%"
                    color="amber"
                    onChange={sampleGainParam.set}
                    defaultValue={0.5}
                  />
                </div>
                <div className="wrd-sample-knobs wrd-grain-knobs">
                  <span className="wrd-grain-label">GRAIN</span>
                  <Knob
                    label="SIZE"
                    normalizedValue={grainSizeParam.value}
                    displayValue={grainSizeDisplay}
                    unit="ms"
                    color="cyan"
                    onChange={grainSizeParam.set}
                    defaultValue={0.4286}
                  />
                  <Knob
                    label="DENSITY"
                    normalizedValue={densityParam.value}
                    displayValue={densityDisplay}
                    color="amber"
                    onChange={densityParam.set}
                    defaultValue={0}
                  />
                  <Knob
                    label="SCATTER"
                    normalizedValue={scatterParam.value}
                    displayValue={scatterDisplay}
                    unit="%"
                    color="light"
                    onChange={scatterParam.set}
                    defaultValue={0}
                  />
                  <div className={synthOn ? "wrd-knob-disabled" : undefined}>
                    <Knob
                      label="PITCH"
                      normalizedValue={pitchScatterParam.value}
                      displayValue={pitchScatterDisplay}
                      unit="st"
                      color="cyan"
                      onChange={pitchScatterParam.set}
                      defaultValue={0.5}
                      bipolar
                    />
                  </div>
                </div>
                <div className="wrd-sample-knobs wrd-grain-knobs">
                  <span className="wrd-grain-label">ENV</span>
                  <Knob
                    label="ATTACK"
                    normalizedValue={envAttackParam.value}
                    displayValue={envAttackDisplay}
                    unit="ms"
                    color="cyan"
                    onChange={envAttackParam.set}
                    defaultValue={Math.sqrt(0.004 / 4.999)}
                  />
                  <Knob
                    label="DECAY"
                    normalizedValue={envDecayParam.value}
                    displayValue={envDecayDisplay}
                    unit="ms"
                    color="amber"
                    onChange={envDecayParam.set}
                    defaultValue={0}
                  />
                  <Knob
                    label="SUSTAIN"
                    normalizedValue={envSustainParam.value}
                    displayValue={envSustainDisplay}
                    unit="%"
                    color="light"
                    onChange={envSustainParam.set}
                    defaultValue={1}
                  />
                  <Knob
                    label="RELEASE"
                    normalizedValue={envReleaseParam.value}
                    displayValue={envReleaseDisplay}
                    unit="ms"
                    color="cyan"
                    onChange={envReleaseParam.set}
                    defaultValue={Math.sqrt(0.049 / 4.999)}
                  />
                </div>
                <div className="wrd-sample-knobs wrd-spectral-knobs">
                  <span className="wrd-grain-label">SPECTRAL</span>
                  <div
                    className={`wrd-freeze-btn ${freezeParam.value ? "wrd-freeze-btn--active" : ""}`}
                    onClick={() => freezeParam.set(!freezeParam.value)}
                  >
                    <div className="wrd-freeze-indicator" />
                    <span className="wrd-freeze-label">FREEZE</span>
                  </div>
                  <Knob
                    label="DRIFT"
                    normalizedValue={driftParam.value}
                    displayValue={driftDisplay}
                    unit="%"
                    color="cyan"
                    onChange={driftParam.set}
                    defaultValue={0}
                  />
                  <Knob
                    label="SMEAR"
                    normalizedValue={smearParam.value}
                    displayValue={smearDisplay}
                    unit="%"
                    color="amber"
                    onChange={smearParam.set}
                    defaultValue={0}
                  />
                </div>
                <div className="wrd-sample-toggles">
                  <Toggle
                    label="LOOP"
                    on={loopParam.value}
                    onClick={() => loopParam.set(!loopParam.value)}
                  />
                  <Toggle
                    label="REV"
                    on={reverseParam.value}
                    onClick={() => reverseParam.set(!reverseParam.value)}
                  />
                  <Toggle
                    label="GATE"
                    on={triggerModeParam.value}
                    onClick={() => triggerModeParam.set(!triggerModeParam.value)}
                    disabled={synthOn}
                  />
                </div>
              </div>
            </SampleWaveform>

            {/* SYNTH panel — MIDI hybrid mode (1/4 scaffold; DSP unwired) */}
            <div className="wrd-glass wrd-monitor-sample wrd-synth-panel">
              <div className="wrd-sample-controls">
                <span className="wrd-grain-label">SYNTH</span>
                <Toggle
                  label="SYNTH"
                  on={synthModeParam.value}
                  onClick={() => synthModeParam.set(!synthModeParam.value)}
                />
                <NotePicker
                  label="ROOT"
                  value={rootNoteValue}
                  onChange={rootNoteParam.set}
                />
                <div className={`wrd-sample-knobs ${!synthOn ? "wrd-knob-disabled" : ""}`}>
                  <Knob
                    label="TRACK"
                    normalizedValue={densityTrackParam.value}
                    displayValue={densityTrackDisplay}
                    unit="%"
                    color="cyan"
                    onChange={densityTrackParam.set}
                    defaultValue={0}
                  />
                  <Knob
                    label="VEL"
                    normalizedValue={velocitySensParam.value}
                    displayValue={velocitySensDisplay}
                    unit="%"
                    color="amber"
                    onChange={velocitySensParam.set}
                    defaultValue={0.7}
                  />
                  <Knob
                    label="GLIDE"
                    normalizedValue={glideParam.value}
                    displayValue={glideDisplay}
                    unit="%"
                    color="cyan"
                    onChange={glideParam.set}
                    defaultValue={0}
                  />
                  <Knob
                    label="FINE"
                    normalizedValue={fineTuneParam.value}
                    displayValue={fineTuneDisplay}
                    unit="ct"
                    color="amber"
                    onChange={fineTuneParam.set}
                    defaultValue={0.5}
                    bipolar
                  />
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* Bottom bar */}
        <div className="wrd-glass wrd-bottombar">
          <span>560 × 420</span>
          <span className="wrd-bottombar-brand">
            TACHIBANA LABS ── PRESENT DAY, PRESENT TIME
          </span>
          <span>▸ STEREO</span>
        </div>
      </div>
    </div>
  );
}
