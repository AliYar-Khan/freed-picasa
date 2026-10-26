#include "imgui.h"
#include "viewer_panel.h"
#include "edit_model.h"
#include "image/image.h"

namespace pic {

void draw_edit_panel() {
  auto& es = app()->session;
  if (!es.active) return;
  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 300, 60), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Once);
  int frame_tab = 0;
  if (ImGui::Begin("Edit", nullptr, 0)) {
    if (ImGui::BeginTabBar("##editbar", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem("Basic Fixes")) {
        frame_tab = 1;
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Tuning")) {
        frame_tab = 2;
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Effects")) {
        frame_tab = 3;
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    if (frame_tab == 1) {
      if (ImGui::Button("Reset All Edits")) {
        es.state = EditState();
        es.preview_dirty = true;
      }
      if (ImGui::Button("Auto Contrast")) {
        es.state.auto_fix = !es.state.auto_fix;
        es.preview_dirty = true;
      }
      ImGui::Spacing();
      ImGui::TextDisabled("Rotate");
      if (ImGui::Button("Rotate CW")) {
        edit_add_rotation(es.state, 1);
        es.preview_dirty = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Rotate CCW")) {
        edit_add_rotation(es.state, 3);
        es.preview_dirty = true;
      }
      ImGui::Spacing();
      ImGui::TextDisabled("Crop");
      if (ImGui::Button("Crop...")) es.tool = es.tool == Tool::Crop ? Tool::None : Tool::Crop;
      if (es.tool == Tool::Crop)
        ImGui::TextDisabled("Drag a rectangle on the image, then Crop/Cancel.");
      if (es.tool == Tool::Crop) {
        if (ImGui::Button("Apply Crop")) {
          float x0 = es.crop_x0, y0 = es.crop_y0, x1 = es.crop_x1, y1 = es.crop_y1;
          // view-space is in current render (rotated) space, base-space of state:
          // crop coordinates relative to current preview (post previous rotation).
          edit_add_rotation(es.state, 0);  // no-op to keep intent clear
          float rx0 = std::min(x0, x1), rx1 = std::max(x0, x1);
          float ry0 = std::min(y0, y1), ry1 = std::max(y0, y1);
          // The crop must compose with existing crop: crop rect in current
          // preview space = fraction of already-rendered box.
          // We bake the new requested rect into existing value by intersecting:
          // For MVP, only first-level crops supported (no crop stacking).
          // Combine: if state.has_crop, intersect normalized rects.
          float cx = rx0, cy = ry0, cw = rx1 - rx0, ch = ry1 - ry0;
          if (es.state.has_crop) {
            float ox0 = es.state.crop_x, oy0 = es.state.crop_y;
            float ox1 = ox0 + es.state.crop_w, oy1 = oy0 + es.state.crop_h;
            float nx0 = std::max(ox0, cx), ny0 = std::max(oy0, cy);
            float nx1 = std::min(ox1, cx + cw), ny1 = std::min(oy1, cy + ch);
            cx = nx0;
            cy = ny0;
            cw = nx1 - nx0;
            ch = ny1 - ny0;
          }
          if (cw > 0.01f && ch > 0.01f) {
            edit_set_crop(es.state, true, cx, cy, cw, ch);
            es.preview_dirty = true;
          }
          es.tool = Tool::None;
          es.cropping = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          es.tool = Tool::None;
          es.cropping = false;
        }
      }
      ImGui::Spacing();
      ImGui::TextDisabled("Redeye");
      if (ImGui::Button("Redeye Tool"))
        es.tool = es.tool == Tool::RedEye ? Tool::None : Tool::RedEye;
      if (es.tool == Tool::RedEye) {
        ImGui::SliderFloat("Radius", &es.redeye_r, 0.005f, 0.08f, "%.3f");
        ImGui::TextDisabled("Click on the pupil to fix it.");
      }
      ImGui::Spacing();
      ImGui::TextDisabled("Retouch");
      if (ImGui::Button("Retouch Tool"))
        es.tool = es.tool == Tool::Retouch ? Tool::None : Tool::Retouch;
      if (es.tool == Tool::Retouch) {
        ImGui::SliderFloat("Brush", &es.retouch_radius, 0.01f, 0.15f, "%.3f");
        ImGui::TextDisabled("1) Click source  2) Click area to fix.");
      }
      ImGui::Spacing();
      if (ImGui::Button("Save Copy As...")) ImGui::OpenPopup("##export");
    }
    if (frame_tab == 2) {
      Tuning& t = es.live_tuning;
      bool changed = false;
      changed |= ImGui::SliderFloat("Brightness", &t.brightness, -1.f, 1.f);
      changed |= ImGui::SliderFloat("Contrast", &t.contrast, -1.f, 1.f);
      changed |= ImGui::SliderFloat("Color temperature", &t.colorTemp, -100.f, 100.f);
      changed |= ImGui::SliderFloat("Saturation", &t.saturation, -1.f, 1.f);
      changed |= ImGui::SliderFloat("Fill light", &t.fillLight, 0.f, 100.f);
      changed |= ImGui::SliderFloat("Highlights", &t.highlights, -100.f, 100.f);
      changed |= ImGui::SliderFloat("Shadows", &t.shadows, -100.f, 100.f);
      if (ImGui::Button("Reset Tuning")) {
        es.live_tuning = Tuning();
        changed = true;
      }
      if (changed) es.preview_dirty = true;
      // live: state mirrors sliders while Tuning tab visible
      es.state.tuning = es.live_tuning;
    }
    if (frame_tab == 3) {
      ImGui::TextDisabled("Black & White");
      static bool bw = false;
      if (ImGui::Checkbox("Black & White", &bw)) {
        es.state.tuning.saturation = bw ? -1.f : 0.f;
        es.preview_dirty = true;
      }
    }
  }
  ImGui::End();
  draw_export_dialog();
}

}  // namespace pic
