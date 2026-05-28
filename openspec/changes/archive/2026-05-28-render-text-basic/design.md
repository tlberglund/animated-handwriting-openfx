## Context

The plugin handles `kOfxActionCreateInstance` and `kOfxActionDestroyInstance` correctly and all five parameters are visible in the Resolve Inspector. The next step is producing actual pixel output. OFX generators must implement two additional actions: `kOfxImageEffectActionGetRegionOfDefinition` (tells the host the output bounds) and `kOfxImageEffectActionRender` (writes pixels into the output image buffer).

FreeType 2 is the standard open-source font rasteriser. It is widely available, has a stable C API, and produces correct anti-aliased glyph bitmaps that can be composited directly into an RGBA float buffer. Open Sans is a clean, readable sans-serif distributed under the Apache 2.0 licence.

## Goals / Non-Goals

**Goals:**
- Produce a visible RGBA frame when the plugin is on a Resolve timeline
- Render the `text` parameter value with Open Sans Regular
- Scale text so that cap-height ≈ `textHeight × frame height`
- Position the text origin at pixel (0, 0) — bottom-left of the frame (OFX coordinate space)
- Pass render-scale through to FreeType so multi-res renders stay sharp

**Non-Goals:**
- Multi-line text layout (newlines rendered as-is or ignored)
- Stroke/outline rendering (`strokeThickness` parameter ignored)
- Animation / timing (`speed` parameter ignored)
- Audio (`audio` parameter ignored)
- Text colour (white on transparent background is sufficient)
- Right-to-left or complex-script shaping

## Decisions

### FreeType via FetchContent

**Decision**: Fetch FreeType source at configure time via CMake FetchContent and build it as a static library linked into the plugin.

**Rationale**: No system FreeType installation can be assumed in a plugin context — the plugin must be self-contained. Static linking avoids dylib load-order issues. FetchContent pins the version and works without the user installing anything.

**Alternative considered**: `find_package(Freetype)` against the system — fragile across machines and build environments.

### Font bundled as a file inside the .ofx.bundle

**Decision**: Ship `OpenSans-Regular.ttf` under `HandwritingFX.ofx.bundle/Contents/Resources/` and load it from that path at render time using `FT_New_Face`.

**Rationale**: OFX bundles are directories; embedding a font file is idiomatic. The font path is derived from the plugin's own binary path using `_dyld_get_image_name` (macOS) so it works regardless of where the bundle is installed.

**Alternative considered**: Embedding the font as a C byte array — eliminates the file I/O but makes the font hard to replace and bloats the object file. Not worth it for a TTF of this size.

### Pixel format and compositing

**Decision**: The output image is `kOfxBitDepthFloat` / `kOfxImageComponentRGBA`. FreeType produces 8-bit grayscale bitmaps per glyph. Each glyph pixel is composited into the float buffer as `(R,G,B,A) = (1,1,1, coverage/255.0)` using over-compositing against the cleared (0,0,0,0) background.

**Rationale**: Float RGBA is the only pixel depth we declared in `kOfxActionDescribe`. White text on a transparent background composes correctly in Resolve.

### Text sizing

**Decision**: Set FreeType pixel size to `round(textHeight × RoD_height × render_scale_y)` where `RoD_height` is the frame height from `kOfxImageEffectPropRegionOfDefinition` and `render_scale_y` comes from `kOfxImageEffectPropRenderScale` in the render inArgs.

**Rationale**: Makes the text scale proportionally with the frame and handles proxy renders correctly. `textHeight = 0.1` on a 1080-line frame → 108px nominal, scaled down at lower render resolutions.

### Region of Definition

**Decision**: Return the project format (the output clip's `kOfxImageEffectPropRegionOfDefinition`) unchanged — i.e., return `kOfxStatReplyDefault` so the host uses its own default bounds.

**Rationale**: A generator that fills the whole frame should simply claim the default region. This avoids having to query the format manually and is correct for a full-frame generator.

## Risks / Trade-offs

- **FreeType build time**: Building FreeType from source adds ~10–20 seconds to a cold CMake configure. Acceptable for a dev build; mitigated by CMake's build cache on incremental builds.
- **Font path lookup**: `_dyld_get_image_name` is a private macOS API. It works reliably in practice but is not officially documented. Alternative: use `dladdr` on a known symbol to get the dylib path — same information, slightly more portable.
- **No text layout**: Rendering only at (0,0) means text can overflow the frame and multi-line text won't wrap. This is intentional scope for this change.
- **Glyph origin at (0,0)**: OFX Y-axis is bottom-up. Rendering at (0,0) places the text baseline at the bottom-left corner. The first line will be partially clipped by the descender. Acceptable for this change; a future change will add configurable positioning.
