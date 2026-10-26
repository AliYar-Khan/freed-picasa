#include "decode.h"

namespace pic {

bool decode_thumb(const std::string& path, int max_side, ThumbImage& out) {
  int ori = 0;
  if (!load_scaled_rgba8(path, out.im, max_side, &ori)) return false;
  out.exif_orientation = ori;
  apply_exif_orientation(out.im, ori);
  return true;
}

}  // namespace pic
