#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"

#include "diagram_loader.h"
#include "renderer.h"

#include <cmath>
#include <cstring>
#include <string>

static OfxHost*               gHost        = nullptr;
static OfxPropertySuiteV1*    gPropSuite   = nullptr;
static OfxImageEffectSuiteV1* gEffectSuite = nullptr;
static OfxParameterSuiteV1*   gParamSuite  = nullptr;

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
    if(strcmp(action, kOfxActionLoad)   == 0 ||
       strcmp(action, kOfxActionUnload) == 0)
        return kOfxStatOK;

    if(strcmp(action, kOfxActionDescribe) == 0) {
        auto effect = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle props = nullptr;
        gEffectSuite->getPropertySet(effect, &props);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts,    0, kOfxImageEffectContextGenerator);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxPropLabel,                            0, "HandwrittenDrawingFX");
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

        gParamSuite->paramDefine(paramSet, kOfxParamTypeString, "diagramFile", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Diagram File");
        gPropSuite->propSetString(props, kOfxParamPropStringMode, 0, kOfxParamStringIsFilePath);
        gPropSuite->propSetString(props, kOfxParamPropDefault,    0, "");

        gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "animate", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,        0, "Animate");
        gPropSuite->propSetInt(props,    kOfxParamPropDefault, 0, 1);

        gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "diagramHeight", &props);
        gPropSuite->propSetString(props, kOfxPropLabel,           0, "Diagram Height");
        gPropSuite->propSetDouble(props, kOfxParamPropDefault,    0, 324.0);
        gPropSuite->propSetDouble(props, kOfxParamPropMin,        0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, 1.0);
        gPropSuite->propSetDouble(props, kOfxParamPropDisplayMax, 0, 8192.0);

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

        DiagramInstanceData* data = new DiagramInstanceData();
        gPropSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, (void*)data);

        double projectSize[2] = {1920.0, 1080.0};
        gPropSuite->propGetDoubleN(effectProps, kOfxImageEffectPropProjectSize, 2, projectSize);

        OfxParamSetHandle paramSet = nullptr;
        gEffectSuite->getParamSet(instance, &paramSet);

        OfxParamHandle posXHandle = nullptr, posYHandle = nullptr, heightHandle = nullptr;
        gParamSuite->paramGetHandle(paramSet, "posX",          &posXHandle,   nullptr);
        gParamSuite->paramGetHandle(paramSet, "posY",          &posYHandle,   nullptr);
        gParamSuite->paramGetHandle(paramSet, "diagramHeight", &heightHandle, nullptr);

        if(posXHandle)   gParamSuite->paramSetValue(posXHandle,   projectSize[0] / 2.0);
        if(posYHandle)   gParamSuite->paramSetValue(posYHandle,   projectSize[1] / 2.0);
        if(heightHandle) gParamSuite->paramSetValue(heightHandle, round(projectSize[1] * 0.3));

        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionDestroyInstance) == 0) {
        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle effectProps = nullptr;
        gEffectSuite->getPropertySet(instance, &effectProps);
        void* ptr = nullptr;
        gPropSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        delete static_cast<DiagramInstanceData*>(ptr);
        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionInstanceChanged) == 0) {
        char* reason = nullptr;
        gPropSuite->propGetString(inArgs, kOfxPropChangeReason, 0, &reason);
        if(!reason || strcmp(reason, kOfxChangeUserEdited) != 0)
            return kOfxStatOK;

        char* paramName = nullptr;
        gPropSuite->propGetString(inArgs, kOfxPropName, 0, &paramName);
        if(!paramName || strcmp(paramName, "diagramFile") != 0)
            return kOfxStatOK;

        auto instance = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle effectProps = nullptr;
        gEffectSuite->getPropertySet(instance, &effectProps);
        void* ptr = nullptr;
        gPropSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, &ptr);
        DiagramInstanceData* data = static_cast<DiagramInstanceData*>(ptr);
        if(!data) return kOfxStatOK;

        OfxParamSetHandle paramSet = nullptr;
        gEffectSuite->getParamSet(instance, &paramSet);
        OfxParamHandle fileParam = nullptr;
        gParamSuite->paramGetHandle(paramSet, "diagramFile", &fileParam, nullptr);
        char* filePath = nullptr;
        gParamSuite->paramGetValue(fileParam, &filePath);
        std::string path = filePath ? filePath : "";

        if(!path.empty()) {
            DiagramData newDiagram = loadDiagram(path);
            if(!newDiagram.strokes.empty()) {
                data->diagram    = std::move(newDiagram);
                data->loadedPath = path;
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
        DiagramInstanceData* data = static_cast<DiagramInstanceData*>(iptr);

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

        // Lazy-load: Resolve doesn't reliably fire InstanceChanged for string params,
        // so reload whenever the param path differs from what we have cached.
        if(data) {
            char* filePathVal = nullptr;
            gParamSuite->paramGetValue(getParam("diagramFile"), &filePathVal);
            std::string currentPath = filePathVal ? filePathVal : "";
            if(!currentPath.empty() && currentPath != data->loadedPath) {
                DiagramData newDiagram = loadDiagram(currentPath);
                if(!newDiagram.strokes.empty()) {
                    data->diagram    = std::move(newDiagram);
                    data->loadedPath = currentPath;
                }
            }
        }

        double diagramHeight = 324.0, speed = 1.0, strokeThickness = 0.02, outlineThickness = 0.01;
        double fillColor[4]    = {1.0, 1.0, 1.0, 1.0};
        double outlineColor[4] = {0.0, 0.0, 0.0, 1.0};
        int outlineEnabled = 1;
        double posX = 0.0, posY = 0.0;
        int hAnchor = 1, vAnchor = 1;
        int animate = 1;
        double rotation = 0.0;

        gParamSuite->paramGetValueAtTime(getParam("diagramHeight"),    renderTime, &diagramHeight);
        gParamSuite->paramGetValueAtTime(getParam("speed"),            renderTime, &speed);
        gParamSuite->paramGetValueAtTime(getParam("strokeThickness"),  renderTime, &strokeThickness);
        gParamSuite->paramGetValueAtTime(getParam("fillColor"),        renderTime,
                                         &fillColor[0], &fillColor[1], &fillColor[2], &fillColor[3]);
        gParamSuite->paramGetValueAtTime(getParam("outlineColor"),     renderTime,
                                         &outlineColor[0], &outlineColor[1], &outlineColor[2], &outlineColor[3]);
        gParamSuite->paramGetValueAtTime(getParam("outlineThickness"), renderTime, &outlineThickness);
        gParamSuite->paramGetValueAtTime(getParam("outlineEnabled"),   renderTime, &outlineEnabled);
        gParamSuite->paramGetValueAtTime(getParam("posX"),             renderTime, &posX);
        gParamSuite->paramGetValueAtTime(getParam("posY"),             renderTime, &posY);
        gParamSuite->paramGetValueAtTime(getParam("hAnchor"),          renderTime, &hAnchor);
        gParamSuite->paramGetValueAtTime(getParam("vAnchor"),          renderTime, &vAnchor);
        gParamSuite->paramGetValueAtTime(getParam("animate"),          renderTime, &animate);
        gParamSuite->paramGetValueAtTime(getParam("rotation"),         renderTime, &rotation);

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

        if(!data || data->diagram.strokes.empty()) {
            gEffectSuite->clipReleaseImage(outputImage);
            return kOfxStatOK;
        }

        float diagramHeightPx = (float)(diagramHeight * renderScale[1]);
        if(diagramHeightPx < 1.0f) diagramHeightPx = 1.0f;

        double draw_time_ms = (renderTime / fps) * 1000.0 * speed;
        if(!animate) draw_time_ms = 1e12;

        RenderContext ctx;
        ctx.pixelData    = pixelData;
        ctx.bounds[0]    = bounds[0]; ctx.bounds[1] = bounds[1];
        ctx.bounds[2]    = bounds[2]; ctx.bounds[3] = bounds[3];
        ctx.rowBytes     = rowBytes;
        ctx.width        = width;
        ctx.height       = height;
        ctx.capHeightPx  = diagramHeightPx;
        ctx.pMax         = 1.0f;
        ctx.strokeThickness = strokeThickness;

        float posX_px = (float)(posX * renderScale[0]);
        float posY_px = (float)(posY * renderScale[1]);

        renderDiagram(ctx, data->diagram, draw_time_ms,
                      outlineThickness, fillColor, outlineColor, outlineEnabled,
                      posX_px, posY_px, hAnchor, vAnchor, rotation, diagramHeightPx);

        gEffectSuite->clipReleaseImage(outputImage);
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

static OfxPlugin gPlugin = {
    kOfxImageEffectPluginApi,
    1,
    "com.handwriting.HandwrittenDrawingFX",
    1, 0,
    setHost,
    pluginMain
};

OfxPlugin* getDrawingPlugin() { return &gPlugin; }
