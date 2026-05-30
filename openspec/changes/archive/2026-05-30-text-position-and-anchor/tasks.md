## 1. Add Parameters in describeInContext

- [x] 1.1 In `plugin.cpp` `describeInContext`, add `posX` double parameter: display range –8192 to 8192, default 0.0 (runtime-overridden at createInstance), label "Position X"
- [x] 1.2 Add `posY` double parameter: display range –8192 to 8192, default 0.0 (runtime-overridden at createInstance), label "Position Y"
- [x] 1.3 Add `hAnchor` choice parameter with options Left / Center / Right, default index 1 (Center), label "H Anchor"
- [x] 1.4 Add `vAnchor` choice parameter with options Top / Middle / Bottom, default index 1 (Middle), label "V Anchor"

## 2. Initialize Position in kOfxActionCreateInstance

- [x] 2.1 In `kOfxActionCreateInstance`, after creating `InstanceData`, get the effect's property set and read `kOfxImageEffectPropProjectSize` into a `double[2]`
- [x] 2.2 Get the param set, retrieve handles for `posX` and `posY`, and call `paramSetValue` with `projectSize[0] / 2.0` and `projectSize[1] / 2.0` respectively

## 3. Read Parameters at Render Time

- [x] 3.1 In the render handler in `plugin.cpp`, read `posX` and `posY` via `paramGetValueAtTime`
- [x] 3.2 Read `hAnchor` and `vAnchor` via `paramGetValueAtTime` (returns int)
- [x] 3.3 Compute `posX_px = posX * renderScale[0]` and `posY_px = posY * renderScale[1]`
- [x] 3.4 Pass `posX_px`, `posY_px`, `hAnchor`, `vAnchor` to `renderHandwriting`

## 4. Update renderer.h Interface

- [x] 4.1 Add `float posX_px, float posY_px, int hAnchor, int vAnchor` parameters to the `renderHandwriting` declaration in `renderer.h`
- [x] 4.2 Add `float originX, float baselineY` parameters to the `splatStrokes` static function signature in `renderer.cpp`
- [x] 4.3 Add `float originX, float baselineY` parameters to the `splatSegment` static function signature in `renderer.cpp`

## 5. Compute Origin in renderHandwriting

- [x] 5.1 After building the resolved glyph list, compute `totalWidth` as the sum of all `rg.width` values (in cap-height units)
- [x] 5.2 Compute `originX` from `posX_px` and `hAnchor`: Left → `posX_px`; Center → `posX_px − totalWidth * capHeightPx / 2`; Right → `posX_px − totalWidth * capHeightPx`
- [x] 5.3 Compute `baselineY` from `posY_px` and `vAnchor`: Top → `posY_px − capHeightPx`; Middle → `posY_px − capHeightPx / 2`; Bottom → `posY_px`
- [x] 5.4 Pass `originX` and `baselineY` through all `splatStrokes` call sites in `renderHandwriting`

## 6. Apply Offset in splatSegment

- [x] 6.1 Replace `float pax = ax * ctx.capHeightPx` with `float pax = originX + ax * ctx.capHeightPx`
- [x] 6.2 Replace `float pay = (1.0f - ay) * ctx.capHeightPx` with `float pay = baselineY + (1.0f - ay) * ctx.capHeightPx`
- [x] 6.3 Update `splatStrokes` to pass `originX` and `baselineY` through to each `splatSegment` call

## 7. Build and Verify

- [x] 7.1 Run `cmake --build build --target bundle` and confirm no errors or warnings
- [x] 7.2 Install to `/Library/OFX/Plugins/`, restart Resolve, confirm text defaults to center of frame
- [x] 7.3 Verify moving Position X/Y repositions the text correctly
- [x] 7.4 Verify H Anchor Left/Center/Right shifts the text left edge / center / right edge to the position point
- [x] 7.5 Verify V Anchor Top/Middle/Bottom aligns cap-top / midpoint / baseline to the position point
- [x] 7.6 Confirm setting posX=0, posY=0, hAnchor=Left, vAnchor=Bottom places text at the bottom-left corner
