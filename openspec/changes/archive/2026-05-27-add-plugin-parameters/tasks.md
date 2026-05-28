## 1. Fetch Parameter Suite

- [x] 1.1 Add `OfxParameterSuiteV1* gParamSuite = nullptr` global to `plugin.cpp`
- [x] 1.2 In `setHost`, fetch `kOfxParameterSuite` version 1 and store in `gParamSuite` alongside the existing suite fetches

## 2. Wire Parameter Set Into describeInContext

- [x] 2.1 In `kOfxImageEffectActionDescribeInContext`, call `gEffectSuite->getParamSet(effect, &paramSet)` to get the `OfxParamSetHandle` — add a null-check guard on `gParamSuite` before proceeding

## 3. Define Text Parameter

- [x] 3.1 Call `gParamSuite->paramDefine(paramSet, kOfxParamTypeString, "text", &props)` to define the text parameter
- [x] 3.2 Set `kOfxPropLabel` to `"Text"` on the returned property set
- [x] 3.3 Set `kOfxParamPropStringMode` to `kOfxParamStringIsMultiLine`

## 4. Define Text Height Parameter

- [x] 4.1 Define `textHeight` as `kOfxParamTypeDouble`; set label `"Text Height"`, default `0.1`, min `0.0`, max `1.0`, display min `0.0`, display max `1.0`

## 5. Define Stroke Thickness Parameter

- [x] 5.1 Define `strokeThickness` as `kOfxParamTypeDouble`; set label `"Stroke Thickness"`, default `2.0`, min `0.0`, max `50.0`, display min `0.0`, display max `50.0`

## 6. Define Speed Parameter

- [x] 6.1 Define `speed` as `kOfxParamTypeDouble`; set label `"Speed"`, default `1.0`, min `0.0`, max `10.0`, display min `0.0`, display max `10.0`

## 7. Define Audio Parameter

- [x] 7.1 Define `audio` as `kOfxParamTypeBoolean`; set label `"Audio"`, default `0` (false)

## 8. Build and Verify

- [x] 8.1 Run `cmake --build build --target bundle` and confirm it compiles without errors
- [x] 8.2 Install to `/Library/OFX/Plugins/`, restart Resolve, and confirm all five parameters (`text`, `textHeight`, `strokeThickness`, `speed`, `audio`) appear in the Inspector when the HandwritingFX generator is on the timeline
