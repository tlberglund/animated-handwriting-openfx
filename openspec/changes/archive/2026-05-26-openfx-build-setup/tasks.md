## 1. Project Structure

- [x] 1.1 Create top-level `CMakeLists.txt` with project name, C++17 standard, and `CMAKE_OSX_ARCHITECTURES` set to `arm64;x86_64`
- [x] 1.2 Create `src/` directory and add a placeholder `plugin.cpp` file

## 2. OFX SDK Headers

- [x] 2.1 Add `FetchContent_Declare` block in `CMakeLists.txt` to fetch `https://github.com/ofxa/openfx` at a pinned release tag
- [x] 2.2 Add `FetchContent_MakeAvailable` and wire the fetched include directory into the target's `target_include_directories`

## 3. Plugin Source

- [x] 3.1 Write `src/plugin.cpp` implementing `OfxGetNumberOfPlugins` (returns 1) and `OfxGetPlugin` (returns a static `OfxPlugin` struct for a Filter context effect named `HandwritingFX`)
- [x] 3.2 Implement `OfxSetHost` (stores the host pointer; can be a no-op body for now)
- [x] 3.3 Implement the effect's `pluginMain` dispatch function handling `kOfxActionLoad`, `kOfxActionUnload`, `kOfxActionDescribe`, and `kOfxImageEffectActionDescribeInContext` (minimal stubs that return `kOfxStatOK`)
- [x] 3.4 Verify exported symbols with `nm -g build/libHandwritingFX.dylib | grep -E "OfxSetHost|OfxGetNumber|OfxGetPlugin"`

## 4. CMake Shared Library Target

- [x] 4.1 Add `add_library(HandwritingFX SHARED src/plugin.cpp)` to `CMakeLists.txt`
- [x] 4.2 Set `SUFFIX ""` and `PREFIX ""` on the target so the output filename is `HandwritingFX` (no `lib` prefix, no `.dylib` suffix) — the bundle step will rename it to `.ofx`
- [x] 4.3 Confirm `cmake -B build && cmake --build build` succeeds and `lipo -info` on the output shows `arm64` and `x86_64`

## 5. Bundle Packaging Target

- [x] 5.1 Add a `bundle` custom target in `CMakeLists.txt` that creates `HandwritingFX.ofx.bundle/Contents/MacOS/` and `HandwritingFX.ofx.bundle/Contents/`
- [x] 5.2 Copy the compiled library into `HandwritingFX.ofx.bundle/Contents/MacOS/HandwritingFX.ofx` as part of the `bundle` target
- [x] 5.3 Write a minimal `Info.plist` (CFBundleExecutable, CFBundleIdentifier, CFBundleVersion) into `HandwritingFX.ofx.bundle/Contents/Info.plist` — either as a generated file or a static template copied during the build
- [x] 5.4 Run `cmake --build build --target bundle` and verify the directory tree matches the expected layout

## 6. Installation and Resolve Verification

- [x] 6.1 Copy `HandwritingFX.ofx.bundle` to `~/Library/OFX/Plugins/` and run `xattr -cr ~/Library/OFX/Plugins/HandwritingFX.ofx.bundle` to clear quarantine
- [x] 6.2 Launch DaVinci Resolve and confirm the plugin appears in the OpenFX browser without errors
