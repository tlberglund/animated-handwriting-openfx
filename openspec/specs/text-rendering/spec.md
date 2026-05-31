## ADDED Requirements

### Requirement: Plugin produces a non-empty output frame
The plugin SHALL implement `kOfxImageEffectActionRender` and write pixel data into the output image buffer. The output SHALL be a float RGBA image with premultiplied alpha.

#### Scenario: Frame is not blank when text is set and animation has progressed
- **WHEN** the `text` parameter contains at least one printable character and the render time is past frame 0
- **THEN** the rendered frame contains non-zero pixel values once any strokes have begun drawing

#### Scenario: Frame is transparent when text is empty
- **WHEN** the `text` parameter is an empty string
- **THEN** the rendered frame is fully transparent (all pixels are 0,0,0,0)

### Requirement: Plugin renders text using handwriting stroke captures
The plugin SHALL rasterise the `text` parameter value using stroke data from the active JSON capture set. Each stroke point SHALL be rendered as a Gaussian disc with radius proportional to the point's pressure value (`p`) and the `strokeThickness` parameter. Stroke color SHALL be determined by the `fillColor` parameter.

#### Scenario: Text is rendered using capture stroke data
- **WHEN** the `text` parameter contains ASCII characters and animation has fully completed
- **THEN** the output frame contains those characters rendered as handwriting strokes

#### Scenario: Fill color controls stroke appearance
- **WHEN** `fillColor` is set to a non-white value
- **THEN** the rendered strokes use that color

### Requirement: Stroke outline halo is rendered behind the fill
The plugin SHALL perform a two-pass render: first drawing all strokes with an enlarged Gaussian sigma using `outlineColor`, then drawing all strokes at normal sigma using `fillColor`. The outline SHALL only be visible beyond the fill boundary.

#### Scenario: Outline appears around strokes when outlineThickness is non-zero
- **WHEN** `outlineThickness` is greater than 0.0
- **THEN** a halo of `outlineColor` is visible around the edges of each stroke

#### Scenario: No outline drawn when outlineThickness is zero
- **WHEN** `outlineThickness` is 0.0
- **THEN** no outline pass is rendered and only the fill strokes are visible

### Requirement: Text is sized proportionally to frame height
The plugin SHALL compute `capHeightPx = round(textHeight × frame_height × render_scale_y)` and use this as the cap-height pixel size for all coordinate transformations. All capture coordinates (normalised to cap-height units) are multiplied by `capHeightPx` to obtain pixel offsets from the text origin.

#### Scenario: Text height scales with textHeight parameter
- **WHEN** `textHeight` is increased
- **THEN** the rendered strokes are proportionally larger

#### Scenario: Text height scales with render scale
- **WHEN** the host renders at half resolution (render_scale = 0.5)
- **THEN** capHeightPx is halved so the text occupies the same fractional area of the output

### Requirement: Text origin is derived from position and anchor parameters
The plugin SHALL compute the pixel-space rendering origin from `posX`, `posY`, `hAnchor`, `vAnchor`, `numLines`, and `lineSpacing`. Let `blockWidth = max(lineWidths)` (maximum per-line glyph-width sum, in cap-height units) and `numLines` be the number of lines (after splitting on `\n`).

- `posX_px = posX × render_scale_x`
- `posY_px = posY × render_scale_y`
- `originX_block`: Left → `posX_px`; Center → `posX_px − blockWidth × capHeightPx / 2`; Right → `posX_px − blockWidth × capHeightPx`
- `firstBaselineY`: Top → `posY_px − capHeightPx`; Bottom → `posY_px + (numLines − 1) × lineSpacing × capHeightPx`; Middle → `posY_px − capHeightPx / 2 − (numLines − 1) × lineSpacing × capHeightPx / 2`

For a single line these reduce to the previous formulas. Stroke pixels that fall outside the output image bounds SHALL be clipped.

#### Scenario: Default settings center the text block on the frame
- **WHEN** `posX` = `projectWidth / 2`, `posY` = `projectHeight / 2`, `hAnchor` = Center, `vAnchor` = Middle
- **THEN** the text block is visually centered on the frame regardless of number of lines

#### Scenario: Multi-line block is centered vertically
- **WHEN** `vAnchor` = Middle and the text contains three lines
- **THEN** the vertical midpoint of the three-line block aligns to `posY_px`

#### Scenario: Bottom-left positioning
- **WHEN** `posX` = 0, `posY` = 0, `hAnchor` = Left, `vAnchor` = Bottom
- **THEN** the baseline of the last line is at the bottom-left corner of the frame

#### Scenario: Right edge of block aligns to position point with Right hAnchor
- **WHEN** `hAnchor` = Right and `posX` = `projectWidth`
- **THEN** the right edge of the widest line aligns to the right edge of the frame

### Requirement: Plugin renders multi-line text split on newline characters
The plugin SHALL split the `text` parameter value on `\n` to produce an ordered list of lines. Each line SHALL be resolved into glyphs independently. Empty lines (produced by consecutive newlines) SHALL advance the baseline by one `lineSpacing` unit without rendering any strokes. Newline characters SHALL NOT consume a capture index in `captureSelections`.

#### Scenario: Newline splits text onto separate baselines
- **WHEN** the `text` parameter value is "Hello\nWorld"
- **THEN** "Hello" and "World" are rendered on separate baseline positions

#### Scenario: Empty line creates blank vertical spacing
- **WHEN** the `text` parameter value contains two consecutive newlines
- **THEN** an extra blank line's worth of vertical space appears between the surrounding lines

#### Scenario: Newline does not consume a capture index
- **WHEN** the `text` parameter contains a newline between two characters
- **THEN** the `captureSelections` parameter contains indices only for the non-newline characters

### Requirement: Plugin spaces lines using the lineSpacing parameter
The plugin SHALL compute successive baseline Y positions using the formula:
`lineBaselineY[i] = firstBaselineY − i × lineSpacing × capHeightPx`

where `firstBaselineY` is computed from `posY`, `vAnchor`, `numLines`, and `lineSpacing` as described in the text origin requirement.

#### Scenario: Line spacing controls vertical distance between baselines
- **WHEN** `lineSpacing` = 2.0 and the text has two lines
- **THEN** the second baseline is `2 × capHeightPx` pixels below the first baseline

### Requirement: Plugin aligns lines within the text block using the textAlignment parameter
For each line with width `lineWidth` and a block of width `blockWidth = max(lineWidths)`:
- Left (0): `lineOriginX = originX_block`
- Center (1): `lineOriginX = originX_block + (blockWidth − lineWidth) × capHeightPx / 2`
- Right (2): `lineOriginX = originX_block + (blockWidth − lineWidth) × capHeightPx`

#### Scenario: Left alignment starts all lines at the same x
- **WHEN** `textAlignment` = Left and lines differ in width
- **THEN** every line begins at the same horizontal position

#### Scenario: Right alignment ends all lines at the same x
- **WHEN** `textAlignment` = Right and lines differ in width
- **THEN** every line ends at the same horizontal position

### Requirement: Characters animate in reading order across all lines
The plugin SHALL maintain a single global `seqCursor` that accumulates across all lines in top-to-bottom order. Characters on later lines begin animating only after all characters on earlier lines have completed their animation.

#### Scenario: Second line begins animating after first line completes
- **WHEN** the animation time has passed the total duration of all glyphs on the first line
- **THEN** strokes on the second line begin to appear

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
