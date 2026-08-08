#pragma once
#include "fluoportrait/icc_profile.hpp"
#include "fluoportrait/types.hpp"
#include <filesystem>

namespace fluoportrait {
void writeJPEG(const std::filesystem::path&, const ImageRGB8&, const ICCProfile&, int quality=95);
struct JPEGInspection { unsigned width{}, height{}; int components{}; ICCProfile profile; };
JPEGInspection inspectJPEG(const std::filesystem::path&);
ImageRGB8 readJPEG(const std::filesystem::path&);
}

