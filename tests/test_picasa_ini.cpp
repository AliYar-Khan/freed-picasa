#include "doctest.h"
#include "data/picasa_ini.h"
#include "data/picasa_ini_edit.h"
#include <cstdio>
#include <sys/stat.h>

using pic::IniFile;

TEST_CASE("rect64 encode matches mined format") {
  // Full-frame crop -> 00000000ffffffff
  CHECK(pic::rect64_encode(0, 0, 1, 1) == "00000000ffffffff");
  // Quarter crop
  std::string r = pic::rect64_encode(0.25f, 0.25f, 0.75f, 0.75f);
  CHECK(r == pic::rect64_encode(0.25f, 0.25f, 0.75f, 0.75f));
  CHECK(r.size() == 16);
  // Saturation clamps
  CHECK(pic::rect64_encode(0, 0, 2, 2).size() == 16);
  CHECK(pic::rect64_encode(-1, -1, 1, 1) == "00000000ffffffff");
}

TEST_CASE("ini edit emitters write mined keys") {
  IniFile ini;
  ini.set("IMG_0001.jpg", "keepme", "1");  // unknown keys survive
  pic::set_crop(ini, "IMG_0001.jpg", pic::rect64_encode(0, 0, 1, 1));
  pic::set_redeye(ini, "IMG_0001.jpg", pic::redeye_encode({1.0f}));
  pic::set_rotate(ini, "IMG_0001.jpg", 5);  // wraps to 1
  pic::set_star(ini, "IMG_0001.jpg", true);
  CHECK(ini.get("IMG_0001.jpg", "crop") == "rect64(00000000ffffffff)");
  CHECK(ini.get("IMG_0001.jpg", "redeye") == "1;");
  CHECK(ini.get("IMG_0001.jpg", "rotate") == "1");
  CHECK(ini.get("IMG_0001.jpg", "star") == "yes");
  CHECK(ini.get("IMG_0001.jpg", "keepme") == "1");
  pic::set_star(ini, "IMG_0001.jpg", false);
  CHECK(ini.get("IMG_0001.jpg", "star") == "");
}

TEST_CASE("ini round trip preserves unknown keys") {
  const char* path = "/tmp/opencode/ini_test/picasa.ini";
  mkdir("/tmp/opencode/ini_test", 0755);
  FILE* f = fopen(path, "wb");
  REQUIRE(f);
  fputs("[System]\nDebug=1\nKeep Bryan=42\n[IMG_0001.jpg]\nrotate=1\nstar=yes\nkeywords=a,b\n[Other]\nmisc=x\n",
        f);
  fclose(f);

  IniFile ini;
  REQUIRE(IniFile::load(path, ini));
  CHECK(ini.get("IMG_0001.jpg", "rotate") == "1");
  CHECK(ini.get("IMG_0001.jpg", "star") == "yes");
  CHECK(ini.get("System", "Keep Bryan") == "42");

  ini.set("IMG_0001.jpg", "crop", "rect64(0x1234)");
  ini.remove_key("IMG_0001.jpg", "rotate");
  REQUIRE(ini.save(path));

  IniFile ini2;
  REQUIRE(IniFile::load(path, ini2));
  CHECK(ini2.get("IMG_0001.jpg", "crop") == "rect64(0x1234)");
  CHECK(ini2.get("IMG_0001.jpg", "rotate") == "");
  CHECK(ini2.get("IMG_0001.jpg", "star") == "yes");
  CHECK(ini2.get("IMG_0001.jpg", "keywords") == "a,b");
  CHECK(ini2.get("System", "Keep Bryan") == "42");
  CHECK(ini2.get("Other", "misc") == "x");
}
