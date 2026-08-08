#include "fluoportrait/color_science.hpp"
#include <algorithm>
#include <cmath>

namespace fluoportrait::color {
namespace {
constexpr double m1 = 2610.0 / 16384.0;
constexpr double m2 = 2523.0 / 32.0;
constexpr double c1 = 3424.0 / 4096.0;
constexpr double c2 = 2413.0 / 128.0;
constexpr double c3 = 2392.0 / 128.0;
}

double srgbToLinear(double c) {
  c = std::clamp(c, 0.0, 1.0);
  return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}
RGBf srgbToLinear(const RGBf& c) {
  return {srgbToLinear(c.r), srgbToLinear(c.g), srgbToLinear(c.b)};
}
RGBf linearSRGBToRec2020(const RGBf& c) {
  return {
    std::max(0.0, 0.6275036748*c.r + 0.3292754500*c.g + 0.0433026772*c.b),
    std::max(0.0, 0.0691083636*c.r + 0.9195191578*c.g + 0.0113595344*c.b),
    std::max(0.0, 0.0163940692*c.r + 0.0880112707*c.g + 0.8953803700*c.b)};
}
double pqEncode(double nits) {
  const double l = std::clamp(nits, 0.0, 10000.0) / 10000.0;
  const double p = std::pow(l, m1);
  return std::pow((c1 + c2*p) / (1.0 + c3*p), m2);
}
double pqDecode(double code) {
  const double p = std::pow(std::clamp(code, 0.0, 1.0), 1.0/m2);
  const double numerator = std::max(p-c1, 0.0);
  const double denominator = c2-c3*p;
  if (denominator <= 0.0) return 10000.0;
  return 10000.0 * std::pow(numerator/denominator, 1.0/m1);
}
RGBf portraitToNits(const RGBf& srgb, double white) {
  const auto c = linearSRGBToRec2020(srgbToLinear(srgb));
  return {c.r*white, c.g*white, c.b*white};
}
RGBf portraitToPQ(const RGBf& srgb, double white) {
  const auto c = portraitToNits(srgb, white);
  return {pqEncode(c.r), pqEncode(c.g), pqEncode(c.b)};
}
double rec2020Luminance(const RGBf& c) {
  return 0.2627*c.r + 0.6780*c.g + 0.0593*c.b;
}
double directPQLuminance(const RGB8& c) {
  return 0.2627*pqDecode(c.r/255.0) + 0.6780*pqDecode(c.g/255.0) + 0.0593*pqDecode(c.b/255.0);
}
std::uint8_t quantizePQ(double code) {
  return static_cast<std::uint8_t>(std::lround(std::clamp(code, 0.0, 1.0)*255.0));
}
}

