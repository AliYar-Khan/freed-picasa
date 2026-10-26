#include "imgui.h"
#include "viewer_panel.h"
#include "thumbs.h"
#include <cmath>

namespace pic {

void draw_slideshow_panel() {
  static Image cur;
  static unsigned tex = 0;
  static int shown = -1;
  static float timer = 0.f;

  bool leaving = false;
  if (app()->mode != UIMode::Slideshow) {
    if (tex) {
      free_tex(tex);
      tex = 0;
      shown = -1;
      cur = Image();
    }
    return;
  }
  float dt = ImGui::GetIO().DeltaTime;
  timer += dt;

  auto advance = [leav = &leaving](int dir) mutable -> bool {
    int n = int(app()->current.size());
    if (n == 0) return false;
    int base = app()->sel >= 0 ? app()->sel : 0;
    int next = (base + dir + n) % n;
    app()->sel = next;
    int gi = app()->current[size_t(next)];
    std::string path = app()->scanner.items[size_t(gi)].path;
    Image im;
    int ori = 0;
    if (!load_scaled_rgba8(path, im, 1600, &ori)) return false;
    apply_exif_orientation(im, ori);
    cur = std::move(im);
    if (tex) free_tex(tex);
    tex = upload_rgba(cur);
    timer = 0.f;
    (void)leav;
    return true;
  };

  if (tex == 0 || shown != app()->sel) {
    if (!advance(0)) {
      app()->mode = UIMode::Library;
      return;
    }
    shown = app()->sel;
  }
  if (timer > app()->slide_ms / 1000.f) {
    advance(1);
    shown = app()->sel;
  }

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->Pos);
  ImGui::SetNextWindowSize(vp->Size);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
  if (ImGui::Begin("##slideshow", nullptr, flags)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetWindowPos();
    ImVec2 sz = ImGui::GetWindowSize();
    dl->AddRectFilled(p0, ImVec2(p0.x + sz.x, p0.y + sz.y), IM_COL32(0, 0, 0, 255));
    if (tex) {
      float aspect = cur.h ? float(cur.w) / float(cur.h) : 1.f;
      float w = sz.x, h = w / aspect;
      if (h > sz.y) {
        h = sz.y;
        w = h * aspect;
      }
      ImVec2 ip0(p0.x + (sz.x - w) * 0.5f, p0.y + (sz.y - h) * 0.5f);
      dl->AddImage((ImTextureID)(intptr_t)tex, ip0, ImVec2(ip0.x + w, ip0.y + h));
    }
    // progress
    float prog = timer / (app()->slide_ms / 1000.f);
    dl->AddRectFilled(ImVec2(p0.x, p0.y + sz.y - 4.f), ImVec2(p0.x + sz.x * prog, p0.y + sz.y),
                      IM_COL32(255, 255, 255, 160));
    if (shown >= 0 && shown < int(app()->current.size())) {
      const Photo& p = app()->scanner.items[size_t(app()->current[size_t(shown)])];
      dl->AddText(ImVec2(p0.x + 12.f, p0.y + sz.y - 24.f), IM_COL32(240, 240, 240, 220),
                  p.name.c_str());
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) leaving = true;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
        ImGui::IsKeyPressed(ImGuiKey_Space)) {
      advance(1);
      shown = app()->sel;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
      advance(-1);
      shown = app()->sel;
    }
    if (ImGui::IsMouseClicked(0)) advance(1), shown = app()->sel;
  }
  ImGui::End();
  ImGui::PopStyleVar(2);
  if (leaving) app()->mode = UIMode::Library;
}

}  // namespace pic
