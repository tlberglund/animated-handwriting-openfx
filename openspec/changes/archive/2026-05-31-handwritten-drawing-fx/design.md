## Context

The current codebase has a clean three-layer structure:

```
glyph_model.h          ← point/stroke/capture data structs (no deps)
glyph_loader.cpp       ← JSON → GlyphSet (no OFX deps)
renderer.cpp           ← splatSegment / splatStrokes (static, generic)
                          renderHandwriting() (public, text-specific)
plugin.cpp             ← all OFX API scaffolding
```

`splatSegment` and `splatStrokes` are already generic — they paint strokes into a pixel buffer with no knowledge of glyphs or text. `renderHandwriting` is the only text-specific public function. The diagram JSON format (`{name, aspectRatio, strokes[]}`) uses identical `{x,y,t,p}` point data and per-stroke-local timing, so the pixel-painting layer is directly reusable.

## Goals / Non-Goals

**Goals:**
- Ship `HandwrittenDrawingFX` as a second plugin in the same `.ofx.bundle`
- Reuse `splatSegment`/`splatStrokes` for diagram rendering without copying them
- Generalize `splatSegment` to handle non-square coordinate normalization (diagram x is scaled by `aspectRatio`)
- Keep `HandwritingFX` behavior bit-for-bit identical after the refactor

**Non-Goals:**
- Multiple drawings in one instance
- Named objects or layers within a diagram file
- Diagram editing or path manipulation
- Any changes to the `HandwritingFX` parameter set or rendering behavior

## Decisions

### splatSegment generalization: separate scaleX / scaleY

**Decision:** Replace the single `ctx.capHeightPx` scale factor in `splatSegment` with two explicit `float scaleX, float scaleY` parameters passed at the call site.

For `renderHandwriting`: `scaleX = scaleY = capHeightPx` (behavior unchanged).
For `renderDiagram`: `scaleX = diagramHeightPx * aspectRatio`, `scaleY = diagramHeightPx`.

**Why not a separate splatSegmentDiagram:** Code duplication. The only difference is the coordinate transform on the first two lines.

**Why not put scaleX/scaleY in RenderContext:** RenderContext is the per-render environment (buffer pointer, bounds, dimensions). Scale factors are per-call, not per-render-environment — and for handwriting they vary per-glyph anyway (capHeightPx is already in ctx, but we'd be adding diagram-only fields).

**Alternative considered:** Pre-scale diagram coordinates to cap-height units before calling into splatStrokes. Rejected — it requires allocating a transformed copy of each stroke and obscures the coordinate model.

### Diagram data model: new structs, not reusing GlyphSet

**Decision:** Define a new `DiagramData` struct:
```cpp
struct DiagramData {
    std::string name;
    float aspectRatio;
    std::vector<Stroke> strokes;  // Stroke = std::vector<GlyphPoint> — reused
    float totalDuration;          // sum of per-stroke durations
};
```

**Why not reuse GlyphSet:** GlyphSet carries a glyph map, ligature keys, captures-per-character, and pMax — none of which apply to diagrams. Forcing diagrams into that shape would require dummy values throughout.

**Why reuse Stroke and GlyphPoint:** They're the right types. The data format is identical.

### Two-plugin bundle: shared entry point file

**Decision:** Rename `plugin.cpp` to `handwriting_plugin.cpp`. Add `diagram_plugin.cpp` for the new plugin. Add a thin `bundle.cpp` that implements `OfxGetNumberOfPlugins()` (returns 2) and `OfxGetPlugin(n)` (dispatches to each plugin's static `getPlugin()` accessor).

**Why a shared bundle.cpp:** The OFX entry points must be in a single translation unit that sees both plugins. A thin dispatcher avoids either plugin needing to `#include` the other.

### Diagram sizing: pixel height, aspect-ratio-derived width

**Decision:** User specifies `diagramHeight` in pixels (integer, default = frame height × 0.3, initialized in `kOfxActionCreateInstance` like posX/posY). Width is computed as `diagramHeight × aspectRatio` at render time. No separate width parameter.

**Why pixels not fraction:** User preference established in exploration — pixel values are more natural in a video editor context.

### Position and anchor for diagrams

**Decision:** Reuse the same `posX`, `posY`, `hAnchor`, `vAnchor` model as `HandwritingFX`, but anchor operates on the full diagram bounding box:
- `hAnchor`: Left → `originX = posX_px`; Center → `posX_px − diagramWidth/2`; Right → `posX_px − diagramWidth`
- `vAnchor`: Top → `originY = posY_px − diagramHeightPx`; Middle → `posY_px − diagramHeightPx/2`; Bottom → `posY_px`

The y-pixel transform inside `splatSegment` is `originY + (1 − point.y) × scaleY`, so `originY` is the bottom of the diagram's bounding box (same convention as `baselineY` in handwriting).

## Risks / Trade-offs

**splatSegment signature change touches all call sites** → All existing callers in `renderHandwriting` pass equal scaleX/scaleY. This is a mechanical change with no behavior difference; confirmed by build.

**diagramHeight default requires project size at CreateInstance** → Already done for posX/posY; same pattern applies.

**Diagram t-values are per-stroke-local (all start at 0)** → The sequential animation cursor (`seqCursor`) accumulates the duration of each completed stroke before starting the next — same pattern as glyph sequencing. Each stroke's duration is `stroke.back().t`.

## Open Questions

None — implementation can proceed.
