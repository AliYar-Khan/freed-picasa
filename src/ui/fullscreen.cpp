#include <SDL.h>
#include "ui/library_panel.h"
#include <SDL.h>

namespace pic {
void toggle_fullscreen() {
  SDL_Window* w = SDL_GL_GetCurrentWindow();
  if (!w) return;
  SDL_SetWindowFullscreen(w, (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN_DESKTOP)
                                 ? 0
                                 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}
}  // namespace pic
