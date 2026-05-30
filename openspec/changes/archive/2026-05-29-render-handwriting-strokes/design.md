## Context

The previous renderer (`render-text-basic`) used FreeType to rasterise a TTF font into a float RGBA buffer. This change replaces it entirely with a stroke-based renderer driven by a JSON handwriting capture set. The capture format normalises all spatial coordinates to cap-height units and records each stroke as a time-ordered sequence of `(x, y, t, p)` points, where `t` is elapsed milliseconds and `p` is pen pressure.

The bundled dataset (`tim-hand.json`) covers 83 glyphs (a–z, A–Z, 0–9, punctuation) and 9 common ligatures (`th`, `sh`, `ch`, `tion`, `er`, `re`, `ff`, `ck`, `nd`). Almost every glyph has 3 captures, enabling organic variety across repeated characters.

## Goals / Non-Goals

**Goals:**
- Animate text stroke-by-stroke at the real captured writing speed (modulated by `speed`)
- Render strokes as Gaussian-splat discs with pressure-modulated width
- Two-pass rendering: outline halo first, fill on top
- Randomise capture selection per character on each text edit; persist selection in a hidden parameter
- Support an optional user-supplied JSON override via a file-path parameter
- Ligature substitution: replace multi-character sequences with single-gesture captures where available

**Non-Goals:**
- Multi-line text layout or word wrap
- Kerning or per-pair advance adjustment
- Hard/crisp outlined strokes (SDF approach) — soft Gaussian halo is sufficient
- Right-to-left or complex-script shaping
- Audio synchronisation (the `audio` parameter remains declared but unread)

## Decisions

### Gaussian splat rendering

**Decision:** Each stroke point is rendered as a Gaussian disc:
```
sigma = (strokeThickness_capUnits + outline_extra) * capHeightPx * p_normalised
alpha_contribution = exp(-d² / (2σ²))   for each pixel within ~3σ
```
Pressure `p` is normalised to [0, 1] at load time using the dataset's observed maximum (`p_max`). Two passes are made per render: outline (larger σ, `outlineColor`) then fill (base σ, `fillColor`). Because fill is composited on top, only the outline halo beyond the fill edge remains visible.

**Alternative considered:** Thick-line segment rasteriser. More correct for sparse strokes but significantly more complex to implement with correct join geometry. Gaussian splatting works well for the point densities in this dataset (~4–5ms between points) and is simpler to get right in a first pass.

### JSON loading and caching in instance data

**Decision:** Parse the JSON once during `kOfxActionCreateInstance` (or lazily on first render if the glyphSet path parameter hasn't been set yet) and cache the parsed glyph table in a C++ struct stored as instance data via `kOfxPropInstanceData`. The cache is invalidated and reloaded when `kOfxActionInstanceChanged` fires for the `glyphSet` parameter.

**Rationale:** `tim-hand.json` is 1.1 MB. Parsing it on every render frame would add ~5–10ms of overhead and is unnecessary.

**Instance data struct:**
```cpp
struct InstanceData {
    GlyphSet          glyphSet;       // parsed capture data
    std::string       loadedPath;     // "" = bundled default
    std::vector<int>  captureIndices; // per-char capture selection
    std::string       lastText;       // text at time of last randomisation
};
```

### Capture selection persistence via hidden parameter

**Decision:** A `captureSelections` string parameter (marked `kOfxParamPropSecret = 1`) stores a comma-separated list of capture indices, one per character of the current `text` value. When `kOfxActionInstanceChanged` fires with reason `kOfxChangeUserEdited` for the `text` parameter, new random indices are generated and written to `captureSelections`. The render reads `captureSelections` to select the capture for each character.

**Rationale:** Parameters are the only per-instance state that Resolve serialises with a project. Storing selections in instance data (heap) would cause them to re-randomise on every Resolve restart. The hidden parameter survives project save/load.

**Edge cases:** If `captureSelections` is empty or its length doesn't match the current text, generate fresh random indices before rendering. This handles new instances and the state before any user edit.

### Animation timing

**Decision:**
```
fps           = kOfxImageEffectPropFrameRate from effect property set
draw_time_ms  = (renderTime / fps) * 1000.0 * speed

per character i:
  seq_start_i = sum of max(t) across captures 0..i-1
  seq_end_i   = seq_start_i + max(t) of capture i

At draw_time_ms:
  char fully drawn  if seq_end_i   <= draw_time_ms
  char being drawn  if seq_start_i <= draw_time_ms < seq_end_i
  char not started  if seq_start_i  > draw_time_ms
```

For the "being drawn" character, iterate stroke points and draw each whose `t <= (draw_time_ms - seq_start_i)`. Interpolate the final in-progress segment linearly between the last drawn point and the next point, so the pen tip moves smoothly rather than jumping.

### Ligature substitution

**Decision:** Before processing each character in the text, scan for the longest matching ligature key in the glyph set starting at the current position. Substitute if found. Scan order: longest key first.

**Implementation:** At load time, collect all multi-character glyph keys sorted descending by length. At render time, for each text position, try each ligature key in order; if the substring matches and the glyph exists, consume those characters as one glyph.

### Space and unknown character handling

**Decision:** 
- Space: advance pen by `0.35 * capHeightPx`, no strokes drawn, no animation time consumed.
- Unknown character: skip silently (advance 0, no strokes).

**Rationale:** The capture set has no space glyph. A fixed-width space of 0.35 cap-height units is slightly narrower than the average glyph width (0.43) and visually appropriate.

### nlohmann/json via FetchContent

**Decision:** Add `nlohmann/json` as a single-header FetchContent dependency pinned to v3.11.3.

**Rationale:** No JSON parser in the C++ stdlib. nlohmann/json is header-only, MIT-licensed, trivial to integrate, and its DOM API maps cleanly to the GlyphSet structure.

### FreeType removal

**Decision:** Remove the FreeType FetchContent block, its cache policy workaround, and all `FT_*` code. Remove `assets/OpenSans-Regular.ttf` from the bundle step.

**Rationale:** FreeType is no longer used. Removing it reduces build time significantly and simplifies `CMakeLists.txt`.

### kOfxParamStringIsFilePath for glyphSet

**Decision:** Define the `glyphSet` parameter with `kOfxParamPropStringMode = kOfxParamStringIsFilePath`. This may trigger a file browser in Resolve's Inspector. If Resolve ignores the mode and shows a plain text field, the parameter still works — the user can type a path directly. Fall back to the bundled JSON on any load error.

## Risks / Trade-offs

- **Gaussian halo outline vs. hard outline**: The outline looks like a soft glow, not a ruled edge. For most handwriting contexts this reads as "ink bleed" and is visually natural. If a hard outline is later required, an SDF-based second pass would be needed.
- **Pressure normalisation**: `p` peaks at ~0.52 in the bundled dataset. Normalising to the dataset maximum makes full use of the [0,1] range but means a different dataset with higher raw pressure values would render with different apparent stroke weights at the same `strokeThickness` setting.
- **kOfxParamStringIsFilePath support**: Resolve may not honour this parameter mode, leaving the user with a text-entry field. Acceptable UX degradation; document in the Inspector label text.
- **Instance data invalidation**: If `glyphSet` path changes between renders without `kOfxActionInstanceChanged` firing (e.g., programmatic host edits), the cache may be stale. Mitigated by also checking in the render handler if `loadedPath` matches the current param value.
