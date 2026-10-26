#include "style.h"

namespace pic {

ImVec4 rgb(int r, int g, int b) { return ImVec4(float(r) / 255.f, float(g) / 255.f, float(b) / 255.f, 1.f); }

ImVec4 P_C_LIGHTGRAY() { return rgb(240, 240, 240); }
ImVec4 P_C_TOOLBAR() { return rgb(190, 192, 193); }
ImVec4 P_C_ACCENT() { return rgb(60, 120, 200); }
ImVec4 P_C_TEXT() { return rgb(45, 50, 55); }
ImVec4 P_C_DARKPANEL() { return rgb(25, 25, 25); }
ImVec4 P_SELECTED() { return rgb(80, 140, 220); }

void pic_style_light() {
  ImGuiStyle st;
  ImGui::StyleColorsLight(&st);
  ImVec4* c = st.Colors;
  c[ImGuiCol_WindowBg] = P_C_LIGHTGRAY();
  c[ImGuiCol_ChildBg] = P_C_LIGHTGRAY();
  c[ImGuiCol_PopupBg] = rgb(250, 250, 250);
  c[ImGuiCol_Text] = P_C_TEXT();
  c[ImGuiCol_Border] = rgb(210, 210, 210);
  c[ImGuiCol_FrameBg] = rgb(255, 255, 255);
  c[ImGuiCol_FrameBgHovered] = rgb(246, 246, 246);
  c[ImGuiCol_ScrollbarBg] = rgb(230, 230, 230);
  c[ImGuiCol_ScrollbarGrab] = rgb(200, 200, 200);
  c[ImGuiCol_ScrollbarGrabHovered] = rgb(180, 180, 180);
  c[ImGuiCol_MenuBarBg] = P_C_TOOLBAR();
  c[ImGuiCol_Button] = rgb(252, 252, 252);
  c[ImGuiCol_ButtonHovered] = rgb(215, 226, 240);
  c[ImGuiCol_ButtonActive] = P_C_ACCENT();
  c[ImGuiCol_Header] = rgb(96, 138, 190);
  c[ImGuiCol_HeaderHovered] = rgb(110, 150, 200);
  c[ImGuiCol_HeaderActive] = rgb(80, 120, 170);
  c[ImGuiCol_Tab] = rgb(210, 220, 230);
  c[ImGuiCol_TabSelected] = rgb(255, 255, 255);
  c[ImGuiCol_TabHovered] = rgb(225, 235, 245);
  st.FrameRounding = 3;
  st.WindowRounding = 0;
  st.TabRounding = 6;
  st.ScrollbarSize = 13;
  st.GrabRounding = 3;
  st.WindowBorderSize = 1;
  st.FrameBorderSize = 1;
  st.ItemSpacing.y = 5;
  ImGui::GetPlatformIO();
  ImGui::GetStyle() = st;
}

void pic_style_dark() {
  ImGuiStyle st;
  ImGui::StyleColorsDark(&st);
  ImVec4* c = st.Colors;
  c[ImGuiCol_WindowBg] = P_C_DARKPANEL();
  c[ImGuiCol_Text] = rgb(213, 213, 213);
  c[ImGuiCol_PopupBg] = rgb(35, 35, 35);
  c[ImGuiCol_Header] = rgb(60, 70, 90);
  c[ImGuiCol_Button] = rgb(52, 52, 52);
  c[ImGuiCol_FrameBg] = rgb(45, 45, 45);
  st.FrameRounding = 3;
  st.WindowRounding = 0;
  ImGui::GetStyle() = st;
}

}  // namespace pic
