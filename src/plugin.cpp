#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"

#include "glyph_loader.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string expandEscapes(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for(size_t i = 0; i < s.size(); ++i) {
        if(s[i] == '\\' && i + 1 < s.size() && s[i+1] == 'n') {
            out += '\n'; ++i;
        } else {
            out += s[i];
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Global suite pointers
// ---------------------------------------------------------------------------

static OfxHost*               gHost        = nullptr;
static OfxPropertySuiteV1*    gPropSuite   = nullptr;
static OfxImageEffectSuiteV1* gEffectSuite = nullptr;
static OfxParameterSuiteV1*   gParamSuite  = nullptr;

// ---------------------------------------------------------------------------
// setHost
// ---------------------------------------------------------------------------

static void setHost(OfxHost* host)
{
    gHost = host;
    if(!host) return;
    gPropSuite   = (OfxPropertySuiteV1*)  host->fetchSuite(host->host, kOfxPropertySuite,    1);
    gEffectSuite = (OfxImageEffectSuiteV1*)host->fetchSuite(host->host, kOfxImageEffectSuite, 1);
    gParamSuite  = (OfxParameterSuiteV1*) host->fetchSuite(host->host, kOfxParameterSuite,   1);
}

// ---------------------------------------------------------------------------
// pluginMain
// ---------------------------------------------------------------------------

static OfxStatus pluginMain(const char*          action,
                             const void*          handle,
                             OfxPropertySetHandle inArgs,
                             OfxPropertySetHandle /*outArgs*/)
{
    if(strcmp(action, kOfxActionLoad)   == 0 ||
       strcmp(action, kOfxActionUnload) == 0)
        return kOfxStatOK;

    if(strcmp(action, kOfxActionDescribe) == 0) {
        auto effect = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle props = nullptr;
        gEffectSuite->getPropertySet(effect, &props);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts,    0, kOfxImageEffectContextGenerator);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxPropLabel,                            0, "HandwritingFX");
        return kOfxStatOK;
    }

    if(strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        auto effect = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));

        OfxPropertySetHandle outProps = nullptr;
        gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &outProps);
        gPropSuite->propSetString(outProps, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

        if(!gParamSuite) return kOfxStatOK;
        OfxParamSetHandle paramSet = nullptr;
        if(gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK || !paramSet)
            return kOfxStatOK;

        OfxPropertySetHandle props = nullptr;

        gParamSuite->paramDefine(paramSet, kOfxParamTypeString, "text", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Text");
        gPropSuite->propSetString(props, kOfxParamPropStringMode, 0, kOfxParamStringIsMultiLine);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "animate", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,        0, "Animate");
        gPropSuite->propSetInt(props,    kOfxParamPropDefault, 0, 1);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "textHeight", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Text Height");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 0.1);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,        0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0, 1.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "strokeThickness", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Stroke Thickness");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 0.02);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,        0, 0.2);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0, 0.2);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "speed", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Speed");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,        0, 10.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0, 10.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeString, "glyphSet", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Glyph Set");
        gPropSuite->propSetString(props, kOfxParamPropStringMode, 0, kOfxParamStringIsFilePath);
        gPropSuite->propSetString(props, kOfxParamPropDefault,    0, "");

        gParamSuite->paramDefine(paramSet, kOfxParamTypeString, "captureSelections", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,        0, "Capture Selections");
        gPropSuite->propSetInt(props,    kOfxParamPropSecret,  0, 1);
        gPropSuite->propSetString(props, kOfxParamPropDefault, 0, "");

        gParamSuite->paramDefine(paramSet, kOfxParamTypeRGBA, "fillColor", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,        0, "Fill Color");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 1, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 2, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 3, 1.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeRGBA, "outlineColor", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,        0, "Outline Color");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 1, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 2, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 3, 1.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "outlineThickness", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Outline Thickness");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 0.01);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,        0, 0.1);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0, 0.1);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "outlineEnabled", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,        0, "Enable Outline");
        gPropSuite->propSetInt(props,    kOfxParamPropDefault, 0, 1);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "posX", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Position X");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, -8192.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0,  8192.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "posY", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Position Y");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, -8192.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0,  8192.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeChoice, "hAnchor", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,              0, "H Anchor");
        gPropSuite->propSetInt(props,    kOfxParamPropDefault,       0, 1);
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  0, "Left");
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  1, "Center");
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  2, "Right");

        gParamSuite->paramDefine(paramSet, kOfxParamTypeChoice, "vAnchor", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,              0, "V Anchor");
        gPropSuite->propSetInt(props,    kOfxParamPropDefault,       0, 1);
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  0, "Top");
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  1, "Middle");
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  2, "Bottom");

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "lineSpacing", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Line Spacing");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, -1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,        0,  5.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, -1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0,  5.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeChoice, "textAlignment", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,              0, "Text Alignment");
        gPropSuite->propSetInt(props,    kOfxParamPropDefault,       0, 0);
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  0, "Left");
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  1, "Center");
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption,  2, "Right");

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "tracking", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Tracking");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, -0.5);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,        0,  2.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, -0.5);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0,  2.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "rotation", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Rotation");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0,    0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, -360.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,        0,  360.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, -360.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0,  360.0);

        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionCreateInstance) == 0) {
        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle effectProps = nullptr;
        gEffectSuite->getPropertySet(instance, &effectProps);

        InstanceData* data = new InstanceData();
        data->glyphSet = loadGlyphSet(getBundledGlyphSetPath());
        gPropSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, (void*)data);

        double projectSize[2] = {1920.0, 1080.0};
        gPropSuite->propGetDoubleN(effectProps, kOfxImageEffectPropProjectSize, 2, projectSize);
        OfxParamSetHandle paramSet = nullptr;
        gEffectSuite->getParamSet(instance, &paramSet);
        OfxParamHandle posXHandle = nullptr, posYHandle = nullptr;
        gParamSuite->paramGetHandle(paramSet, "posX", &posXHandle, nullptr);
        gParamSuite->paramGetHandle(paramSet, "posY", &posYHandle, nullptr);
        if(posXHandle) gParamSuite->paramSetValue(posXHandle, projectSize[0] / 2.0);
        if(posYHandle) gParamSuite->paramSetValue(posYHandle, projectSize[1] / 2.0);

        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionDestroyInstance) == 0) {
        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle effectProps = nullptr;
        gEffectSuite->getPropertySet(instance, &effectProps);
        void* ptr = nullptr;
        gPropSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        delete static_cast<InstanceData*>(ptr);
        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionInstanceChanged) == 0) {
        char* reason = nullptr;
        gPropSuite->propGetString(inArgs, kOfxPropChangeReason, 0, &reason);
        if(!reason || strcmp(reason, kOfxChangeUserEdited) != 0)
            return kOfxStatOK;

        char* paramName = nullptr;
        gPropSuite->propGetString(inArgs, kOfxPropName, 0, &paramName);
        if(!paramName) return kOfxStatOK;

        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle effectProps = nullptr;
        gEffectSuite->getPropertySet(instance, &effectProps);
        void* ptr = nullptr;
        gPropSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        InstanceData* data = static_cast<InstanceData*>(ptr);
        if(!data) return kOfxStatOK;

        OfxParamSetHandle paramSet = nullptr;
        gEffectSuite->getParamSet(instance, &paramSet);

        if(strcmp(paramName, "text") == 0) {
            OfxParamHandle textParam = nullptr;
            gParamSuite->paramGetHandle(paramSet, "text", &textParam, nullptr);
            char* textValue = nullptr;
            gParamSuite->paramGetValue(textParam, &textValue);
            std::string text = expandEscapes(textValue ? textValue : "");

            data->captureIndices.clear();
            size_t pos = 0;
            while(pos < text.size()) {
                if(text[pos] == '\n') { ++pos; continue; }
                if(text[pos] == ' ') {
                    data->captureIndices.push_back(0);
                    ++pos; continue;
                }
                bool found = false;
                for(auto& key : data->glyphSet.ligatureKeys) {
                    if(pos + key.size() <= text.size() &&
                       text.substr(pos, key.size()) == key) {
                        auto it = data->glyphSet.glyphs.find(key);
                        int n = (it != data->glyphSet.glyphs.end())
                                    ? (int)it->second.captures.size() : 1;
                        data->captureIndices.push_back(n > 0 ? rand() % n : 0);
                        pos += key.size(); found = true; break;
                    }
                }
                if(!found) {
                    auto it = data->glyphSet.glyphs.find(text.substr(pos, 1));
                    int n = (it != data->glyphSet.glyphs.end())
                                ? (int)it->second.captures.size() : 1;
                    data->captureIndices.push_back(n > 0 ? rand() % n : 0);
                    ++pos;
                }
            }
            data->lastText = text;

            std::string sel;
            for(size_t i = 0; i < data->captureIndices.size(); ++i) {
                if(i > 0) sel += ",";
                sel += std::to_string(data->captureIndices[i]);
            }
            OfxParamHandle selParam = nullptr;
            gParamSuite->paramGetHandle(paramSet, "captureSelections", &selParam, nullptr);
            if(selParam) gParamSuite->paramSetValue(selParam, sel.c_str());
        }
        else if(strcmp(paramName, "glyphSet") == 0) {
            OfxParamHandle gsParam = nullptr;
            gParamSuite->paramGetHandle(paramSet, "glyphSet", &gsParam, nullptr);
            char* gsPath = nullptr;
            gParamSuite->paramGetValue(gsParam, &gsPath);
            std::string path = gsPath ? gsPath : "";

            std::string loadPath = path.empty() ? getBundledGlyphSetPath() : path;
            GlyphSet newSet = loadGlyphSet(loadPath);
            if(!newSet.glyphs.empty()) {
                data->glyphSet = std::move(newSet);
                data->loadedPath = path;
            } else if(!path.empty()) {
                data->glyphSet = loadGlyphSet(getBundledGlyphSetPath());
                data->loadedPath = "";
            }
        }

        return kOfxStatOK;
    }

    if(strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0)
        return kOfxStatReplyDefault;

    if(strcmp(action, kOfxImageEffectActionRender) == 0) {
        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));

        OfxPropertySetHandle effectProps = nullptr;
        gEffectSuite->getPropertySet(instance, &effectProps);
        void* iptr = nullptr;
        gPropSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &iptr);
        InstanceData* data = static_cast<InstanceData*>(iptr);

        double renderTime = 0.0;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &renderTime);
        double renderScale[2] = {1.0, 1.0};
        gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropRenderScale, 2, renderScale);

        double fps = 24.0;
        gPropSuite->propGetDouble(effectProps, kOfxImageEffectPropFrameRate, 0, &fps);
        if(fps <= 0.0) fps = 24.0;

        OfxParamSetHandle paramSet = nullptr;
        gEffectSuite->getParamSet(instance, &paramSet);

        auto getParam = [&](const char* name) -> OfxParamHandle {
            OfxParamHandle h = nullptr;
            gParamSuite->paramGetHandle(paramSet, name, &h, nullptr);
            return h;
        };

        char* textValue            = nullptr;
        char* captureSelectionsStr = nullptr;
        double textHeight = 0.1, speed = 1.0, strokeThickness = 0.02, outlineThickness = 0.01;
        double fillColor[4]    = {1.0, 1.0, 1.0, 1.0};
        double outlineColor[4] = {0.0, 0.0, 0.0, 1.0};
        int outlineEnabled = 1;
        double posX = 0.0, posY = 0.0;
        int hAnchor = 1, vAnchor = 1;
        double lineSpacing = 1.0;
        int textAlignment = 0;
        int animate = 1;
        double tracking = 0.0;
        double rotation = 0.0;

        gParamSuite->paramGetValueAtTime(getParam("text"),              renderTime, &textValue);
        gParamSuite->paramGetValueAtTime(getParam("textHeight"),        renderTime, &textHeight);
        gParamSuite->paramGetValueAtTime(getParam("speed"),             renderTime, &speed);
        gParamSuite->paramGetValueAtTime(getParam("strokeThickness"),   renderTime, &strokeThickness);
        gParamSuite->paramGetValueAtTime(getParam("fillColor"),         renderTime,
                                         &fillColor[0], &fillColor[1], &fillColor[2], &fillColor[3]);
        gParamSuite->paramGetValueAtTime(getParam("outlineColor"),      renderTime,
                                         &outlineColor[0], &outlineColor[1], &outlineColor[2], &outlineColor[3]);
        gParamSuite->paramGetValueAtTime(getParam("outlineThickness"),  renderTime, &outlineThickness);
        gParamSuite->paramGetValueAtTime(getParam("outlineEnabled"),    renderTime, &outlineEnabled);
        gParamSuite->paramGetValueAtTime(getParam("captureSelections"), renderTime, &captureSelectionsStr);
        gParamSuite->paramGetValueAtTime(getParam("posX"),              renderTime, &posX);
        gParamSuite->paramGetValueAtTime(getParam("posY"),              renderTime, &posY);
        gParamSuite->paramGetValueAtTime(getParam("hAnchor"),           renderTime, &hAnchor);
        gParamSuite->paramGetValueAtTime(getParam("vAnchor"),           renderTime, &vAnchor);
        gParamSuite->paramGetValueAtTime(getParam("lineSpacing"),       renderTime, &lineSpacing);
        gParamSuite->paramGetValueAtTime(getParam("textAlignment"),     renderTime, &textAlignment);
        gParamSuite->paramGetValueAtTime(getParam("animate"),           renderTime, &animate);
        gParamSuite->paramGetValueAtTime(getParam("tracking"),          renderTime, &tracking);
        gParamSuite->paramGetValueAtTime(getParam("rotation"),          renderTime, &rotation);

        OfxImageClipHandle outputClip = nullptr;
        gEffectSuite->clipGetHandle(instance, kOfxImageEffectOutputClipName, &outputClip, nullptr);
        OfxPropertySetHandle outputImage = nullptr;
        gEffectSuite->clipGetImage(outputClip, renderTime, nullptr, &outputImage);
        if(!outputImage) return kOfxStatFailed;

        int bounds[4] = {};
        gPropSuite->propGetInt(outputImage, kOfxImagePropBounds, 0, &bounds[0]);
        gPropSuite->propGetInt(outputImage, kOfxImagePropBounds, 1, &bounds[1]);
        gPropSuite->propGetInt(outputImage, kOfxImagePropBounds, 2, &bounds[2]);
        gPropSuite->propGetInt(outputImage, kOfxImagePropBounds, 3, &bounds[3]);
        int rowBytes = 0;
        gPropSuite->propGetInt(outputImage, kOfxImagePropRowBytes, 0, &rowBytes);
        void* pixelData = nullptr;
        gPropSuite->propGetPointer(outputImage, kOfxImagePropData, 0, &pixelData);

        if(!pixelData) {
            gEffectSuite->clipReleaseImage(outputImage);
            return kOfxStatFailed;
        }

        int width  = bounds[2] - bounds[0];
        int height = bounds[3] - bounds[1];

        for(int row = 0; row < height; ++row)
            memset((char*)pixelData + row * rowBytes, 0, (size_t)width * 4 * sizeof(float));

        if(!textValue || textValue[0] == '\0' || !data || data->glyphSet.glyphs.empty()) {
            gEffectSuite->clipReleaseImage(outputImage);
            return kOfxStatOK;
        }

        float capHeightPx = (float)round(textHeight * height * renderScale[1]);
        if(capHeightPx < 1.0f) capHeightPx = 1.0f;
        double draw_time_ms = (renderTime / fps) * 1000.0 * speed;

        std::vector<int> captureIdxVec;
        if(captureSelectionsStr && captureSelectionsStr[0] != '\0') {
            std::string s = captureSelectionsStr;
            size_t p = 0;
            while(p <= s.size()) {
                size_t comma = s.find(',', p);
                if(comma == std::string::npos) comma = s.size();
                if(comma > p) {
                    try { captureIdxVec.push_back(std::stoi(s.substr(p, comma - p))); }
                    catch(...) { captureIdxVec.push_back(0); }
                }
                if(comma == s.size()) break;
                p = comma + 1;
            }
        }

        RenderContext ctx;
        ctx.pixelData = pixelData;
        ctx.bounds[0] = bounds[0]; ctx.bounds[1] = bounds[1];
        ctx.bounds[2] = bounds[2]; ctx.bounds[3] = bounds[3];
        ctx.rowBytes = rowBytes;
        ctx.width = width;
        ctx.height = height;
        ctx.capHeightPx = capHeightPx;
        ctx.pMax = data->glyphSet.pMax;
        ctx.strokeThickness = strokeThickness;

        float posX_px = (float)(posX * renderScale[0]);
        float posY_px = (float)(posY * renderScale[1]);

        if(!animate) draw_time_ms = 1e12;

        renderHandwriting(ctx, data->glyphSet, expandEscapes(textValue ? textValue : ""), captureIdxVec,
                          draw_time_ms, outlineThickness, fillColor, outlineColor, outlineEnabled,
                          posX_px, posY_px, hAnchor, vAnchor, lineSpacing, textAlignment,
                          tracking, rotation);

        gEffectSuite->clipReleaseImage(outputImage);
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

// ---------------------------------------------------------------------------
// Plugin registration
// ---------------------------------------------------------------------------

static OfxPlugin gPlugin = {
    kOfxImageEffectPluginApi,
    1,
    "com.handwriting.HandwritingFX",
    1, 0,
    setHost,
    pluginMain
};

extern "C" {
    __attribute__((visibility("default")))
    void OfxSetHost(OfxHost* host) { setHost(host); }

    __attribute__((visibility("default")))
    int OfxGetNumberOfPlugins(void) { return 1; }

    __attribute__((visibility("default")))
    OfxPlugin* OfxGetPlugin(int nth) { return nth == 0 ? &gPlugin : nullptr; }
}
