# GitHub Actions CI/CD

This directory contains GitHub Actions workflows for automated building and releasing of the Scale Scaler CLAP plugin.

## Workflows

### `build-and-release.yml`

This workflow handles both CI (continuous integration) and CD (continuous deployment) for the project.

#### Triggers

The workflow runs on:
- **Push to main/master branch** - Builds all platforms
- **Pull requests** - Builds all platforms (for testing)
- **Tags starting with 'v'** - Builds and creates a release (e.g., `v1.0.0`)
- **Manual trigger** - Can be run manually with a custom version tag

#### Jobs

1. **Build Jobs** (run in parallel):
   - `build-linux`: Builds on Ubuntu (x64)
   - `build-macos`: Builds on macOS (Universal binary)
   - `build-windows`: Builds on Windows (x64)

2. **Release Job**:
   - Downloads all built artifacts
   - Packages them as ZIP files
   - Creates a GitHub Release with all assets

#### Artifacts

Each build produces:
- `scale-scaler-linux-x64.zip` - Linux CLAP plugin
- `scale-scaler-macos-universal.zip` - macOS CLAP plugin bundle
- `scale-scaler-windows-x64.zip` - Windows CLAP plugin

## Creating a Release

### Option 1: Push a Git Tag

```bash
git tag v1.0.0
git push origin v1.0.0
```

This will automatically trigger the build and create a release.

### Option 2: Manual Trigger

1. Go to the **Actions** tab in your GitHub repository
2. Select the **Build and Release** workflow
3. Click **Run workflow**
4. Enter a version tag (e.g., `v1.0.0`)
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
- The version tag follows semantic versioning (e.g., `v1.0.0`)
- You have write permissions to the repository
