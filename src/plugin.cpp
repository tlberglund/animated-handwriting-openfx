#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstring>
#include <cmath>
#include <string>
#include <dlfcn.h>

static OfxHost*               gHost        = nullptr;
static OfxPropertySuiteV1*    gPropSuite   = nullptr;
static OfxImageEffectSuiteV1* gEffectSuite = nullptr;
static OfxParameterSuiteV1*   gParamSuite  = nullptr;

static FT_Library gFTLibrary = nullptr;
static FT_Face    gFace      = nullptr;

// Derive font path from this dylib's location:
// .../Contents/MacOS/HandwritingFX.ofx → .../Contents/Resources/OpenSans-Regular.ttf
static std::string getFontPath()
{
    Dl_info info;
    if(!dladdr((void*)getFontPath, &info) || !info.dli_fname) return "";
    std::string path = info.dli_fname;
    size_t macosPos = path.rfind("/MacOS/");
    if(macosPos == std::string::npos) return "";
    return path.substr(0, macosPos) + "/Resources/OpenSans-Regular.ttf";
}

static void setHost(OfxHost* host)
{
    gHost = host;
    if(!host) return;
    gPropSuite   = (OfxPropertySuiteV1*)  host->fetchSuite(host->host, kOfxPropertySuite,    1);
    gEffectSuite = (OfxImageEffectSuiteV1*)host->fetchSuite(host->host, kOfxImageEffectSuite, 1);
    gParamSuite  = (OfxParameterSuiteV1*) host->fetchSuite(host->host, kOfxParameterSuite,   1);
}

static OfxStatus pluginMain(const char*          action,
                             const void*          handle,
                             OfxPropertySetHandle inArgs,
                             OfxPropertySetHandle /*outArgs*/)
{
    if(strcmp(action, kOfxActionLoad) == 0) {
        FT_Init_FreeType(&gFTLibrary);
        std::string fontPath = getFontPath();
        if(gFTLibrary && !fontPath.empty())
            FT_New_Face(gFTLibrary, fontPath.c_str(), 0, &gFace);
        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionUnload) == 0) {
        if(gFace)      { FT_Done_Face(gFace);           gFace      = nullptr; }
        if(gFTLibrary) { FT_Done_FreeType(gFTLibrary);  gFTLibrary = nullptr; }
        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionCreateInstance)  == 0 ||
       strcmp(action, kOfxActionDestroyInstance) == 0) {
        return kOfxStatOK;
    }

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
        gPropSuite->propSetString(props, kOfxPropLabel,            0, "Text");
        gPropSuite->propSetString(props, kOfxParamPropStringMode,  0, kOfxParamStringIsMultiLine);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "textHeight", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,              0, "Text Height");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,       0, 0.1);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,           0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,           0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin,    0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax,    0, 1.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "strokeThickness", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,              0, "Stroke Thickness");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,       0, 2.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,           0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,           0, 50.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin,    0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax,    0, 50.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "speed", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,              0, "Speed");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,       0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,           0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMax,           0, 10.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin,    0, 0.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax,    0, 10.0);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "audio", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,    0, "Audio");
        gPropSuite->propSetInt(props,    kOfxParamPropDefault, 0, 0);

        return kOfxStatOK;
    }

    if(strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        return kOfxStatReplyDefault;
    }

    if(strcmp(action, kOfxImageEffectActionRender) == 0) {
        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));

        // Read render time
        double renderTime = 0.0;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &renderTime);

        // Read render scale
        double renderScale[2] = {1.0, 1.0};
        gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropRenderScale, 2, renderScale);

        // Read parameter values at render time
        OfxParamSetHandle paramSet = nullptr;
        gEffectSuite->getParamSet(instance, &paramSet);

        OfxParamHandle textParam = nullptr, textHeightParam = nullptr;
        gParamSuite->paramGetHandle(paramSet, "text",       &textParam,       nullptr);
        gParamSuite->paramGetHandle(paramSet, "textHeight", &textHeightParam, nullptr);

        char*  textValue  = nullptr;
        double textHeight = 0.1;
        gParamSuite->paramGetValueAtTime(textParam,       renderTime, &textValue);
        gParamSuite->paramGetValueAtTime(textHeightParam, renderTime, &textHeight);

        // Fetch output image
        OfxImageClipHandle outputClip = nullptr;
        gEffectSuite->clipGetHandle(instance, kOfxImageEffectOutputClipName, &outputClip, nullptr);

        OfxPropertySetHandle outputImage = nullptr;
        gEffectSuite->clipGetImage(outputClip, renderTime, nullptr, &outputImage);

        if(!outputImage) return kOfxStatFailed;

        // Image geometry
        int bounds[4] = {0, 0, 0, 0};  // x1, y1, x2, y2
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

        // Clear to transparent black
        for(int row = 0; row < height; ++row) {
            float* p = (float*)((char*)pixelData + row * rowBytes);
            for(int col = 0; col < width * 4; ++col)
                p[col] = 0.0f;
        }

        // Render text if FreeType is ready and we have text
        if(gFace && textValue && textValue[0] != '\0') {
            int pixelSize = (int)round(textHeight * height * renderScale[1]);
            if(pixelSize < 1) pixelSize = 1;
            FT_Set_Pixel_Sizes(gFace, 0, (FT_UInt)pixelSize);

            // Pen starts at (0, 0) in OFX space — bottom-left corner, baseline
            int penX = 0;

            for(const char* c = textValue; *c; ++c) {
                if(FT_Load_Char(gFace, (unsigned char)*c, FT_LOAD_RENDER) != 0)
                    continue;

                FT_GlyphSlot slot   = gFace->glyph;
                FT_Bitmap&   bitmap = slot->bitmap;

                int glyphLeft = penX + slot->bitmap_left;
                // bitmap_top is distance from baseline to top of bitmap, y-up
                int glyphBaseRow = slot->bitmap_top;  // row in OFX y-space relative to baseline

                for(int gr = 0; gr < (int)bitmap.rows; ++gr) {
                    // OFX y of this glyph row (y increases upward)
                    int ofxY = glyphBaseRow - gr;
                    // Buffer row (0 = bottom of image = y1 in OFX)
                    int bufRow = ofxY - bounds[1];
                    if(bufRow < 0 || bufRow >= height) continue;

                    unsigned char* glyphRow = bitmap.buffer + gr * bitmap.pitch;

                    for(int gc = 0; gc < (int)bitmap.width; ++gc) {
                        int bufCol = glyphLeft + gc - bounds[0];
                        if(bufCol < 0 || bufCol >= width) continue;

                        float alpha = glyphRow[gc] / 255.0f;
                        if(alpha <= 0.0f) continue;

                        float* pixel = (float*)((char*)pixelData + bufRow * rowBytes) + bufCol * 4;
                        // Over composite white glyph onto cleared (0,0,0,0) background
                        float dstA  = pixel[3];
                        float outA  = alpha + dstA * (1.0f - alpha);
                        if(outA > 0.0f) {
                            pixel[0] = (alpha * 1.0f + dstA * pixel[0] * (1.0f - alpha)) / outA;
                            pixel[1] = (alpha * 1.0f + dstA * pixel[1] * (1.0f - alpha)) / outA;
                            pixel[2] = (alpha * 1.0f + dstA * pixel[2] * (1.0f - alpha)) / outA;
                        }
                        pixel[3] = outA;
                    }
                }

                penX += slot->advance.x >> 6;
            }
        }

        gEffectSuite->clipReleaseImage(outputImage);
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

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
