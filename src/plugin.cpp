#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"

#include <cstring>

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
    if(strcmp(action, kOfxActionLoad)            == 0 ||
       strcmp(action, kOfxActionUnload)          == 0 ||
       strcmp(action, kOfxActionCreateInstance)  == 0 ||
       strcmp(action, kOfxActionDestroyInstance) == 0) {
        return kOfxStatOK;
    }

    if(strcmp(action, kOfxActionDescribe) == 0) {
        auto effect = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
        OfxPropertySetHandle props = nullptr;
        gEffectSuite->getPropertySet(effect, &props);

        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts,   0, kOfxImageEffectContextGenerator);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxPropLabel,                           0, "HandwritingFX");

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
