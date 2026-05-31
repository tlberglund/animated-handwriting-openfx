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
                          float originX, float baselineY)
{
    float pax = originX + ax * ctx.capHeightPx,  pay = baselineY + (1.0f - ay) * ctx.capHeightPx;
    float pbx = originX + bx * ctx.capHeightPx,  pby = baselineY + (1.0f - by) * ctx.capHeightPx;

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

static void splatStrokes(const RenderContext& ctx,
                          const Capture* cap, float tLocal, float glyphPenX,
                          float sigmaExtra, bool hardEdge, const double color[4],
                          float originX, float baselineY)
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
                         sigmaExtra, hardEdge, color, originX, baselineY);
        }

        if(hasPartial) {
            splatSegment(ctx,
                         glyphPenX + stroke[lastVisible].x, stroke[lastVisible].y, stroke[lastVisible].p,
                         glyphPenX + endX, endY, endP,
                         sigmaExtra, hardEdge, color, originX, baselineY);
        } else if(lastVisible == 0) {
            float ax = glyphPenX + stroke[0].x;
            splatSegment(ctx, ax, stroke[0].y, stroke[0].p, ax, stroke[0].y, stroke[0].p,
                         sigmaExtra, hardEdge, color, originX, baselineY);
        }
    }
}

// ---------------------------------------------------------------------------
// Public entry point
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
                       double lineSpacing, int textAlignment)
{
    // Split text on newlines
    std::vector<std::string> lines;
    {
        size_t start = 0;
        while(true) {
            size_t nl = text.find('\n', start);
            if(nl == std::string::npos) {
                lines.push_back(text.substr(start));
                break;
            }
            lines.push_back(text.substr(start, nl - start));
            start = nl + 1;
        }
    }
    int numLines = (int)lines.size();

    // Resolve glyphs for each line; captureIdxVec is indexed globally across lines
    struct ResolvedGlyph { const Capture* capture; float width; };
    struct LineData { std::vector<ResolvedGlyph> glyphs; float lineWidth; };
    std::vector<LineData> perLine;
    perLine.reserve(numLines);

    int selIdx = 0;
    for(auto& line : lines) {
        std::vector<ResolvedGlyph> resolved;
        size_t pos = 0;
        while(pos < line.size()) {
            if(line[pos] == ' ') {
                resolved.push_back({nullptr, 0.35f});
                ++pos; continue;
            }
            bool found = false;
            for(auto& key : glyphSet.ligatureKeys) {
                if(pos + key.size() <= line.size() &&
                   line.substr(pos, key.size()) == key) {
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
        for(auto& rg : resolved) lineWidth += rg.width;
        perLine.push_back({std::move(resolved), lineWidth});
    }

    // blockWidth = max per-line width; used by hAnchor and textAlignment
    float blockWidth = 0.0f;
    for(auto& ld : perLine) blockWidth = std::max(blockWidth, ld.lineWidth);

    // Block-level horizontal origin (hAnchor uses blockWidth)
    float originX_block;
    switch(hAnchor) {
        case 0:  originX_block = posX_px; break;                                             // Left
        case 2:  originX_block = posX_px - blockWidth * ctx.capHeightPx; break;             // Right
        default: originX_block = posX_px - blockWidth * ctx.capHeightPx * 0.5f; break;      // Center
    }

    // First baseline Y (vAnchor operates on the full block height)
    float lsf = (float)lineSpacing;
    float firstBaselineY;
    switch(vAnchor) {
        case 0:  firstBaselineY = posY_px - ctx.capHeightPx; break;                                              // Top
        case 2:  firstBaselineY = posY_px + (numLines - 1) * lsf * ctx.capHeightPx; break;                     // Bottom
        default: firstBaselineY = posY_px - ctx.capHeightPx * 0.5f                                              // Middle
                                  + (numLines - 1) * lsf * ctx.capHeightPx * 0.5f; break;
    }

    // Build animation sequence with per-glyph origin (varies by line)
    struct GlyphSeq { const Capture* capture; float penX; float seqStart; float originX; float baselineY; };
    std::vector<GlyphSeq> sequence;
    float seqCursor = 0.0f;

    for(int li = 0; li < numLines; ++li) {
        float lineBaselineY = firstBaselineY - li * lsf * ctx.capHeightPx;
        float lineWidth = perLine[li].lineWidth;

        float lineOriginX;
        switch(textAlignment) {
            case 1:  lineOriginX = originX_block + (blockWidth - lineWidth) * ctx.capHeightPx * 0.5f; break; // Center
            case 2:  lineOriginX = originX_block + (blockWidth - lineWidth) * ctx.capHeightPx; break;        // Right
            default: lineOriginX = originX_block; break;                                                      // Left
        }

        float penX = 0.0f;
        for(auto& rg : perLine[li].glyphs) {
            float dur = rg.capture ? rg.capture->duration : 0.0f;
            sequence.push_back({rg.capture, penX, seqCursor, lineOriginX, lineBaselineY});
            penX      += rg.width;
            seqCursor += dur;
        }
    }

    // Outline pass: large hard disc drawn first
    if(outlineEnabled) {
        for(auto& gs : sequence) {
            if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
            float tLocal = (float)(draw_time_ms - gs.seqStart);
            if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
            splatStrokes(ctx, gs.capture, tLocal, gs.penX, (float)outlineThickness, true, outlineColor,
                         gs.originX, gs.baselineY);
        }
    }

    // Fill pass: smaller hard disc composited on top
    for(auto& gs : sequence) {
        if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
        float tLocal = (float)(draw_time_ms - gs.seqStart);
        if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
        splatStrokes(ctx, gs.capture, tLocal, gs.penX, 0.0f, true, fillColor,
                     gs.originX, gs.baselineY);
    }
}
