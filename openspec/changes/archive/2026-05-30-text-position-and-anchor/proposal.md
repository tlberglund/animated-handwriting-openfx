## Why

Text is currently rendered anchored to the bottom-left of the frame with no way to reposition it. Operators need to place text at arbitrary positions in the frame and control which point on the text block aligns to that position.

## What Changes

- Add a 2D position parameter (X, Y in pixels) that sets where the text is placed in the frame
- Add a horizontal anchor choice (Left / Center / Right) that controls which horizontal edge of the text block aligns to the position X coordinate
- Add a vertical anchor choice (Top / Middle / Bottom) that controls which vertical edge of the text block aligns to the position Y coordinate
- Default position is the center of the output frame (computed at render time from frame dimensions)
- Default anchor is Center / Middle, so text is centered on the position point by default

## Capabilities

### New Capabilities

### Modified Capabilities
- `plugin-parameters`: adds `position` (Double2D, pixels), `hAnchor` (choice: Left/Center/Right), and `vAnchor` (choice: Top/Middle/Bottom)
- `text-rendering`: rendering origin is now derived from position and anchor rather than fixed at (0, 0)

## Impact

- `src/plugin.cpp`: new params in `describeInContext`; read at render time
- `src/renderer.cpp` / `src/renderer.h`: `renderHandwriting` receives an origin offset; pen-X and baseline-Y shift accordingly
- No new dependencies
