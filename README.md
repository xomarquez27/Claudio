# Claudio

It is not Claude's Hispanic cousin.
Claudio is a tiny command-line music player written in C++.
It stands for CL Audio (command line audio).

It parses simple text-based music notation, generates a WAV file, and plays it using macOS `afplay`.

## Features

- Tempo control
- Time signature parsing
- Persistent note durations
- Per-note duration overrides
- Rests
- Sharp and flat notes
- Text file playback

---

# Usage

## Command Line

```bash
claudio t-180 4/4 -8 e5 e5 r e5 r c5 -4 e5 g5 r g4 r
```

## Text File

```bash
claudio song.txt
```

or

```bash
claudio -f song.txt
```

---

# Syntax

## Tempo

```text
t-180
```

Sets tempo to 180 BPM.

---

## Time Signature

```text
4/4
3/4
6/8
```

Currently parsed but not yet enforced.

---

## Global Duration

```text
-1   whole note
-2   half note
-4   quarter note
-8   eighth note
-16  sixteenth note
-32  thirty-second note
```

Example:

```text
-8 c5 d5 e5
```

All following notes use eighth-note duration until changed.

---

## Notes

```text
c4
e5
g#3
bb4
```

Format:

```text
[pitch][octave]
```

Examples:

- `c4`
- `f#5`
- `bb3`

---

## Rests

```text
r
```

Uses the current duration.

Example:

```text
-4 c5 r g5
```

---

## Per-Note Duration Override

Use `/` to override duration for a single note or rest.

Example:

```text
-4 e5 e5 r/8 e5/16
```

This means:

- quarter note `e5`
- quarter note `e5`
- eighth-note rest
- sixteenth-note `e5`

After the override, duration returns to the current global duration.

---

# Example Song File

```text
# Simple melody

t-180
4/4

-8
e5 e5 r e5 r c5

-4
e5 g5 r g4 r
```

---

# Current Audio Engine

Claudio currently:

1. Parses notes into events
2. Generates a sine-wave WAV file
3. Plays audio using:

```bash
afplay
```

The generated WAV file is written to:

```text
/tmp/claudio.wav
```

---

# Build

## Xcode

Open the project and run.

If no arguments are provided, Claudio automatically plays a built-in demo phrase.

---

# Future Ideas

- Dotted notes
- Ties
- Chords
- ADSR envelopes
- Multiple waveforms
- Polyphony
- MIDI export
- Real-time playback
- Looping
- Instruments
