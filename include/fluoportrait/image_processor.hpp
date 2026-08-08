#pragma once
#include "fluoportrait/types.hpp"

namespace fluoportrait {
struct ProcessOptions {
  double portraitSdrWhiteNits = 75.0;
  RGB8 backgroundPQ{196, 202, 156};
};
ImageRGB8 composePortrait(const ImageRGBA8& source, const ProcessOptions& options);
}

