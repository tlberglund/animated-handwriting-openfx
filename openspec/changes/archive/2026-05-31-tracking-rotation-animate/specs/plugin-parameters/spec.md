## ADDED Requirements

### Requirement: Plugin exposes a tracking parameter
The plugin SHALL define a double parameter named `tracking` ranging from −0.5 to 2.0 with a default of 0.0. The value is added to each glyph's width when accumulating the pen-advance position; 0.0 produces the default spacing baked into the capture data, positive values widen spacing, negative values tighten it.

#### Scenario: Tracking parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Tracking" is visible

#### Scenario: Tracking defaults to 0.0
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `tracking` parameter value is 0.0

#### Scenario: Positive tracking widens character spacing
- **WHEN** `tracking` is set to 0.5
- **THEN** each character is positioned further from the previous one than at the default spacing

#### Scenario: Negative tracking tightens character spacing
- **WHEN** `tracking` is set to −0.3
- **THEN** characters are positioned closer together than at the default spacing

### Requirement: Plugin exposes a rotation parameter
The plugin SHALL define a double parameter named `rotation` ranging from −180.0 to 180.0 degrees with a default of 0.0. The value controls the clockwise rotation of the entire rendered text block around the position anchor point `(posX_px, posY_px)`.

#### Scenario: Rotation parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Rotation" is visible

#### Scenario: Rotation defaults to 0.0
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `rotation` parameter value is 0.0

#### Scenario: Non-zero rotation tilts the text block
- **WHEN** `rotation` is set to 45.0
- **THEN** the rendered text block appears rotated 45 degrees clockwise around the position anchor

#### Scenario: Rotation of 0 produces identical output to previous behavior
- **WHEN** `rotation` is 0.0
- **THEN** the rendered output is identical to a render with no rotation applied

### Requirement: Plugin exposes an animate parameter
The plugin SHALL define a boolean parameter named `animate` with a default of true, displayed in the Inspector immediately below the `text` parameter. When `animate` is false, the plugin SHALL render all glyphs fully drawn on every frame, ignoring playback time.

#### Scenario: Animate parameter appears in Resolve Inspector below the text box
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a checkbox labelled "Animate" is visible immediately below the text input field

#### Scenario: Animate defaults to true
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `animate` parameter value is true

#### Scenario: animate=false renders all glyphs fully on frame 0
- **WHEN** `animate` is false and the playhead is at frame 0
- **THEN** all glyphs are rendered fully drawn as if the animation had already completed

#### Scenario: animate=false renders identically on all frames
- **WHEN** `animate` is false
- **THEN** the rendered output is the same on every frame regardless of playback time
