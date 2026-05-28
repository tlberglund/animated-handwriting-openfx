## MODIFIED Requirements

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
