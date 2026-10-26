#pragma once
#include <string>

namespace pic {
// Minimal JPEG/EXIF reader. Returns orientation 1..8 (0/1 = none).
int exif_orientation_of(const std::string& path);
// Date taken string "YYYY:MM:DD HH:MM:SS" ("" if none).
std::string exif_datetime_of(const std::string& path);
}
