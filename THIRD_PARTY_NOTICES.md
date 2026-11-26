# Third-Party Notices

MIDI Note Translator is licensed under the MIT License. See [LICENSE.md](LICENSE.md).

The plugin includes code from the following projects.

## CLAP (CLever Audio Plugin) SDK

Used for the CLAP plugin API and headers.

Copyright (c) 2021 Alexandre BIQUE

Licensed under the MIT License. The full license text is in
[LICENSES/CLAP-SDK.txt](LICENSES/CLAP-SDK.txt).

## clap-helpers

Used for the CLAP C++ plugin and host helper classes.

Copyright (c) 2021 Alexandre BIQUE

Licensed under the MIT License. The full license text is in
[LICENSES/CLAP-HELPERS.txt](LICENSES/CLAP-HELPERS.txt).

## clap-wrapper

Used to build the VST3 version from the same plugin implementation.

Copyright (c) 2022 defiantnerd

Licensed under the MIT License. The full license text is in
[LICENSES/CLAP-WRAPPER.txt](LICENSES/CLAP-WRAPPER.txt).

## VST3 SDK

Used by clap-wrapper to build the VST3 version.

Copyright (c) 2025 Steinberg Media Technologies GmbH

Licensed under the MIT License. The full license text is in
[LICENSES/VST3-SDK.txt](LICENSES/VST3-SDK.txt).

## Build-time tools

The CI workflow uses [clap-validator](https://github.com/free-audio/clap-validator)
and [pluginval](https://github.com/Tracktion/pluginval) to validate the plugin
during builds. They are not included in the distributed plugin.
