## ADDED Requirements

### Requirement: Tracking scales glyph pen-advance
The plugin SHALL compute each glyph's pen-advance as `glyph.width + tracking` (in cap-height units) when accumulating `penX` within a line. The `tracking` value applies uniformly to every glyph advance including spaces. The glyph's internal stroke coordinates are not affected.

#### Scenario: Tracking 0.0 produces default spacing
- **WHEN** `tracking` is 0.0
- **THEN** glyph positions match the capture `width` values exactly, identical to previous behavior

#### Scenario: Tracking 0.5 shifts second glyph right by half a cap-height
- **WHEN** `tracking` is 0.5 and the text contains two characters
- **THEN** the second character's `penX` is `glyph[0].width + 0.5` cap-height units from the origin

### Requirement: Rotation rotates the text block around the position anchor
The plugin SHALL rotate every stroke pixel coordinate by the `rotation` angle (degrees, clockwise) around `(posX_px, posY_px)` before rasterising. The rotation SHALL be applied inside `splatSegment` by transforming the already-offset pixel coordinates `(pax, pay)` and `(pbx, pby)` using a standard 2D rotation matrix. The bounding-box clamp used for pixel iteration SHALL be computed from the rotated coordinates.

#### Scenario: Rotation 0 produces no change
- **WHEN** `rotation` is 0.0
- **THEN** pixel output is identical to renders with no rotation parameter

#### Scenario: Rotation 90 tilts text so it reads top-to-bottom
- **WHEN** `rotation` is 90.0
- **THEN** the text block is rotated 90 degrees clockwise around the position anchor

#### Scenario: Rotation clips out-of-bounds pixels
- **WHEN** `rotation` causes some stroke pixels to fall outside the output image bounds
- **THEN** those pixels are silently clipped and do not cause out-of-bounds memory access

### Requirement: animate=false bypasses time-based reveal
When `animate` is false the plugin SHALL pass a large sentinel value for `draw_time_ms` (sufficient to exceed the total duration of all glyphs) so that every glyph is rendered at its full completed state. This sentinel SHALL be applied in the render handler before calling `renderHandwriting`; no change to `renderHandwriting` itself is required.

#### Scenario: All glyphs rendered at frame 0 when animate is false
- **WHEN** `animate` is false and the render time is frame 0
- **THEN** every glyph in the text is rendered fully drawn

#### Scenario: animate=true preserves existing time-driven behavior
- **WHEN** `animate` is true
- **THEN** glyphs reveal progressively based on `draw_time_ms` exactly as before
