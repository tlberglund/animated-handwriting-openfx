## 1. Add Parameters in describeInContext

- [x] 1.1 In `plugin.cpp` `describeInContext`, add `animate` boolean parameter immediately after `text`, label "Animate", default true
- [x] 1.2 Add `tracking` double parameter: range −0.5 to 2.0, default 0.0, label "Tracking"
- [x] 1.3 Add `rotation` double parameter: range −180.0 to 180.0, display range −180.0 to 180.0, default 0.0, label "Rotation"

## 2. Read New Parameters at Render Time

- [x] 2.1 In the render handler, declare `int animate = 1`, `double tracking = 0.0`, `double rotation = 0.0`
- [x] 2.2 Read `animate` via `paramGetValueAtTime`
- [x] 2.3 Read `tracking` via `paramGetValueAtTime`
- [x] 2.4 Read `rotation` via `paramGetValueAtTime`
- [x] 2.5 When `animate == 0`, set `draw_time_ms = 1e12` before calling `renderHandwriting`
- [x] 2.6 Pass `tracking` and `rotation` to `renderHandwriting`

## 3. Update renderer.h Interface

- [x] 3.1 Add `double tracking` and `double rotation` to the `renderHandwriting` declaration in `renderer.h`

## 4. Apply Tracking in renderHandwriting

- [x] 4.1 Add `double tracking` and `double rotation` parameters to the `renderHandwriting` definition in `renderer.cpp`
- [x] 4.2 In the per-line glyph-sequence loop, replace `penX += rg.width` with `penX += rg.width + (float)tracking`

## 5. Apply Rotation in splatSegment

- [x] 5.1 Add `float rotSin` and `float rotCos` parameters to `splatSegment`
- [x] 5.2 After computing `pax/pay/pbx/pby`, apply 2D rotation around `(posX_px, posY_px)`: translate to origin, rotate by `rotSin`/`rotCos`, translate back. Pass `posX_px` and `posY_px` into `splatSegment` for this purpose.
- [x] 5.3 Add `float rotSin`, `float rotCos`, `float rotCenterX`, `float rotCenterY` parameters to `splatStrokes` and thread them through to each `splatSegment` call
- [x] 5.4 In `renderHandwriting`, precompute `rotSin = sinf((float)rotation * M_PI / 180.0f)` and `rotCos = cosf((float)rotation * M_PI / 180.0f)` and pass to both render passes via `splatStrokes`

## 6. Build and Verify

- [x] 6.1 Run `cmake --build build --target bundle` and confirm no errors or warnings
- [x] 6.2 Install to `/Library/OFX/Plugins/`, restart Resolve; confirm existing single-line text renders identically (tracking=0, rotation=0, animate=true)
- [x] 6.3 Set tracking to 0.5; confirm characters spread apart proportionally
- [x] 6.4 Set rotation to 45; confirm text block rotates clockwise around the position anchor
- [x] 6.5 Set animate to false; confirm all glyphs appear fully drawn on frame 0
- [x] 6.6 Confirm animate=true still reveals glyphs progressively as before
