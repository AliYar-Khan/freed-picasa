#pragma once
// App-level state shared by UI panels.
#include "scan/scanner.h"
#include "scan/thumb_cache.h"
#include "ui/edit_model.h"
#include "image/image.h"
#include <string>
#include <vector>

namespace pic {

enum class UIMode { Library, Viewer, Slideshow };

struct App {
  Scanner scanner;
  ThumbCache thumbs;
  std::string cache_dir;   // ~/.cache/open-picasa
  std::string config_dir;  // ~/.config/open-picasa
  UIMode mode = UIMode::Library;
  std::vector<uint32_t> current;  // indices into scanner items (this folder)
  std::string cur_dir;
  int sel = -1;                   // index into current[]
  float grid_thumb_size = 200.0f;

  EditSession session;            // viewer edit session
  uint32_t fullscreen_tex = 0;    // viewer texture (GL id)
  int slide_ms = 3500;

  void ensure_started();
  std::string home();
  void set_folder(const std::string& dir);  // refresh current[]
  void open_viewer(int sel_idx);
  void leave_viewer();  // save session edits? close session (pending save handled)
  void next_photo(int dir);
  void toggle_star_current();
  void set_keywords_current(const std::string& csv);
};

App* app();

}  // namespace pic
