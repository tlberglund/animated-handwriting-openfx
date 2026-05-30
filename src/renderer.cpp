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
                          float sigmaExtra, bool hardEdge, const double color[4])
{
    float pax = ax * ctx.capHeightPx,  pay = (1.0f - ay) * ctx.capHeightPx;
    float pbx = bx * ctx.capHeightPx,  pby = (1.0f - by) * ctx.capHeightPx;

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
                          float sigmaExtra, bool hardEdge, const double color[4])
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
                         sigmaExtra, hardEdge, color);
        }

        if(hasPartial) {
            splatSegment(ctx,
                         glyphPenX + stroke[lastVisible].x, stroke[lastVisible].y, stroke[lastVisible].p,
                         glyphPenX + endX, endY, endP,
                         sigmaExtra, hardEdge, color);
        } else if(lastVisible == 0) {
            float ax = glyphPenX + stroke[0].x;
            splatSegment(ctx, ax, stroke[0].y, stroke[0].p, ax, stroke[0].y, stroke[0].p,
                         sigmaExtra, hardEdge, color);
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
                       int outlineEnabled)
{
    // Resolve glyphs with ligature substitution
    struct ResolvedGlyph { const Capture* capture; float width; };
    std::vector<ResolvedGlyph> resolved;
    size_t pos = 0;
    int selIdx = 0;

    while(pos < text.size()) {
        if(text[pos] == ' ') {
            resolved.push_back({nullptr, 0.35f});
            ++pos; continue;
        }
        bool found = false;
        for(auto& key : glyphSet.ligatureKeys) {
            if(pos + key.size() <= text.size() &&
               text.substr(pos, key.size()) == key) {
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
            auto it = glyphSet.glyphs.find(text.substr(pos, 1));
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

    // Build animation sequence: each glyph gets a pen-x position and a start time in ms
    struct GlyphSeq { const Capture* capture; float penX; float seqStart; };
    std::vector<GlyphSeq> sequence;
    float penX = 0.0f;
    float seqCursor = 0.0f;
    for(auto& rg : resolved) {
        float dur = rg.capture ? rg.capture->duration : 0.0f;
        sequence.push_back({rg.capture, penX, seqCursor});
        penX      += rg.width;
        seqCursor += dur;
    }

    // Outline pass: large hard disc drawn first
    if(outlineEnabled) {
        for(auto& gs : sequence) {
            if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
            float tLocal = (float)(draw_time_ms - gs.seqStart);
            if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
            splatStrokes(ctx, gs.capture, tLocal, gs.penX, (float)outlineThickness, true, outlineColor);
        }
    }

    // Fill pass: smaller hard disc composited on top, geometrically covering the outline interior
    for(auto& gs : sequence) {
        if(!gs.capture || draw_time_ms <= (double)gs.seqStart) continue;
        float tLocal = (float)(draw_time_ms - gs.seqStart);
        if(tLocal > gs.capture->duration) tLocal = gs.capture->duration;
        splatStrokes(ctx, gs.capture, tLocal, gs.penX, 0.0f, true, fillColor);
    }
}
