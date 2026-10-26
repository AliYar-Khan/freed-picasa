#pragma once
// Small GL helpers. GL calls must be on the repaint thread.
#include "image/image.h"

namespace pic {
unsigned upload_rgba(const struct Image& im);      // creates GL_RGBA texture
void free_tex(unsigned);
}
