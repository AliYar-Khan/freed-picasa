#include "ops.h"
#include "image/image.h"
#include <algorithm>
#include <string>
#include <cmath>
#include <cstring>

namespace pic {

bool Tuning::zero() const {
  return brightness == 0 && contrast == 0 && colorTemp == 0 && saturation == 0 && fillLight == 0 &&
         highlights == 0 && shadows == 0;
}

void edit_add_rotation(EditState& s, int turns) { s.rotation = (s.rotation + turns + 4) % 4; }

void edit_set_crop(EditState& s, bool has, float x, float y, float w, float h) {
  s.has_crop = has;
  if (has) {
    s.crop_x = x;
    s.crop_y = y;
    s.crop_w = w;
    s.crop_h = h;
  }
}

void edit_add_redeye(EditState& s, float x, float y, float r) {
  RedEyeSpot spot;
  spot.x = x;
  spot.y = y;
  spot.r = r;
  s.redeye.push_back(spot);
}

void edit_add_retouch(EditState& s, float sx, float sy, float tx, float ty, float r) {
  RetouchStamp st;
  st.sx = sx;
  st.sy = sy;
  st.tx = tx;
  st.ty = ty;
  st.r = r;
  s.retouch.push_back(st);
}

// ---------- tunings ----------

static inline int clamp255(int x) { return uint8_t(x <= 0 ? 0 : x >= 255 ? 255 : x); }

static void build_tone_luts(const Tuning& t, uint8_t lut[3][256]) {
  double b = double(t.brightness) * 0.5;  // -0.5..0.5 exposure-ish
  double c = double(t.contrast);
  double fl = double(t.fillLight) / 100.0;
  double hi = double(t.highlights) / 100.0;
  double sh = double(t.shadows) / 100.0;

  for (int i = 0; i < 256; i++) {
    double x = double(i) / 255.0;
    double y = x;
    // fill light: lift shadows (soft)
    if (fl > 0) y += fl * 0.5 * pow(1.0 - x, 2.0);
    // shadows: restore lifted blacks
    y += sh * 0.35 * pow(1.0 - x, 3.0);
    // highlights: + blow out softly, - protect
    if (hi > 0) y += hi * 0.35 * pow(x, 2.2);
    else if (hi < 0) y *= (1.0 + hi * 0.55 * pow(x, 1.4));
    // brightness: midtone exposure
    y += b;
    // contrast: expand/compress around mid
    if (c != 0) {
      double k = c > 0 ? 1.0 + c * 1.6 : 1.0 + c * 0.9;
      y = 0.5 + (y - 0.5) * k;
    }
    if (y < 0) y = 0;
    if (y > 1) y = 1;
    int v = int(y * 255.0 + 0.5);
    for (int ch = 0; ch < 3; ch++) lut[ch][i] = clamp255(v);
  }
}

void apply_tuning(Image& im, const Tuning& t) {
  if (!im.ok() && false) return;
  uint8_t lut[3][256];
  build_tone_luts(t, lut);
  // colorTemp: warm -> +R -B; cool -> -R +B (scaled small)
  double ct = double(t.colorTemp) / 100.0;  // -1..1
  // saturation 0..: -1 grayscale, +1 boost linearly beyond identity at +0.6-ish
  double sat = double(std::clamp(t.saturation * 1.0f, -1.0f, 1.0f));
  for (int y = 0; y < im.h; y++) {
    uint8_t* row = im.row(y);
    for (int x = 0; x < im.w; x++) {
      uint8_t* p = row + size_t(x) * 4;
      int r = int(p[0]), g = int(p[1]), b = int(p[2]);
      // temp
      r = clamp255(int(double(r) * (1.0 + 0.10 * ct)));
      b = clamp255(int(double(b) * (1.0 + 0.10 * -ct)));
      // lut (same for all channels adjusted after temp)
      r = lut[0][r];
      g = lut[1][g];
      b = lut[2][b];
      // saturation via luma mix
      if (sat != 0) {
        int l = (r * 299 + g * 587 + b * 114) / 1000;
        if (sat < 0) {
          r = r + int(double(r - l) * sat);
          g = g + int(double(g - l) * sat);
          b = b + int(double(b - l) * sat);
        } else {
          double k = 1.0 + sat * 0.8;
          r = clamp255(int(double(l) + (double(r) - l) * k));
          g = clamp255(int(double(l) + (double(g) - l) * k));
          b = clamp255(int(double(l) + (double(b) - l) * k));
        }
      }
      p[0] = uint8_t(r);
      p[1] = uint8_t(g);
      p[2] = uint8_t(b);
    }
  }
}

void apply_auto_fix(Image& im) {
  if (!im.ok()) return;
  unsigned long hist[3][256];
  compute_histogram_fast(im, hist);
  uint8_t lut[3][256];
  double total = 0;
  for (int c = 0; c < 3; c++)
    for (int i = 0; i < 256; i++) total += double(hist[c][i]);
  for (int c = 0; c < 3; c++) {
    double acc = 0;
    int lo = 0, hi_ = 255;
    for (int i = 0; i < 256; i++) {
      acc += double(hist[c][i]);
      if (acc > total * 0.0045) {
        lo = i;
        break;
      }
    }
    acc = 0;
    for (int i = 255; i >= 0; i--) {
      acc += double(hist[c][i]);
      if (acc > total * 0.0045) {
        hi_ = i;
        break;
      }
    }
    if (hi_ <= lo) {
      lo = 0, hi_ = 255;
    }
    for (int i = 0; i < 256; i++) {
      double y = (double(i) - double(lo)) / double(hi_ - lo) * 255.0;
      lut[c][i] = clamp255(int(y + 0.5));
    }
  }
  for (int y = 0; y < im.h; y++) {
    uint8_t* row = im.row(y);
    for (int x = 0; x < im.w; x++) {
      uint8_t* p = row + size_t(x) * 4;
      p[0] = lut[0][p[0]];
      p[1] = lut[1][p[1]];
      p[2] = lut[2][p[2]];
    }
  }
}

void apply_redeye(Image& im, const std::vector<RedEyeSpot>& spots) {
  if (!im.ok()) return;
  for (const RedEyeSpot& s : spots) {
    int cx = int(s.x * im.w), cy = int(s.y * im.h);
    int rad = int(std::max(2, int(0.5 + s.r * std::max(im.w, im.h))));
    for (int y = std::max(0, cy - rad); y <= std::min(im.h - 1, cy + rad); y++) {
      uint8_t* row = im.row(y);
      for (int x = std::max(0, cx - rad); x <= std::min(im.w - 1, cx + rad); x++) {
        double d = std::hypot(double(x - cx), double(y - cy)) / double(rad);
        if (d > 1.0) continue;
        double fall = d < 0.7 ? 1.0 : 1.0 - (d - 0.7) / 0.3;
        uint8_t* p = row + size_t(x) * 4;
        int r = int(p[0]), g = int(p[1]), b = int(p[2]);
        int lim = (g + b) / 2 + 6;
        int nr = r < lim ? r : lim;
        p[0] = uint8_t(int(double(r) * (1.0 - 0.85 * fall) + double(nr) * (0.85 * fall)));
      }
    }
  }
}

// simple feathered clone stamp
void apply_retouch(Image& im, const std::vector<RetouchStamp>& stamps) {
  if (!im.ok()) return;
  Image base = im;  // clone from base
  for (const RetouchStamp& st : stamps) {
    int r = int(std::max(2, int(0.5 + st.r * std::max(im.w, im.h))));
    int cx = int(st.tx * im.w), cy = int(st.ty * im.h);
    int shx = int(st.sx * im.w) - cx, shy = int(st.sy * im.h) - cy;
    for (int y = std::max(0, cy - r); y <= std::min(im.h - 1, cy + r); y++) {
      for (int x = std::max(0, cx - r); x <= std::min(im.w - 1, cx + r); x++) {
        double d = std::hypot(double(x - cx), double(y - cy)) / double(r);
        if (d > 1.0) continue;
        double a = 1.0 - d;  // feather
        int sxx = std::clamp(x + shx, 0, base.w - 1);
        int syy = std::clamp(y + shy, 0, base.h - 1);
        const uint8_t* sp = base.row(syy) + size_t(sxx) * 4;
        uint8_t* dp = im.row(y) + size_t(x) * 4;
        for (int ch = 0; ch < 3; ch++)
          dp[ch] = uint8_t(int(double(dp[ch]) * (1.0 - a) + double(sp[ch]) * a + 0.5));
      }
    }
  }
}

void render_edited(const Image& base, const EditState& s, Image& out) {
  out = base;
  if (s.rotation)
    for (int i = 0; i < s.rotation; i++) rotate90(out, false);  // 90 CW each
  if (s.has_crop)
    crop(out, int(s.crop_x * out.w), int(s.crop_y * out.h), std::max(1, int(s.crop_w * out.w + 0.5f)),
         std::max(1, int(s.crop_h * out.h + 0.5f)));
  if (!s.retouch.empty()) apply_retouch(out, s.retouch);
  if (s.auto_fix) apply_auto_fix(out);
  if (!s.tuning.zero()) apply_tuning(out, s.tuning);
  if (!s.redeye.empty()) apply_redeye(out, s.redeye);
}

}  // namespace pic
