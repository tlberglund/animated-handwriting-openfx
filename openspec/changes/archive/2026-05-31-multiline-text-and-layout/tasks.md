## 1. Add Parameters in describeInContext

- [x] 1.1 In `plugin.cpp` `describeInContext`, add `lineSpacing` double parameter: range –1.0 to 5.0, default 1.0, label "Line Spacing"
- [x] 1.2 Add `textAlignment` choice parameter with options Left / Center / Right, default index 0 (Left), label "Text Alignment"

## 2. Fix Newline Handling in kOfxActionInstanceChanged

- [x] 2.1 In the `kOfxActionInstanceChanged` "text" handler, add `if(text[pos] == '\n') { ++pos; continue; }` at the top of the walk loop so newlines are skipped without pushing a capture index

## 3. Read New Parameters at Render Time

- [x] 3.1 In the render handler, add `double lineSpacing = 1.0` and `int textAlignment = 0` variables
- [x] 3.2 Read `lineSpacing` via `paramGetValueAtTime`
- [x] 3.3 Read `textAlignment` via `paramGetValueAtTime`
- [x] 3.4 Pass `lineSpacing` and `textAlignment` to `renderHandwriting`

## 4. Update renderer.h Interface

- [x] 4.1 Add `double lineSpacing` and `int textAlignment` to the `renderHandwriting` declaration in `renderer.h`

## 5. Refactor renderHandwriting for Multi-Line

- [x] 5.1 Add `lineSpacing` and `textAlignment` parameters to the `renderHandwriting` definition in `renderer.cpp`
- [x] 5.2 Before glyph resolution, split `text` on `\n` to produce a `std::vector<std::string> lines`
- [x] 5.3 For each line, resolve glyphs (same ligature-substitution logic as before) and compute `lineWidth` (sum of resolved glyph widths); accumulate into a `perLineResolved` structure
- [x] 5.4 Compute `blockWidth = max(lineWidths)` across all lines (0.0 if no lines)
- [x] 5.5 Replace the existing single-line hAnchor `originX` computation with: Left → `posX_px`; Center → `posX_px − blockWidth × capHeightPx / 2`; Right → `posX_px − blockWidth × capHeightPx`; call this `originX_block`
- [x] 5.6 Replace the existing single-line vAnchor `baselineY` computation with the multi-line formula for `firstBaselineY`: Top → `posY_px − capHeightPx`; Bottom → `posY_px + (numLines−1) × lineSpacing × capHeightPx`; Middle → `posY_px − capHeightPx/2 − (numLines−1) × lineSpacing × capHeightPx / 2`
- [x] 5.7 Build the animation sequence by iterating over lines and within each line over resolved glyphs; `seqCursor` accumulates globally; `lineBaselineY[i] = firstBaselineY − i × lineSpacing × capHeightPx`
- [x] 5.8 For each glyph compute `lineOriginX` from `textAlignment`, `originX_block`, `lineWidth`, and `blockWidth`; store `lineOriginX` and `lineBaselineY` in the GlyphSeq struct (or pass alongside)

## 6. Thread Per-Glyph Origin Through Render Passes

- [x] 6.1 Update the `GlyphSeq` local struct to carry `float originX` and `float baselineY` (previously these were the same for all glyphs; now they vary per line)
- [x] 6.2 In the outline pass, pass `gs.originX` and `gs.baselineY` to `splatStrokes`
- [x] 6.3 In the fill pass, pass `gs.originX` and `gs.baselineY` to `splatStrokes`

## 7. Build and Verify

- [x] 7.1 Run `cmake --build build --target bundle` and confirm no errors or warnings
- [x] 7.2 Install to `/Library/OFX/Plugins/`, restart Resolve; confirm single-line text still works as before
- [ ] 7.3 Enter two-line text (with Enter key); confirm both lines render at separate baselines
- [x] 7.4 Verify `lineSpacing` = 2.0 doubles the baseline gap; verify `lineSpacing` = 0.5 halves it
- [x] 7.5 Verify `textAlignment` Left / Center / Right positions shorter lines correctly within the block
- [x] 7.6 Verify vAnchor Middle with three lines centers the block vertically on the frame
- [x] 7.7 Verify hAnchor Right with two lines of different widths aligns the widest line's right edge to posX
