#include "imgui.h"
#include "viewer_panel.h"

namespace pic {

void draw_export_dialog() {
  static char outpath[1024] = "";
  static int quality = 85;
  static int long_side = 1600;
  static bool size_check = true;
  if (!ImGui::BeginPopupModal("Export", nullptr, 0)) return;
  auto& es = app()->session;
  ImGui::InputText("Destination", outpath, sizeof(outpath));
  ImGui::Checkbox("Resize", &size_check);
  if (size_check) {
    static const char* presets[] = {"1024 (large)", "1600", "2048", "Original size"};
    const char* current = presets[0];
    if (long_side <= 1024) current = presets[0];
    else if (long_side <= 1600) current = presets[1];
    else if (long_side <= 2048) current = presets[2];
    else current = presets[3];
    if (ImGui::BeginCombo("Size", current)) {
      for (const char* pr : presets) {
        if (ImGui::Selectable(pr)) {
          if (!strcmp(pr, "1024 (large)")) long_side = 1024;
          else if (!strcmp(pr, "1600")) long_side = 1600;
          else if (!strcmp(pr, "2048")) long_side = 2048;
          else long_side = 0;
        }
      }
      ImGui::EndCombo();
    }
  }
  ImGui::SliderInt("Quality", &quality, 20, 100);
  if (ImGui::Button("Export")) {
    if (outpath[0]) {
      Image out;
      render_edited(es.base, es.state, out);
      if (size_check && long_side > 0) {
        int mx = std::max(out.w, out.h);
        if (mx > long_side) {
          float k = float(long_side) / float(mx);
          Image scaled(int(out.w * k), int(out.h * k));
          resize_bilinear(out, scaled);
          out = std::move(scaled);
        }
      }
      save_jpeg(outpath, out, quality);
      ImGui::CloseCurrentPopup();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}

}  // namespace pic
