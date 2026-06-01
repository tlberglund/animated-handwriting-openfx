## MODIFIED Requirements

### Requirement: Bundle exports two OFX plugins
The `.ofx.bundle` SHALL export two plugins: `HandwritingFX` (the existing text generator) and `HandwrittenDrawingFX` (the new diagram generator). `OfxGetNumberOfPlugins()` SHALL return 2. `OfxGetPlugin(0)` SHALL return the `HandwritingFX` plugin descriptor and `OfxGetPlugin(1)` SHALL return the `HandwrittenDrawingFX` plugin descriptor. Both plugins SHALL appear as separate Generator entries in the host's Effects library.

#### Scenario: Both plugins appear in Resolve's Effects library
- **WHEN** the bundle is installed and Resolve is launched
- **THEN** both `HandwritingFX` and `HandwrittenDrawingFX` appear as distinct generator entries

#### Scenario: HandwritingFX behavior is unchanged
- **WHEN** `HandwritingFX` is used after the multi-plugin refactor
- **THEN** its rendering output and parameter set are bit-for-bit identical to the single-plugin build
