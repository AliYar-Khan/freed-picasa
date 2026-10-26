#pragma once
#include "imgui.h"

namespace pic {
// Picasa-3.9-inspired palette + shared widget helpers.
ImVec4 P_C_LIGHTGRAY();   // panel bg (#f5f5f5)
ImVec4 P_C_TOOLBAR();     // toolbar gray
ImVec4 P_C_ACCENT();      // blue accent
ImVec4 P_C_TEXT();
ImVec4 P_C_DARKPANEL();
void pic_style_dark();
void pic_style_light();

// draw one "filmstrip-like" thumbnail cell with selected box
}
