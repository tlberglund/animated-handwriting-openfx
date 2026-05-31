## Why

The plugin currently renders text as a single line only — newlines in the text parameter are ignored and there is no way to control how multiple lines are spaced or aligned. Title cards, captions, and multi-line lyric overlays all require this, making the plugin unusable for any multi-line use case.

## What Changes

- Multi-line text rendering: split the `text` value on `\n` and render each line at a separate baseline position
- Add `lineSpacing` double parameter (multiplier of cap-height, default 1.0, range –1.0 to 5.0) controlling the baseline-to-baseline distance between consecutive lines
- Add `textAlignment` choice parameter (Left / Center / Right, default Left) controlling how shorter lines are aligned within the text block when lines differ in width
- Update `kOfxActionInstanceChanged` to skip `\n` characters when generating `captureSelections` (newlines have no glyph and need no capture index)
- Update the renderer's hAnchor and vAnchor offset calculations to account for the full block width (max line width) and total block height (number of lines × line spacing)

## Capabilities

### New Capabilities

### Modified Capabilities
- `plugin-parameters`: adds `lineSpacing` (double) and `textAlignment` (choice) parameters
- `text-rendering`: multi-line rendering, per-line alignment, line spacing, updated block-size calculations for hAnchor/vAnchor

## Impact

- `src/plugin.cpp`: two new param definitions in `describeInContext`; `kOfxActionInstanceChanged` skips `\n` when building captureIndices; render handler reads two new params and passes them to `renderHandwriting`
- `src/renderer.h` / `src/renderer.cpp`: `renderHandwriting` gains `lineSpacing` and `textAlignment` params; text splitting, per-line layout, and multi-line sequencing replace the current single-line loop
- No new dependencies
