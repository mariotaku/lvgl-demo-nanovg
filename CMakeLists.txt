cmake_minimum_required(VERSION 3.20)
project(lvgl_demo_nanovg C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

pkg_check_modules(SDL2 sdl2 SDL2_image REQUIRED)
pkg_check_modules(GLEW glew REQUIRED)

option(LV_CONF_INCLUDE_SIMPLE "Use lv_conf.h" OFF)
add_subdirectory(lvgl)
add_subdirectory(lv_demos)
add_subdirectory(nanovg)

target_link_libraries(nanovg PUBLIC ${FREETYPE_LIBRARIES})
target_include_directories(nanovg SYSTEM PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(nanovg PUBLIC FONS_USE_FREETYPE)

target_link_libraries(lvgl PUBLIC nanovg)

add_executable(lvgl_demo_nanovg src/main.c src/nvg_driver.c src/nvg_backend.c)
target_include_directories(lvgl_demo_nanovg PRIVATE nanovg/example)
target_compile_definitions(lvgl_demo_nanovg PRIVATE NANOVG_USE_GLEW)
target_link_libraries(lvgl_demo_nanovg PRIVATE lvgl lv_demos nanovg nanovg_gl3 ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}
        ${SDL2_LIBRARIES})
target_include_directories(lvgl_demo_nanovg SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
