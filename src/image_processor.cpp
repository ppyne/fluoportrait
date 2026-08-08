#include "fluoportrait/image_processor.hpp"
#include "fluoportrait/color_science.hpp"
#include <stdexcept>

namespace fluoportrait {
ImageRGB8 composePortrait(const ImageRGBA8& source, const ProcessOptions& o) {
  if (source.pixels.size() != static_cast<std::size_t>(source.width)*source.height)
    throw std::invalid_argument("invalid source image dimensions");
  if (!(o.portraitSdrWhiteNits > 0.0 && o.portraitSdrWhiteNits <= 10000.0))
    throw std::invalid_argument("portrait SDR white must be in (0, 10000] nits");
  ImageRGB8 out{source.width, source.height, {}};
  out.pixels.reserve(source.pixels.size());
  const RGBf bgNits{color::pqDecode(o.backgroundPQ.r/255.0),
                    color::pqDecode(o.backgroundPQ.g/255.0),
                    color::pqDecode(o.backgroundPQ.b/255.0)};
  for (const auto p : source.pixels) {
    // Preserve direct PQ input codes bit-for-bit before JPEG compression.
    if (p.a == 0) { out.pixels.push_back(o.backgroundPQ); continue; }
    const RGBf srgb{p.r/255.0, p.g/255.0, p.b/255.0};
    const auto fg = color::portraitToNits(srgb, o.portraitSdrWhiteNits);
    if (p.a == 255) {
      out.pixels.push_back({color::quantizePQ(color::pqEncode(fg.r)),
                            color::quantizePQ(color::pqEncode(fg.g)),
                            color::quantizePQ(color::pqEncode(fg.b))});
      continue;
    }
    // Alpha is coverage. Blend compatible absolute-light Rec.2020 channels,
    // never nonlinear sRGB values with nonlinear PQ values.
    const double a = p.a/255.0;
    const RGBf nits{fg.r*a + bgNits.r*(1-a), fg.g*a + bgNits.g*(1-a),
                    fg.b*a + bgNits.b*(1-a)};
    out.pixels.push_back({color::quantizePQ(color::pqEncode(nits.r)),
                          color::quantizePQ(color::pqEncode(nits.g)),
                          color::quantizePQ(color::pqEncode(nits.b))});
  }
  return out;
}
}

