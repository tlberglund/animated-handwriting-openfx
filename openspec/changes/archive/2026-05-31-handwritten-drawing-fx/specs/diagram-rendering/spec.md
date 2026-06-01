## ADDED Requirements

### Requirement: Plugin loads a diagram JSON file
The `HandwrittenDrawingFX` plugin SHALL load a diagram JSON file specified by a `diagramFile` string parameter. The JSON SHALL conform to the format `{name, aspectRatio, strokes[]}` where `strokes` is an array of stroke arrays, each stroke being an array of `{x, y, t, p}` points. If the parameter is empty or the file cannot be loaded, the plugin SHALL render a transparent frame.

#### Scenario: Valid diagram file renders strokes
- **WHEN** `diagramFile` points to a valid diagram JSON
- **THEN** the plugin renders the diagram's strokes onto the output frame

#### Scenario: Empty diagramFile produces transparent output
- **WHEN** the `diagramFile` parameter is empty
- **THEN** the output frame is fully transparent

#### Scenario: Missing or unreadable file produces transparent output
- **WHEN** `diagramFile` points to a path that does not exist or cannot be read
- **THEN** the output frame is fully transparent without crashing

### Requirement: Diagram is sized by diagramHeight in pixels
The plugin SHALL expose a `diagramHeight` integer parameter representing the pixel height of the diagram at full render scale. The pixel width SHALL be computed as `diagramHeight × aspectRatio` where `aspectRatio` is read from the loaded JSON. When a new instance is created the plugin SHALL initialize `diagramHeight` to `round(projectHeight × 0.3)`.

#### Scenario: diagramHeight controls vertical extent
- **WHEN** `diagramHeight` is doubled
- **THEN** the rendered diagram is twice as tall and twice as wide on screen

#### Scenario: Width is derived from aspectRatio
- **WHEN** the diagram JSON has `aspectRatio` = 2.0 and `diagramHeight` = 400
- **THEN** the diagram occupies 800 pixels horizontally and 400 pixels vertically

#### Scenario: Default height is 30% of frame height
- **WHEN** a new instance is created on a 1920×1080 project
- **THEN** `diagramHeight` is initialized to 324 (round(1080 × 0.3))

### Requirement: Diagram strokes animate sequentially
The plugin SHALL animate strokes in array order. Each stroke's duration is the `t` value of its last point. A global sequence cursor accumulates completed stroke durations; a stroke begins animating only after all preceding strokes have completed. The `speed` and `animate` parameters SHALL control playback exactly as they do in `HandwritingFX`.

#### Scenario: Second stroke begins after first completes
- **WHEN** draw_time_ms equals the duration of the first stroke
- **THEN** the second stroke begins to appear

#### Scenario: animate=false renders all strokes fully
- **WHEN** `animate` is false
- **THEN** all strokes are rendered fully drawn on every frame

### Requirement: Diagram position uses posX, posY, hAnchor, vAnchor
The plugin SHALL expose `posX`, `posY`, `hAnchor`, and `vAnchor` parameters with identical semantics to `HandwritingFX`. The anchor SHALL operate on the full diagram bounding box: width = `diagramHeight × aspectRatio`, height = `diagramHeight`.

#### Scenario: hAnchor Center, vAnchor Middle centers the diagram on posX/posY
- **WHEN** `hAnchor` = Center, `vAnchor` = Middle, `posX` = projectWidth/2, `posY` = projectHeight/2
- **THEN** the diagram is visually centered on the frame

#### Scenario: hAnchor Left aligns left edge to posX
- **WHEN** `hAnchor` = Left
- **THEN** the left edge of the diagram bounding box aligns to `posX_px`

### Requirement: Diagram rendering uses fill and outline passes
The plugin SHALL render diagrams using the same two-pass approach as `HandwritingFX`: outline pass (enlarged sigma, `outlineColor`) followed by fill pass (normal sigma, `fillColor`). The `strokeThickness`, `fillColor`, `outlineColor`, `outlineThickness`, and `outlineEnabled` parameters SHALL behave identically to their counterparts in `HandwritingFX`. Rotation SHALL be applied around `(posX_px, posY_px)` using the same pixel-coordinate rotation as `HandwritingFX`.

#### Scenario: Outline appears around strokes when outlineEnabled and outlineThickness > 0
- **WHEN** `outlineEnabled` is true and `outlineThickness` is greater than 0
- **THEN** a halo of `outlineColor` is visible around the edges of each stroke

#### Scenario: Rotation tilts the diagram around the position anchor
- **WHEN** `rotation` is non-zero
- **THEN** the diagram is rotated around `(posX_px, posY_px)` by that angle
