## ADDED Requirements

### Requirement: Plugin loads a JSON handwriting capture set
The plugin SHALL load a glyph set from a JSON file containing handwriting capture data. When the `glyphSet` parameter is empty, the plugin SHALL use the capture set bundled inside the `.ofx.bundle`. When `glyphSet` contains a valid file path, the plugin SHALL load from that path. On any load failure the plugin SHALL fall back to the bundled default.

#### Scenario: Bundled default loads automatically
- **WHEN** the `glyphSet` parameter is empty and the plugin is placed on a timeline
- **THEN** the bundled `tim-hand.json` is loaded and used for rendering without any user configuration

#### Scenario: User-supplied path overrides bundled default
- **WHEN** the `glyphSet` parameter contains the path to a valid JSON capture file
- **THEN** glyphs are rendered using captures from that file

#### Scenario: Invalid path falls back to bundled default
- **WHEN** the `glyphSet` parameter contains a path to a file that does not exist or cannot be parsed
- **THEN** the plugin falls back to the bundled default and does not crash

### Requirement: Plugin selects captures randomly per text edit and locks the selection
The plugin SHALL assign each character in the `text` value a randomly selected capture index when the user edits the `text` parameter. The selection SHALL be persisted in the `captureSelections` hidden parameter and SHALL NOT change between renders unless the user edits `text` again.

#### Scenario: Capture selection is stable across renders
- **WHEN** the `text` parameter has not been changed and the timeline is scrubbed
- **THEN** each character uses the same capture in every rendered frame

#### Scenario: Capture selection re-randomises on text edit
- **WHEN** the user changes the `text` parameter value
- **THEN** new random capture indices are assigned to each character in the new text

#### Scenario: New characters get fresh random selection
- **WHEN** the user appends characters to the existing `text` value
- **THEN** existing character capture assignments are preserved and only the new characters receive fresh random selections

### Requirement: Plugin animates handwriting strokes over time
The plugin SHALL reveal strokes progressively over time, mapping the OFX render time to a draw cursor in milliseconds using the formula `draw_time_ms = (frame / fps) * 1000 * speed`. Characters SHALL be sequenced end-to-end: each character begins drawing after the previous character's last stroke completes.

#### Scenario: Frame 0 produces a blank frame
- **WHEN** the current render time is frame 0
- **THEN** the output frame is fully transparent (no strokes drawn)

#### Scenario: Strokes reveal progressively
- **WHEN** the render time advances from frame 0
- **THEN** strokes appear progressively, with each character beginning after the previous one completes

#### Scenario: All strokes visible when draw time exceeds total duration
- **WHEN** the render time corresponds to a draw_time_ms greater than the sum of all character capture durations
- **THEN** all strokes are fully drawn

#### Scenario: In-progress stroke tip interpolates smoothly
- **WHEN** the draw cursor falls between two consecutive points within a stroke
- **THEN** the rendered stroke tip is at the linearly interpolated position between those two points

### Requirement: Plugin performs ligature substitution
Before looking up each character individually, the plugin SHALL scan the text for the longest matching multi-character key in the glyph set. When a ligature match is found, the plugin SHALL render the ligature capture as a single glyph and advance the pen by the ligature's capture width.

#### Scenario: Common digraph renders as single ligature
- **WHEN** the `text` value contains the sequence "th"
- **THEN** the "th" ligature capture is used rather than separate "t" and "h" glyphs

#### Scenario: Longest ligature wins
- **WHEN** the text contains "tion" and both "ti" and "tion" exist in the glyph set
- **THEN** "tion" is matched and rendered as a single glyph

### Requirement: Space character advances the pen without drawing
The plugin SHALL advance the pen position by 0.35 cap-height units when a space character is encountered. No strokes SHALL be drawn and no animation time SHALL be consumed for a space.

#### Scenario: Space produces a gap between words
- **WHEN** the `text` value contains a space between two words
- **THEN** a visible gap appears between the rendered words in the output frame
