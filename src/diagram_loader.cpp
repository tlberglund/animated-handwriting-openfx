#include "diagram_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>

DiagramData loadDiagram(const std::string& path)
{
    DiagramData result;
    if(path.empty()) return result;

    std::ifstream f(path);
    if(!f.is_open()) return result;

    nlohmann::json j;
    try { f >> j; } catch(...) { return result; }

    result.name        = j.value("name", "");
    result.aspectRatio = j.value("aspectRatio", 1.0f);

    for(auto& jStroke : j["strokes"]) {
        Stroke stroke;
        for(auto& jp : jStroke) {
            GlyphPoint pt;
            pt.x = jp.value("x", 0.0f);
            pt.y = jp.value("y", 0.0f);
            pt.t = jp.value("t", 0.0f);
            pt.p = jp.value("p", 1.0f);
            stroke.push_back(pt);
        }
        if(!stroke.empty()) {
            result.totalDuration += stroke.back().t;
            result.strokes.push_back(std::move(stroke));
        }
    }

    return result;
}
