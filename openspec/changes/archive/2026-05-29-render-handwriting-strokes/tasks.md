## 1. CMake: Remove FreeType, Add nlohmann/json, Update Bundle

- [x] 1.1 Remove the FreeType `FetchContent_Declare` / `FetchContent_MakeAvailable` block and its cache policy workaround from `CMakeLists.txt`
- [x] 1.2 Remove `target_link_libraries(HandwritingFX PRIVATE freetype)` from `CMakeLists.txt`
- [x] 1.3 Remove `CMAKE_POLICY_VERSION_MINIMUM` workaround line
- [x] 1.4 Add `nlohmann/json` as a single-header FetchContent dependency (download `json.hpp` from the v3.11.3 release, `DOWNLOAD_NO_EXTRACT TRUE`)
- [x] 1.5 Add `target_include_directories` for the nlohmann json header directory
- [x] 1.6 In the `bundle` custom target, replace the `OpenSans-Regular.ttf` copy with a copy of `assets/tim-hand.json` into `HandwritingFX.ofx.bundle/Contents/Resources/`
- [x] 1.7 Remove the `file(DOWNLOAD ...)` block that fetches Open Sans

## 2. Define Instance Data Struct

- [x] 2.1 In `plugin.cpp`, define a `GlyphPoint` struct `{ float x, y, t, p; }`
- [x] 2.2 Define a `Stroke` as `std::vector<GlyphPoint>`
- [x] 2.3 Define a `Capture` struct `{ std::string id; float width; std::vector<Stroke> strokes; float duration; }` where `duration = max(t)` across all strokes, computed at load time
- [x] 2.4 Define a `Glyph` struct `{ std::string character; std::vector<Capture> captures; }`
- [x] 2.5 Define a `GlyphSet` struct `{ std::string name; std::map<std::string, Glyph> glyphs; std::vector<std::string> ligatureKeys; float pMax; }` where `ligatureKeys` holds multi-char keys sorted descending by length, and `pMax` is the maximum `p` value across all points
- [x] 2.6 Define an `InstanceData` struct `{ GlyphSet glyphSet; std::string loadedPath; std::vector<int> captureIndices; std::string lastText; }`

## 3. JSON Loading

- [x] 3.1 Add `#include <nlohmann/json.hpp>` and `#include <fstream>` to `plugin.cpp`
- [x] 3.2 Add a `getBundledGlyphSetPath()` helper (same `dladdr`-based approach as old `getFontPath()`, targeting `../Resources/tim-hand.json`)
- [x] 3.3 Implement `loadGlyphSet(const std::string& path)` returning a `GlyphSet`: open file, parse JSON, populate structs, compute `duration` per capture, collect `ligatureKeys` (multi-char glyph keys sorted descending by length), compute `pMax`
- [x] 3.4 Handle missing or malformed JSON gracefully — return an empty `GlyphSet` without crashing

## 4. Instance Lifecycle

- [x] 4.1 In the `kOfxActionLoad` handler, remove FreeType init; keep the action returning `kOfxStatOK`
- [x] 4.2 In the `kOfxActionUnload` handler, remove FreeType teardown; keep returning `kOfxStatOK`
- [x] 4.3 In `kOfxActionCreateInstance`, allocate an `InstanceData` on the heap, load the bundled glyph set into it, and store the pointer via `gPropSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, ptr)`
- [x] 4.4 In `kOfxActionDestroyInstance`, read the pointer back from `kOfxPropInstanceData` and `delete` it

## 5. Remove FreeType Code, Remove Old Headers

- [x] 5.1 Remove `#include <ft2build.h>` / `#include FT_FREETYPE_H` and all `FT_*` globals and calls from `plugin.cpp`
- [x] 5.2 Remove the `getFontPath()` function

## 6. Update Parameters in describeInContext

- [x] 6.1 Change `strokeThickness` range to 0.0–0.2 and default to 0.02 (cap-height units, not pixels)
- [x] 6.2 Add `glyphSet` string parameter with `kOfxParamPropStringMode = kOfxParamStringIsFilePath`, label "Glyph Set", default ""
- [x] 6.3 Add `captureSelections` string parameter with `kOfxParamPropSecret = 1`, label "Capture Selections", default ""
- [x] 6.4 Add `fillColor` RGBA parameter, label "Fill Color", default (1.0, 1.0, 1.0, 1.0)
- [x] 6.5 Add `outlineColor` RGBA parameter, label "Outline Color", default (0.0, 0.0, 0.0, 1.0)
- [x] 6.6 Add `outlineThickness` double parameter, label "Outline Thickness", range 0.0–0.1, default 0.01

## 7. Implement kOfxActionInstanceChanged

- [x] 7.1 Add a handler for `kOfxActionInstanceChanged` in `pluginMain`
- [x] 7.2 Read `kOfxPropChangeReason` from `inArgs`; proceed only if reason is `kOfxChangeUserEdited`
- [x] 7.3 Read `kOfxPropName` from `inArgs` to identify which parameter changed
- [x] 7.4 If the changed parameter is `text`: read current text value, generate a random capture index for each character (`rand() % numCaptures`, or 0 if glyph not found), serialize as comma-separated string, write to `captureSelections` parameter via `paramSetValue`
- [x] 7.5 If the changed parameter is `glyphSet`: reload the glyph set from the new path (or bundled default if empty) into the instance's `InstanceData`; update `loadedPath`

## 8. Implement Animation Render

- [x] 8.1 In the render handler, retrieve `InstanceData*` from `kOfxPropInstanceData` on the effect instance
- [x] 8.2 Read `glyphSet`, `text`, `captureSelections`, `textHeight`, `strokeThickness`, `speed`, `fillColor` (4 doubles), `outlineColor` (4 doubles), `outlineThickness` at render time using `paramGetValueAtTime`
- [x] 8.3 Read `kOfxImageEffectPropFrameRate` from the effect property set to get `fps`
- [x] 8.4 Compute `capHeightPx = round(textHeight * frameHeight * renderScaleY)`; compute `draw_time_ms = (renderTime / fps) * 1000.0 * speed`
- [x] 8.5 Parse `captureSelections` string into a `std::vector<int>`; if empty or wrong length, generate default indices (0 for each character)
- [x] 8.6 Walk the text string performing ligature substitution: for each position, try each key in `glyphSet.ligatureKeys` for a match; if found consume those characters as one glyph, else consume one character
- [x] 8.7 For each resolved glyph, retrieve the selected capture (by index from captureSelections); compute `seq_start` and `seq_end` for this character in the animation sequence
- [x] 8.8 Clear the output buffer to (0,0,0,0)
- [x] 8.9 **Outline pass**: for each fully or partially visible glyph, call a `splatStrokes` helper with `sigma_extra = outlineThickness * capHeightPx` and `outlineColor`
- [x] 8.10 **Fill pass**: for each fully or partially visible glyph, call `splatStrokes` with `sigma_extra = 0` and `fillColor`
- [x] 8.11 Implement `splatStrokes(capture, t_local, penX, capHeightPx, strokeThickness, sigmaExtra, color, pixelData, bounds, rowBytes)`: iterate stroke points up to `t_local`; for the in-progress segment, interpolate the endpoint; for each drawn point, compute `sigma = (strokeThickness * p_normalised + sigmaExtra) * capHeightPx`; iterate pixels within 3σ of the point; accumulate `alpha += exp(-d²/(2σ²))` into the pixel; composite color over the existing pixel value

## 9. Build and Verify

- [x] 9.1 Run `cmake -S . -B build` and `cmake --build build --target bundle`; confirm it compiles without errors or warnings
- [x] 9.2 Verify `build/HandwritingFX.ofx.bundle/Contents/Resources/tim-hand.json` is present and `OpenSans-Regular.ttf` is absent
- [x] 9.3 Install to `/Library/OFX/Plugins/`, restart Resolve, place generator on timeline, type text, scrub the timeline, and confirm strokes animate progressively from left to right
- [x] 9.4 Confirm fill and outline colors are visible and respond to parameter changes in the Inspector
