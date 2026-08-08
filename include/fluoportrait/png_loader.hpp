#pragma once
#include "fluoportrait/types.hpp"
#include <filesystem>
#include <string>

namespace fluoportrait {
struct LoadedPNG { ImageRGBA8 image; std::string sourceColorDescription; bool unsupportedICC{}; };
LoadedPNG loadPNG(const std::filesystem::path& path);
}

