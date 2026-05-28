## Why

The plugin currently declares parameters but produces no output — dropping it on a Resolve timeline yields a blank frame. Rendering the `text` parameter using the `textHeight` parameter is the first observable behaviour and the foundation all future rendering features build on.

## What Changes

- Add FreeType 2 as a CMake dependency (via FetchContent) for font rasterisation
- Bundle the Open Sans Regular TTF inside the `.ofx.bundle`
- Implement `kOfxImageEffectActionGetRegionOfDefinition` to report the output frame bounds
- Implement `kOfxImageEffectActionRender` to rasterise the `text` parameter value into the output clip using Open Sans at a point size derived from `textHeight × frame height`, positioned at (0, 0) in frame coordinates
- Other parameters (`strokeThickness`, `speed`, `audio`) are read but ignored in this change

## Capabilities

### New Capabilities

- `text-rendering`: Plugin produces a visible RGBA frame containing rasterised text drawn with FreeType at a size proportional to frame height

### Modified Capabilities

- `plugin-parameters`: `text` and `textHeight` parameters are now actively read during render (behaviour change — parameters were previously declared but unused)

## Impact

- `src/plugin.cpp` — add render and region-of-definition action handlers, FreeType calls
- `CMakeLists.txt` — add FreeType FetchContent dependency, copy font into bundle
- `cmake/Info.plist` — no change
- New font asset: `assets/OpenSans-Regular.ttf` (bundled at build time)
- No change to parameter definitions or OFX entry points
