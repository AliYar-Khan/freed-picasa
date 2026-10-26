#include "app.h"
#include "data/picasa_ini.h"
#include "data/picasa_ini_edit.h"
#include <cstdlib>
#include <sys/stat.h>

namespace pic {

static App* g_app = nullptr;

App* app() {
  if (!g_app) g_app = new App();
  return g_app;
}

std::string App::home() {
  const char* h = getenv("HOME");
  return h ? h : "/";
}

void App::ensure_started() {
  if (!cache_dir.empty()) return;
  cache_dir = home() + "/.cache/open-picasa";
  config_dir = home() + "/.config/open-picasa";
  mkdir(cache_dir.c_str(), 0755);
  mkdir(config_dir.c_str(), 0755);
}

void App::set_folder(const std::string& dir) {
  cur_dir = dir;
  std::lock_guard<std::mutex> lk(scanner.mu);
  current.clear();
  for (size_t i = 0; i < scanner.items.size(); i++) {
    const Photo& p = scanner.items[i];
    if (p.folder == dir && !p.hidden) current.push_back(uint32_t(i));
  }
}

void App::open_viewer(int sel_idx) {
  sel = sel_idx;
  int i = current[size_t(sel_idx)];
  session.open(scanner.items[i].path);
  mode = UIMode::Viewer;
}

static void persist_session_edits(const EditSession& s) {
  if (!s.active || !s.state.dirty()) return;
  persist_edit_metadata(s.path, s.state);
}

void App::leave_viewer() {
  persist_session_edits(session);
  mode = UIMode::Library;
}

void App::next_photo(int dir) {
  if ((int)current.size() <= 1 || sel < 0) return;
  persist_session_edits(session);
  int n = int(current.size());
  sel = (sel + dir + n) % n;
  session.open(scanner.items[current[sel]].path);
}

void App::toggle_star_current() {
  if (sel < 0 || sel >= int(current.size())) return;
  int i = current[sel];
  Photo& p = scanner.items[size_t(i)];
  p.starred = !p.starred;
  // persist star into picasa.ini [General] star=yes; (format verified in P1)
  IniFile ini;
  std::string inipath = p.folder + "/picasa.ini";
  IniFile::load(inipath, ini);
  if (p.starred)
    ini.set(p.name, "star", "yes");
  else
    ini.remove_key(p.name, "star");
  ini.save(inipath);
}

void App::set_keywords_current(const std::string& csv) {
  if (sel < 0 || sel >= int(current.size())) return;
  int i = current[sel];
  Photo& p = scanner.items[size_t(i)];
  p.keywords = csv;
  IniFile ini;
  std::string inipath = p.folder + "/picasa.ini";
  IniFile::load(inipath, ini);
  if (csv.empty())
    ini.remove_key(p.name, "keywords");
  else
    ini.set(p.name, "keywords", csv);
  ini.save(inipath);
}

}  // namespace pic
