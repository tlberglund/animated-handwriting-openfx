## Context

This is a greenfield C++ project. No source code exists yet. The goal is a minimal but correct OpenFX plugin that DaVinci Resolve can discover, load, and list — a foundation for adding real effects later.

OpenFX (.ofx) plugins on macOS must be delivered as a `.ofx.bundle` directory with a specific layout:

```
HandwritingFX.ofx.bundle/
└── Contents/
    ├── Info.plist
    └── MacOS/
        └── HandwritingFX.ofx   ← universal dylib (renamed from .dylib)
```

DaVinci Resolve scans `~/Library/OFX/Plugins/` and `/Library/OFX/Plugins/` at startup.

## Goals / Non-Goals

**Goals:**
- CMake project that compiles a universal (arm64 + x86_64) shared library
- OFX SDK headers available in-tree via CMake FetchContent
- Minimal identity plugin skeleton (passes frames through, registers one effect)
- `cmake --install` or a custom target produces the ready-to-install `.ofx.bundle`
- Bundle can be installed to `~/Library/OFX/Plugins/` and discovered by DaVinci Resolve

**Non-Goals:**
- Any actual image processing effect (that comes in a later change)
- GPU/OpenCL/CUDA support
- Windows or Linux builds (macOS only for now)
- Automated tests (the acceptance test is "Resolve lists the plugin")

## Decisions

### OFX SDK headers via CMake FetchContent

**Decision**: Pull `openfx` headers from the official GitHub repo (`https://github.com/ofxa/openfx`) using `FetchContent_Declare` / `FetchContent_MakeAvailable` rather than vendoring them or using a submodule.

**Rationale**: FetchContent keeps the repo clean (no submodule bookkeeping), pins to a specific git tag for reproducibility, and requires no manual setup beyond CMake. The OFX SDK is headers-only for our purposes so there is no compile step.

**Alternative considered**: Git submodule — adds `.gitmodules` ceremony and requires `git submodule update --init` on every fresh clone. Rejected for simplicity.

### Universal binary (arm64 + x86_64)

**Decision**: Build with `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"` to produce a fat binary.

**Rationale**: DaVinci Resolve ships as a universal app. A fat `.ofx` works on both Apple Silicon and Intel Macs without separate builds.

**Alternative considered**: arm64-only — would fail to load on Intel Macs and potentially cause Resolve issues even on M-series if it spawns x86 helper processes.

### Plugin entry point: `OfxSetHost` + `OfxGetNumberOfPlugins` + `OfxGetPlugin`

**Decision**: Implement the three required OFX C entry points directly (no wrapper library).

**Rationale**: The OFX C API is small and explicit. Using the raw API makes it easier to understand what Resolve calls and avoids a dependency on `openfx-supportlib` (which has its own build complexity). The skeleton can be promoted to a real plugin with no structural changes.

### Bundle packaging via CMake custom target

**Decision**: Add a `bundle` CMake custom target that:
1. Creates the `.ofx.bundle/Contents/MacOS/` directory tree
2. Copies the compiled dylib, renames it to `.ofx`
3. Writes a minimal `Info.plist`

**Rationale**: Keeps packaging reproducible and integrated into the normal build workflow (`cmake --build . --target bundle`). No external scripts or Makefiles needed.

## Risks / Trade-offs

- **Resolve version compatibility** → Mitigation: target OFX API version 1.4 (widely supported); use `kOfxImageEffectPropSupportedContexts` to advertise only the `Filter` context
- **Code-signing** → macOS may refuse to load unsigned dylibs from unknown paths. Mitigation: run `xattr -cr` on the bundle after copying to remove quarantine attribute; ad-hoc signing with `codesign --sign -` if needed
- **Info.plist format** → A malformed plist silently prevents loading. Mitigation: write the minimal known-good plist and document it

## Open Questions

- Should the plugin name be `HandwritingFX` or something else? (Assumed `HandwritingFX` for now — easy to rename.)
- Does the user want a `make install` target that copies the bundle to `~/Library/OFX/Plugins/` automatically?
