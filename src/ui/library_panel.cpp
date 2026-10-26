#include "imgui.h"
#include "library_panel.h"
#include "style.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

namespace pic {

static const char* dir_tail(const std::string& dir) {
  size_t p = dir.find_last_of('/');
  return p == std::string::npos ? dir.c_str() : dir.c_str() + p + 1;
}

static void collect_subs(const std::string& parent, const std::vector<Photo>& items,
                         std::vector<std::string>& out) {
  std::set<std::string> dirs;
  for (auto& p : items) {
    if (p.folder.rfind(parent + "/", 0) != 0) continue;
    std::string rel = p.folder.substr(parent.size() + 1);
    size_t slash = rel.find('/');
    if (slash != std::string::npos) rel = rel.substr(0, slash);
    if (!rel.empty()) dirs.insert(rel);
  }
  out.assign(dirs.begin(), dirs.end());
}

static void draw_tree_node(const std::string& dir, const std::vector<Photo>& items) {
  std::vector<std::string> subs;
  collect_subs(dir, items, subs);
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
  if (dir == app()->cur_dir) flags |= ImGuiTreeNodeFlags_Selected;
  if (subs.empty())
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
  ImGui::PushID(dir.c_str());
  bool open = ImGui::TreeNodeEx("##tv", flags, "%s", dir_tail(dir));
  if (ImGui::IsItemClicked(0) && !ImGui::IsItemToggledOpen()) app()->set_folder(dir);
  if (open) {
    for (auto& s : subs) draw_tree_node(dir + "/" + s, items);
    if (!subs.empty()) ImGui::TreePop();
  }
  ImGui::PopID();
}

static void draw_folder_tree() {
  auto& scn = app()->scanner;
  ImGui::BeginChild("##foldertree", ImVec2(230, 0),
                    ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
  ImGui::TextDisabled("FOLDER LIST");
  ImGui::Separator();
  auto snap = scn.snapshot();
  for (auto& root : scn.roots()) draw_tree_node(root, snap);
  ImGui::Spacing();
  if (ImGui::Button("Add Folder...")) ImGui::OpenPopup("##addfolder");
  draw_add_folder_popup();
  ImGui::Separator();
  if (ImGui::Button("Rescan")) scn.rescan_async();
  if (scn.scanning) {
    ImGui::SameLine();
    ImGui::TextDisabled("scanning...");
  }
  ImGui::EndChild();
}

void draw_add_folder_popup() {
  if (!ImGui::BeginPopup("##addfolder")) return;
  static char buf[768] = "";
  ImGui::Text("Folder to add to the library:");
  bool done = ImGui::InputText("##path", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::SetNextItemAllowOverlap();
  ImGui::SameLine();
  if (ImGui::Button("OK")) done = true;
  ImGui::TextDisabled("e.g. %s/Pictures", app()->home().c_str());
  if (done && buf[0]) {
    app()->scanner.add_root(buf);
    ImGui::CloseCurrentPopup();
    app()->scanner.rescan_async();
  }
  ImGui::EndPopup();
}

static void fit_box(int tw, int th, float boxw, float boxh, float& w, float& h) {
  if (tw <= 0 || th <= 0) {
    w = h = 0;
    return;
  }
  float aspect = float(tw) / float(th);
  w = boxw;
  h = w / aspect;
  if (h > boxh) {
    h = boxh;
    w = h * aspect;
  }
}

static void draw_star(ImDrawList* dl, ImVec2 c, float r) {
  for (int i = 0; i < 5; i++) {
    float a1 = -1.5708f + float(i) * 1.2566f;
    float a2 = a1 + 0.62832f;
    float a3 = a1 + 1.2566f;
    dl->AddTriangleFilled(ImVec2(c.x, c.y - r * 0.4f),
                          ImVec2(c.x + cosf(a1) * r, c.y + sinf(a1) * r),
                          ImVec2(c.x + cosf(a2) * r, c.y + sinf(a2) * r), IM_COL32(255, 200, 40, 255));
    dl->AddTriangleFilled(ImVec2(c.x, c.y + r * 0.45f),
                          ImVec2(c.x + cosf(a2) * r, c.y + sinf(a2) * r),
                          ImVec2(c.x + cosf(a3) * r, c.y + sinf(a3) * r), IM_COL32(255, 200, 40, 255));
  }
}

static void draw_thumb_grid(const std::vector<uint32_t>& ids, const std::vector<Photo>& snap,
                            float cell) {
  if (ids.empty() || snap.empty()) {
    ImGui::TextDisabled("No pictures have been found in this folder.");
    return;
  }
  static const float pad = 8.f;
  float img_h = cell * 0.75f;
  float name_h = ImGui::GetTextLineHeight() + 2.f;
  float cell_w_total = cell + pad;
  float cell_h_total = img_h + name_h + pad;
  float avail_x = ImGui::GetContentRegionAvail().x;
  int columns = std::max(1, int(avail_x / cell_w_total));
  size_t n = ids.size();
  size_t rows = (n + size_t(columns) - 1) / size_t(columns);
  ImVec2 origin = ImGui::GetCursorScreenPos();
  float t = float(ImGui::GetTime());

  for (size_t row = 0; row < rows; row++) {
    for (int col = 0; col < columns; col++) {
      size_t i = row * size_t(columns) + size_t(col);
      if (i >= n) break;
      uint32_t ind = ids[i];
      const Photo& item = snap[size_t(ind)];
      ImVec2 sp(origin.x + float(col) * cell_w_total, origin.y + float(row) * cell_h_total);
      float w = std::min(cell, avail_x - cell_w_total * 0.2f);
      ImVec2 sz(w, img_h + name_h);
      ImDrawList* dl = ImGui::GetWindowDrawList();
      ImVec2 rect_min = sp;
      ImVec2 rect_max(sp.x + sz.x, sp.y + sz.y);

      bool hovered = ImGui::IsMouseHoveringRect(rect_min, rect_max);
      if (hovered) ImGui::SetTooltip("%s", item.name.c_str());
      int pos = int(i);
      bool clicked = hovered && ImGui::IsMouseClicked(0) &&
                      !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
      bool dbl = hovered && ImGui::IsMouseDoubleClicked(0);
      if (clicked) app()->sel = pos;
      if (dbl) app()->open_viewer(pos);

      ImU32 bg = app()->sel == pos ? IM_COL32(205, 224, 245, 255)
                                    : IM_COL32(246, 246, 246, 255);
      dl->AddRectFilled(rect_min, rect_max, bg);
      dl->AddRect(rect_min, rect_max,
                  hovered ? IM_COL32(150, 175, 210, 255) : IM_COL32(210, 210, 210, 255));

      GlTex tex{};
      bool have = app()->thumbs.get(item.key, tex);
      if (!have && !app()->thumbs.pending(item.key))
        app()->thumbs.request(item.key, item.path, item.mtime + double(item.size) * 0.001);
      if (have) {
        float tw2, th2;
        fit_box(tex.w, tex.h, sz.x - 6.f, img_h - 6.f, tw2, th2);
        dl->AddImage((ImTextureID)(intptr_t)tex.id,
                     ImVec2(sp.x + (sz.x - tw2) * 0.5f, sp.y + (img_h - th2) * 0.5f),
                     ImVec2(sp.x + (sz.x + tw2) * 0.5f, sp.y + (img_h + th2) * 0.5f));
      } else {
        float a = t * 6.0f + float(i) * 0.4f;
        dl->AddCircleFilled(ImVec2(sp.x + sz.x * 0.5f, sp.y + img_h * 0.5f), 3.f,
                            IM_COL32(200, 200, 200, 140));
        dl->AddCircleFilled(ImVec2(sp.x + sz.x * 0.5f + cosf(a) * 15.f,
                                   sp.y + img_h * 0.5f + sinf(a) * 11.f),
                            2.5f, IM_COL32(110, 120, 130, 240));
      }
      if (item.starred) draw_star(dl, ImVec2(sp.x + sz.x - 14.f, sp.y + 12.f), 6.f);
      dl->AddText(ImVec2(sp.x + 5.f, sp.y + img_h + 3.f), IM_COL32(84, 88, 92, 255),
                  item.name.c_str());
    }
  }
  ImGui::Dummy(ImVec2(avail_x, float(rows) * cell_h_total + 20.f));
}

void draw_library_panel() {
  auto& scn = app()->scanner;
  // auto-select first root once scanning settles
  if (app()->cur_dir.empty() && !scn.scanning) {
    auto roots = scn.roots();
    if (!roots.empty()) {
      app()->set_folder(roots[0]);
      const char* ao = getenv("OPENPICSA_AUTO_OPEN");
      if (ao && ao[0] == '1' && !app()->current.empty()) app()->open_viewer(0);
    }
  }
  // ---- toolbar
  if (ImGui::Button("  Get Photos  ")) {}
  ImGui::SameLine();
  if (ImGui::Button(" Add Folder ")) ImGui::OpenPopup("##addfolder");
  ImGui::SameLine();
  if (ImGui::Button("Rescan")) scn.rescan_async();
  ImGui::SameLine();
  ImGui::PushItemWidth(160);
  ImGui::SliderFloat("##thumbsize", &app()->grid_thumb_size, 100.f, 340.f, "");
  ImGui::PopItemWidth();
  ImGui::SameLine();
  if (ImGui::Button("Slideshow") && app()->sel >= 0 &&
      app()->sel < int(app()->current.size())) {
    app()->mode = UIMode::Slideshow;
  }
  ImGui::SameLine();
  if (ImGui::Button("Full Screen")) toggle_fullscreen();
  ImGui::SameLine();
  size_t total = scn.count();
  if (scn.scanning) {
    ImGui::TextDisabled("Scanning... %zu items", total);
  } else {
    ImGui::TextDisabled("%zu items in library", total);
  }
  ImGui::Separator();

  // ---- left tree + main grid
  draw_folder_tree();
  ImGui::SameLine();
  ImGui::BeginChild("##grid", ImVec2(0, 0), ImGuiChildFlags_None);
  auto snap = scn.snapshot();
  const auto& ids = app()->current;
  draw_thumb_grid(ids, snap, app()->grid_thumb_size);
  ImGui::EndChild();
}

}  // namespace pic
