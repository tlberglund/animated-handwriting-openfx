## 1. Generalize splatSegment for non-square coordinate scaling

- [x] 1.1 In `renderer.cpp`, add `float scaleX, float scaleY` parameters to `splatSegment` replacing the implicit use of `ctx.capHeightPx` for both axes. Update the pixel-coordinate computation: `pax = originX + ax * scaleX`, `pay = baselineY + (1 - ay) * scaleY` (and same for pb)
- [x] 1.2 Update `splatStrokes` to accept and pass through `scaleX` and `scaleY` to each `splatSegment` call
- [x] 1.3 Update all `splatStrokes` call sites in `renderHandwriting` to pass `scaleX = scaleY = ctx.capHeightPx`
- [x] 1.4 Update `renderer.h` declaration of `splatStrokes` (if exposed) — note: both are currently `static`, so only internal call sites need updating
- [x] 1.5 Build and confirm no errors: `cmake --build build --target bundle`

## 2. Reorganize source files for the two-plugin split

- [x] 2.1 Rename `src/plugin.cpp` to `src/handwriting_plugin.cpp`
- [x] 2.2 Create `src/bundle.cpp` that implements `OfxGetNumberOfPlugins()` (returns 2) and `OfxGetPlugin(int nth)` dispatching to `getHandwritingPlugin()` (nth=0) and `getDrawingPlugin()` (nth=1). Remove `OfxGetNumberOfPlugins` and `OfxGetPlugin` from `handwriting_plugin.cpp` and expose a `getHandwritingPlugin()` function instead
- [x] 2.3 Update `CMakeLists.txt` to replace `plugin.cpp` with `handwriting_plugin.cpp` and add `bundle.cpp` (and `diagram_plugin.cpp`, `diagram_loader.cpp`, `diagram_renderer.cpp` as stubs for now)
- [x] 2.4 Build and confirm the refactored single-plugin bundle still works: `cmake --build build --target bundle`

## 3. Add diagram data model and loader

- [x] 3.1 Add `DiagramData` struct to `glyph_model.h`: `{ std::string name; float aspectRatio; std::vector<Stroke> strokes; float totalDuration; }`
- [x] 3.2 Create `src/diagram_loader.h` declaring `DiagramData loadDiagram(const std::string& path)`
- [x] 3.3 Create `src/diagram_loader.cpp` implementing `loadDiagram`: open file, parse JSON `{name, aspectRatio, strokes}`, populate `DiagramData`, compute `totalDuration` as sum of each stroke's last `t` value. Return empty `DiagramData` (aspectRatio=1, no strokes) on any error
- [x] 3.4 Build and confirm no errors

## 4. Add renderDiagram

- [x] 4.1 Add `renderDiagram` declaration to `renderer.h` with parameters: `RenderContext`, `DiagramData`, `draw_time_ms`, `outlineThickness`, `fillColor[4]`, `outlineColor[4]`, `outlineEnabled`, `posX_px`, `posY_px`, `hAnchor`, `vAnchor`, `rotation`, `diagramHeightPx`
- [x] 4.2 Implement `renderDiagram` in `renderer.cpp`:
  - Compute `diagramWidthPx = diagramHeightPx * data.aspectRatio`
  - Compute `originX` from `hAnchor` and `diagramWidthPx` (Left → `posX_px`; Center → `posX_px - diagramWidthPx/2`; Right → `posX_px - diagramWidthPx`)
  - Compute `originY` (bottom of diagram box) from `vAnchor` (Top → `posY_px - diagramHeightPx`; Middle → `posY_px - diagramHeightPx/2`; Bottom → `posY_px`)
  - Precompute `rotSin`, `rotCos` from `rotation`
  - Build sequence: for each stroke, record `{&stroke, seqStart}`; `seqStart` accumulates `stroke.back().t`; `seqCursor += stroke.back().t`
  - Outline pass: for each stroke in sequence, if `draw_time_ms > seqStart`, compute `tLocal`, call `splatStrokes` with `scaleX = diagramWidthPx`, `scaleY = diagramHeightPx`, `glyphPenX = 0`
  - Fill pass: same with `sigmaExtra = 0`

## 5. Add HandwrittenDrawingFX OFX plugin

- [x] 5.1 Create `src/diagram_plugin.cpp` with full OFX scaffolding for `HandwrittenDrawingFX`:
  - `kOfxActionLoad` / `kOfxActionUnload`: same suite acquisition as `handwriting_plugin.cpp`
  - `kOfxActionDescribe`: label "HandwrittenDrawingFX", unique identifier `"com.tlberglund.HandwrittenDrawingFX"`, generator context, float RGBA
  - `kOfxActionDescribeInContext`: define parameters: `diagramFile` (string, filePath mode), `diagramHeight` (int, display 1–4320, default 0), `speed`, `animate`, `strokeThickness`, `fillColor`, `outlineColor`, `outlineThickness`, `outlineEnabled`, `posX`, `posY`, `hAnchor`, `vAnchor`, `rotation`
  - `kOfxActionCreateInstance`: allocate instance data, load diagram if path non-empty, initialize `diagramHeight = round(projectHeight * 0.3)`, initialize `posX/posY` to project center
  - `kOfxActionDestroyInstance`: free instance data
  - `kOfxActionInstanceChanged` for `diagramFile`: reload diagram
  - `kOfxImageEffectActionRender`: read all params, compute `draw_time_ms`, call `renderDiagram`
  - Expose `getDrawingPlugin()` returning the plugin's `OfxPlugin` struct
- [x] 5.2 Update `bundle.cpp` to include the `getDrawingPlugin()` declaration and wire it into `OfxGetPlugin(1)`
- [x] 5.3 Update `CMakeLists.txt` to add `diagram_plugin.cpp`, `diagram_loader.cpp` to the build

## 6. Build and Verify

- [x] 6.1 Run `cmake --build build --target bundle` and confirm no errors or warnings
- [x] 6.2 Install to `/Library/OFX/Plugins/`, restart Resolve; confirm `HandwritingFX` still works identically
- [x] 6.3 Confirm `HandwrittenDrawingFX` appears as a separate generator in Resolve's Effects library
- [x] 6.4 Drop `HandwrittenDrawingFX` on the timeline, set `diagramFile` to `assets/TimBerglund.com.json` path; confirm drawing renders and animates
- [x] 6.5 Verify `diagramHeight` scales the drawing proportionally; verify width follows `aspectRatio`
- [x] 6.6 Verify `animate=false` shows the full drawing on frame 0
- [x] 6.7 Verify `rotation` rotates the drawing around the position anchor
