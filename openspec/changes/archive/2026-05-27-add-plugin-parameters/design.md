## Context

The plugin skeleton compiles and loads in DaVinci Resolve as a Generator with no user-controllable inputs. To do any useful rendering the plugin needs a set of parameters that describe what to draw and where. This change adds the parameter definitions only — no rendering code reads them yet.

OFX parameters are defined during `kOfxImageEffectActionDescribeInContext` using `OfxParameterSuiteV1`. The suite is fetched from the host in `OfxSetHost` (alongside the existing property and image effect suites).

## Goals / Non-Goals

**Goals:**
- Register five parameters visible in the Resolve Inspector panel: `text`, `textHeight`, `strokeThickness`, `speed`, `audio`
- Each parameter has a sensible default, label, and numeric range where applicable
- Parameters persist across Resolve sessions (OFX handles serialization automatically)

**Non-Goals:**
- Reading parameter values during render (future change)
- Animation keyframes (OFX supports this automatically once parameters exist)
- Custom UI overlays or on-screen controls

## Decisions

### Text height as a frame-height fraction

**Decision**: `textHeight` ranges 0.0–1.0, default 0.1 (10% of frame height).

**Rationale**: Same resolution-independence argument as position. Rendering will convert to pixels at draw time.

### Stroke thickness in pixels

**Decision**: `strokeThickness` is in pixels (0.0–50.0, default 2.0) rather than normalised.

**Rationale**: Stroke weight is a perceived-size property, not a layout property. Users expect to type "2 pixels" not "0.003 of frame height". Rendering code will apply render-scale to handle multi-res rendering.

### Speed as a multiplier (0.0–10.0, default 1.0)

**Decision**: `speed` is a normalised multiplier where 1.0 means "normal speed", not an absolute time value or frame count.

**Rationale**: A multiplier is unit-free and composable — rendering code multiplies its internal time cursor by this value regardless of frame rate or clip length.

**Alternative considered**: BPM or frames-per-character — too tied to specific rendering strategies not yet designed.

### Audio as a plain boolean

**Decision**: `audio` is `kOfxParamTypeBoolean`, defaulting to `false`.

**Rationale**: The simplest representation of an on/off toggle. If audio source selection is needed later, a separate string/choice parameter can be added without breaking this one.

## Risks / Trade-offs

- **Parameter name permanence** → OFX parameter identifiers are baked into saved Resolve projects. Any rename after projects are saved is a breaking change. Mitigation: choose stable, descriptive names now (`text`, `textHeight`, `strokeThickness`, `speed`, `audio`).
- **Suite availability** → If `kOfxParameterSuite` is unavailable (e.g., minimal host), `gParamSuite` will be null and the describe will crash. Mitigation: null-check in describeInContext before calling paramDefine.
