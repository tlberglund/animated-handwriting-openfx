## ADDED Requirements

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

## MODIFIED Requirements

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
