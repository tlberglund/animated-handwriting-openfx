#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static void splatSegment(const RenderContext& ctx,
                          float ax, float ay, float ap,
                          float bx, float by, float bp,
                          float sigmaExtra, bool hardEdge, const double color[4],
                          float originX, float baselineY,
                          float scaleX, float scaleY,
                          float rotSin, float rotCos,
                          float rotCenterX, float rotCenterY)
{
    float pax = originX + ax * scaleX,  pay = baselineY + (1.0f - ay) * scaleY;
    float pbx = originX + bx * scaleX,  pby = baselineY + (1.0f - by) * scaleY;

    // Rotate pixel coords around (rotCenterX, rotCenterY)
    {
        float dx = pax - rotCenterX, dy = pay - rotCenterY;
        pax = rotCenterX + dx * rotCos - dy * rotSin;
        pay = rotCenterY + dx * rotSin + dy * rotCos;
    }
    {
        float dx = pbx - rotCenterX, dy = pby - rotCenterY;
        pbx = rotCenterX + dx * rotCos - dy * rotSin;
        pby = rotCenterY + dx * rotSin + dy * rotCos;
    }

    float sigma_a = ((float)ctx.strokeThickness * (ap / ctx.pMax) + sigmaExtra) * ctx.capHeightPx;
    float sigma_b = ((float)ctx.strokeThickness * (bp / ctx.pMax) + sigmaExtra) * ctx.capHeightPx;
    if(sigma_a < 0.5f) sigma_a = 0.5f;
    if(sigma_b < 0.5f) sigma_b = 0.5f;
    float max_sigma = std::max(sigma_a, sigma_b);

    float sdx = pbx - pax, sdy = pby - pay;
    float len2 = sdx * sdx + sdy * sdy;

    float pad = hardEdge ? (max_sigma + 1.0f) : (3.0f * max_sigma);
    int x0 = std::max(0,              (int)std::floor(std::min(pax, pbx) - pad) - ctx.bounds[0]);
    int x1 = std::min(ctx.width  - 1, (int)std::ceil( std::max(pax, pbx) + pad) - ctx.bounds[0]);
    int y0 = std::max(0,              (int)std::floor(std::min(pay, pby) - pad) - ctx.bounds[1]);
    int y1 = std::min(ctx.height - 1, (int)std::ceil( std::max(pay, pby) + pad) - ctx.bounds[1]);

    for(int ry = y0; ry <= y1; ++ry) {
        float fy = (float)(ry + ctx.bounds[1]);
        float* rowPtr = (float*)((char*)ctx.pixelData + ry * ctx.rowBytes);
        for(int rx = x0; rx <= x1; ++rx) {
            float fx = (float)(rx + ctx.bounds[0]);

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
}

// Renders all strokes within a single Capture (used by handwriting path)
static void splatStrokes(const RenderContext& ctx,
                          const Capture* cap, float tLocal, float glyphPenX,
                          float sigmaExtra, bool hardEdge, const double color[4],
                          float originX, float baselineY,
                          float scaleX, float scaleY,
                          float rotSin, float rotCos,
                          float rotCenterX, float rotCenterY)
{
    if(!cap) return;
    for(auto& stroke : cap->strokes) {
        if(stroke.empty()) continue;

        int lastVisible = -1;
        for(int i = 0; i < (int)stroke.size(); ++i) {
            if(stroke[i].t <= tLocal) lastVisible = i;
            else break;
        }
        if(lastVisible < 0) continue;

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

        for(int i = 0; i < lastVisible; ++i) {
            splatSegment(ctx,
                         glyphPenX + stroke[i].x,   stroke[i].y,   stroke[i].p,
                         glyphPenX + stroke[i+1].x, stroke[i+1].y, stroke[i+1].p,
                         sigmaExtra, hardEdge, color, originX, baselineY,
                         scaleX, scaleY, rotSin, rotCos, rotCenterX, rotCenterY);
        }

        if(hasPartial) {
            splatSegment(ctx,
                         glyphPenX + stroke[lastVisible].x, stroke[lastVisible].y, stroke[lastVisible].p,
                         glyphPenX + endX, endY, endP,
                         sigmaExtra, hardEdge, color, originX, baselineY,
                         scaleX, scaleY, rotSin, rotCos, rotCenterX, rotCenterY);
        } else if(lastVisible == 0) {
            float ax = glyphPenX + stroke[0].x;
            splatSegment(ctx, ax, stroke[0].y, stroke[0].p, ax, stroke[0].y, stroke[0].p,
                         sigmaExtra, hardEdge, color, originX, baselineY,
                         scaleX, scaleY, rotSin, rotCos, rotCenterX, rotCenterY);
        }
    }
}

// Renders a single Stroke up to tLocal (used by diagram path)
static void splatOneStroke(const RenderContext& ctx,
                            const Stroke& stroke, float tLocal,
                            float sigmaExtra, bool hardEdge, const double color[4],
                            float originX, float baselineY,
                            float scaleX, float scaleY,
                            float rotSin, float rotCos,
                            float rotCenterX, float rotCenterY)
{
    if(stroke.empty()) return;

    int lastVisible = -1;
    for(int i = 0; i < (int)stroke.size(); ++i) {
        if(stroke[i].t <= tLocal) lastVisible = i;
        else break;
    }
    if(lastVisible < 0) return;

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

    for(int i = 0; i < lastVisible; ++i) {
        splatSegment(ctx,
                     stroke[i].x,   stroke[i].y,   stroke[i].p,
                     stroke[i+1].x, stroke[i+1].y, stroke[i+1].p,
                     sigmaExtra, hardEdge, color, originX, baselineY,
                     scaleX, scaleY, rotSin, rotCos, rotCenterX, rotCenterY);
    }

    if(hasPartial) {
        splatSegment(ctx,
                     stroke[lastVisible].x, stroke[lastVisible].y, stroke[lastVisible].p,
                     endX, endY, endP,
                     sigmaExtra, hardEdge, color, originX, baselineY,
                     scaleX, scaleY, rotSin, rotCos, rotCenterX, rotCenterY);
    } else if(lastVisible == 0) {
        splatSegment(ctx,
                     stroke[0].x, stroke[0].y, stroke[0].p,
                     stroke[0].x, stroke[0].y, stroke[0].p,
                     sigmaExtra, hardEdge, color, originX, baselineY,
                     scaleX, scaleY, rotSin, rotCos, rotCenterX, rotCenterY);
    }
}

// ---------------------------------------------------------------------------
// renderHandwriting
// ---------------------------------------------------------------------------

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
                       double lineSpacing, int textAlignment,
                       double tracking, double rotation)
{
    float rotRad = (float)rotation * (float)M_PI / 180.0f;
    float rotSin = sinf(rotRad);
    float rotCos = cosf(rotRad);
    float scaleXY = ctx.capHeightPx; // square normalization for glyphs

    std::vector<std::string> lines;
    {
        size_t start = 0;
        while(true) {
            size_t nl = text.find('\n', start);
            if(nl == std::string::npos) { lines.push_back(text.substr(start)); break; }
            lines.push_back(text.substr(start, nl - start));
            start = nl + 1;
        }
    }
    int numLines = (int)lines.size();

    struct ResolvedGlyph { const Capture* capture; float width; };
    struct LineData { std::vector<ResolvedGlyph> glyphs; float lineWidth; };
    std::vector<LineData> perLine;
    perLine.reserve(numLines);

    int selIdx = 0;
    for(auto& line : lines) {
        std::vector<ResolvedGlyph> resolved;
        size_t pos = 0;
        while(pos < line.size()) {
            if(line[pos] == ' ') { resolved.push_back({nullptr, 0.35f}); ++pos; continue; }
            bool found = false;
            for(auto& key : glyphSet.ligatureKeys) {
                if(pos + key.size() <= line.size() && line.substr(pos, key.size()) == key) {
                    auto it = glyphSet.glyphs.find(key);
                    if(it != glyphSet.glyphs.end() && !it->second.captures.empty()) {
                        int idx = (selIdx < (int)captureIdxVec.size()) ? captureIdxVec[selIdx] : 0;
                        idx = idx % (int)it->second.captures.size();
                        resolved.push_back({&it->second.captures[idx], it->second.captures[idx].width});
                        pos += key.size(); ++selIdx; found = true; break;
                    }
                }
            }
            if(!found) {
                auto it = glyphSet.glyphs.find(line.substr(pos, 1));
                if(it != glyphSet.glyphs.end() && !it->second.captures.empty()) {
                    int idx = (selIdx < (int)captureIdxVec.size()) ? captureIdxVec[selIdx] : 0;
                    idx = idx % (int)it->second.captures.size();
                    resolved.push_back({&it->second.captures[idx], it->second.captures[idx].width});
                } else {
                    resolved.push_back({nullptr, 0.35f});
                }
                ++pos; ++selIdx;
            }
        }
        float lineWidth = 0.0f;
        for(auto& rg : resolved) lineWidth += rg.width + (float)tracking;
        perLine.push_back({std::move(resolved), lineWidth});
    }

    float blockWidth = 0.0f;
    for(auto& ld : perLine) blockWidth = std::max(blockWidth, ld.lineWidth);

    float originX_block;
    switch(hAnchor) {
        case 0:  originX_block = posX_px; break;
        case 2:  originX_block = posX_px - blockWidth * scaleXY; break;
        default: originX_block = posX_px - blockWidth * scaleXY * 0.5f; break;
    }

    float lsf = (float)lineSpacing;
    float firstBaselineY;
    switch(vAnchor) {
        case 0:  firstBaselineY = posY_px - scaleXY; break;
        case 2:  firstBaselineY = posY_px + (numLines - 1) * lsf * scaleXY; break;
        default: firstBaselineY = posY_px - scaleXY * 0.5f + (numLines - 1) * lsf * scaleXY * 0.5f; break;
    }

    struct GlyphSeq { const Capture* capture; float penX; float seqStart; float originX; float baselineY; };
    std::vector<GlyphSeq> sequence;
    float seqCursor = 0.0f;

    for(int li = 0; li < numLines; ++li) {
        float lineBaselineY = firstBaselineY - li * lsf * scaleXY;
        float lineWidth = perLine[li].lineWidth;
        float lineOriginX;
        switch(textAlignment) {
            case 1:  lineOriginX = originX_block + (blockWidth - lineWidth) * scaleXY * 0.5f; break;
            case 2:  lineOriginX = originX_block + (blockWidth - lineWidth) * scaleXY; break;
            default: lineOriginX = originX_block; break;
        }
        float penX = 0.0f;
        for(auto& rg : perLine[li].glyphs) {
            float dur = rg.capture ? rg.capture->duration : 0.0f;
            sequence.push_back({rg.capture, penX, seqCursor, lineOriginX, lineBaselineY});
            penX      += rg.width + (float)tracking;
            seqCursor += dur;
        }
    }

    if(outlineEnabled) {
        for(auto& gs : sequence) {
            if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
            float tLocal = (float)(draw_time_ms - gs.seqStart);
            if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
            splatStrokes(ctx, gs.capture, tLocal, gs.penX, (float)outlineThickness, true, outlineColor,
                         gs.originX, gs.baselineY, scaleXY, scaleXY,
                         rotSin, rotCos, posX_px, posY_px);
        }
    }

    for(auto& gs : sequence) {
        if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
        float tLocal = (float)(draw_time_ms - gs.seqStart);
        if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
        splatStrokes(ctx, gs.capture, tLocal, gs.penX, 0.0f, true, fillColor,
                     gs.originX, gs.baselineY, scaleXY, scaleXY,
                     rotSin, rotCos, posX_px, posY_px);
    }
}

// ---------------------------------------------------------------------------
// renderDiagram
// ---------------------------------------------------------------------------

void renderDiagram(const RenderContext& ctx,
                   const DiagramData&   diagram,
                   double draw_time_ms,
                   double outlineThickness,
                   const double fillColor[4],
                   const double outlineColor[4],
                   int outlineEnabled,
                   float posX_px, float posY_px,
                   int hAnchor, int vAnchor,
                   double rotation,
                   float diagramHeightPx)
{
    if(diagram.strokes.empty()) return;

    float rotRad = (float)rotation * (float)M_PI / 180.0f;
    float rotSin = sinf(rotRad);
    float rotCos = cosf(rotRad);

    float diagramWidthPx = diagramHeightPx * diagram.aspectRatio;
    float scaleX = diagramWidthPx;
    float scaleY = diagramHeightPx;

    // originX = left edge of bounding box
    float originX;
    switch(hAnchor) {
        case 0:  originX = posX_px; break;                             // Left
        case 2:  originX = posX_px - diagramWidthPx; break;           // Right
        default: originX = posX_px - diagramWidthPx * 0.5f; break;    // Center
    }

    // originY = bottom of bounding box (splatSegment uses: pay = originY + (1-y)*scaleY)
    float originY;
    switch(vAnchor) {
        case 0:  originY = posY_px - diagramHeightPx; break;          // Top
        case 2:  originY = posY_px; break;                             // Bottom
        default: originY = posY_px - diagramHeightPx * 0.5f; break;   // Middle
    }

    // Build per-stroke sequence cursors
    struct StrokeEntry { const Stroke* stroke; float seqStart; float duration; };
    std::vector<StrokeEntry> sequence;
    sequence.reserve(diagram.strokes.size());
    float seqCursor = 0.0f;
    for(auto& stroke : diagram.strokes) {
        float dur = stroke.empty() ? 0.0f : stroke.back().t;
        sequence.push_back({&stroke, seqCursor, dur});
        seqCursor += dur;
    }

    if(outlineEnabled) {
        for(auto& se : sequence) {
            if(se.stroke->empty() || draw_time_ms <= (double)se.seqStart) continue;
            float tLocal = (float)(draw_time_ms - se.seqStart);
            if(tLocal > se.duration) tLocal = se.duration;
            splatOneStroke(ctx, *se.stroke, tLocal,
                           (float)outlineThickness, true, outlineColor,
                           originX, originY, scaleX, scaleY,
                           rotSin, rotCos, posX_px, posY_px);
        }
    }

    for(auto& se : sequence) {
        if(se.stroke->empty() || draw_time_ms <= (double)se.seqStart) continue;
        float tLocal = (float)(draw_time_ms - se.seqStart);
        if(tLocal > se.duration) tLocal = se.duration;
        splatOneStroke(ctx, *se.stroke, tLocal,
                       0.0f, true, fillColor,
                       originX, originY, scaleX, scaleY,
                       rotSin, rotCos, posX_px, posY_px);
    }
}
