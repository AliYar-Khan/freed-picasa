// Edit-metadata emitters for picasa.ini, matching strings mined from
// Picasa3.exe 3.9.141 ("crop=rect64(%s)", "redeye=1;", "rotate(...)" invoked,
// "star=yes"). Values are written exactly in the observed formats; a live
// wine round-trip remains blocklisted in docs/formats/picasa-ini.md.
#include "data/picasa_ini_edit.h"

#include <cstdint>
#include <cstdio>
#include <string>

namespace pic {

static std::string fmt4(int v) {
  char b[8];
  std::snprintf(b, sizeof(b), "%04x", static_cast<int>(v & 0xffff));
  return b;
}

std::string rect64_encode(float x1, float y1, float x2, float y2) {
  auto q = [](float f) {
    int v = static_cast<int>(f * 65535.0f + 0.5f);
    if (v < 0) v = 0;
    if (v > 0xffff) v = 0xffff;
    return v;
  };
  return fmt4(q(x1)) + fmt4(q(y1)) + fmt4(q(x2)) + fmt4(q(y2));
}

void set_crop(IniFile& ini, const std::string& section, const std::string& rect) {
  ini.set(section, "crop", "rect64(" + rect + ")");
}

std::string redeye_encode(const std::vector<float>& params) {
  std::string s;
  for (float p : params) {
    char b[24];
    std::snprintf(b, sizeof(b), "%g;", p);
    s += b;
  }
  return s;
}

void set_redeye(IniFile& ini, const std::string& section, const std::string& params) {
  ini.set(section, "redeye", params.empty() ? "1;" : params);
}

void set_rotate(IniFile& ini, const std::string& section, int quarter_turns) {
  quarter_turns &= 3;
  ini.set(section, "rotate", std::to_string(quarter_turns));
}

void set_star(IniFile& ini, const std::string& section, bool on) {
  if (on)
    ini.set(section, "star", "yes");
  else
    ini.remove_key(section, "star");
}

static void split_path(const std::string& p, std::string& dir, std::string& base) {
  auto pos = p.find_last_of('/');
  if (pos == std::string::npos) {
    dir = ".";
    base = p;
  } else {
    dir = p.substr(0, pos);
    base = p.substr(pos + 1);
  }
}

void persist_edit_metadata(const std::string& path, const EditState& s) {
  std::string dir, base;
  split_path(path, dir, base);
  IniFile ini;
  IniFile::load(dir + "/picasa.ini", ini);
  if (s.rotation != 0)
    set_rotate(ini, base, s.rotation);
  else
    ini.remove_key(base, "rotate");
  if (s.has_crop) {
    std::string rect = rect64_encode(s.crop_x, s.crop_y, s.crop_x + s.crop_w, s.crop_y + s.crop_h);
    set_crop(ini, base, rect);
  } else {
    ini.remove_key(base, "crop");
  }
  if (!s.redeye.empty())
    set_redeye(ini, base, std::string());
  else
    ini.remove_key(base, "redeye");
  if (!s.retouch.empty())
    ini.set(base, "retouch", "1;");
  else
    ini.remove_key(base, "retouch");
  ini.save(dir + "/picasa.ini");
}

}  // namespace pic
