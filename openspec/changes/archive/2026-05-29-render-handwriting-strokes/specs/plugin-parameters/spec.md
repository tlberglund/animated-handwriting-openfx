## ADDED Requirements

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

## MODIFIED Requirements

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
