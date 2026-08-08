#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fluoportrait {
struct ICCProfile { std::vector<std::uint8_t> bytes; std::string description; };
ICCProfile loadICCProfile(const std::filesystem::path& path);
}

