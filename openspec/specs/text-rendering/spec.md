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
The plugin SHALL compute the pixel-space rendering origin from `posX`, `posY`, `hAnchor`, and `vAnchor` rather than using a fixed bottom-left anchor. The baseline pixel Y coordinate (`baselineY`) and the pen-start pixel X coordinate (`originX`) SHALL be computed as follows:

- `posX_px = posX × render_scale_x`
- `posY_px = posY × render_scale_y`
- `originX`: Left → `posX_px`; Center → `posX_px − totalWidth × capHeightPx / 2`; Right → `posX_px − totalWidth × capHeightPx`
- `baselineY`: Top → `posY_px − capHeightPx`; Middle → `posY_px − capHeightPx / 2`; Bottom → `posY_px`

where `totalWidth` is the sum of all resolved glyph `width` values (in cap-height units). Stroke pixels that fall outside the output image bounds SHALL be clipped.

#### Scenario: Default settings center the text on the frame
- **WHEN** `posX` = `projectWidth / 2` (e.g., 960 on a 1920-wide project), `posY` = `projectHeight / 2`, `hAnchor` = Center, `vAnchor` = Middle
- **THEN** stroke pixels are concentrated near the center of the output frame

#### Scenario: Bottom-left positioning
- **WHEN** `posX` = 0, `posY` = 0, `hAnchor` = Left, `vAnchor` = Bottom
- **THEN** the text baseline begins at the bottom-left corner of the frame

#### Scenario: Right-aligned text
- **WHEN** `hAnchor` = Right and `posX` = `projectWidth` (e.g., 1920 on a 1920-wide project)
- **THEN** the right edge of the final glyph aligns to the right edge of the frame

#### Scenario: Anchor offset scales with capHeightPx
- **WHEN** `textHeight` is increased
- **THEN** the anchor offset grows proportionally so the text remains visually centered on the position point
