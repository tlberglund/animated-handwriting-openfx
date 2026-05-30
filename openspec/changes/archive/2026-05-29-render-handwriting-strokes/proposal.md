## Why

The current renderer uses FreeType to rasterize a TTF font — generic, static, and with no connection to real handwriting. This change replaces it entirely with an animated stroke renderer driven by a captured handwriting dataset in a custom JSON format. The result is text that appears to write itself, stroke by stroke, in an actual person's handwriting.

## What Changes

- **Remove** FreeType dependency and the bundled Open Sans font entirely
- **Add** `nlohmann/json` (header-only) for JSON parsing
- **Bundle** `assets/tim-hand.json` as the default glyph set inside the `.ofx.bundle`
- **Add** `glyphSet` string parameter (`kOfxParamStringIsFilePath` mode) — empty string uses the bundled default; a valid path loads an external capture file
- **Add** `captureSelections` hidden string parameter — stores per-character capture index selections, re-randomized whenever the `text` parameter changes (via `kOfxActionInstanceChanged`), persisted with the Resolve project
- **Add** `fillColor` RGBA parameter — color of the rendered strokes (default: opaque white)
- **Add** `outlineColor` RGBA parameter — color of the outline halo (default: opaque black)
- **Add** `outlineThickness` double parameter — outline width in cap-height units (0–0.1, default 0.01)
- **Replace** FreeType glyph loop with Gaussian-splat stroke renderer: each stroke point becomes a soft disc whose radius scales with pressure (`p`) and `strokeThickness`; two-pass rendering draws outline first, fill second
- **Implement** animation: map current frame + fps + `speed` to a draw time in milliseconds; characters reveal stroke-by-stroke in sequence; frame 0 = blank, animation completes when all strokes are drawn
- **Implement** `kOfxActionInstanceChanged` handler to re-randomize capture selections when `text` changes

## Capabilities

### New Capabilities

- `handwriting-animation`: Time-based progressive reveal of handwriting strokes, driven by captured JSON glyph data with per-character capture selection and Gaussian splat rendering

### Modified Capabilities

- `text-rendering`: Rendering is no longer FreeType/TTF — it is stroke-based, animated, and driven by a JSON capture set. Output appearance, timing, and parameters all change significantly.
- `plugin-parameters`: Five new parameters added (`glyphSet`, `captureSelections`, `fillColor`, `outlineColor`, `outlineThickness`); `strokeThickness` meaning changes (now controls Gaussian sigma base, not pixel thickness)

## Impact

- `src/plugin.cpp` — near-complete rewrite of render path; new instance data struct; new `kOfxActionInstanceChanged` handler
- `CMakeLists.txt` — remove FreeType; add nlohmann/json; bundle `tim-hand.json` instead of `OpenSans-Regular.ttf`
- `assets/tim-hand.json` — new bundled asset (already present)
- `assets/OpenSans-Regular.ttf` — **removed** (no longer needed)
