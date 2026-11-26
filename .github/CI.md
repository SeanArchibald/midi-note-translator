# GitHub Actions CI/CD

This directory contains GitHub Actions workflows for automated building and releasing of the MIDI Note Translator CLAP plugin.

## Workflows

### `build-and-release.yml`

This workflow handles both CI (continuous integration) and CD (continuous deployment) for the project.

#### Triggers

The workflow runs on:
- **Push to master branch** - Builds all platforms
- **Pull requests** - Builds all platforms (for testing)
- **Tags starting with 'v'** - Builds and creates a release (e.g., `v0.2.0`)
- **Manual trigger** - Can be run manually with a custom version tag

#### Jobs

1. **Build Jobs** (run in parallel):
   - `build-linux`: Builds CLAP + VST3 on Ubuntu 22.04 (x64)
   - `build-macos`: Builds universal CLAP + VST3 bundles (arm64 + x86_64), ad-hoc signed
   - `build-windows`: Builds CLAP + VST3 on Windows (x64, MSVC + Ninja)

   Each job downloads the VST3 SDK at configure time, runs the unit tests, and
   stages both formats with `LICENSE.md`, `THIRD_PARTY_NOTICES.md`, and
   `LICENSES/`. The Linux job validates the CLAP with `clap-validator`; the macOS
   job validates the VST3 with `pluginval`. The Linux job also checks that the
   bundled license texts match the submodules and the downloaded SDK.

2. **Release Job**:
   - Downloads all built artifacts
   - Packages them as ZIP files
   - Creates a GitHub Release with all assets

#### Artifacts

Each release produces:
- `MidiNoteTranslator-linux-x64.zip`
- `MidiNoteTranslator-macos-universal.zip`
- `MidiNoteTranslator-windows-x64.zip`

Each archive contains both `MidiNoteTranslator.clap` and
`MidiNoteTranslator.vst3`, plus `LICENSE.md`, `THIRD_PARTY_NOTICES.md`, and
`LICENSES/`.

## Creating a Release

### Option 1: Push a Git Tag

```bash
git tag v0.2.0
git push origin v0.2.0
```

This will automatically trigger the build and create a release.

### Option 2: Manual Trigger

1. Go to the **Actions** tab in your GitHub repository
2. Select the **Build and Release** workflow
3. Click **Run workflow**
4. Enter a version tag (e.g., `v0.2.0`)
5. Click **Run workflow**

## Release Notes

The workflow automatically generates release notes based on:
- Commits since the last release
- Contributors
- Pull requests merged

## Prerequisites

No special secrets are needed for basic operation. The workflow uses:
- `GITHUB_TOKEN` - Automatically provided by GitHub Actions

## Troubleshooting

### Build Failures

Check the Actions logs for:
- Missing dependencies
- CMake configuration errors
- Compilation errors

### Release Failures

Ensure:
- All three build jobs completed successfully
- The version tag follows semantic versioning (e.g., `v0.2.0`)
- You have write permissions to the repository
