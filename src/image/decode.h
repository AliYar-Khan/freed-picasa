#pragma once
#include "image.h"

namespace pic {
// Decodes a thumbnail-size image honoring EXIF rotation.
struct ThumbImage {
  Image im;
  int exif_orientation = 0;
};
bool decode_thumb(const std::string& path, int max_side, ThumbImage& out);
}
