#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "ui/library_panel.h"
#include "ui/viewer_panel.h"
#include "ui/style.h"
#include <SDL.h>
#include <GL/gl.h>
#include <stdio.h>
#include <string>

int main(int argc, char** argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_Window* window = SDL_CreateWindow("open-picasa", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        1280, 800,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                            SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_GLContext ctx = SDL_GL_CreateContext(window);
  if (!ctx) {
    fprintf(stderr, "GL context failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_GL_MakeCurrent(window, ctx);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = "imgui.ini";
  pic::pic_style_light();

  ImGui_ImplSDL2_InitForOpenGL(window, ctx);
  ImGui_ImplOpenGL3_Init("#version 130");

  pic::app()->ensure_started();
  pic::app()->thumbs.init(pic::app()->cache_dir, 640);

  if (argc > 1) {
    pic::app()->scanner.add_root(argv[1]);
    pic::app()->scanner.rescan_async();
  }

  bool done = false;
  while (!done) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      ImGui_ImplSDL2_ProcessEvent(&ev);
      if (ev.type == SDL_QUIT) done = true;
      if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE) done = true;
      if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F11) {
        Uint32 f = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
        SDL_SetWindowFullscreen(window, f ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
        ev.key.keysym.sym = 0;
      }
      if (ev.type == SDL_KEYDOWN) {
        // global app shortcuts
        SDL_Keycode k = ev.key.keysym.sym;
        if (k == SDLK_LEFT && pic::app()->mode == pic::UIMode::Viewer) pic::app()->next_photo(-1);
        if (k == SDLK_RIGHT && pic::app()->mode == pic::UIMode::Viewer) pic::app()->next_photo(1);
        if (k == SDLK_ESCAPE && pic::app()->mode == pic::UIMode::Viewer) pic::app()->leave_viewer();
        if ((k == SDLK_F5)) pic::app()->scanner.rescan_async();
      }
    }
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    pic::app()->thumbs.upload_pending();
    ImGui::NewFrame();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin("##host", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar(3);
    switch (pic::app()->mode) {
      case pic::UIMode::Library:
        pic::draw_library_panel();
        break;
      case pic::UIMode::Viewer:
        pic::draw_viewer_panel();
        break;
      case pic::UIMode::Slideshow:
        pic::draw_slideshow_panel();
        break;
    }
    ImGui::End();
    ImGui::Render();
    glViewport(0, 0, int(io.DisplaySize.x * io.DisplayFramebufferScale.x),
               int(io.DisplaySize.y * io.DisplayFramebufferScale.y));
    glClearColor(0.94f, 0.94f, 0.94f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
  pic::app()->thumbs.stop();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
