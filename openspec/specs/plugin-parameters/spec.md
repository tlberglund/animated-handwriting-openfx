## ADDED Requirements

### Requirement: Plugin exposes a multiline text parameter
The plugin SHALL define a string parameter named `text` with string mode set to multiline. The parameter SHALL have an empty string as its default value and SHALL be visible in the host UI.

#### Scenario: Text parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a multiline text input field labelled "Text" is visible

#### Scenario: Text parameter default is empty
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the text parameter value is an empty string

### Requirement: Plugin exposes a text height parameter
The plugin SHALL define a double parameter named `textHeight` ranging from 0.0 to 1.0 with a default of 0.1, representing text height as a fraction of the output frame height.

#### Scenario: Text height parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Text Height" is visible

#### Scenario: Text height defaults to 10% of frame
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `textHeight` parameter value is 0.1

### Requirement: Plugin exposes a stroke thickness parameter
The plugin SHALL define a double parameter named `strokeThickness` ranging from 0.0 to 50.0 pixels with a default of 2.0.

#### Scenario: Stroke thickness parameter appears in Resolve Inspector
- **WHEN** the HandwritingFX generator is added to a Resolve timeline and the Inspector panel is open
- **THEN** a numeric slider labelled "Stroke Thickness" is visible

#### Scenario: Stroke thickness defaults to 2 pixels
- **WHEN** a new instance of the HandwritingFX generator is created
- **THEN** the `strokeThickness` parameter value is 2.0

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
