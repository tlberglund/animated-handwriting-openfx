## ADDED Requirements

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
