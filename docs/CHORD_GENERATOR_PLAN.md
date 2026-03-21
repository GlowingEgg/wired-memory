# MIDI Chord Generator — Design Plan

A MIDI-in / MIDI-out plugin that transforms single-note input into contextually harmonised chords within a chosen key. Emphasis on being **immediately playable without theory knowledge**, while offering a **deep complexity dimension** for jazz voicings.

---

## Core Design Constraint

> Someone who has never studied music theory should be able to sit down, pick a key, and reliably play every chord in that key by pressing a different single note — with one hand, in time, expressively.

That constraint rules out anything that requires memorising intervals or holding modifier chords. The complexity axis must feel *gestural*, not intellectual.

---

## Interaction Model Options

Two approaches under consideration.

---

### Model A — Scale Remap + Pitch Bend Complexity (recommended primary)

**How it works:**

The plugin remaps all 12 chromatic input notes to the 7 scale degrees of the chosen key. The entire chromatic keyboard becomes a "key-locked" keyboard where every note is guaranteed in-key and triggers a chord.

**Mapping (example: C major):**

| Input note (any octave) | Chord triggered |
|---|---|
| C | I — C maj |
| C# | II — D min |
| D | III — E min |
| D# | IV — F maj |
| E | V — G dom |
| F | VI — A min |
| F# | VII — B dim |
| G | I (up an octave) — C maj |
| … | (wraps every 7 semitones) |

Black keys are not "wrong" — they just jump to the next scale degree. The keyboard feels like it has only 7 unique destinations, infinitely tiled.

**Complexity axis — Pitch Bend:**

Pitch bend is an ideal complexity controller because it's physically accessible, returns to zero on release, and doesn't require a free hand.

| Bend position | Chord quality |
|---|---|
| Center (0) | Triad (3 notes) |
| Slightly up | 7th chord |
| Mid up | 9th chord |
| High up | 11th or 13th |
| Slightly down | Suspended 2 or 4 version |
| Low down | Altered dominant (b9, #11, b13) |

Bend down = tension/colour variants. Bend up = stacking extensions. This maps naturally onto the body's intuition: "reaching up" for more complex, "pulling back" for simpler/tense.

**Why it's good:** Single-handed playable. Pitch bend returns to zero automatically so you always land on triads by default. The complexity sweep is continuous and expressive.

---

### Model B — Chord Stacking via Simultaneous Notes

**How it works:**

The same scale-degree remapping applies. The *number of keys held at once* determines chord complexity. The lowest held note is always treated as the chord root; any additional notes held simultaneously are interpreted as "add complexity" gestures rather than independent chord triggers.

| Keys held | Chord quality |
|---|---|
| 1 key | Triad rooted on that scale degree |
| 2 keys | 7th chord on the lowest key's degree |
| 3 keys | 9th chord on the lowest key's degree |
| 4+ keys | Full jazz voicing (11th/13th with idiomatic colour tones) |

The upper notes in a multi-key grip are *not* treated as separate chord triggers — they are consumed as complexity signals and swallowed from the output. Only the chords generated from the lowest note are emitted.

**Example in C major:**
- Hold C alone → C major triad
- Hold C + D → Cmaj7
- Hold C + D + E → Cmaj9
- Hold C + D + E + F → Cmaj9(#11) or Cmaj13

The interval or scale-degree distance between the held notes could optionally influence *which* flavour of extension is added (e.g. holding a note a 4th above the root nudges toward a sus4 quality), but a simpler first pass just uses note count.

**Why it's good:** Entirely hands-on and physical — squeezing more keys feels like squeezing more harmonic weight. No mod wheel, no pitch bend, no extra controllers. Works well for players who naturally cluster their hand when they want "more". Each grip size has a consistent, reproducible result.

**Interesting tension:** This mode *does* require two hands to reach higher complexity levels, but the gesture is intuitive in a way that the dual-zone split (Model C, removed) wasn't — you're not operating a hidden modifier system, you're just pressing *more of the same thing*.

**Caveats:**
- Requires defining a clear "root detection" rule (lowest note wins, or first-struck note wins).
- If a player accidentally brushes a second key their chord unexpectedly jumps up a complexity tier — needs a short grace window or a "minimum simultaneous duration" threshold before counting a multi-key grip.
- Less suited to fast melodic runs where accidental polyphony is common.

---

## Shared Features Across All Models

### Key & Scale Selector
- Root note (C–B) and scale type
- Scales to support: Major, Natural Minor, Harmonic Minor, Melodic Minor, Dorian, Mixolydian, Lydian, Phrygian, Whole Tone, Diminished (octatonic)

### Voicing Engine
Controls *how* the notes of a chord are arranged in pitch space:

- **Spread**: Close position → Shell voicing → Open/Jazz spread
- **Register**: Root octave offset — keeps generated chords in a musical register regardless of input octave
- **Inversion**: Force root position, first inversion, second, auto-voice-lead
- **Voice Leading**: When enabled, moves individual voices as little as possible between chord changes (smooth jazz movement)

### Output Options
- **Velocity passthrough** vs **fixed output velocity**
- **Note duration**: Pass MIDI note-off immediately, or hold chords for a set duration (good for staccato playing → full legato chords)
- **Arpeggiator mode**: Output chord tones as a rhythmic arpeggio instead of simultaneously
- **Mono/Poly latch**: Hold last chord until next one is played

### MIDI Learn
All parameters mappable to any CC. Complexity can be wired to any mod source (mod wheel, expression pedal, breath controller, etc.) not just pitch bend.

---

## Suggested Chord Vocabulary (by degree)

The plugin should know which extensions are idiomatic for each scale degree rather than mechanically stacking thirds. Example for major key:

| Degree | Triad | 7th | 9th | Jazz full |
|---|---|---|---|---|
| I | maj | maj7 | maj9 | maj9 (#11) |
| II | min | min7 | min9 | min11 |
| III | min | min7 | min9 | min11 |
| IV | maj | maj7 | maj9 | maj9 |
| V | dom | dom7 | dom9 | dom13 or dom7(b9,#11) |
| VI | min | min7 | min9 | min11 |
| VII | dim | min7b5 | min9b5 | min11b5 |

This gives the plugin musical intelligence rather than just stacking dumb intervals.

---

## Name Options

All names lean into the **film noir, smoke-filled jazz club** atmosphere.

| Name | Rationale |
|---|---|
| **After Hours** | Quintessential late-night jazz club phrase. Evocative, slightly illicit. Directly implies the setting. |
| **The Changes** | Jazz insider slang for chord progressions ("playing the changes"). Perfectly on-brand for a chord plugin. Has a knowing, initiated quality. |
| **Smoke & Ivory** | Piano keys + cigarette smoke. Most visually evocative of the noir atmosphere. Could be shortened to just **Ivory**. |
| **Nocturn** | Deliberate near-misspelling of "Nocturne" (a piece written for night). Clean, elegant, slightly unsettling. Nods to Chopin without being classical-stuffy. |
| **Voicing** | Literal double-meaning: chord voicing (music theory) + the act of giving voice to something (noir monologue quality). Sophisticated, minimal. |
| **The Cipher** | Noir mystery + a musical "key" as a cipher/code for the uninitiated. Slightly cryptic. |
| **House Keys** | Triple meaning: home key of a scale, piano keys, the keys to the speakeasy. Playful. |

**Top pick:** **After Hours** — immediately conjures the right image, has zero ambiguity, looks great on a plugin faceplate.

**Runner-up:** **The Changes** — best insider-baseball name for musicians who will get the joke.

---

## Art Direction

### Visual Concept: *"The Piano Bar at 2 AM"*

The player is a lone pianist in a nearly empty club. The only light is warm amber spilling from a single bulb above the piano. Smoke drifts through the beam. Rain streaks the window behind them.

### Colour Palette

| Role | Colour | Notes |
|---|---|---|
| Background | `#0d0b0f` — near-black with a warm violet undertone | Feels like a dark room, not a computer screen |
| Surface / panels | `#1a1620` — deep indigo-black | Slightly lighter, creates depth |
| Accent / glow | `#c8922a` — warm amber-gold | Stage light, cigarette ember, brass fixture |
| Secondary accent | `#5c8fa8` — desaturated steel blue | Neon sign outside, wet pavement reflection |
| Text primary | `#e8dcc8` — aged ivory/cream | Piano keys, old paper |
| Text muted | `#7a6e5c` — warm grey | Secondary labels |
| Danger / altered | `#b04040` — dark crimson | Altered chords, tension indicators |

### Typography

- **Display / plugin name**: Geometric art deco sans-serif. Think the lettering on a 1940s marquee. Bold, condensed, slightly wide letterforms. Suggestions: *Josefin Sans*, *Bebas Neue*, or a custom stencil feel.
- **Labels / knobs**: Small monospace or a clean geometric sans, all caps, tracked out. Clinical contrast to the display face.
- **Numeric readouts**: Mono, amber on dark, like an old LED display.

### UI Layout Concept

```
┌─────────────────────────────────────────────────────────┐
│                    AFTER HOURS                          │  ← Art deco title, amber glow
│  ─────────────────────────────────────────────────────  │
│                                                         │
│   KEY   [C ▾]   SCALE  [Major ▾]   MODE  [Remap ▾]    │  ← Top bar, clean
│                                                         │
│  ╔════════════════════════════════════════════════╗     │
│  ║                                                ║     │
│  ║    [ COMPLEXITY ]          [ SPREAD ]          ║     │
│  ║       ╭────╮                  ╭────╮            ║     │
│  ║       │    │                  │    │            ║     │  ← Large Bakelite-style knobs
│  ║       ╰────╯                  ╰────╯            ║     │
│  ║     0 ————●——————— 100      CLOSE ——●—— OPEN   ║     │
│  ║                                                ║     │
│  ║    [ REGISTER ]             [ VOICE LEAD ]     ║     │
│  ║       ╭────╮                  ╭─ ON ─╮         ║     │
│  ║       │    │                  │      │          ║     │
│  ║       ╰────╯                  ╰──────╯          ║     │
│  ╚════════════════════════════════════════════════╝     │
│                                                         │
│   [I]  [II]  [III]  [IV]  [V]  [VI]  [VII]            │  ← Chord degree display, lights up
│    C    Dm    Em     F     G    Am    Bdim              │     when chord is playing
│                                                         │
│  ░░░░░ COMPLEXITY SOURCE: [PITCH BEND ▾] ░░░░░░░░░░░  │  ← Subtle footer
└─────────────────────────────────────────────────────────┘
```

### Texture & Detail

- **Knobs**: Dark phenolic Bakelite feel — deep brown-black with amber indicator dots. Not chrome/modern.
- **Panel texture**: Subtle brushed dark metal or worn leather grain on the main surface. Almost invisible but tactile-feeling.
- **Chord degree indicators**: Seven small amber "bulbs" at the bottom that light up (bright amber glow with bloom) when each chord is active. Like the lights on an old organ.
- **Smoke effect**: A very subtle, slow-drifting particle layer or blurred gradient in the background — barely perceptible, just suggests atmosphere. Optional/toggle for performance.
- **Logo treatment**: The plugin name in a vertical condensed stencil font, slightly distressed. Could be rendered as if it were on a marquee sign or a painted door.

### Loading Screen / Splash (optional)
A moody illustration: hands on piano keys seen from above, one cigarette resting on the ashtray, amber light from above. Monochromatic with single amber accent. Could be a simple vector illustration or a stylised photograph treatment.

---

## Implementation Roadmap (suggested phases)

1. **Phase 1 — Core engine**: Scale degree remapping + triad output. Model A (pitch bend complexity) only. No UI beyond a key selector.
2. **Phase 2 — Voicing intelligence**: Full chord vocabulary table per degree, spread/inversion controls, voice leading.
3. **Phase 3 — Full UI**: Full art direction applied via React/Vite.
4. **Phase 4 — Model B & C**: Velocity layers and dual-zone split as switchable modes.
5. **Phase 5 — Polish**: Arpeggiator, MIDI learn, chord display, animated indicators.

---

## Open Design Questions

- **Chord display**: Should the UI show the *name* of the chord being played (e.g. "Cmaj9")? Useful for learning, slightly clutters the dark aesthetic. Perhaps toggleable.
- **Multi-note input in Model A**: If the user plays two notes simultaneously in remap mode, do we (a) play two separate chords, (b) treat the lower note as root and upper as extension, or (c) ignore the second note?
- **Output octave range**: Should the plugin maintain the octave of the played note, or always produce chords in a fixed register (e.g. always middle-register piano voicings)?
- **Black key behaviour in remap mode**: Option to keep black keys silent (effectively 7-note keyboard) vs always remapping them. Silent mode could feel like a "white-keys-only" guide.
