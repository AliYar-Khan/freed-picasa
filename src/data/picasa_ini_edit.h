#pragma once
#include "data/picasa_ini.h"
#include "edit/ops.h"

#include <string>
#include <vector>

namespace pic {

// Coordinates are image-relative fractions in [0,1]; encoded as four 16-bit
// hex words (x1 y1 x2 y2) left-to-right, each scaled by 65535.
std::string rect64_encode(float x1, float y1, float x2, float y2);

void set_crop(IniFile& ini, const std::string& section, const std::string& rect64_words);
void set_redeye(IniFile& ini, const std::string& section, const std::string& params);
std::string redeye_encode(const std::vector<float>& params);
void set_rotate(IniFile& ini, const std::string& section, int quarter_turns);
void set_star(IniFile& ini, const std::string& section, bool on);

// Writes rotate/crop/redeye/retouch entries (and clears them when absent)
// into <folder>/picasa.ini for the section named by the file's basename.
void persist_edit_metadata(const std::string& path, const EditState& s);

}  // namespace pic
