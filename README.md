# Scale Scaler

A CLAP plugin that scales MIDI note numbers between different equal temperament tuning systems.

## Purpose

Scale Scaler is a note effect plugin designed to be placed before a synthesizer in the plugin chain. It intercepts MIDI notes coming from the piano roll and scales them according to configurable parameters.

The primary use case is to play a MIDI sequence intended for one tuning system (e.g., 12-tone equal temperament / 12edo) and have the note numbers scaled up or down for a synthesizer tuned to a different system (e.g., 24edo). This allows interval sizes to be proportionally scaled.

## Parameters

The plugin presents 4 DAW-automatable parameters:

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| Input Note Count | int | 12 | 1-128 | Number of notes in the input scale (e.g., 12 for standard 12edo) |
| Output Note Count | int | 12 | 1-128 | Number of notes in the output scale (e.g., 24 for 24edo) |
| Base MIDI Note | int | 60 | 0-127 | The center note around which scaling occurs (60 = Middle C) |
| Nudge | float | 0.0 | -0.5 to 0.5 | Fine adjustment added after scaling |

## Formula

The plugin processes MIDI notes using the following formula:

```
output = round(Base MIDI Note + Nudge + ((input - Base MIDI Note) × (Output Note Count / Input Note Count)))
```

Results are clamped to the valid MIDI note range of 0-127.

### Example

With default settings (Input=12, Output=12, Base=60, Nudge=0):
- An input note of 60 (Middle C) outputs 60
- An input note of 72 (C one octave up) outputs 72

With Input=12, Output=24, Base=60, Nudge=0:
- An input note of 60 outputs 60
- An input note of 72 (12 semitones up) outputs 84 (24 quarter-tones up)

## Building

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler

### Clone with Submodules

This project uses Git submodules for the CLAP SDK. Clone with:

```bash
git clone --recursive https://github.com/yourusername/scale-scaler-clap.git
```

Or if already cloned:

```bash
git submodule update --init --recursive
```

### macOS / Linux

```bash
./build.sh
```

### Windows

Open a "Developer Command Prompt for Visual Studio" and run:

```batch
build.bat
```

### Manual CMake Build

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Installation

After building, the plugin is automatically installed to the platform-specific location:

- **macOS**: `~/Library/Audio/Plug-Ins/CLAP/`
- **Linux**: `~/.clap/`
- **Windows**: `%LOCALAPPDATA%\Programs\Common\CLAP\`

Or copy the `.clap` bundle/directory from the `install` folder to your DAW's CLAP plugin folder.

## Usage

1. Insert Scale Scaler before your synthesizer in the plugin chain
2. Configure the Input and Output note counts to match your tuning systems
3. Set the Base MIDI Note to the note that should remain unchanged
4. Adjust Nudge if needed for fine tuning
5. Play your MIDI sequence - notes will be automatically scaled

## Technical Details

- **Format**: CLAP (CLever Audio Plugin)
- **Type**: Note Effect (no audio processing)
- **MIDI Support**: Note On, Note Off, and pass-through for all other events
- **Platforms**: macOS, Linux, Windows

## License

MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.