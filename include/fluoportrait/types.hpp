#pragma once
#include <cstdint>
#include <vector>

namespace fluoportrait {
struct RGBf { double r{}, g{}, b{}; };
struct RGBA8 { std::uint8_t r{}, g{}, b{}, a{}; };
struct RGB8 { std::uint8_t r{}, g{}, b{}; };

template<class Pixel> struct Image {
  unsigned width{};
  unsigned height{};
  std::vector<Pixel> pixels;
};
using ImageRGBA8 = Image<RGBA8>;
using ImageRGB8 = Image<RGB8>;
}

