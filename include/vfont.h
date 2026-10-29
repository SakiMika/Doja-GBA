#pragma once
#include <cstdint>
constexpr int VFONT_COUNT=238;
constexpr int VFONT_LINE_STEP=17;
extern const uint16_t vfont_codepoints[VFONT_COUNT];
extern const uint8_t vfont_widths[VFONT_COUNT];
extern const uint8_t vfont_rows[VFONT_COUNT][14];
