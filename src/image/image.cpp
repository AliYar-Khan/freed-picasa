#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_ONLY_HDR
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include "image.h"
#include "exif_parse.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace pic {

bool load_rgba8(const std::string& path, Image& out, int* exif_orientation, std::string* err) {
  int w = 0, h = 0, ch = 0;
  stbi_uc* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
  if (!data) {
    if (err) *err = stbi_failure_reason() ? stbi_failure_reason() : "unknown";
    return false;
  }
  if (exif_orientation) *exif_orientation = exif_orientation_of(path);
  out.alloc(w, h);
  size_t sz = size_t(w) * h * 4;
  out.px.assign(data, data + sz);
  stbi_image_free(data);
  return true;
}

bool load_scaled_rgba8(const std::string& path, Image& out, int max_side, int* exif_orientation) {
  int w = 0, h = 0, ch = 0;
  if (!stbi_info(path.c_str(), &w, &h, &ch)) return false;
  if (exif_orientation) *exif_orientation = exif_orientation_of(path);
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return false;
  stbi_uc* data = stbi_load_from_file(f, &w, &h, &ch, 4);
  fclose(f);
  if (!data) return false;
  Image full;
  full.w = w;
  full.h = h;
  full.stride = w * 4;
  size_t sz = size_t(w) * h * 4;
  full.px.assign(data, data + sz);
  stbi_image_free(data);
  int maxs = std::max(w, h);
  int tw = w, th = h;
  if (maxs > max_side && maxs > 0) {
    float k = float(max_side) / float(maxs);
    tw = std::max(1, int(w * k));
    th = std::max(1, int(h * k));
  }
  if (tw == w && th == h) {
    out = full;
    return true;
  }
  out.alloc(tw, th);
  stbir_resize_uint8_linear(full.row(0), full.w, full.h, full.stride, out.row(0), tw, th, out.stride,
                            STBIR_RGBA);
  return true;
}

void apply_exif_orientation(Image& im, int o) {
  switch (o) {
    case 2:
      hmirror(im);
      break;
    case 3:
      rot180(im);
      break;
    case 4:
      vmirror(im);
      break;
    case 5:
      hmirror(im);
      rotate90(im, false);
      break;
    case 6:
      rotate90(im, false);
      break;
    case 7:
      hmirror(im);
      rotate90(im, true);
      break;
    case 8:
      rotate90(im, true);
      break;
    default:
      break;
  }
}

bool save_jpeg(const std::string& path, const Image& im, int quality, std::string* err) {
  if (!im.ok()) {
    if (err) *err = "bad image";
    return false;
  }
  if (stbi_write_jpg(path.c_str(), im.w, im.h, 4, im.row(0), quality)) return true;
  if (err) *err = "write failed";
  return false;
}

void rotate90(Image& im, bool ccw) {
  // CW: dst(x,y) = dst[y][x]?? formal: pixel (x,y) lands at (im.h-1-y, x) for CW; for CCW at (y, im.w-1-x)
  Image dst(im.h, im.w);
  for (int y = 0; y < im.h; y++) {
    const uint8_t* s = im.row(y);
    for (int x = 0; x < im.w; x++) {
      uint8_t* dp;
      if (ccw)
        dp = dst.row(im.w - 1 - x) + size_t(y) * 4;
      else
        dp = dst.row(x) + size_t(im.h - 1 - y) * 4;
      dp[0] = s[x * 4 + 0];
      dp[1] = s[x * 4 + 1];
      dp[2] = s[x * 4 + 2];
      dp[3] = s[x * 4 + 3];
    }
  }
  im = dst;
}

void rot180(Image& im) {
  Image dst(im.w, im.h);
  for (int y = 0; y < im.h; y++) {
    const uint8_t* s = im.row(im.h - 1 - y);
    for (int x = 0; x < im.w; x++) {
      uint8_t* d = dst.row(y) + size_t(im.w - 1 - x) * 4;
      const uint8_t* p = s + size_t(x) * 4;
      for (int c = 0; c < 4; c++) d[c] = p[c];
    }
  }
  im = dst;
}

void hmirror(Image& im) {
  for (int y = 0; y < im.h; y++) {
    uint8_t* row = im.row(y);
    for (int x = 0; x < im.w / 2; x++) {
      uint8_t* a = row + size_t(x) * 4;
      uint8_t* b = row + size_t(im.w - 1 - x) * 4;
      for (int c = 0; c < 4; c++) std::swap(a[c], b[c]);
    }
  }
}

void vmirror(Image& im) {
  for (int y = 0; y < im.h / 2; y++) {
    uint8_t* a = im.row(y);
    uint8_t* b = im.row(im.h - 1 - y);
    for (size_t i = 0; i < size_t(im.w) * 4; i++) std::swap(a[i], b[i]);
  }
}

void resize_bilinear(const Image& src, Image& dst) {
  if (dst.w <= 0 || dst.h <= 0) return;
  if (dst.w == src.w && dst.h == src.h) {
    dst.px = src.px;
    return;
  }
  stbir_resize_uint8_linear(src.row(0), src.w, src.h, src.stride, dst.row(0), dst.w, dst.h,
                            dst.stride, STBIR_RGBA);
}

void resize_box(const Image& src, Image& dst) {
  if (dst.w <= 0 || dst.h <= 0) return;
  static uint8_t clut[4];
  for (int dy = 0; dy < dst.h; dy++) {
    int sy0 = dy * src.h / dst.h;
    int sy1 = std::max(sy0 + 1, (dy + 1) * src.h / dst.h);
    for (int dx = 0; dx < dst.w; dx++) {
      int sx0 = dx * src.w / dst.w;
      int sx1 = std::max(sx0 + 1, (dx + 1) * src.w / dst.w);
      for (int c = 0; c < 3; c++) {
        unsigned acc = 0, cnt = 0;
        for (int y = sy0; y < sy1; y++)
          for (int x = sx0; x < sx1; x++, cnt++) acc += src.row(y)[size_t(x) * 4 + c];
        dst.row(dy)[size_t(dx) * 4 + c] = uint8_t(acc / (cnt ? cnt : 1));
      }
      dst.row(dy)[size_t(dx) * 4 + 3] = 255;
    }
  }
}

void crop(Image& im, int x, int y, int w, int h) {
  x = std::clamp(x, 0, std::max(0, im.w - w));
  y = std::clamp(y, 0, std::max(0, im.h - h));
  Image dst(std::max(1, w), std::max(1, h));
  for (int dy = 0; dy < dst.h; dy++) {
    int sy = std::clamp(y + dy, 0, im.h - 1);
    const uint8_t* s = im.row(sy);
    for (int dx = 0; dx < dst.w; dx++) {
      int sx = std::clamp(x + dx, 0, im.w - 1);
      uint8_t* d = dst.row(dy) + size_t(dx) * 4;
      const uint8_t* p = s + size_t(sx) * 4;
      for (int c = 0; c < 4; c++) d[c] = p[c];
    }
  }
  im = dst;
}

void compute_histogram(const Image& im, unsigned long hist[3][256]) {
  for (int c = 0; c < 3; c++)
    for (int i = 0; i < 256; i++) hist[c][i] = 0;
  for (int y = 0; y < im.h; y++) {
    const uint8_t* row = im.row(y);
    for (int x = 0; x < im.w; x++) {
      const uint8_t* p = row + size_t(x) * 4;
      hist[0][p[0]]++;
      hist[1][p[1]]++;
      hist[2][p[2]]++;
    }
  }
}

void compute_histogram_fast(const Image& im, unsigned long hist[3][256]) {
  for (int c = 0; c < 3; c++)
    for (int i = 0; i < 256; i++) hist[c][i] = 0;
  int step = std::max(1, int(std::sqrt(double(im.w) * im.h / 30000.0)));
  for (int y = 0; y < im.h; y += step) {
    const uint8_t* row = im.row(y);
    for (int x = 0; x < im.w; x += step) {
      const uint8_t* p = row + size_t(x) * 4;
      hist[0][p[0]]++;
      hist[1][p[1]]++;
      hist[2][p[2]]++;
    }
  }
}

}  // namespace pic
