# MIDI Note Translator

A CLAP plugin that translates MIDI note numbers between different sized tuning systems.

## Purpose

MIDI Note Translator is a note effect plugin designed to be placed before a synthesizer in the plugin chain. It intercepts MIDI notes coming from the piano roll and translates them according to configurable parameters.

The primary use case is to play a MIDI sequence intended for one tuning system (e.g., 12-tone equal temperament / 12edo) and have the note numbers translated up or down for a synthesizer tuned to a different system (e.g., 24edo). This allows interval sizes to be proportionally scaled.

## Parameters

The plugin presents 4 DAW-automatable parameters:

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| Input Note Count | int | 12 | 1-128 | Number of notes in the input scale (e.g., 12 for standard 12edo) |
| Output Note Count | int | 12 | 1-128 | Number of notes in the output scale (e.g., 24 for 24edo) |
| Base MIDI Note | int | 60 | 0-127 | The center note around which translation occurs (60 = Middle C) |
| Nudge | float | 0.0 | -0.5 to 0.5 | Fine adjustment added after translation |

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

## Releases

There are no releases yet. CI builds, tests, and validates every push; pushing a `v*` tag creates a release with binaries for macOS, Linux, and Windows. See [`.github/CI.md`](.github/CI.md) for details.

## Building from Source

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler

### Clone with Submodules

This project uses Git submodules for the CLAP SDK. Clone with:

```bash
git clone --recursive https://github.com/SeanArchibald/midi-note-translator.git
```

Or if already cloned:

```bash
git submodule update --init --recursive
```

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install
cmake --build build --config Release
cmake --install build --config Release
```

On Windows, run this from a "Developer Command Prompt for Visual Studio"; the default generator works.

Omit `-DCMAKE_INSTALL_PREFIX=install` to install directly into your user CLAP folder instead of the repository's `install/` directory.

### Run the Tests

```bash
cd build && ctest --output-on-failure
```

## Installation

`cmake --install build` installs to the user CLAP folder:

- **macOS**: `~/Library/Audio/Plug-Ins/CLAP/`
- **Linux**: `~/.clap/`
- **Windows**: `%LOCALAPPDATA%\Programs\CLAP\`

Passing `-DCMAKE_INSTALL_PREFIX=...` overrides that location. With `-DCMAKE_INSTALL_PREFIX=install`, the plugin is staged in the repository's `install/` folder; copy `MidiNoteTranslator.clap` from there to your DAW's CLAP folder.

macOS builds are ad-hoc signed but not notarized. If macOS quarantines a downloaded build, remove the attribute:

```bash
xattr -dr com.apple.quarantine /path/to/MidiNoteTranslator.clap
```

## Usage

1. Insert MIDI Note Translator before your synthesizer in the plugin chain
2. Configure the Input and Output note counts to match your tuning systems
3. Set the Base MIDI Note to the note that should remain unchanged
4. Adjust Nudge if needed for fine tuning
5. Play your MIDI sequence - notes will be translated automatically

## Technical Details

- **Format**: CLAP (CLever Audio Plugin)
- **Type**: Note Effect
- **MIDI Support**: Note On, Note Off, note expression, choke, and end events; other events pass through
- **Platforms**: macOS (universal), Linux, Windows

## License

MIDI Note Translator is released under the MIT License. See [LICENSE.md](LICENSE.md).
Third-party components are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Creating a Release

To create a new release with pre-built binaries:

### Option 1: Push a Git Tag

```bash
git tag v1.0.0
git push origin v1.0.0
```

This will automatically trigger the GitHub Actions workflow to build all platforms and create a release.

### Option 2: Manual Trigger

1. Go to the **Actions** tab in the GitHub repository
2. Select the **Build and Release** workflow
3. Click **Run workflow**
4. Enter a version tag (e.g., `v1.0.0`)
5. Click **Run workflow**

The release will appear on the [Releases](https://github.com/SeanArchibald/midi-note-translator/releases) page with all platform binaries attached.
