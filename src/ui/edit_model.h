#pragma once
// Editing session model: one photo's EditState + preview rendering.
#include "edit/ops.h"
#include "image/image.h"
#include <string>
#include <vector>

namespace pic {

enum class Tool { None, Rotate, Crop, Tuning, RedEye, Retouch, Autofix };

struct EditSession {
  std::string path;
  bool active = false;
  Image base;         // EXIF-oriented full-size decode (or scaled base for huge imgs)
  EditState state;    // committed edit ops (not inc. in-progress tool drag)
  Image preview;      // rendered preview at view size
  int view_w = 0, view_h = 0;
  bool preview_dirty = true;

  Tool tool = Tool::None;
  // crop-session data (view-space rect floats 0..1 of current preview)
  bool cropping = false;
  float crop_x0 = 0, crop_y0 = 0, crop_x1 = 1, crop_y1 = 1;
  // retouch pending pair
  bool retouch_have_src = false;
  float retouch_sx = 0, retouch_sy = 0;
  float retouch_radius = 0.05f;
  // redeye
  float redeye_r = 0.02f;
  // tuning (live, committed into state on release)
  Tuning live_tuning;

  bool open(const std::string& path, void (*progress)() = nullptr);
  void render_preview(int w, int h);
  float aspect_ratio() const { return preview.h ? float(preview.w) / float(preview.h) : 1.f; }
  // Commit current tool state permanently (called when leaving tool / editing)
  void commit_tool();
};

}  // namespace pic
