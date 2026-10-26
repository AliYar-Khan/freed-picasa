#include "doctest.h"
#include "edit/ops.h"
#include "image/image.h"
#include <cstring>
#include <sys/stat.h>

using namespace pic;

TEST_CASE("rotate 4 times returns to original") {
  Image im(3, 2);
  for (int y = 0; y < 2; y++)
    for (int x = 0; x < 3; x++) {
      uint8_t* p = im.row(y) + x * 4;
      p[0] = uint8_t(x + y * 3);
      p[1] = 1;
      p[2] = 2;
      p[3] = 255;
    }
  Image orig = im;
  for (int i = 0; i < 4; i++) rotate90(im, false);
  CHECK(im.w == orig.w);
  CHECK(im.h == orig.h);
  CHECK(memcmp(im.px.data(), orig.px.data(), im.px.size()) == 0);
}

TEST_CASE("crop 1x1 at origin") {
  Image im(4, 4);
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 4; x++) {
      uint8_t* p = im.row(y) + x * 4;
      p[0] = uint8_t(x);
      p[1] = uint8_t(y);
      p[2] = 0;
      p[3] = 255;
    }
  crop(im, 0, 0, 2, 2);
  CHECK(im.w == 2);
  CHECK(im.h == 2);
  CHECK(im.row(0)[0] == 0);
  CHECK(im.row(0)[4] == 1);
}

TEST_CASE("edit state dirty flags") {
  EditState s;
  CHECK_FALSE(s.dirty());
  edit_add_rotation(s, 1);
  CHECK(s.rotation == 1);
  edit_add_rotation(s, 3);
  CHECK(s.rotation == 0);
  CHECK_FALSE(s.dirty());
  edit_set_crop(s, true, .1f, .1f, .5f, .5f);
  CHECK(s.dirty());
}

TEST_CASE("tuning zero detection and brightness LUT direction") {
  Tuning t;
  CHECK(t.zero());
  Image im(4, 4);
  im.px.assign(im.px.size(), 90);
  t.brightness = 0.5f;
  apply_tuning(im, t);
  CHECK(int(im.row(0)[0]) > 90);
  Image im2(4, 4);
  im2.px.assign(im2.px.size(), 90);
  t.brightness = -0.5f;
  t.contrast = 0;
  apply_tuning(im2, t);
  CHECK(int(im2.row(0)[0]) < 90);
}

TEST_CASE("redeye reduces red dominance") {
  Image im(16, 16);
  for (auto& b : im.px) b = 255;
  for (int y = 4; y < 12; y++)
    for (int x = 4; x < 12; x++) {
      uint8_t* p = im.row(y) + x * 4;
      p[0] = 255;
      p[1] = 40;
      p[2] = 40;
    }
  RedEyeSpot s;
  s.x = 0.5f;
  s.y = 0.5f;
  s.r = 0.1f;
  std::vector<RedEyeSpot> spots{s};
  apply_redeye(im, spots);
  // center pixel should no longer be strongly red
  uint8_t* c = im.row(8) + 8 * 4;
  CHECK(c[0] < 255);
  CHECK(c[1] == 40);
  CHECK(c[2] == 40);
}

TEST_CASE("jpeg save/load roundtrip") {
  Image im(64, 48);
  for (int y = 0; y < 48; y++)
    for (int x = 0; x < 64; x++) {
      uint8_t* p = im.row(y) + x * 4;
      p[0] = uint8_t(x * 4);
      p[1] = uint8_t(y * 5);
      p[2] = 60;
      p[3] = 255;
    }
  std::string path = "/tmp/opencode/ini_test/rt.jpg";
  REQUIRE(save_jpeg(path, im, 90));
  Image out;
  REQUIRE(load_rgba8(path, out));
  CHECK(out.w == 64);
  CHECK(out.h == 48);
  CHECK(int(out.row(10)[40]) > 20);
}
