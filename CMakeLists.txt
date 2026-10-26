cmake_minimum_required(VERSION 3.16)
project(open-picasa LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

FetchContent_Declare(
  sdl2
  URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.32.8.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(sdl2)
if(NOT sdl2_POPULATED)
  FetchContent_Populate(sdl2)
  set(SDL_TEST_LIBRARY FALSE CACHE BOOL "" FORCE)
  set(SDL_TESTS FALSE CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES FALSE CACHE BOOL "" FORCE)
  set(SDL_SHARED TRUE CACHE BOOL "" FORCE)
  set(SDL_STATIC FALSE CACHE BOOL "" FORCE)
  add_subdirectory(${sdl2_SOURCE_DIR} ${sdl2_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9b.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Populate(imgui)

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_sources(imgui PRIVATE
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
find_package(OpenGL REQUIRED)
target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)
if(UNIX)
  set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

add_library(pic_core STATIC
  src/app/app.cpp
  src/image/image.cpp
  src/image/decode.cpp
  src/image/exif_parse.cpp
  src/edit/ops.cpp
  src/data/picasa_ini.cpp
  src/data/picasa_ini_edit.cpp
  src/scan/scanner.cpp
  src/scan/thumb_cache.cpp
  src/ui/style.cpp
  src/ui/edit_model.cpp
  src/ui/thumbs.cpp
  src/ui/fullscreen.cpp
  src/ui/library_panel.cpp
  src/ui/viewer_panel.cpp
  src/ui/edit_panel.cpp
  src/ui/slideshow_panel.cpp
  src/ui/export_dialog.cpp
)
target_include_directories(pic_core PUBLIC src third_party)
target_link_libraries(pic_core PUBLIC imgui)

add_executable(picasa src/main.cpp)
target_link_libraries(picasa PRIVATE pic_core)

add_executable(picasa_test
  tests/test_main.cpp
  tests/test_picasa_ini.cpp
  tests/test_image_ops.cpp
)
target_link_libraries(picasa_test PRIVATE pic_core)
enable_testing()
add_test(NAME unit COMMAND picasa_test)
