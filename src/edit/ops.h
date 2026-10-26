#pragma once
// Non-destructive edit state, Picasa-style. One active edit model per photo.
#include <cstdint>
#include <string>
#include <vector>

namespace pic {

struct Tuning {
  float brightness = 0;    // -1..1
  float contrast = 0;      // -1..1
  float colorTemp = 0;     // -100..100 (cool..warm)
  float saturation = 0;    // -1..1
  float fillLight = 0;     // 0..100
  float highlights = 0;    // -100..100
  float shadows = 0;       // -100..100
  bool zero() const;
};

struct RedEyeSpot {
  float x = 0, y = 0;   // relative 0..1 within displayed (uncropped) image
  float r = 0;          // relative radius
};

struct RetouchStamp {
  float sx = 0, sy = 0;  // source (clone from)
  float tx = 0, ty = 0;  // target
  float r = 0;           // relative radius
};

struct EditState {
  int rotation = 0;                 // 0..3 quarter-turns clockwise
  bool has_crop = false;            // crop rect in rotated pixel space
  float crop_x = 0, crop_y = 0, crop_w = 0, crop_h = 0;  // relative 0..1 of rotated base
  Tuning tuning;
  std::vector<RedEyeSpot> redeye;
  std::vector<RetouchStamp> retouch;
  bool auto_fix = false;            // "I'm Feeling Lucky"

  bool dirty() const {
    return rotation != 0 || has_crop || !tuning.zero() || !redeye.empty() || !retouch.empty() ||
           auto_fix;
  }
};

// Adjusts state in place.
void edit_add_rotation(EditState& s, int quarter_turns_cw);  // sum modulo 4
void edit_set_crop(EditState& s, bool has, float x, float y, float w, float h);
void edit_add_redeye(EditState& s, float x, float y, float r);
void edit_add_retouch(EditState& s, float sx, float sy, float tx, float ty, float r);

// Renders final RGBA image for a base (already EXIF-orientation-corrected) image.
void render_edited(const struct Image& base, const EditState& s, struct Image& out);

// Applies one tuning pass in-place at render res (call after crop).
void apply_tuning(struct Image& im, const Tuning& t);
void apply_auto_fix(struct Image& im);
void apply_redeye(struct Image& im, const std::vector<RedEyeSpot>& spots);
void apply_retouch(struct Image& im, const std::vector<RetouchStamp>& stamps);

}  // namespace pic
