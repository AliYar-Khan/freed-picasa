#pragma once
// Generic order-preserving INI with section/key level round-trip.
// Used for picasa.ini / album ini etc. Unknown keys and sections are
// preserved verbatim so (real) Picasa sees no difference after an edit.
#include <string>
#include <vector>
#include <unordered_map>

namespace pic {

class IniFile {
 public:
  std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> sections;

  static bool load(const std::string& path, IniFile& out);
  bool save(const std::string& path, bool utf8_marker = false) const;

  std::string get(const std::string& section, const std::string& key,
                  const std::string& def = "") const;

  // Replace value of first key occurrence in section; if section exists and
  // key is new, appends inside it. If section missing, appends at end.
  void set(const std::string& section, const std::string& key, const std::string& value);
  void remove_key(const std::string& section, const std::string& key);

  bool has_section(const std::string& name) const;
  // First section whose name begins with a filename without extension? Not needed; exact.
 private:
  int index_of(const std::string& section) const;
};

}  // namespace pic
