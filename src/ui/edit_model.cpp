#include "edit_model.h"
#include <cmath>

namespace pic {

bool EditSession::open(const std::string& p, void (*progress)()) {
  path = p;
  active = false;
  state = EditState();
  tool = Tool::None;
  live_tuning = Tuning();
  cropping = false;
  retouch_have_src = false;
  Image raw;
  int ori = 0;
  // Downscale giant photos to 3000px working size (Picasa did similar).
  int max_side = 3000;
  if (!load_scaled_rgba8(path, raw, max_side, &ori)) return false;
  apply_exif_orientation(raw, ori);
  base = std::move(raw);
  preview = base;
  preview_dirty = true;
  active = true;
  if (progress) progress();
  return true;
}

void EditSession::render_preview(int w, int h) {
  if (!active) return;
  if (!preview_dirty && w == view_w && h == view_h) return;
  view_w = w;
  view_h = h;
  render_edited(base, state, preview);
  preview_dirty = false;
}

void EditSession::commit_tool() {
  switch (tool) {
    case Tool::Tuning:
      if (!live_tuning.zero()) {
        state.tuning = live_tuning;
        preview_dirty = true;
      }
      break;
    default:
      break;
  }
}

}  // namespace pic
