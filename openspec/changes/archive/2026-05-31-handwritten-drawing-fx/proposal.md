## Why

The project has a diagram capture system that records handwriting strokes for arbitrary drawings (logos, diagrams, illustrations) in the same `{x, y, t, p}` format as the glyph captures. There is no way to render these in Resolve today. Adding a second OFX plugin — `HandwrittenDrawingFX` — to the same bundle gives editors a dedicated tool for animating drawn graphics alongside `HandwritingFX` for text.

## What Changes

- Add a `HandwrittenDrawingFX` OFX Generator plugin to the existing bundle (bundle now exports 2 plugins)
- Add a diagram JSON loader (`loadDiagram`) that reads the flat-strokes format: `{name, aspectRatio, strokes[]}`
- Add a `renderDiagram` function that animates all strokes sequentially using the existing pixel-painting primitives
- Generalize `splatSegment` to accept independent x/y scale factors so diagrams (aspect-ratio-scaled) and glyphs (square cap-height units) can share the same primitive
- Reorganize source files to make the shared/per-plugin split explicit

## Capabilities

### New Capabilities

- `diagram-rendering`: The `HandwrittenDrawingFX` plugin loads a diagram JSON file and animates its strokes onto a transparent frame, with position, rotation, sizing, color, and outline controls

### Modified Capabilities

- `openfx-build-scaffold`: The bundle now registers and exports two plugins instead of one

## Impact

- `src/renderer.cpp` / `renderer.h`: `splatSegment` signature generalized (x/y scale factors); existing `renderHandwriting` call sites updated
- New files: `src/diagram_loader.h/.cpp`, `src/diagram_renderer.h/.cpp`, `src/diagram_plugin.cpp`
- `CMakeLists.txt`: adds new source files; bundle assembly unchanged (same `.ofx.bundle`, two plugins inside)
- `src/plugin.cpp`: adds second plugin registration to `OfxGetNumberOfPlugins` / `OfxGetPlugin`
