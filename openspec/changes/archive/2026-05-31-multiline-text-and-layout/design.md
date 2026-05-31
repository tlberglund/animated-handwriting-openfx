## Context

The renderer currently treats the entire `text` parameter as a single string of glyphs. Newlines are either silently ignored or produce an unknown-glyph slot. There is no concept of a text block with multiple baseline positions. The existing position/anchor system was designed for a single line and uses `totalWidth` (flat sum of all glyph widths) for hAnchor offsets; with multiple lines this must become `blockWidth = max(lineWidths)`.

## Goals / Non-Goals

**Goals:**
- Split `text` on `\n` and render each segment at a separate baseline
- Add `lineSpacing` parameter controlling baseline-to-baseline distance as a multiple of cap-height
- Add `textAlignment` parameter controlling per-line horizontal alignment within the block
- Keep animation sequential: line 1 fully before line 2 (reading order)
- Update hAnchor and vAnchor to operate on the full block dimensions

**Non-Goals:**
- Vertical text
- Rich text (mixed sizes, colors per line)
- Automatic word-wrap
- Kerning or inter-glyph spacing adjustments

## Decisions

### Line splitting and empty lines

**Decision:** Split on `\n`. Empty segments (consecutive newlines) advance the baseline by one `lineSpacing` unit without rendering any strokes, acting as a blank line.

**Why:** Intuitive — the user hits Enter twice to get extra spacing, matching every text editor.

### captureSelections and newlines

**Decision:** In `kOfxActionInstanceChanged`, skip `\n` characters — do not push a capture index. In `renderHandwriting`, split text into lines before glyph resolution, so `\n` is never presented to the glyph resolver.

**Why not push a placeholder index for `\n`:** The renderer needs to know which characters consumed indices so it can look up the right capture. Skipping on both sides (producer and consumer) keeps the index list unambiguous without adding a special sentinel value.

### Block width for hAnchor

**Decision:** `blockWidth = max(lineWidths)` across all lines. hAnchor uses `blockWidth` to compute `originX_block`.

**Why not first-line width:** Right-anchoring or center-anchoring must align the block as a whole to the position point, not just the first line.

### Per-line alignment within block

For each line with width `lineWidth`:
- Left: `lineOriginX = originX_block`
- Center: `lineOriginX = originX_block + (blockWidth − lineWidth) × capHeightPx / 2`
- Right: `lineOriginX = originX_block + (blockWidth − lineWidth) × capHeightPx`

**Note:** `textAlignment` is independent of `hAnchor`. hAnchor anchors the block to the position point; `textAlignment` aligns shorter lines within the block.

### vAnchor block height

Block height (cap-top of first line to baseline of last line):

```
blockHeight = capHeightPx × (1 + (numLines − 1) × lineSpacing)
```

Updated formulas:
- Top: `firstBaselineY = posY_px − capHeightPx`
- Bottom: `firstBaselineY = posY_px + (numLines − 1) × lineSpacing × capHeightPx`
- Middle: `firstBaselineY = posY_px − capHeightPx / 2 − (numLines − 1) × lineSpacing × capHeightPx / 2`

For `numLines = 1` these reduce to the existing single-line formulas. ✓

### Animation sequencing across lines

**Decision:** seqCursor accumulates across all lines in reading order (line 0 → line 1 → …). Each glyph's `seqStart` is its cumulative start time regardless of line.

**Why not per-line independent timers:** A single draw_time_ms drives the whole reveal. Resetting the timer per line would require an additional "line offset" parameter and complicates scrubbing.

### lineSpacing range and default

Range: −1.0 to 5.0. Default: 1.0 (baselines one cap-height apart — compact, appropriate for handwriting).

Negative values are valid: they cause lines to overlap, which can be used for stylistic layering effects.

## Risks / Trade-offs

**Existing single-line projects unaffected** → All changes are additive. `numLines = 1` codepath reduces to previous behavior.

**Animation plays all lines sequentially** → A long multi-line string animates slowly. The existing `speed` parameter mitigates this; no additional control is needed now.

**lineSpacing = 0 collapses all lines to the same baseline** → Valid (intentional overlap); the renderer handles it without division by zero.

## Open Questions

None — implementation can proceed.
