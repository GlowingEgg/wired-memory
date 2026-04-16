import { useEffect, useRef, useState, useCallback } from "react";
import { createNoise2D } from "simplex-noise";
import { useJuceSlider, useJuceToggle } from "./lib/useJuceParam";
import {
  isInsidePlugin,
  addSourcesListener,
  addStatusListener,
  addWaveformListener,
  addSampleListener,
  setSource,
  refreshSources,
  type AudioSourceInfo,
} from "./plugin-bridge";
import "./App.css";

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

function fbm(x: number, y: number, octaves: number, lac = 2.0, gain = 0.5): number {
  let value = 0, amp = 1, freq = 1, max = 0;
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
      const g = noise2D(x * 0.05, y * 0.05) * 0.3 + noise2D(x * 0.14, y * 0.14) * 0.12;
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

  const bloom1 = ctx.createRadialGradient(w * 0.45, h * 0.2, 0, w * 0.45, h * 0.2, w * 0.6);
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

  const bloom3 = ctx.createRadialGradient(w * 0.2, h * 0.15, 0, w * 0.2, h * 0.15, w * 0.45);
  bloom3.addColorStop(0, "rgba(255,250,240,0.2)");
  bloom3.addColorStop(0.5, "rgba(255,248,230,0.08)");
  bloom3.addColorStop(1, "transparent");
  ctx.fillStyle = bloom3;
  ctx.fillRect(0, 0, w, h);

  applyDither(ctx, w, h);
}

function drawPole(ctx: CanvasRenderingContext2D, x: number, topY: number, bottomY: number) {
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

/* ── SVG knob ── */
function Knob({ label, value, unit, color = "light" }: {
  label: string; value: string; unit?: string; color?: "light" | "amber" | "cyan"
}) {
  const norm = Math.max(0, Math.min(1, (parseFloat(value) || 0) / 100));
  const startA = -225;
  const sweep = norm * 270;
  const endA = startA + sweep;
  const trackEndA = startA + 270;

  const r = 15, cx = 18, cy = 18;
  const rad = (d: number) => (d * Math.PI) / 180;
  const px = (a: number) => cx + r * Math.cos(rad(a));
  const py = (a: number) => cy + r * Math.sin(rad(a));

  return (
    <div className="wrd-knob">
      <svg width="36" height="36" viewBox="0 0 36 36" className="wrd-knob-svg">
        <path
          d={`M ${px(startA)} ${py(startA)} A ${r} ${r} 0 1 1 ${px(trackEndA)} ${py(trackEndA)}`}
          fill="none" stroke="rgba(0,0,0,0.1)" strokeWidth="1.5" strokeLinecap="round"
        />
        {sweep > 0.5 && (
          <path
            d={`M ${px(startA)} ${py(startA)} A ${r} ${r} 0 ${sweep > 180 ? 1 : 0} 1 ${px(endA)} ${py(endA)}`}
            fill="none" className={`wrd-arc--${color}`} strokeWidth="1.5" strokeLinecap="round"
          />
        )}
        <circle cx={cx} cy={cy} r="2.5" className={`wrd-dot--${color}`} />
        <line
          x1={cx + 6 * Math.cos(rad(endA))} y1={cy + 6 * Math.sin(rad(endA))}
          x2={cx + 11 * Math.cos(rad(endA))} y2={cy + 11 * Math.sin(rad(endA))}
          className={`wrd-tick--${color}`} strokeWidth="1.5" strokeLinecap="round"
        />
      </svg>
      <div className="wrd-knob-readout">
        <span className={`wrd-val--${color}`}>{value}</span>
        {unit && <span className="wrd-unit">{unit}</span>}
      </div>
      <div className="wrd-knob-label">{label}</div>
    </div>
  );
}

/* ── Toggle ── */
function Toggle({ label, on, onClick }: { label: string; on: boolean; onClick?: () => void }) {
  return (
    <div className="wrd-toggle" onClick={onClick} style={onClick ? { cursor: "pointer" } : undefined}>
      <div className={`wrd-toggle-track ${on ? "wrd-toggle--on" : ""}`}>
        <div className="wrd-toggle-thumb" />
      </div>
      <div className="wrd-toggle-label">{label}</div>
    </div>
  );
}

/* ── Data readout ── */
function Readout({ value, label }: { value: string; label: string }) {
  return (
    <div className="wrd-readout">
      <div className="wrd-readout-val">{value}</div>
      <div className="wrd-readout-label">{label}</div>
    </div>
  );
}

/* ── Waveform scope ── */
function Scope() {
  const pts: string[] = [];
  for (let i = 0; i <= 100; i++) {
    const x = (i / 100) * 180;
    const y = 16 + Math.sin((i / 100) * Math.PI * 5) * 10
      + Math.sin((i / 100) * Math.PI * 13) * 3
      + noise2D(i * 0.07, 4.4) * 4;
    pts.push(`${i === 0 ? "M" : "L"} ${x.toFixed(1)} ${y.toFixed(1)}`);
  }
  return (
    <div className="wrd-scope">
      <svg viewBox="0 0 180 32" preserveAspectRatio="none">
        <path d={pts.join(" ")} fill="none" stroke="rgba(60,100,160,0.12)" strokeWidth="3" />
        <path d={pts.join(" ")} fill="none" stroke="rgba(60,100,160,0.6)" strokeWidth="0.8" />
      </svg>
    </div>
  );
}

/* ── Live waveform visualiser (incoming signal) ── */
function LiveWaveform() {
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
    <div className="wrd-glass wrd-monitor-live">
      <div className="wrd-monitor-header">
        <span className="wrd-monitor-label">INPUT</span>
        <span className="wrd-monitor-tag">LIVE</span>
      </div>
      <canvas ref={canvasRef} className="wrd-monitor-canvas" />
    </div>
  );
}

/* ── Capture button ── */
type CaptureState = "idle" | "recording" | "processing" | "done";

function CaptureButton({ state, onClick }: { state: CaptureState; onClick: () => void }) {
  const isRecording = state === "recording";
  return (
    <div className="wrd-capture-row">
      <div className="wrd-capture-line" />
      <button
        className={`wrd-capture-btn ${isRecording ? "wrd-capture-btn--recording" : ""}`}
        onClick={onClick}
        disabled={state === "processing"}
      >
        <div className={`wrd-capture-btn-ring ${isRecording ? "wrd-capture-btn-ring--active" : ""}`}>
          <div className={isRecording ? "wrd-capture-btn-stop" : "wrd-capture-btn-circle"} />
        </div>
        <span className="wrd-capture-btn-label">
          {state === "idle" && "REC"}
          {state === "recording" && "STOP"}
          {state === "processing" && "..."}
          {state === "done" && "REC"}
        </span>
      </button>
      <div className="wrd-capture-line" />
    </div>
  );
}

/* ── Sample waveform viewer ── */
function SampleWaveform({ state, sampleData }: { state: CaptureState; sampleData: number[] }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

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
      ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
    }

    // Center line
    ctx.strokeStyle = "rgba(60, 100, 160, 0.15)";
    ctx.beginPath(); ctx.moveTo(0, midY); ctx.lineTo(w, midY); ctx.stroke();

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
  }, [state, sampleData]);

  const tagLabel = state === "recording" ? "RECORDING" : state === "processing" ? "PROCESSING" : state === "done" ? "LOADED" : "IDLE";
  const tagClass = state === "recording" ? "wrd-monitor-tag--rec" : state === "processing" ? "wrd-monitor-tag--proc" : "";

  return (
    <div className="wrd-glass wrd-monitor-sample">
      <div className="wrd-monitor-header">
        <span className="wrd-monitor-label">SAMPLE</span>
        <span className={`wrd-monitor-tag ${tagClass}`}>{tagLabel}</span>
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
              <div key={i} className="wrd-recording-bar" style={{ animationDelay: `${i * 0.08}s` }} />
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
        <canvas ref={canvasRef} className="wrd-monitor-canvas wrd-sample-canvas" />
      )}
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
    <div className="wrd-source-selector">
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
      <button className="wrd-source-refresh" onClick={() => refreshSources()} title="Refresh sources">
        ↻
      </button>
    </div>
  );
}

/* ── Main app ── */
export default function App() {
  const gainParam = useJuceSlider("gain");
  const gainPct = (gainParam.value * 100).toFixed(0);

  const captureParam = useJuceToggle("capture");

  // Capture state
  const [captureState, setCaptureState] = useState<CaptureState>("idle");
  const [sampleData, setSampleData] = useState<number[]>([]);
  const captureTimerRef = useRef<number | null>(null);

  const handleCapture = useCallback(() => {
    if (captureState === "idle" || captureState === "done") {
      // Start recording
      setCaptureState("recording");
      setSampleData([]);
      captureParam.set(true);
    } else if (captureState === "recording") {
      // Stop recording — sample data arrives via sck:sample event
      captureParam.set(false);
      setCaptureState("processing");

      if (!isInsidePlugin()) {
        // Dev mode: generate fake sample data after a short delay
        captureTimerRef.current = window.setTimeout(() => {
          const fakeSample: number[] = [];
          for (let i = 0; i < 512; i++) {
            const t = i / 512;
            fakeSample.push(
              Math.sin(t * Math.PI * 12) * 0.6 * (1 - t * 0.3)
              + noise2D(i * 0.04, 1.5) * 0.3
              + noise2D(i * 0.12, 3.2) * 0.15
            );
          }
          setSampleData(fakeSample);
          setCaptureState("done");
        }, 800);
      }
    }
  }, [captureState, captureParam]);

  // Listen for real sample data from the C++ backend
  useEffect(() => {
    const unsub = addSampleListener((samples) => {
      setSampleData(samples);
      setCaptureState("done");
    });
    return unsub;
  }, []);

  useEffect(() => {
    return () => {
      if (captureTimerRef.current) clearTimeout(captureTimerRef.current);
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

    // Request initial sources list
    refreshSources();

    return () => { unsub1(); unsub2(); };
  }, []);

  const handleSourceSelect = useCallback((bundleId: string) => {
    setSelectedSource(bundleId);
    setSource(bundleId);
  }, []);

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
          {!isInsidePlugin() && (
            <span className="wrd-topbar-sub">DEV</span>
          )}
          <span className="wrd-topbar-status">待機中</span>
          <span className="wrd-topbar-time">--:--</span>
        </div>

        {/* Monitor section — live input + sample workbench */}
        <div className="wrd-monitor-section">
          <div className="wrd-viewfinder">
            <div className="wrd-corner wrd-corner--tl" />
            <div className="wrd-corner wrd-corner--tr" />
            <div className="wrd-corner wrd-corner--bl" />
            <div className="wrd-corner wrd-corner--br" />
            <div className="wrd-vf-data">
              <span className={captureState === "recording" ? "wrd-rec wrd-rec--active" : "wrd-rec"}>REC ●</span>
              <span>SCStream</span>
              <span>{selectedSource ? sources.find(s => s.bundleId === selectedSource)?.displayName ?? "..." : "---"}</span>
            </div>
          </div>
          <LiveWaveform />
          <CaptureButton state={captureState} onClick={handleCapture} />
          <SampleWaveform state={captureState} sampleData={sampleData} />
        </div>

        {/* Source selector */}
        <div className="wrd-glass wrd-source-bar">
          <SourceSelector
            sources={sources}
            selected={selectedSource}
            onSelect={handleSourceSelect}
            permissionDenied={permissionDenied}
          />
        </div>

        {/* Control panel */}
        <div className="wrd-glass wrd-panel">
          {/* Row 1: readouts + scope + transport */}
          <div className="wrd-panel-top">
            <div className="wrd-readouts">
              <Readout value={`${gainPct}%`} label="GAIN" />
              <Readout value="44.1k" label="RATE" />
              <Readout value="0:00" label="POS" />
            </div>
            <Scope />
            <div className="wrd-transport">
              <div className="wrd-transport-btn">⏮</div>
              <div className="wrd-transport-btn">▶</div>
              <div className="wrd-transport-btn wrd-transport-btn--active">●</div>
              <div className="wrd-transport-btn">⏹</div>
            </div>
          </div>

          {/* Row 2: knobs + toggles */}
          <div className="wrd-panel-mid">
            <div className="wrd-knobs">
              <Knob label="GAIN" value={gainPct} unit="%" color="light" />
              <Knob label="SPEED" value="100" unit="%" color="amber" />
              <Knob label="START" value="0" unit="ms" color="cyan" />
              <Knob label="LENGTH" value="100" unit="%" color="light" />
              <Knob label="LOOP" value="0" unit="" color="amber" />
            </div>

            <div className="wrd-toggles">
              <Toggle label="LOOP" on={false} />
              <Toggle label="REVERSE" on={false} />
              <Toggle label="MONITOR" on={false} />
            </div>
          </div>
        </div>

        {/* Bottom bar */}
        <div className="wrd-glass wrd-bottombar">
          <span>560 × 420</span>
          <span className="wrd-bottombar-brand">TACHIBANA LABS ── PRESENT DAY, PRESENT TIME</span>
          <span>▸ STEREO</span>
        </div>
      </div>
    </div>
  );
}
