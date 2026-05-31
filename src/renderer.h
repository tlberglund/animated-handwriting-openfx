#pragma once
#include "glyph_model.h"
#include <string>
#include <vector>

struct RenderContext {
    void*  pixelData;
    int    bounds[4];
    int    rowBytes;
    int    width;
    int    height;
    float  capHeightPx;
    float  pMax;
    double strokeThickness;
};

void renderHandwriting(const RenderContext& ctx,
                       const GlyphSet&      glyphSet,
                       const std::string&   text,
                       const std::vector<int>& captureIdxVec,
                       double draw_time_ms,
                       double outlineThickness,
                       const double fillColor[4],
                       const double outlineColor[4],
                       int outlineEnabled,
                       float posX_px, float posY_px,
                       int hAnchor, int vAnchor,
                       double lineSpacing, int textAlignment);
