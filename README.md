# HandwritingFX

An OpenFX plugin bundle for DaVinci Resolve that renders the animated handwriting and hand-drawn diagrams captured by the application over in [this repo](https://github.com/tlberglund/animated-handwriting). The bundle contains two generator plugins:

- **HandwritingFX** — Renders text using a captured handwriting glyph set, animating each character stroke-by-stroke in real time.
- **HandwrittenDrawingFX** — Renders a diagram from a captured diagram file, animating each stroke sequentially.

Both plugins paint directly onto a transparent generator layer using a soft Gaussian brush that reproduces the pressure and timing of the original pen strokes. Cool, huh?

## Prerequisites

- macOS (arm64 or x86_64)
- Xcode Command Line Tools (provides `clang++`)
- CMake 3.21 or later
- Internet access during the first configure (CMake fetches the OFX SDK headers and nlohmann/json automatically)

## Building

```bash
cmake -B build
cmake --build build --target bundle
```

The `bundle` target assembles `build/HandwritingFX.ofx.bundle` with the correct macOS OFX layout, including the bundled default glyph set at `Contents/Resources/tim-hand.json`.

## Installation

It's a pretty high-tech installation process. Please be careful to follow every step in this procedure:

```bash
sudo cp -R build/HandwritingFX.ofx.bundle /Library/OFX/Plugins/
```

Restart DaVinci Resolve. Both **HandwritingFX** and **HandwrittenDrawingFX** will appear as Generator entries in the Effects Library visible in the Edit tab.

## HandwritingFX Parameters

| Parameter | Type | Range | Default | Description |
|---|---|---|---|---|
| Text | String (multiline) | — | — | The text to render. Because DaVinci Resolve intercepts the Enter key in parameter fields, use the literal escape sequence `\n` to insert line breaks. |
| Glyph Set | File path | — | Bundled | Path to a glyph set JSON file. Defaults to the bundled `tim-hand.json` when left empty. See [JSON Formats](#json-formats). |
| Animate | Boolean | — | On | When on, strokes draw on progressively over time. When off, the full text is rendered on every frame. |
| Speed | Double | 0 – 10 | 1.0 | Multiplier on the draw speed. Higher values draw the text faster relative to the timeline. |
| Text Height | Double | 0 – 1 | 0.1 | Cap height of the text as a fraction of the frame height. |
| Stroke Thickness | Double | 0 – 0.2 | 0.02 | Radius of the painted brush as a fraction of the cap height. |
| Fill Color | RGBA | — | White | Color of the stroke fill. |
| Enable Outline | Boolean | — | On | Enables the outline pass. |
| Outline Color | RGBA | — | Black | Color of the outline halo. |
| Outline Thickness | Double | 0 – 0.1 | 0.01 | Sigma of the outline pass relative to cap height. |
| Position X | Double | — | Project center | Horizontal position of the text anchor in pixels. |
| Position Y | Double | — | Project center | Vertical position of the text anchor in pixels. |
| H Anchor | Choice | Left / Center / Right | Center | Which horizontal edge of the text block aligns to Position X. |
| V Anchor | Choice | Top / Middle / Bottom | Middle | Which vertical edge of the text block aligns to Position Y. |
| Line Spacing | Double | -1 – 5 | 1.0 | Multiplier on the default line height (one cap height). |
| Text Alignment | Choice | Left / Center / Right | Left | Alignment of lines within a multi-line block. |
| Tracking | Double | -0.5 – 2.0 | 0.0 | Extra spacing added between characters, in units of cap height. |
| Rotation | Double | -360 – 360 | 0.0 | Rotation in degrees, applied around the position anchor. |

---

## HandwrittenDrawingFX Parameters

### Content

| Parameter | Type | Range | Default | Description |
|---|---|---|---|---|
| Diagram File | File path | — | — | Path to a diagram stroke JSON file. See [JSON Formats](#json-formats). |
| Animate | Boolean | — | On | When on, strokes draw on sequentially over time. When off, the full diagram is rendered on every frame. |
| Speed | Double | 0 – 10 | 1.0 | Multiplier on the draw speed. |
| Diagram Height | Double | 1 – 8192 | 30% of frame height | Height of the diagram in pixels at full render scale. Width is derived automatically from the `aspectRatio` field in the diagram file. |
| Stroke Thickness | Double | 0 – 0.2 | 0.02 | Radius of the painted brush as a fraction of diagram height. |
| Fill Color | RGBA | — | White | Color of the stroke fill. |
| Enable Outline | Boolean | — | On | Enables the outline pass. |
| Outline Color | RGBA | — | Black | Color of the outline halo. |
| Outline Thickness | Double | 0 – 0.1 | 0.01 | Sigma of the outline pass relative to diagram height. |
| Position X | Double | — | Project center | Horizontal position of the diagram anchor in pixels. |
| Position Y | Double | — | Project center | Vertical position of the diagram anchor in pixels. |
| H Anchor | Choice | Left / Center / Right | Center | Which horizontal edge of the diagram bounding box aligns to Position X. |
| V Anchor | Choice | Top / Middle / Bottom | Middle | Which vertical edge of the diagram bounding box aligns to Position Y. |
| Rotation | Double | -360 – 360 | 0.0 | Rotation in degrees, applied around the position anchor. |

---
## JSON Formats

Both the glyph set format (used by HandwritingFX) and the diagram stroke format (used by HandwrittenDrawingFX) are documented in the [capture tool repository](https://github.com/tlberglund/animated-handwriting).


## Notes

### DaVinci Resolve limitations

- **File path fields** — Resolve does not open a file picker for OFX string parameters, because the OpenFX API hates usability. Type or paste the full absolute path directly into the field. Believe me that I am very sorry for this, but using the DaVinci Resolve API directly consigns us to the Fusion tab, for which I would arguably be even more sorry.
- **Multiline text** — Resolve intercepts the Enter key in parameter fields. Use `\n` in the Text field to insert line breaks (e.g., `Hello\nWorld`), or write the text elsewhere with newlines in it and paste it into the text field. Yes, really. Once again, I would fix this if I could.
