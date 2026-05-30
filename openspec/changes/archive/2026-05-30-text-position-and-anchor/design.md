## Context

Text currently renders with its baseline fixed at y=0 and pen-x at x=0 (bottom-left of the output frame). There is no way to reposition it. This blocks common compositional use cases — a title centered on screen, a caption pinned to the lower-right, etc.

OFX parameter defaults are fixed at definition time, so the initial center-of-frame value must be set procedurally when the instance is first created.

## Goals / Non-Goals

**Goals:**
- Add an X/Y position parameter that places the text at any point in the frame
- Add horizontal and vertical anchor choices that control which point on the text block aligns to the position coordinate
- Default behavior: text is centered on the frame (center/middle anchor, position initialized to project center in pixels)

**Non-Goals:**
- Text rotation or path-following
- Per-line or per-word alignment
- Sub-pixel rendering or position anti-aliasing

## Decisions

### Absolute pixel coordinates with dynamic initialization

**Decision:** Store `posX` and `posY` as absolute pixel values (full-resolution). In `kOfxActionCreateInstance`, read `kOfxImageEffectPropProjectSize` from the effect's property set and call `paramSetValue` to initialize both params to `projectWidth / 2` and `projectHeight / 2`.

**Why not normalized [0, 1]:** Pixel values are what operators expect (the Inspector reads "1920  1080", not "0.5  0.5"). Absolute coordinates also compose directly with other pixel-based tools in Resolve.

**Why not a sentinel value:** Treating (0, 0) as "use center" is ambiguous — the user may legitimately want bottom-left. Dynamic initialization in `createInstance` gives a real center value with no special cases.

**Render-time scaling:** At render time, scale the stored pixel position by `renderScale` before use: `posX_px = posX * renderScale[0]`, `posY_px = posY * renderScale[1]`. This keeps the position correct at proxy resolutions.

### Two choice params vs. a 3×3 combo

**Decision:** Two separate `kOfxParamTypeChoice` parameters — `hAnchor` (Left / Center / Right, default Center) and `vAnchor` (Top / Middle / Bottom, default Middle).

**Why not a single nine-option choice:** A flat 9-option list ("top-left", "top-center", …) clutters the Inspector. Two short dropdowns are easier to scan and edit independently.

**Why not icon buttons:** OFX has no icon-selector parameter type. Choice params are the native equivalent.

### Where anchor offsets are computed

**Decision:** Compute `originX` and `baselineY` inside `renderHandwriting`, after glyph resolution and total-width accumulation. Pass them as plain floats down to `splatStrokes` and `splatSegment`.

**Why not in plugin.cpp:** Total text width (needed for hAnchor Center / Right offsets) is only known after resolving ligatures — that logic lives in the renderer.

**Why not add fields to RenderContext:** `RenderContext` carries image-buffer state that is fixed before `renderHandwriting` is called; anchor offsets change per call. Passing them explicitly keeps the context struct stable.

### Coordinate mapping

Given `posX_px = posX × renderScale[0]` and `posY_px = posY × renderScale[1]` (pixel coordinates in the scaled render, y=0 at image bottom):

| vAnchor | baselineY |
|---------|-----------|
| Top     | `posY_px − capHeightPx` |
| Middle  | `posY_px − capHeightPx / 2` |
| Bottom  | `posY_px` |

| hAnchor | originX |
|---------|---------|
| Left    | `posX_px` |
| Center  | `posX_px − totalWidth × capHeightPx / 2` |
| Right   | `posX_px − totalWidth × capHeightPx` |

In `splatSegment`, the pixel coordinates become:

```
pax = originX + ax * capHeightPx          // ax already includes glyphPenX
pay = baselineY + (1.0f - ay) * capHeightPx
```

(Previously: `pax = ax * capHeightPx`, `pay = (1.0f - ay) * capHeightPx`.)

## Risks / Trade-offs

**Project format change leaves position stale** → If the operator changes the project from 1920×1080 to 3840×2160 after dropping the generator, the stored position (e.g., 960, 540) no longer corresponds to center. This matches how every other absolute-pixel tool in Resolve behaves; operators are expected to re-adjust after a format change.

**Text partially off-frame is silently clipped** → Already true for the current bottom-left rendering. The bounds check in `splatSegment` clamps to the image rect; no crash risk. [Risk] Intentional; out-of-bounds pixels are discarded.

## Open Questions

None — implementation can proceed.
