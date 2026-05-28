## ADDED Requirements

### Requirement: Plugin produces a non-empty output frame
The plugin SHALL implement `kOfxImageEffectActionRender` and write pixel data into the output image buffer. The output SHALL be a float RGBA image with premultiplied alpha.

#### Scenario: Frame is not blank when text is set
- **WHEN** the `text` parameter contains at least one printable character
- **THEN** the rendered frame contains non-zero pixel values

#### Scenario: Frame is transparent when text is empty
- **WHEN** the `text` parameter is an empty string
- **THEN** the rendered frame is fully transparent (all pixels are 0,0,0,0)

### Requirement: Plugin renders text using Open Sans Regular
The plugin SHALL rasterise the `text` parameter value using the Open Sans Regular typeface bundled inside the `.ofx.bundle`. Glyphs SHALL be white (R=1, G=1, B=1) with alpha derived from the FreeType coverage value.

#### Scenario: Text is rendered in Open Sans
- **WHEN** the `text` parameter contains ASCII characters
- **THEN** the output frame contains those characters rendered in Open Sans Regular

#### Scenario: Font is loaded from the bundle
- **WHEN** the plugin is loaded from any install path
- **THEN** the font is located relative to the plugin binary and loads without error

### Requirement: Text is sized proportionally to frame height
The plugin SHALL set the FreeType pixel size to `round(textHeight × frame_height × render_scale_y)`. At the default `textHeight` of 0.1, text SHALL occupy approximately 10% of the frame height.

#### Scenario: Text height scales with textHeight parameter
- **WHEN** `textHeight` is set to 0.2 on a 1080-line frame at full render scale
- **THEN** the rendered glyphs are approximately 216 pixels tall

#### Scenario: Text height scales with render scale
- **WHEN** the host renders at half resolution (render_scale = 0.5)
- **THEN** the pixel size is halved so the text occupies the same fractional area of the output

### Requirement: Text origin is at the bottom-left of the frame
The plugin SHALL position the text baseline at pixel coordinate (0, 0) in OFX image space (bottom-left origin). Glyph pixels that fall outside the output image bounds SHALL be clipped.

#### Scenario: Text appears at lower-left
- **WHEN** the plugin renders with default settings and a non-empty `text` value
- **THEN** glyph pixels are visible near the bottom-left corner of the output frame
