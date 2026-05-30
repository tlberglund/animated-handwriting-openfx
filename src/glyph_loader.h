#pragma once
#include "glyph_model.h"
#include <string>

std::string getBundledGlyphSetPath();
GlyphSet    loadGlyphSet(const std::string& path);
