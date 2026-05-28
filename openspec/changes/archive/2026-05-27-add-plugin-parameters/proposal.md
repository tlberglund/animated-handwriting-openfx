## Why

The plugin currently exposes no user-controllable parameters — it is a blank generator with no inputs. Adding the core parameter set (text content, position, size, and stroke thickness) is the prerequisite for all future rendering work; every subsequent change will depend on these values being available in the plugin instance.

## What Changes

- Add a multiline string parameter for the text to be rendered (`text`)
- Add a double parameter for text height as a fraction of the output frame height (`textHeight`)
- Add a double parameter for stroke thickness in pixels (`strokeThickness`)
- Add a double parameter for animation speed as a multiplier (`speed`)
- Add a boolean parameter (checkbox) for whether audio should play (`audio`)
- Parameters are registered in `kOfxImageEffectActionDescribeInContext` and are visible in the Resolve UI — no rendering logic is wired up yet

## Capabilities

### New Capabilities

- `plugin-parameters`: OFX parameter definitions for text, height, stroke, speed, and audio — the authoring surface that all rendering code will read from

### Modified Capabilities

## Impact

- `src/plugin.cpp` — add parameter suite usage in `OfxSetHost` and parameter definitions in `describeInContext`
- No change to `CMakeLists.txt` or the bundle structure
- No behaviour change for existing renders (parameters are declared but not yet read during render)
