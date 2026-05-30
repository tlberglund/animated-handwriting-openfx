#pragma once
#include <map>
#include <string>
#include <vector>

struct GlyphPoint { float x, y, t, p; };
using Stroke = std::vector<GlyphPoint>;

struct Capture {
    std::string id;
    float width;
    std::vector<Stroke> strokes;
    float duration;
};

struct Glyph {
    std::string character;
    std::vector<Capture> captures;
};

struct GlyphSet {
    std::string name;
    std::map<std::string, Glyph> glyphs;
    std::vector<std::string> ligatureKeys;
    float pMax;
};

struct InstanceData {
    GlyphSet glyphSet;
    std::string loadedPath;
    std::vector<int> captureIndices;
    std::string lastText;
};
