## Context

The renderer builds a flat `GlyphSeq` sequence whose entries carry per-glyph pen position, sequence start time, line origin, and baseline. Stroke pixels are written directly into the OFX output buffer using `splatStrokes` → `splatSegment`. There is currently no character-spacing adjustment, no geometric transform on the block, and no way to disable the time-based reveal.

## Goals / Non-Goals

**Goals:**
- Add `tracking` to scale glyph pen-advance so characters spread apart or tighten
- Add `rotation` to rotate the entire rendered text block (all lines, all glyphs) around the position anchor point `(posX_px, posY_px)`
- Add `animate` boolean; when false, pass a `draw_time_ms` large enough to render every glyph fully drawn on every frame

**Non-Goals:**
- Per-character or per-word rotation
- Rotation of individual glyph strokes independently
- Sub-pixel antialiasing improvements
- Skew or perspective transforms

## Decisions

### Tracking: scale pen-advance between glyphs

**Decision:** Multiply each `ResolvedGlyph::width` by `(1.0 + tracking)` when accumulating `penX`, where `tracking` defaults to 0.0. Range −0.5 to 2.0 gives a usable spread. Tracking does not affect the glyph's internal stroke coordinates, only the advance.

**Why not an absolute pixel offset:** Cap-height-unit tracking scales correctly with `textHeight` without extra math.

**Alternative considered:** Add tracking only between glyphs (not after the last). Rejected — simpler to apply uniformly; the difference is negligible for text blocks.

### Rotation: rotate pixel coordinates around the anchor

**Decision:** Apply a 2D rotation to `(pax, pay)` and `(pbx, pby)` inside `splatSegment`, rotating around `(posX_px, posY_px)`. Pass `rotSin` and `rotCos` (precomputed from the rotation angle in degrees) as additional parameters through `splatStrokes` → `splatSegment`.

**Why at the segment level:** The bounding-box clamp in `splatSegment` already uses the rotated pixel coordinates, so computing rotation there avoids a second pass or an intermediate buffer. The cost is two extra float multiplications per pixel lookup, which is negligible.

**Alternative considered:** Rotate the entire `originX`/`baselineY` coordinate system before building the sequence, then render normally. Rejected — straight horizontal strokes would remain horizontal; rotation must happen at the pixel coordinate level.

**Rotation center:** Always `(posX_px, posY_px)` — the same point that hAnchor/vAnchor align to. This keeps the block pinned to its anchor while it spins.

### Animate: bypass time gating

**Decision:** In `plugin.cpp`, when `animate == false` pass `draw_time_ms = 1e12` (a large sentinel) to `renderHandwriting`. No change to the renderer itself.

**Why a sentinel rather than a renderer flag:** The renderer already handles `draw_time_ms > total_duration` correctly by clamping `tLocal`. Adding a flag would duplicate that logic. A large sentinel is already idiomatic in the existing code.

### UI placement of Animate checkbox

The OFX parameter order in `describeInContext` determines Inspector order. Place `animate` immediately after `text` so it appears just below the text box as requested.

## Risks / Trade-offs

**Rotation bounding-box conservatism** → The pad used in `splatSegment` is computed from the unrotated endpoints. After rotation the actual segment may extend slightly beyond the pad. For small angles this is invisible; for 90°+ the bounding box may clip a few pixels at the extremes. Mitigation: add a small constant to `pad` proportional to the segment length when rotation is non-zero. Acceptable for V1; note in code.

**Tracking = −0.5 causes overlap** → Valid intentional use (tight monogram-style text). No mitigation needed.

**Existing single-line projects unaffected** → `tracking=0`, `rotation=0`, `animate=true` reproduce previous behavior exactly.

## Open Questions

None — implementation can proceed.
