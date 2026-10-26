#include "imgui.h"
#include "viewer_panel.h"
#include "library_panel.h"
#include "style.h"
#include "thumbs.h"
#include <cmath>
#include <cstring>
#include <string>

namespace pic {

struct ViewerState {
  float zoom = 1.f;          // relative to fit
  ImVec2 pan{0, 0};
  bool fit = true;
  float zoom_anchor_x = -1, zoom_anchor_y = -1;
  unsigned tex = 0;
  int tex_w = 0, tex_h = 0;
  bool new_tex_ready = false;
};
static ViewerState vs;

static void reset_view(float aspect, int w, int h) {
  vs.fit = true;
  vs.zoom = 1.f;
  vs.pan = ImVec2(0, 0);
  (void)aspect;
  (void)w;
  (void)h;
}

static ImTextureID upload_preview(EditSession& es) {
  int avail_w = int(ImGui::GetContentRegionAvail().x);
  int avail_h = int(ImGui::GetContentRegionAvail().y);
  if (avail_w < 16 || avail_h < 16) {
    avail_w = 1920;
    avail_h = 1080;
  }
  int tw = avail_w, th = int(avail_w * es.aspect_ratio());
  if (th > avail_h) {
    th = avail_h;
    tw = int(th / es.aspect_ratio());
  }
  es.render_preview(tw, th);
  unsigned t = upload_rgba(es.preview);
  if (vs.tex) free_tex(vs.tex);
  vs.tex = t;
  vs.tex_w = tw;
  vs.tex_h = th;
  return (ImTextureID)(intptr_t)vs.tex;
}

void draw_viewer_panel() {
  auto& es = app()->session;
  if (!es.active) {
    app()->mode = UIMode::Library;
    return;
  }
  ImGuiID dock = ImGui::GetID("##viewerroot");

  // top bar
  static char breadcrumb[512];
  snprintf(breadcrumb, sizeof(breadcrumb), "%s  (%d / %d)", es.path.c_str(), app()->sel + 1,
           int(app()->current.size()));
  if (ImGui::Button("< Back")) app()->leave_viewer();
  ImGui::SameLine();
  ImGui::TextDisabled("%s", breadcrumb);
  ImGui::SameLine();
  if (ImGui::Button("Rotate CCW")) {
    edit_add_rotation(es.state, 3);
    es.preview_dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Rotate CW")) {
    edit_add_rotation(es.state, 1);
    es.preview_dirty = true;
  }
  ImGui::SameLine();
  bool starred = false;
  {
    auto snap = app()->scanner.snapshot();
    int gi = app()->current[size_t(app()->sel)];
    if (gi >= 0 && gi < int(snap.size())) starred = snap[size_t(gi)].starred;
  }
  if (ImGui::Button(starred ? "Starred  *" : "Star")) app()->toggle_star_current();
  ImGui::SameLine();
  if (ImGui::Button("Export...")) ImGui::OpenPopup("##export");
  ImGui::SameLine();
  if (ImGui::Button("Full Screen")) toggle_fullscreen();
  ImGui::SameLine();
  ImGui::TextDisabled("%s", app()->scanner.rescan_status().c_str());

  // tags row (picasa.ini "keywords=")
  static char tag_buf[256];
  static std::string last_path;
  static bool saved_flash = false;
  if (last_path != es.path) {
    last_path = es.path;
    tag_buf[0] = 0;
    if (Photo* ph = app()->scanner.find(es.path)) {
      snprintf(tag_buf, sizeof(tag_buf), "%s", ph->keywords.c_str());
    }
    saved_flash = false;
  }
  ImGui::Text("Tags:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(360);
  bool tag_enter = ImGui::InputText("##tags", tag_buf, sizeof(tag_buf),
                                    ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::SameLine();
  if (ImGui::Button("Save Tags") || tag_enter) {
    app()->set_keywords_current(std::string(tag_buf));
    saved_flash = true;
  }
  if (saved_flash) {
    ImGui::SameLine();
    ImGui::TextDisabled("saved");
  }

  // canvas
  ImGui::BeginChild("##canvas", ImVec2(0, 0), ImGuiChildFlags_None,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                        ImGuiWindowFlags_NoMove);
  ImTextureID tex = upload_preview(es);
  ImVec2 avail = ImGui::GetContentRegionAvail();
  float aspect = es.aspect_ratio();
  float fit_w = avail.x, fit_h = fit_w / aspect;
  if (fit_h > avail.y) {
    fit_h = avail.y;
    fit_w = fit_h * aspect;
  }
  float z = vs.fit ? 1.f : vs.zoom;
  float w = fit_w * z, h = fit_h * z;
  float user_w = w - fit_w, user_h = h - fit_h;
  ImVec2 base = ImVec2((avail.x - w) * 0.5f, (avail.y - h) * 0.5f);
  ImVec2 p0 = ImVec2(base.x + vs.pan.x, base.y + vs.pan.y);
  ImVec2 p1 = ImVec2(p0.x + w, p0.y + h);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddImage(tex, p0, p1);
  ImVec2 wpos = ImGui::GetWindowPos();
  dl->AddRect(wpos, ImVec2(wpos.x + avail.x, wpos.y + avail.y), IM_COL32(40, 40, 40, 255));
  // input handling: pan with drag, zoom with wheel
  if (ImGui::IsWindowHovered()) {
    if (vs.fit) {
      if (ImGui::IsMouseDragging(0)) {
        vs.fit = false;
        vs.zoom = 1.4f;
      }
    }
    if (ImGui::IsMouseDragging(0) && !vs.fit) {
      vs.pan.x += ImGui::GetIO().MouseDelta.x;
      vs.pan.y += ImGui::GetIO().MouseDelta.y;
    }
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.f) {
      vs.zoom = std::max(0.05f, std::min(16.f, vs.zoom * (wheel > 0.f ? 1.2f : 1.f / 1.2f)));
      vs.fit = false;
    }
  }
  if (ImGui::IsWindowFocused()) {
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) app()->next_photo(-1);
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) app()->next_photo(1);
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) app()->leave_viewer();
  }
  ImGui::EndChild();
  (void)user_h;
  (void)user_w;
  (void)dock;
  draw_edit_panel();
  draw_export_dialog();
}

}  // namespace pic
