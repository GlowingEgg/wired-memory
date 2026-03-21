# After Hours — MIDI Chord Generator Design Document

A MIDI-in / MIDI-out plugin that transforms single-note and multi-note input into contextually harmonised chords within a chosen key. The plugin is named **After Hours**. Emphasis on being **immediately playable without theory knowledge**, while offering a **deep complexity dimension** for jazz voicings.

---

## Core Design Constraint

> Someone who has never studied music theory should be able to sit down, pick a key, and reliably play every chord in that key by pressing a different single note — with one hand, in time, expressively.

The complexity axis must feel *gestural*, not intellectual. Squeezing more keys should feel like squeezing more harmonic weight.

---

## Interaction Model — Chord Stacking via Simultaneous Notes

### Scale Degree Remapping

All 12 chromatic input notes are remapped to the 7 scale degrees of the chosen key. Every note is guaranteed in-key and triggers a chord. Black keys are not "wrong" — they advance to the next scale degree.

**Mapping example (C major):**

| Input note | Chord triggered |
|---|---|
| C | I — C maj |
| C# | II — D min |
| D | III — E min |
| D# | IV — F maj |
| E | V — G dom |
| F | VI — A min |
| F# | VII — B dim |
| G | I (octave up) — C maj |
| … | wraps every 7 semitones |

### Complexity via Note Count

The *number of keys held simultaneously* determines chord complexity. The first struck note is the root; any additional notes held simultaneously are consumed as complexity signals and suppressed from the output. Only the generated chord is emitted.

| Keys held | Chord quality |
|---|---|
| 1 | Triad rooted on first-struck degree |
| 2 | 7th chord |
| 3 | 9th chord |
| 4+ | Full jazz voicing (11th/13th with idiomatic colour tones) |

**Example in C major:**
- Hold C alone → C major triad
- Hold C + D → Cmaj7
- Hold C + D + E → Cmaj9
- Hold C + D + E + F → Cmaj9(#11) or Cmaj13

### Root Detection

A toggle switch on the UI selects between two root detection modes:

- **First Struck** *(default)*: Whichever note is physically pressed first is the root, regardless of its pitch. Strum up from C and C is always the root even if you're holding higher notes.
- **Lowest Note**: The lowest-pitched held note is always the root, regardless of order struck. More predictable for players who tend to reach down for bass notes.

### Octave & Register Behaviour

The first struck note determines both the **scale degree** (chord identity) and the **octave anchor** of the generated chord. Playing C2 produces a lower-register C major triad than playing C3. The plugin follows the player's register directly — no fixed-register override.

When stacking notes to add complexity, the **octave spread of the held notes influences the voicing spread** of the generated chord. If the root is C3 and the stacked notes reach up to D4, the chord tones are voiced across that pitch range rather than clustered in close position. This means a physically wide grip produces an open, spread voicing naturally — a tight grip produces a close voicing. The relationship between the player's hand shape and the sound is direct.

Concretely: the generated chord tones are distributed so that the lowest output note is near the root's octave and the highest output note is near the highest stacked note's octave, with inner voices filling the space according to the voicing engine settings.

---

## Key & Scale Selector

- Root note: C–B
- Scale types: Major, Natural Minor, Harmonic Minor, Melodic Minor, Dorian, Mixolydian, Lydian, Phrygian, Whole Tone, Diminished (octatonic)

---

## Voicing Engine

Controls how the notes of a chord are arranged in pitch space. Exposed as user-facing controls on the UI.

- **Spread**: Close position → Shell voicing → Open/Jazz spread. Interacts with the octave-spread behaviour described above — the spread knob sets the *minimum* spread tendency; the player's hand position can open it further.
- **Inversion**: Root position, first inversion, second inversion, or auto voice-lead (moves voices as little as possible between consecutive chords for smooth jazz movement).
- **Voice Leading** toggle: When on, the voicing engine resolves each chord change with minimal voice movement. When off, chords are voiced independently each time.

---

## Output Passthrough

All MIDI data that is not part of a stacking gesture passes through the plugin unmodified. Note-offs are forwarded immediately. Velocity, aftertouch, CC messages, and pitch bend all pass through untouched. No arpeggiator, no latch, no duration manipulation — players who want those behaviours can add them outside the plugin in their DAW.

---

## Chord Vocabulary (by degree)

The plugin uses idiomatic extensions per scale degree rather than mechanically stacking thirds.

**Major key:**

| Degree | Triad | 7th | 9th | Jazz full |
|---|---|---|---|---|
| I | maj | maj7 | maj9 | maj9 (#11) |
| II | min | min7 | min9 | min11 |
| III | min | min7 | min9 | min11 |
| IV | maj | maj7 | maj9 | maj9 |
| V | dom | dom7 | dom9 | dom13 or dom7(b9,#11) |
| VI | min | min7 | min9 | min11 |
| VII | dim | min7b5 | min9b5 | min11b5 |

The same idiomatic table approach applies for each supported scale type.

---

## UI Design

### Visual Concept: *"The Piano Bar at 2 AM"*

A lone pianist in a nearly empty club. Warm amber light from a single bulb above the piano. Smoke drifts through the beam. Rain on the window.

### Colour Palette

| Role | Colour | Notes |
|---|---|---|
| Background | `#0d0b0f` — near-black, warm violet undertone | Dark room, not a computer screen |
| Surface / panels | `#1a1620` — deep indigo-black | Slightly lighter, creates depth |
| Accent / glow | `#c8922a` — warm amber-gold | Stage light, cigarette ember, brass fixture |
| Secondary accent | `#5c8fa8` — desaturated steel blue | Neon sign, wet pavement reflection |
| Text primary | `#e8dcc8` — aged ivory/cream | Piano keys, old paper |
| Text muted | `#7a6e5c` — warm grey | Secondary labels |
| Danger / altered | `#b04040` — dark crimson | Altered chords, tension indicators |

### Typography

- **Display / plugin name**: Geometric art deco sans-serif — bold, condensed, 1940s marquee feel. Suggestions: *Josefin Sans*, *Bebas Neue*, or a stencil variant.
- **Labels / knobs**: Clean geometric sans, all caps, tracked out. Clinical contrast to the display face.
- **Chord name readout**: Mono, amber on dark, like an old LED display.

### UI Layout

```
┌─────────────────────────────────────────────────────────────┐
│                       AFTER HOURS                           │  ← Art deco, amber glow
│  ───────────────────────────────────────────────────────    │
│                                                             │
│   KEY  [C ▾]    SCALE  [Major ▾]    ROOT  [FIRST ▾]        │  ← Top bar
│                                                             │
│  ╔══════════════════════════════════════════════════╗       │
│  ║                                                  ║       │
│  ║    [ SPREAD ]               [ INVERSION ]        ║       │
│  ║       ╭────╮                   ╭────╮             ║       │  ← Bakelite knobs
│  ║       │    │                   │    │             ║       │
│  ║       ╰────╯                   ╰────╯             ║       │
│  ║    CLOSE ——●—— OPEN         ROOT ——●—— AUTO       ║       │
│  ║                                                  ║       │
│  ║                  [ VOICE LEAD ]                  ║       │
│  ║                   ╭── ON ──╮                     ║       │
│  ║                   │        │                     ║       │
│  ║                   ╰────────╯                     ║       │
│  ╚══════════════════════════════════════════════════╝       │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                   C m a j 9                         │   │  ← Chord name, large amber mono
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  C  C# D  D# E  F  F# G  G# A  A# B  C             │   │  ← Visual keyboard
│  │  ██     ██        ██     ██     ██     ██            │   │     lit notes glow amber
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│   [I]  [II]  [III]  [IV]  [V]  [VI]  [VII]               │  ← Degree bulbs, amber on active
│    C    Dm    Em     F     G    Am    Bdim                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**Chord name display:** The current chord name (e.g. "Cmaj9", "G7(b9)") is shown prominently in a large amber monospace readout in the center of the UI. Updates immediately on each new chord.

**Visual keyboard:** A one-octave keyboard diagram below the chord name illuminates the exact MIDI notes being output, not the input notes. The player sees which pitches the plugin is generating, in amber. This doubles as a learning tool — players start to recognise chord shapes on the keys.

**Degree bulbs:** Seven small amber "bulbs" at the bottom (I through VII) illuminate when each scale degree is active, labelled with the actual chord name for the current key (C, Dm, Em, F, G, Am, Bdim). These are always visible so the player can see all seven chords available to them at a glance.

### Texture & Detail

- **Knobs**: Dark phenolic Bakelite — deep brown-black with amber indicator dots. Not chrome.
- **Panel texture**: Subtle brushed dark metal or worn leather grain. Almost invisible but tactile-feeling.
- **Smoke effect**: Very slow-drifting semi-transparent gradient in the background. Barely perceptible. Optional toggle for performance.
- **Logo treatment**: Plugin name in condensed stencil, slightly distressed — as if painted on a speakeasy door.

### Loading Screen / Splash (optional)
Hands on piano keys seen from above, one cigarette on the ashtray, amber light from above. Monochromatic with single amber accent. Simple vector illustration or stylised photograph treatment.

---

## Implementation Roadmap

1. **Phase 1 — Core engine**: Scale degree remapping + chord stacking logic. Triad output only. Key and scale selector. No UI beyond a stub.
2. **Phase 2 — Voicing intelligence**: Full chord vocabulary table per degree and scale type. Spread, inversion, and voice leading controls. Octave-spread-to-voicing logic.
3. **Phase 3 — UI**: Full art direction in React/Vite. Chord name readout, visual keyboard, degree bulbs, all controls.
4. **Phase 4 — Polish**: Root detection toggle, smoke effect, splash screen.

