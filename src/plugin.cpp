#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxProperty.h"

#include <cstring>

static OfxHost*               gHost        = nullptr;
static OfxPropertySuiteV1*    gPropSuite   = nullptr;
static OfxImageEffectSuiteV1* gEffectSuite = nullptr;

static void setHost(OfxHost* host)
{
    gHost = host;
    if(!host) return;
    gPropSuite   = (OfxPropertySuiteV1*)  host->fetchSuite(host->host, kOfxPropertySuite,   1);
    gEffectSuite = (OfxImageEffectSuiteV1*)host->fetchSuite(host->host, kOfxImageEffectSuite, 1);
}

static OfxStatus pluginMain(const char*          action,
                             const void*          handle,
                             OfxPropertySetHandle inArgs,
                             OfxPropertySetHandle /*outArgs*/)
{
    if(strcmp(action, kOfxActionLoad) == 0 ||
       strcmp(action, kOfxActionUnload) == 0) {
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

        // Generators produce output with no source clip
        OfxPropertySetHandle outProps = nullptr;
        gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &outProps);
        gPropSuite->propSetString(outProps, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

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
