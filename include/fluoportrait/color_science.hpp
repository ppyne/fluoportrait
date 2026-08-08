#pragma once
#include "fluoportrait/types.hpp"

namespace fluoportrait::color {
double srgbToLinear(double code);
RGBf srgbToLinear(const RGBf& code);
RGBf linearSRGBToRec2020(const RGBf& linear);
double pqEncode(double luminanceNits);
double pqDecode(double code);
RGBf portraitToPQ(const RGBf& srgb, double sdrWhiteNits);
RGBf portraitToNits(const RGBf& srgb, double sdrWhiteNits);
double rec2020Luminance(const RGBf& linearRec2020);
double directPQLuminance(const RGB8& pq);
std::uint8_t quantizePQ(double code);
}

