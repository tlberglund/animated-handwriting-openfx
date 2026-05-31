## ADDED Requirements

### Requirement: Plugin exposes a multiline text parameter
The plugin SHALL define a string parameter named `text` with string mode set to multiline. The parameter SHALL have an empty string as its default value and SHALL be visible in the host UI. The plugin SHALL read this parameter during render and use its value as the string to rasterise.

#### Scenario: Text parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a multiline text input field labelled "Text" is visible

#### Scenario: Text parameter default is empty
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the text parameter value is an empty string

#### Scenario: Changing text updates the rendered output
- **WHEN** the `text` parameter value is changed
- **THEN** the next rendered frame reflects the new text content

### Requirement: Plugin exposes a text height parameter
The plugin SHALL define a double parameter named `textHeight` ranging from 0.0 to 1.0 with a default of 0.1, representing text height as a fraction of the output frame height. The plugin SHALL read this parameter during render and use its value to determine the FreeType pixel size.

#### Scenario: Text height parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Text Height" is visible

#### Scenario: Text height defaults to 10% of frame
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `textHeight` parameter value is 0.1

#### Scenario: Changing text height updates rendered text size
- **WHEN** the `textHeight` parameter value is changed
- **THEN** the next rendered frame shows the text at the new proportional size

### Requirement: Plugin exposes a stroke thickness parameter
The plugin SHALL define a double parameter named `strokeThickness` ranging from 0.0 to 0.2 (cap-height units) with a default of 0.02. This value controls the base Gaussian sigma for stroke rendering, modulated per point by the pressure value `p`.

#### Scenario: Stroke thickness parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Stroke Thickness" is visible

#### Scenario: Stroke thickness defaults to 0.02 cap-height units
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `strokeThickness` parameter value is 0.02

#### Scenario: Increasing stroke thickness produces visually thicker strokes
- **WHEN** the `strokeThickness` parameter value is increased
- **THEN** the rendered strokes appear wider

### Requirement: Plugin exposes a speed parameter
The plugin SHALL define a double parameter named `speed` ranging from 0.0 to 10.0 with a default of 1.0, representing a playback speed multiplier where 1.0 is normal speed.

#### Scenario: Speed parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Speed" is visible

#### Scenario: Speed parameter defaults to 1.0
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `speed` parameter value is 1.0

### Requirement: Plugin exposes an audio toggle parameter
The plugin SHALL define a boolean parameter named `audio` with a default of false, presented as a checkbox in the host UI.

#### Scenario: Audio parameter appears in Resolve Inspector as a checkbox
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a checkbox labelled "Audio" is visible

#### Scenario: Audio parameter defaults to off
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `audio` parameter value is false

### Requirement: Plugin exposes a glyph set file path parameter
The plugin SHALL define a string parameter named `glyphSet` with string mode set to `kOfxParamStringIsFilePath`. When empty, the plugin SHALL use the bundled default capture set. The parameter SHALL be visible in the host UI.

#### Scenario: Glyph set parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a file path input labelled "Glyph Set" is visible

#### Scenario: Empty value uses bundled default
- **WHEN** the `glyphSet` parameter is empty
- **THEN** the plugin loads and renders using the bundled `tim-hand.json`

### Requirement: Plugin exposes a hidden capture selections parameter
The plugin SHALL define a string parameter named `captureSelections` with `kOfxParamPropSecret` set to true. This parameter SHALL store a comma-separated list of capture indices, one per character of the current `text` value, and SHALL be persisted with the Resolve project.

#### Scenario: Capture selections are not visible in the Inspector
- **WHEN** the HandwritingFX Inspector panel is open
- **THEN** no control labelled "captureSelections" or similar is visible to the user

### Requirement: Plugin exposes a fill color parameter
The plugin SHALL define an RGBA parameter named `fillColor` with a default of opaque white (1.0, 1.0, 1.0, 1.0). This color SHALL be used for the main stroke rendering pass.

#### Scenario: Fill color parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a color picker labelled "Fill Color" is visible

#### Scenario: Fill color defaults to opaque white
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `fillColor` parameter value is (1.0, 1.0, 1.0, 1.0)

### Requirement: Plugin exposes an outline color parameter
The plugin SHALL define an RGBA parameter named `outlineColor` with a default of opaque black (0.0, 0.0, 0.0, 1.0). This color SHALL be used for the outline halo rendering pass.

#### Scenario: Outline color parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a color picker labelled "Outline Color" is visible

#### Scenario: Outline color defaults to opaque black
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `outlineColor` parameter value is (0.0, 0.0, 0.0, 1.0)

### Requirement: Plugin exposes an outline thickness parameter
The plugin SHALL define a double parameter named `outlineThickness` ranging from 0.0 to 0.1 (cap-height units) with a default of 0.01. A value of 0.0 disables the outline pass entirely.

#### Scenario: Outline thickness parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Outline Thickness" is visible

#### Scenario: Outline thickness defaults to 0.01
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `outlineThickness` parameter value is 0.01

### Requirement: Plugin exposes X and Y position parameters
The plugin SHALL define two double parameters named `posX` and `posY` storing absolute pixel coordinates at full project resolution. Both parameters SHALL have an unconstrained range with a display range of –8192 to 8192. The parameter default SHALL be 0 at definition time; when a new instance is created the plugin SHALL read `kOfxImageEffectPropProjectSize` and call `paramSetValue` to initialize `posX` to `projectWidth / 2` and `posY` to `projectHeight / 2`.

#### Scenario: Position parameters appear in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** two numeric fields labelled "Position X" and "Position Y" are visible

#### Scenario: Position parameters initialize to project center
- **WHEN** a new instance of the HandwritingFX generator is created on a 1920×1080 project
- **THEN** `posX` is 960 and `posY` is 540

#### Scenario: Changing position moves the rendered text
- **WHEN** `posX` or `posY` is changed
- **THEN** the text block moves to the new position on the next rendered frame

### Requirement: Plugin exposes horizontal and vertical anchor parameters
The plugin SHALL define two choice parameters: `hAnchor` (choices: Left / Center / Right, default Center) and `vAnchor` (choices: Top / Middle / Bottom, default Middle). These control which point on the text block is aligned to the position coordinate.

#### Scenario: Anchor parameters appear in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** two dropdown selectors labelled "H Anchor" and "V Anchor" are visible

#### Scenario: Anchor parameters default to Center and Middle
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** `hAnchor` is Center (index 1) and `vAnchor` is Middle (index 1)

#### Scenario: Changing hAnchor shifts text horizontally relative to the position point
- **WHEN** `hAnchor` is set to Left
- **THEN** the left edge of the text block aligns to the `posX` coordinate
- **WHEN** `hAnchor` is set to Right
- **THEN** the right edge of the text block aligns to the `posX` coordinate

#### Scenario: Changing vAnchor shifts text vertically relative to the position point
- **WHEN** `vAnchor` is set to Top
- **THEN** the cap-top of the text block aligns to the `posY` coordinate
- **WHEN** `vAnchor` is set to Bottom
- **THEN** the baseline of the text block aligns to the `posY` coordinate

### Requirement: Plugin exposes a line spacing parameter
The plugin SHALL define a double parameter named `lineSpacing` ranging from −1.0 to 5.0 with a default of 1.0. The value is a multiplier of `capHeightPx`; it controls the baseline-to-baseline distance between consecutive lines of text. A value of 1.0 places baselines exactly one cap-height apart. Negative values cause lines to overlap.

#### Scenario: Line spacing parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Line Spacing" is visible

#### Scenario: Line spacing defaults to 1.0
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `lineSpacing` parameter value is 1.0

#### Scenario: Increasing line spacing spreads lines further apart
- **WHEN** `lineSpacing` is set to 2.0 and the `text` parameter contains two lines
- **THEN** the baseline-to-baseline distance between the two lines is twice the cap-height

#### Scenario: Negative line spacing causes lines to overlap
- **WHEN** `lineSpacing` is set to −0.5
- **THEN** consecutive lines overlap each other visually

### Requirement: Plugin exposes a text alignment parameter
The plugin SHALL define a choice parameter named `textAlignment` with options Left (0), Center (1), and Right (2), defaulting to Left (0). This parameter controls how shorter lines are aligned horizontally within the text block when lines differ in width. It is independent of the `hAnchor` parameter, which controls where the block as a whole is positioned.

#### Scenario: Text alignment parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a dropdown selector labelled "Text Alignment" is visible with options Left, Center, Right

#### Scenario: Text alignment defaults to Left
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** `textAlignment` is Left (index 0)

#### Scenario: Center alignment centers shorter lines within the block
- **WHEN** `textAlignment` is Center and the second line is shorter than the first
- **THEN** the second line is horizontally centered relative to the first line
