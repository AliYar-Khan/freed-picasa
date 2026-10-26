#include "exif_parse.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace pic {

namespace {

uint16_t rd16(const uint8_t* p, bool le) {
  return le ? uint16_t((p[1] << 8) | p[0]) : uint16_t((p[0] << 8) | p[1]);
}
uint32_t rd32(const uint8_t* p, bool le) {
  return le ? (uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24)
            : (uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]));
}

struct Exif {
  int orientation = 0;
  std::string dt;
};

static const int kTSize[13] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};

void parse_ifd(const uint8_t* d, size_t end, uint32_t off, bool le, Exif& e) {
  if (off + 2 > end) return;
  uint16_t n = rd16(d + off, le);
  size_t entries_end = off + 2 + size_t(n) * 12;
  if (n > 0x1000 || entries_end + 4 > end) return;
  for (uint16_t i = 0; i < n; i++) {
    const uint8_t* ep = d + off + 2 + size_t(i) * 12;
    uint16_t tag = rd16(ep, le);
    uint16_t ty = rd16(ep + 2, le);
    uint32_t ct = rd32(ep + 4, le);
    int ts = (ty < 13) ? kTSize[ty] : 0;
    if (!ts) continue;
    uint32_t bytes = ts * ct;
    const uint8_t* v = bytes <= 4 ? ep + 8 : d + rd32(ep + 8, le);
    if (v + bytes > d + end) continue;
    switch (tag) {
      case 0x0112: {  // Orientation SHORT
        uint16_t val = le ? rd16(ep + 8, true) : rd16(ep + 8, le);
        e.orientation = val;
        break;
      }
      case 0x0132:  // DateTime
      case 0x9003:  // DateTimeOriginal
      case 0x9004:  // DateTimeDigitized
        if (e.dt.empty() && v && bytes > 0) {
          // skip if already set
          if (tag == 0x0132 || e.dt.empty()) {
            std::string s((const char*)v, std::min(bytes, uint32_t(19)));
            if (!s.empty() && s[0] >= '0' && s[0] <= '9') {
              if (tag == 0x0132) e.dt = s;
              else if (e.dt.empty()) e.dt = s;  // prefer original/digitized when no DateTime yet
            }
          }
        }
        break;
      default:
        break;
    }
  }
  // Follow Exif IFD pointer
  for (uint16_t i = 0; i < n; i++) {
    const uint8_t* ep = d + off + 2 + size_t(i) * 12;
    uint16_t tag = rd16(ep, le);
    uint16_t ty = rd16(ep + 2, le);
    if (tag == 0x8769 && ty == 4) {
      uint32_t voff = rd32(ep + 8, le);
      parse_ifd(d, end, voff, le, e);
    }
  }
}

Exif parse(const uint8_t* buf, size_t sz) {
  Exif e;
  if (sz < 8 || memcmp(buf, "Exif\0\0", 6) != 0) return e;
  bool le = false;
  if (memcmp(buf + 6, "II", 2) == 0)
    le = true;
  else if (memcmp(buf + 6, "MM", 2) == 0)
    le = false;
  else
    return e;
  uint16_t magic = rd16(buf + 8, le);
  if (magic != 0x2A) return e;
  uint32_t first = rd32(buf + 10, le);
  parse_ifd(buf + 6, sz - 6, first, le, e);
  return e;
}

bool find_app1_jpeg_path(const std::string& path, std::vector<uint8_t>& app1) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return false;
  uint8_t hdr[2];
  if (fread(hdr, 1, 2, f) != 2 || hdr[0] != 0xFF || hdr[1] != 0xD8) {
    fclose(f);
    return false;
  }
  while (true) {
    uint8_t m[2];
    if (fread(m, 1, 2, f) != 2) break;
    if (m[0] != 0xFF) break;
    if (m[1] == 0xD8 || m[1] == 0x01 || (m[1] >= 0xD0 && m[1] <= 0xD7)) continue;
    if (m[1] == 0xDA || m[1] == 0xC0 || (m[1] >= 0xC1 && m[1] <= 0xCF)) break;
    if (m[1] >= 0xE1 && m[1] <= 0xEF) {
      uint8_t len2[2];
      if (fread(len2, 1, 2, f) != 2) break;
      int len = (len2[0] << 8) | len2[1];
      int body = len - 2;
      if (body <= 0) continue;
      std::vector<uint8_t> buf;
      if (fread(buf.data(), 1, buf.size(), f) != buf.size()) break;
      if (m[1] == 0xE1 && buf.size() > 6 && memcmp(buf.data(), "Exif\0\0", 6) == 0) {
        app1 = buf;
        fclose(f);
        return true;
      }
      continue;
    }
    break;
  }
  fclose(f);
  return false;
}

}  // namespace

int exif_orientation_of(const std::string& path) {
  std::vector<uint8_t> app1;
  if (!find_app1_jpeg_path(path, app1)) return 0;
  return parse(app1.data(), app1.size()).orientation;
}

std::string exif_datetime_of(const std::string& path) {
  std::vector<uint8_t> app1;
  if (!find_app1_jpeg_path(path, app1)) return "";
  return parse(app1.data(), app1.size()).dt;
}

}  // namespace pic
