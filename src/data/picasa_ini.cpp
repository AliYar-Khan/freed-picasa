#include "picasa_ini.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace pic {

static std::string trim(const std::string& s) {
  size_t b = 0, e = s.size();
  while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r')) b++;
  while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) e--;
  return s.substr(b, e - b);
}

bool IniFile::load(const std::string& path, IniFile& out) {
  std::ifstream f(path);
  if (!f) return false;
  out.sections.clear();
  int cur = -1;
  std::string line;
  while (std::getline(f, line)) {
    std::string l = trim(line);
    if (l.empty() || l[0] == ';' || l[0] == '#') {
      // skip comments for now (round-trip of comments is nice-to-have)
      continue;
    }
    if (l[0] == '[') {
      size_t c = l.find(']');
      if (c == std::string::npos) continue;
      std::string name = trim(l.substr(1, c - 1));
      out.sections.emplace_back(name, std::vector<std::pair<std::string, std::string>>{});
      cur = int(out.sections.size()) - 1;
    } else {
      size_t eq = l.find('=');
      if (eq == std::string::npos) continue;
      std::string key = trim(l.substr(0, eq));
      std::string val = trim(l.substr(eq + 1));
      if (cur < 0) {
        out.sections.emplace_back("", std::vector<std::pair<std::string, std::string>>{});
        cur = 0;
      }
      out.sections[cur].second.emplace_back(key, val);
    }
  }
  return true;
}

bool IniFile::save(const std::string& path, bool utf8_marker) const {
  std::ostringstream o;
  if (utf8_marker) o << ";  *** THIS FILE IS WORTHLESS WITHOUT THEORETICAL BACON ***;v1" << "\n";
  for (auto& sec : sections) {
    if (!sec.first.empty()) o << "[" << sec.first << "]\n";
    for (auto& kv : sec.second) o << kv.first << "=" << kv.second << "\n";
    o << "\n";
  }
  std::string s = o.str();
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) return false;
  bool ok = fwrite(s.data(), 1, s.size(), f) == s.size();
  fclose(f);
  return ok;
}

int IniFile::index_of(const std::string& section) const {
  for (size_t i = 0; i < sections.size(); i++)
    if (sections[i].first == section) return int(i);
  return -1;
}

bool IniFile::has_section(const std::string& name) const { return index_of(name) >= 0; }

std::string IniFile::get(const std::string& section, const std::string& key,
                         const std::string& def) const {
  int i = index_of(section);
  if (i >= 0) {
    for (auto& kv : sections[i].second)
      if (kv.first == key) return kv.second;
  }
  return def;
}

void IniFile::set(const std::string& section, const std::string& key, const std::string& value) {
  int i = index_of(section);
  if (i < 0) {
    sections.emplace_back(section, std::vector<std::pair<std::string, std::string>>{});
    i = int(sections.size()) - 1;
  }
  for (auto& kv : sections[i].second) {
    if (kv.first == key) {
      kv.second = value;
      return;
    }
  }
  sections[i].second.emplace_back(key, value);
}

void IniFile::remove_key(const std::string& section, const std::string& key) {
  int i = index_of(section);
  if (i < 0) return;
  auto& kvs = sections[i].second;
  for (auto it = kvs.begin(); it != kvs.end(); ++it) {
    if (it->first == key) {
      kvs.erase(it);
      return;
    }
  }
}

}  // namespace pic
