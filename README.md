# meh-synth-wave

A standalone wavetable synthesizer for macOS. No DAW, no plugins — just a terminal-driven synth you can play with a MIDI controller or your keyboard.

Built from scratch in C++ on top of CoreAudio and CoreMIDI, with a focus on production-quality DSP and real-time safety.

## What it does

- **4 wavetable oscillators** with mip-mapped lookup, frame scanning, and FM phase modulation
- **FM synthesis** — any oscillator can modulate any other (including self-feedback)
- **Dual filters** — State Variable Filter (LP/BP/HP) and a Moog-style 4-pole Ladder, with a configurable signal chain
- **Modulation matrix** — 16 routable slots connecting envelopes, LFOs, velocity, noise, and mod wheel to filter, pitch, scan position, FM depth, and more
- **3 envelopes** (amp, filter, mod) and **3 LFOs** with modulatable rate and amplitude
- **Saturation** stage in the signal chain
- **MIDI input** via CoreMIDI — plug in a controller and play
- **Keyboard input** — play notes from the terminal without any hardware

## Building

```bash
make debug     # Debug build
make release   # Optimized (-O3 -ffast-math)
make clean
```

Requires macOS and Xcode command line tools.

## Running

```bash
./main
```

Connects to your default CoreAudio output and any attached MIDI devices automatically.

## Architecture

Four libraries under `libs/` handle the platform layer — CoreAudio I/O, CoreMIDI + keyboard capture, a MIDI/param event protocol, and a platform-independent DSP primitives library. The synth engine in `src/synth/` sits on top of those and knows nothing about audio hardware.

Voice state is laid out SoA for SIMD-friendliness. Modulation runs a block-rate pre-pass then interpolates per-sample inside the voice loop. No heap allocations on the audio thread.
