#include "scanner.h"
#include "data/picasa_ini.h"
#include "image/exif_parse.h"
#include "stb_image.h"
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <set>
#include <sys/stat.h>
#include <thread>

namespace pic {

static bool ends_with_ci(const std::string& s, const char* suf) {
  size_t n = strlen(suf);
  if (s.size() < n) return false;
  for (size_t i = 0; i < n; i++)
    if (std::tolower(s[s.size() - n + i]) != std::tolower(suf[i])) return false;
  return true;
}

bool Scanner::is_image(const std::string& name) {
  // JPEG/PNG/GIF/BMP/TGA decode via stb; Picasa-ish set.
  if (ends_with_ci(name, ".jpg") || ends_with_ci(name, ".jpeg") || ends_with_ci(name, ".png") ||
      ends_with_ci(name, ".gif") || ends_with_ci(name, ".bmp") || ends_with_ci(name, ".tga"))
    return true;
  return false;
}

void Scanner::add_root(const std::string& dir) {
  for (auto& r : roots_)
    if (r == dir) return;
  roots_.push_back(dir);
}

void Scanner::clear_all() {
  std::lock_guard<std::mutex> lk(mu);
  items.clear();
  by_path.clear();
  roots_.clear();
}

void Scanner::scan_dir(const std::string& dir, std::vector<Photo>& out) {
  DIR* d = opendir(dir.c_str());
  if (!d) return;
  std::set<std::string> files;  // deterministic order
  std::vector<std::string> subs;
  while (auto* en = readdir(d)) {
    std::string n = en->d_name;
    if (n == "." || n == "..") continue;
    std::string full = dir + "/" + n;
    struct stat st{};
    if (stat(full.c_str(), &st) != 0) continue;
    if (S_ISDIR(st.st_mode)) {
      if (n == ".picasaoriginals" || n == ".tmp") continue;
      subs.push_back(full);
    } else if (S_ISREG(st.st_mode) && is_image(n)) {
      files.insert(full);
    }
  }
  closedir(d);

  IniFile ini;
  bool has_ini = false;
  for (auto& (f) : files) {
    std::string folder = dir;
    auto base = f.substr(f.find_last_of('/') + 1);
    Photo p;
    p.path = f;
    p.name = base;
    p.folder = dir;
    struct stat st{};
    if (stat(f.c_str(), &st) == 0) {
      p.mtime = double(st.st_mtime);
      p.size = uint64_t(st.st_size);
    }
    stbi_info(f.c_str(), &p.w, &p.h, nullptr);
    // Quick EXIF read (exact orientation of raw file)
    p.orientation = exif_orientation_of(f);
    p.dt = exif_datetime_of(f);
    // picasa.ini edits
    if (!has_ini) has_ini = IniFile::load(dir + "/picasa.ini", ini);
    if (has_ini) {
      std::string v("");
      p.starred = (ini.get(base, "star", v) == "yes");
      p.keywords = ini.get(base, "keywords", v);
      // edits mapping: see docs/formats/picasa-ini.md
    }
    out.push_back(std::move(p));
  }
  for (auto& s : subs) scan_dir(s, out);
}

void Scanner::rescan() {
  scanning = true;
  std::vector<Photo> next;
  for (auto& root : roots_) scan_dir(root, next);
  {
    std::lock_guard<std::mutex> lk(mu);
    items = std::move(next);
    by_path.clear();
    for (size_t i = 0; i < items.size(); i++) by_path[items[i].path] = i;
  }
  scanning = false;
}

void Scanner::rescan_async() {
  if (scanning) return;
  std::thread([this] { rescan(); }).detach();
}

std::string Scanner::rescan_status() { return scanning ? "scanning..." : ""; }

std::vector<Photo> Scanner::snapshot() {
  std::lock_guard<std::mutex> lk(mu);
  return items;
}

size_t Scanner::count() {
  std::lock_guard<std::mutex> lk(mu);
  return items.size();
}

Photo* Scanner::find(const std::string& path) {
  std::lock_guard<std::mutex> lk(mu);
  auto it = by_path.find(path);
  if (it == by_path.end()) return nullptr;
  return &items[it->second];
}

std::vector<std::string> Scanner::roots() {
  std::lock_guard<std::mutex> lk(mu);
  return roots_;
}

std::vector<std::string> Scanner::list_dirs(const std::string& parent) {
  std::lock_guard<std::mutex> lk(mu);
  std::set<std::string> dirs;
  for (auto& p : items) {
    if (p.folder.rfind(parent + "/", 0) == 0 || p.folder == parent) {
      std::string rel = p.folder == parent ? "" : p.folder.substr(parent.size() + 1);
      size_t slash = rel.find('/');
      if (slash != std::string::npos) rel = rel.substr(0, slash);
      if (rel.empty())
        ;
      else
        dirs.insert(rel);
    }
  }
  return std::vector<std::string>(dirs.begin(), dirs.end());
}

std::vector<uint32_t> Scanner::indices_in_dir(const std::string& dir) {
  std::lock_guard<std::mutex> lk(mu);
  std::vector<uint32_t> out;
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].folder == dir && !items[i].hidden) out.push_back(uint32_t(i));
  }
  return out;
}

bool Scanner::read_edits(Photo&) const { return true; }

bool Scanner::persist_edits(const Photo&) { return true; }

}  // namespace pic
