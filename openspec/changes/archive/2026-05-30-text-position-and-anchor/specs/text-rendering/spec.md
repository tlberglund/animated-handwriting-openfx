## MODIFIED Requirements

### Requirement: Text origin is derived from position and anchor parameters
The plugin SHALL compute the pixel-space rendering origin from `posX`, `posY`, `hAnchor`, and `vAnchor` rather than using a fixed bottom-left anchor. The baseline pixel Y coordinate (`baselineY`) and the pen-start pixel X coordinate (`originX`) SHALL be computed as follows:

- `posX_px = posX × render_scale_x`
- `posY_px = posY × render_scale_y`
- `originX`: Left → `posX_px`; Center → `posX_px − totalWidth × capHeightPx / 2`; Right → `posX_px − totalWidth × capHeightPx`
- `baselineY`: Top → `posY_px − capHeightPx`; Middle → `posY_px − capHeightPx / 2`; Bottom → `posY_px`

where `totalWidth` is the sum of all resolved glyph `width` values (in cap-height units). Stroke pixels that fall outside the output image bounds SHALL be clipped.

#### Scenario: Default settings center the text on the frame
- **WHEN** `posX` = 0.5, `posY` = 0.5, `hAnchor` = Center, `vAnchor` = Middle
- **THEN** stroke pixels are concentrated near the center of the output frame

#### Scenario: Bottom-left positioning
- **WHEN** `posX` = 0.0, `posY` = 0.0, `hAnchor` = Left, `vAnchor` = Bottom
- **THEN** the text baseline begins at the bottom-left corner of the frame (matching the previous default behavior)

#### Scenario: Right-aligned text
- **WHEN** `hAnchor` = Right and `posX` = 1.0
- **THEN** the right edge of the final glyph aligns to the right edge of the frame

#### Scenario: Anchor offset scales with capHeightPx
- **WHEN** `textHeight` is increased
- **THEN** the anchor offset grows proportionally so the text remains visually centered on the position point
