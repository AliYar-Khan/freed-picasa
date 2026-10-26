#pragma once
// Open Picasa - basic image types shared by decode/edit/UI.
#include <cstdint>
#include <string>
#include <vector>

namespace pic {

// Straight-alpha RGBA8 row-major image in host memory.
struct Image {
  int w = 0, h = 0;
  int stride = 0;  // bytes per row (>= w*4)
  std::vector<uint8_t> px;

  Image() = default;
  Image(int width, int height) { alloc(width, height); }
  void alloc(int width, int height) {
    w = width;
    h = height;
    stride = width * 4;
    px.assign(size_t(width) * height * 4, 0);
  }
  uint8_t* row(int y) { return px.data() + size_t(y) * stride; }
  const uint8_t* row(int y) const { return px.data() + size_t(y) * stride; }
  bool ok() const { return w > 0 && h > 0 && stride == w * 4; }
};

// Returns false on failure. rgba8 out. exif_orientation (1..8) if found.
bool load_rgba8(const std::string& path, Image& out, int* exif_orientation = nullptr,
                std::string* err = nullptr);

// Loads an image and scales so max side <= max_side (thumbnail fast-path).
bool load_scaled_rgba8(const std::string& path, Image& out, int max_side,
                       int* exif_orientation = nullptr);

// Exif orientation transform into pixels (bakes orientation into buffer).
void apply_exif_orientation(Image& im, int orientation);

// Save as JPEG (quality 1..100). Writes RGB from RGBA color.
bool save_jpeg(const std::string& path, const Image& im, int quality, std::string* err = nullptr);

// Bitmap ops (implemented in image.cpp)
void rotate90(Image& im, bool ccw);
void rot180(Image& im);
void hmirror(Image& im);
void vmirror(Image& im);
void resize_box(const Image& src, Image& dst);  // dst.w/h set beforehand
void resize_bilinear(const Image& src, Image& dst);
void crop(Image& im, int x, int y, int w, int h);
void gamma_gray_gray(const Image& src, Image& dst, float gamma, float contrast_bias,
                     float brightness_gain);
void compute_histogram_fast(const Image& im, unsigned long hist[3][256]);

}  // namespace pic
