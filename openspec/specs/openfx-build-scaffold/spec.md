### Requirement: CMake project builds shared library
The build system SHALL compile the plugin source into a universal (arm64 + x86_64) shared library using CMake ≥ 3.21 and a C++17 compiler.

#### Scenario: Clean build succeeds
- **WHEN** the developer runs `cmake -B build && cmake --build build`
- **THEN** a compiled shared library is produced in the build directory with no errors

#### Scenario: Build targets both architectures
- **WHEN** the compiled library is inspected with `lipo -info`
- **THEN** the output reports both `arm64` and `x86_64` architectures

### Requirement: OFX SDK headers are available without manual setup
The project SHALL fetch the OFX SDK headers automatically via CMake FetchContent, pinned to a specific release tag.

#### Scenario: Headers available after configure
- **WHEN** the developer runs `cmake -B build` on a fresh clone with no OFX headers present
- **THEN** CMake fetches the headers and the configure step completes without errors

### Requirement: Bundle exports two OFX plugins
The `.ofx.bundle` SHALL export two plugins: `HandwritingFX` (the existing text generator) and `HandwrittenDrawingFX` (the new diagram generator). `OfxGetNumberOfPlugins()` SHALL return 2. `OfxGetPlugin(0)` SHALL return the `HandwritingFX` plugin descriptor and `OfxGetPlugin(1)` SHALL return the `HandwrittenDrawingFX` plugin descriptor. Both plugins SHALL appear as separate Generator entries in the host's Effects library.

#### Scenario: Entry points are exported
- **WHEN** the compiled shared library is inspected with `nm -g` or `objdump --syms`
- **THEN** `OfxSetHost`, `OfxGetNumberOfPlugins`, and `OfxGetPlugin` appear as exported symbols

#### Scenario: Plugin advertises two effects
- **WHEN** the OFX host calls `OfxGetNumberOfPlugins`
- **THEN** the function returns 2

#### Scenario: Both plugins appear in Resolve's Effects library
- **WHEN** the bundle is installed and Resolve is launched
- **THEN** both `HandwritingFX` and `HandwrittenDrawingFX` appear as distinct generator entries

#### Scenario: HandwritingFX behavior is unchanged
- **WHEN** `HandwritingFX` is used after the multi-plugin refactor
- **THEN** its rendering output and parameter set are bit-for-bit identical to the single-plugin build

### Requirement: Bundle packaging target produces installable artifact
The build system SHALL provide a CMake target (`bundle`) that assembles the `.ofx.bundle` directory with the correct macOS bundle layout.

#### Scenario: Bundle target creates correct structure
- **WHEN** the developer runs `cmake --build build --target bundle`
- **THEN** the output contains `HandwritingFX.ofx.bundle/Contents/MacOS/HandwritingFX.ofx` and `HandwritingFX.ofx.bundle/Contents/Info.plist`

#### Scenario: Bundle is discoverable by DaVinci Resolve
- **WHEN** the bundle is copied to `/Library/OFX/Plugins/` and DaVinci Resolve is launched
- **THEN** the plugin appears in the OpenFX effect list without errors in the Resolve log
