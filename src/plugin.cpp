#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

struct GlyphPoint { float x, y, t, p; };
using Stroke = std::vector<GlyphPoint>;

struct Capture {
    std::string id;
    float width;
    std::vector<Stroke> strokes;
    float duration;
};

struct Glyph {
    std::string character;
    std::vector<Capture> captures;
};

struct GlyphSet {
    std::string name;
    std::map<std::string, Glyph> glyphs;
    std::vector<std::string> ligatureKeys;
    float pMax;
};

struct InstanceData {
    GlyphSet glyphSet;
    std::string loadedPath;
    std::vector<int> captureIndices;
    std::string lastText;
};

// ---------------------------------------------------------------------------
// Global suite pointers
// ---------------------------------------------------------------------------

static OfxHost*               gHost        = nullptr;
static OfxPropertySuiteV1*    gPropSuite   = nullptr;
static OfxImageEffectSuiteV1* gEffectSuite = nullptr;
static OfxParameterSuiteV1*   gParamSuite  = nullptr;

// ---------------------------------------------------------------------------
// Resource helpers
// ---------------------------------------------------------------------------

static std::string getBundledGlyphSetPath()
{
    Dl_info info;
    if(!dladdr((void*)getBundledGlyphSetPath, &info) || !info.dli_fname) return "";
    std::string path = info.dli_fname;
    size_t macosPos = path.rfind("/MacOS/");
    if(macosPos == std::string::npos) return "";
    return path.substr(0, macosPos) + "/Resources/tim-hand.json";
}

static GlyphSet loadGlyphSet(const std::string& path)
{
    GlyphSet result;
    result.pMax = 1.0f;

    std::ifstream file(path);
    if(!file.is_open()) return result;

    nlohmann::json j;
    try { file >> j; }
    catch(...) { return result; }

    result.name = j.value("captureSetName", "");

    if(!j.contains("glyphs") || !j["glyphs"].is_object()) return result;

    float pMax = 0.0f;

    for(auto& [key, glyphJson] : j["glyphs"].items()) {
        if(!glyphJson.contains("captures")) continue;

        Glyph glyph;
        glyph.character = glyphJson.value("character", key);

        for(auto& captureJson : glyphJson["captures"]) {
            Capture capture;
            capture.id    = captureJson.value("id", "");
            capture.width = captureJson.value("width", 0.5f);

            float maxT = 0.0f;
            if(captureJson.contains("strokes")) {
                for(auto& strokeJson : captureJson["strokes"]) {
                    Stroke stroke;
                    for(auto& ptJson : strokeJson) {
                        GlyphPoint pt;
                        pt.x = ptJson.value("x", 0.0f);
                        pt.y = ptJson.value("y", 0.0f);
                        pt.t = ptJson.value("t", 0.0f);
                        pt.p = ptJson.value("p", 0.5f);
                        if(pt.t > maxT) maxT = pt.t;
                        if(pt.p > pMax) pMax = pt.p;
                        stroke.push_back(pt);
                    }
                    capture.strokes.push_back(std::move(stroke));
                }
            }
            capture.duration = maxT;
            glyph.captures.push_back(std::move(capture));
        }
        result.glyphs[key] = std::move(glyph);
    }

    for(auto& [key, unused] : result.glyphs) {
        if(key.size() > 1) result.ligatureKeys.push_back(key);
    }
    std::sort(result.ligatureKeys.begin(), result.ligatureKeys.end(),
              [](const std::string& a, const std::string& b){ return a.size() > b.size(); });

    result.pMax = (pMax > 0.0f) ? pMax : 1.0f;
    return result;
}

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

        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionCreateInstance) == 0) {
        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle effectProps = nullptr;
        gEffectSuite->getPropertySet(instance, &effectProps);

        InstanceData* data = new InstanceData();
        data->glyphSet = loadGlyphSet(getBundledGlyphSetPath());
        gPropSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, (void*)data);
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
            std::string text = textValue ? textValue : "";

            data->captureIndices.clear();
            size_t pos = 0;
            while(pos < text.size()) {
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
        void* ptr = nullptr;
        gPropSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        InstanceData* data = static_cast<InstanceData*>(ptr);

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

        // Parse captureSelections into a vector
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

        // Resolve glyphs with ligature substitution
        struct ResolvedGlyph { const Capture* capture; float width; };
        std::vector<ResolvedGlyph> resolved;
        std::string text = textValue;
        size_t pos = 0;
        int selIdx = 0;

        while(pos < text.size()) {
            if(text[pos] == ' ') {
                resolved.push_back({nullptr, 0.35f});
                ++pos; continue;
            }
            bool found = false;
            for(auto& key : data->glyphSet.ligatureKeys) {
                if(pos + key.size() <= text.size() &&
                   text.substr(pos, key.size()) == key) {
                    auto it = data->glyphSet.glyphs.find(key);
                    if(it != data->glyphSet.glyphs.end() && !it->second.captures.empty()) {
                        int idx = (selIdx < (int)captureIdxVec.size()) ? captureIdxVec[selIdx] : 0;
                        idx = idx % (int)it->second.captures.size();
                        resolved.push_back({&it->second.captures[idx],
                                            it->second.captures[idx].width});
                        pos += key.size(); ++selIdx; found = true; break;
                    }
                }
            }
            if(!found) {
                auto it = data->glyphSet.glyphs.find(text.substr(pos, 1));
                if(it != data->glyphSet.glyphs.end() && !it->second.captures.empty()) {
                    int idx = (selIdx < (int)captureIdxVec.size()) ? captureIdxVec[selIdx] : 0;
                    idx = idx % (int)it->second.captures.size();
                    resolved.push_back({&it->second.captures[idx],
                                        it->second.captures[idx].width});
                } else {
                    resolved.push_back({nullptr, 0.35f});
                }
                ++pos; ++selIdx;
            }
        }

        // Build animation sequence
        struct GlyphSeq { const Capture* capture; float penX; float seqStart; };
        std::vector<GlyphSeq> sequence;
        float penX = 0.0f;
        float seqCursor = 0.0f;
        for(auto& rg : resolved) {
            float dur = rg.capture ? rg.capture->duration : 0.0f;
            sequence.push_back({rg.capture, penX, seqCursor});
            penX      += rg.width;
            seqCursor += dur;
        }

        float pMax = (data->glyphSet.pMax > 0.0f) ? data->glyphSet.pMax : 1.0f;

        // Rasterize one stroke segment using perpendicular distance to the segment.
        // Data y increases downward (screen coords); OFX y increases upward, so we flip:
        //   pixY = (1 - y_data) * capHeightPx  →  baseline at y=0, cap-top at y=capHeightPx
        // ax/bx are already in cap-height units including the pen-x offset.
        // hardEdge=true: disc with 1px AA border (for outline); false: Gaussian falloff (for fill).
        auto splatSegment = [&](float ax, float ay, float ap,
                                 float bx, float by, float bp,
                                 float sigmaExtra, bool hardEdge, const double color[4]) {
            float pax = ax * capHeightPx,  pay = (1.0f - ay) * capHeightPx;
            float pbx = bx * capHeightPx,  pby = (1.0f - by) * capHeightPx;

            float sigma_a = ((float)strokeThickness * (ap / pMax) + sigmaExtra) * capHeightPx;
            float sigma_b = ((float)strokeThickness * (bp / pMax) + sigmaExtra) * capHeightPx;
            if(sigma_a < 0.5f) sigma_a = 0.5f;
            if(sigma_b < 0.5f) sigma_b = 0.5f;
            float max_sigma = std::max(sigma_a, sigma_b);

            float sdx = pbx - pax, sdy = pby - pay;
            float len2 = sdx * sdx + sdy * sdy;

            // Tight bounding box: hard-edge only needs radius+1, Gaussian needs 3*sigma
            float pad = hardEdge ? (max_sigma + 1.0f) : (3.0f * max_sigma);
            int x0 = std::max(0,          (int)std::floor(std::min(pax, pbx) - pad) - bounds[0]);
            int x1 = std::min(width  - 1, (int)std::ceil( std::max(pax, pbx) + pad) - bounds[0]);
            int y0 = std::max(0,          (int)std::floor(std::min(pay, pby) - pad) - bounds[1]);
            int y1 = std::min(height - 1, (int)std::ceil( std::max(pay, pby) + pad) - bounds[1]);

            for(int ry = y0; ry <= y1; ++ry) {
                float fy = (float)(ry + bounds[1]);
                float* rowPtr = (float*)((char*)pixelData + ry * rowBytes);
                for(int rx = x0; rx <= x1; ++rx) {
                    float fx = (float)(rx + bounds[0]);

                    // Clamp projection t onto [0,1] to get nearest segment point
                    float t = 0.0f;
                    if(len2 > 0.0f) {
                        t = ((fx - pax) * sdx + (fy - pay) * sdy) / len2;
                        if(t < 0.0f) t = 0.0f;
                        else if(t > 1.0f) t = 1.0f;
                    }

                    float nx = pax + t * sdx, ny = pay + t * sdy;
                    float dist2 = (fx - nx) * (fx - nx) + (fy - ny) * (fy - ny);
                    float sigma = sigma_a + t * (sigma_b - sigma_a);

                    float alpha;
                    if(hardEdge) {
                        float dist = sqrtf(dist2);
                        alpha = std::max(0.0f, std::min(1.0f, sigma + 0.5f - dist));
                    } else {
                        alpha = expf(-dist2 / (2.0f * sigma * sigma));
                    }
                    if(alpha < 0.002f) continue;

                    float* pixel = rowPtr + rx * 4;
                    float srcA = alpha * (float)color[3];
                    float dstA = pixel[3];
                    float outA = srcA + dstA * (1.0f - srcA);
                    if(outA > 0.0f) {
                        pixel[0] = (srcA * (float)color[0] + dstA * pixel[0] * (1.0f - srcA)) / outA;
                        pixel[1] = (srcA * (float)color[1] + dstA * pixel[1] * (1.0f - srcA)) / outA;
                        pixel[2] = (srcA * (float)color[2] + dstA * pixel[2] * (1.0f - srcA)) / outA;
                    }
                    pixel[3] = outA;
                }
            }
        };

        auto splatStrokes = [&](const Capture* cap, float tLocal, float glyphPenX,
                                 float sigmaExtra, bool hardEdge, const double color[4]) {
            if(!cap) return;
            for(auto& stroke : cap->strokes) {
                if(stroke.empty()) continue;

                int lastVisible = -1;
                for(int i = 0; i < (int)stroke.size(); ++i) {
                    if(stroke[i].t <= tLocal) lastVisible = i;
                    else break;
                }
                if(lastVisible < 0) continue;

                // Interpolated endpoint for any in-progress segment
                float endX = stroke[lastVisible].x;
                float endY = stroke[lastVisible].y;
                float endP = stroke[lastVisible].p;
                bool hasPartial = (lastVisible + 1 < (int)stroke.size());
                if(hasPartial) {
                    const auto& a = stroke[lastVisible];
                    const auto& b = stroke[lastVisible + 1];
                    float frac = (b.t > a.t) ? (tLocal - a.t) / (b.t - a.t) : 0.0f;
                    endX = a.x + frac * (b.x - a.x);
                    endY = a.y + frac * (b.y - a.y);
                    endP = a.p + frac * (b.p - a.p);
                }

                // Draw segments between consecutive visible points
                for(int i = 0; i < lastVisible; ++i) {
                    splatSegment(glyphPenX + stroke[i].x,   stroke[i].y,   stroke[i].p,
                                 glyphPenX + stroke[i+1].x, stroke[i+1].y, stroke[i+1].p,
                                 sigmaExtra, hardEdge, color);
                }

                if(hasPartial) {
                    splatSegment(glyphPenX + stroke[lastVisible].x, stroke[lastVisible].y, stroke[lastVisible].p,
                                 glyphPenX + endX, endY, endP,
                                 sigmaExtra, hardEdge, color);
                } else if(lastVisible == 0) {
                    float ax = glyphPenX + stroke[0].x;
                    splatSegment(ax, stroke[0].y, stroke[0].p, ax, stroke[0].y, stroke[0].p,
                                 sigmaExtra, hardEdge, color);
                }
            }
        };

        // Outline pass first: larger hard disc in outline color
        if(outlineEnabled) {
            for(auto& gs : sequence) {
                if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
                float tLocal = (float)(draw_time_ms - gs.seqStart);
                if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
                splatStrokes(gs.capture, tLocal, gs.penX, (float)outlineThickness, true, outlineColor);
            }
        }

        // Fill pass second: smaller hard disc composited on top, covering the outline interior
        for(auto& gs : sequence) {
            if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
            float tLocal = (float)(draw_time_ms - gs.seqStart);
            if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
            splatStrokes(gs.capture, tLocal, gs.penX, 0.0f, true, fillColor);
        }

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
