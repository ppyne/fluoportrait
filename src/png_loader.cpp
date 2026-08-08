#include "fluoportrait/png_loader.hpp"
#include <png.h>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace fluoportrait {
LoadedPNG loadPNG(const std::filesystem::path& path) {
  FILE* file = std::fopen(path.string().c_str(), "rb");
  if (!file) throw std::runtime_error("cannot open PNG: " + path.string());
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info = png ? png_create_info_struct(png) : nullptr;
  if (!png || !info) { if (png) png_destroy_read_struct(&png, nullptr, nullptr); std::fclose(file); throw std::runtime_error("libpng allocation failed"); }
  if (setjmp(png_jmpbuf(png))) { png_destroy_read_struct(&png, &info, nullptr); std::fclose(file); throw std::runtime_error("invalid or unsupported PNG"); }
  png_init_io(png, file); png_read_info(png, info);
  const auto w = png_get_image_width(png, info), h = png_get_image_height(png, info);
  int depth = png_get_bit_depth(png, info), type = png_get_color_type(png, info);
  LoadedPNG result;
  png_charp profileName=nullptr; int compression=0; png_bytep profile=nullptr; png_uint_32 profileLen=0;
  if (png_get_iCCP(png, info, &profileName, &compression, &profile, &profileLen)) {
    result.sourceColorDescription = profileName ? profileName : "embedded ICC profile";
    std::string n = result.sourceColorDescription;
    for (auto& c : n) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    result.unsupportedICC = n.find("srgb") == std::string::npos;
  } else if (png_get_valid(png, info, PNG_INFO_sRGB)) result.sourceColorDescription = "sRGB chunk";
  else result.sourceColorDescription = "assumed sRGB";
  if (depth == 16) png_set_strip_16(png);
  if (type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
  if (type == PNG_COLOR_TYPE_GRAY && depth < 8) png_set_expand_gray_1_2_4_to_8(png);
  if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
  if (type == PNG_COLOR_TYPE_GRAY || type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
  if (!(type & PNG_COLOR_MASK_ALPHA) && !png_get_valid(png, info, PNG_INFO_tRNS)) png_set_add_alpha(png, 255, PNG_FILLER_AFTER);
  png_read_update_info(png, info);
  std::vector<png_byte> bytes(static_cast<std::size_t>(png_get_rowbytes(png, info))*h);
  std::vector<png_bytep> rows(h); for (png_uint_32 y=0; y<h; ++y) rows[y]=bytes.data()+y*png_get_rowbytes(png, info);
  png_read_image(png, rows.data()); png_read_end(png, nullptr);
  png_destroy_read_struct(&png, &info, nullptr); std::fclose(file);
  result.image = {w, h, {}}; result.image.pixels.resize(static_cast<std::size_t>(w)*h);
  for (std::size_t i=0; i<result.image.pixels.size(); ++i)
    result.image.pixels[i] = {bytes[4*i], bytes[4*i+1], bytes[4*i+2], bytes[4*i+3]};
  return result;
}
}
