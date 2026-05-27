## Why

There is currently no code in this repository. To ship an OpenFX plugin for DaVinci Resolve, we need a working C++ build system, the OFX SDK headers, a minimal plugin skeleton, and a correctly structured `.ofx.bundle` artifact that Resolve can discover and load.

## What Changes

- Add CMake-based build system targeting macOS (arm64 + x86_64 universal binary)
- Integrate the OpenFX SDK headers (OFX Image Effect API)
- Create a minimal "identity" plugin (passes frames through unchanged) as the buildable skeleton
- Produce a properly structured `.ofx.bundle` that can be installed to `~/Library/OFX/Plugins/`

## Capabilities

### New Capabilities

- `openfx-build-scaffold`: CMake project with OFX SDK headers, minimal plugin source, and bundle packaging — produces an installable `.ofx.bundle` artifact

### Modified Capabilities

## Impact

- New files only; nothing to break
- Build dependencies: CMake ≥ 3.21, a C++17 compiler (AppleClang via Xcode Command Line Tools)
- No runtime dependencies beyond what ships with macOS
- Output artifact is installed manually into `~/Library/OFX/Plugins/` for testing in DaVinci Resolve
