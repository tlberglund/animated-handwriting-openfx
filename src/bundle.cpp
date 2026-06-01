#include "ofxCore.h"
#include "ofxImageEffect.h"

// Forward declarations — each plugin file exposes one accessor
OfxPlugin* getHandwritingPlugin();
OfxPlugin* getDrawingPlugin();

extern "C" {
    __attribute__((visibility("default")))
    int OfxGetNumberOfPlugins(void) { return 2; }

    __attribute__((visibility("default")))
    OfxPlugin* OfxGetPlugin(int nth)
    {
        if(nth == 0) return getHandwritingPlugin();
        if(nth == 1) return getDrawingPlugin();
        return nullptr;
    }
}
