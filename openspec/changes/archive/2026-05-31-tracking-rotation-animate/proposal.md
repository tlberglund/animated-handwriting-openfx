## Why

Three independent but low-complexity parameters — tracking (character spacing), rotation, and an animate toggle — are natural additions to a handwriting generator but currently absent, limiting the plugin's utility for title cards, kinetic text, and static display use cases.

## What Changes

- Add a `tracking` double parameter that scales the pen-advance distance between glyphs, widening or tightening character spacing
- Add a `rotation` double parameter (−180 to 180 degrees) that rotates the entire rendered text block around the position anchor point
- Add an `animate` boolean parameter (default true, displayed immediately below the text input) that, when false, renders all glyphs fully drawn on every frame regardless of playback time

## Capabilities

### New Capabilities

*(none — all changes extend existing capabilities)*

### Modified Capabilities

- `plugin-parameters`: Three new parameters (`tracking`, `rotation`, `animate`) with their ranges, defaults, and UI placement
- `text-rendering`: Tracking modifies pen-advance; rotation transforms the block around the anchor; animate=false bypasses the time-based reveal

## Impact

- `src/plugin.cpp`: New parameter definitions in `describeInContext`; read at render time
- `src/renderer.h` / `src/renderer.cpp`: `renderHandwriting` signature gains `tracking`, `rotation`, `animate`; glyph advance and pixel-space transforms updated
