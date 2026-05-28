## 1. Add FreeType Dependency

- [x] 1.1 Add FreeType FetchContent block to `CMakeLists.txt` (pin a stable release tag, e.g. `VER-2-13-2`)
- [x] 1.2 Link FreeType static library into the `HandwritingFX` target

## 2. Bundle the Open Sans Font

- [x] 2.1 Download `OpenSans-Regular.ttf` (Apache 2.0) and place it at `assets/OpenSans-Regular.ttf`
- [x] 2.2 In the `bundle` custom target, copy `assets/OpenSans-Regular.ttf` into `HandwritingFX.ofx.bundle/Contents/Resources/`

## 3. Font Path Resolution

- [x] 3.1 In `plugin.cpp`, add a helper `getFontPath()` that uses `dladdr` on a known local symbol to obtain the plugin dylib path, then constructs the `../Resources/OpenSans-Regular.ttf` path relative to it

## 4. FreeType Initialisation

- [x] 4.1 Add `FT_Library gFTLibrary = nullptr` global and initialise it in the `kOfxActionLoad` handler; destroy it in `kOfxActionUnload`
- [x] 4.2 Add `FT_Face gFace = nullptr` global; load the font face from `getFontPath()` in `kOfxActionLoad`; done in `kOfxActionUnload`

## 5. Implement GetRegionOfDefinition

- [x] 5.1 Add a handler for `kOfxImageEffectActionGetRegionOfDefinition` that returns `kOfxStatReplyDefault` (delegates to host default bounds)

## 6. Implement Render

- [x] 6.1 Add a handler for `kOfxImageEffectActionRender` in `pluginMain`
- [x] 6.2 Read the render-time value of the `text` parameter using `gParamSuite->paramGetValue`
- [x] 6.3 Read the render-time value of the `textHeight` parameter using `gParamSuite->paramGetValue`
- [x] 6.4 Fetch the output image from the output clip using `gEffectSuite->clipGetImage`; get its bounds, row bytes, and pixel data pointer via property suite
- [x] 6.5 Read `kOfxImageEffectPropRenderScale` from render `inArgs` to get `(scaleX, scaleY)`
- [x] 6.6 Compute pixel size: `pixelSize = round(textHeight × (bounds.y2 - bounds.y1) × scaleY)`; call `FT_Set_Pixel_Sizes(gFace, 0, pixelSize)`
- [x] 6.7 Clear the output buffer to `(0,0,0,0)`
- [x] 6.8 Loop over each character in the text string; load and render each glyph with `FT_Load_Char` + `FT_LOAD_RENDER`; composite the glyph bitmap into the output float buffer (white RGBA, alpha = coverage/255.0f) at the current pen position; advance pen by `slot->advance.x >> 6`
- [x] 6.9 Release the output image with `gEffectSuite->clipReleaseImage`

## 7. Build and Verify

- [x] 7.1 Run `cmake --build build --target bundle` and confirm it compiles without errors
- [x] 7.2 Install to `/Library/OFX/Plugins/`, restart Resolve, place the generator on a timeline, enter text in the Inspector, and confirm white text appears in the viewer
