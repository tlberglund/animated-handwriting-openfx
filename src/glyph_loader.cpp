#include "glyph_loader.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <dlfcn.h>
#include <fstream>

std::string getBundledGlyphSetPath()
{
    Dl_info info;
    if(!dladdr((void*)getBundledGlyphSetPath, &info) || !info.dli_fname) return "";
    std::string path = info.dli_fname;
    size_t macosPos = path.rfind("/MacOS/");
    if(macosPos == std::string::npos) return "";
    return path.substr(0, macosPos) + "/Resources/tim-hand.json";
}

GlyphSet loadGlyphSet(const std::string& path)
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

    for(auto& [key, unused] : result.glyphs)
        if(key.size() > 1) result.ligatureKeys.push_back(key);

    std::sort(result.ligatureKeys.begin(), result.ligatureKeys.end(),
              [](const std::string& a, const std::string& b){ return a.size() > b.size(); });

    result.pMax = (pMax > 0.0f) ? pMax : 1.0f;
    return result;
}
