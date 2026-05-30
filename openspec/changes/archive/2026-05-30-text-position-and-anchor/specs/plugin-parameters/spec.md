## ADDED Requirements

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

## MODIFIED Requirements

### Requirement: Text origin is at the bottom-left of the frame
The plugin SHALL position the text using the `posX`, `posY`, `hAnchor`, and `vAnchor` parameters rather than a fixed bottom-left origin. When `posX` = 0.5, `posY` = 0.5, `hAnchor` = Center, and `vAnchor` = Middle the text SHALL be centered on the frame. Stroke pixels that fall outside the output image bounds SHALL be clipped.

#### Scenario: Default settings center the text on the frame
- **WHEN** the plugin renders with default parameter values and a non-empty `text` value
- **THEN** stroke pixels are concentrated near the center of the output frame
