cmake_minimum_required(VERSION 3.16)
project(demo C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

pkg_check_modules(SDL2 sdl2 SDL2_image REQUIRED)
pkg_check_modules(GLEW glew REQUIRED)

option(LV_CONF_INCLUDE_SIMPLE "Use lv_conf.h" OFF)
option(DEMO_GLES "Use GLES" OFF)

add_subdirectory(lvgl)
add_subdirectory(lv_demos)
add_subdirectory(nanovg)

target_link_libraries(nanovg PUBLIC ${FREETYPE_LIBRARIES} m)
target_include_directories(nanovg SYSTEM PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(nanovg PUBLIC FONS_USE_FREETYPE NVG_NO_STB)

target_link_libraries(lvgl PUBLIC nanovg)

add_executable(demo src/main.c src/lv_nvg_disp_drv.c src/nvg_backend.c src/lv_sdl_drv_pointer_input.c)
target_include_directories(demo PRIVATE nanovg/example)
target_compile_definitions(demo PRIVATE NANOVG_USE_GLEW)
target_link_libraries(demo PRIVATE lvgl lv_demos nanovg ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}
        ${SDL2_LIBRARIES})

if (DEMO_GLES)
    target_link_libraries(demo PRIVATE nanovg_gles2)
    target_compile_definitions(demo PRIVATE NANOVG_GLES2)
else ()
    target_link_libraries(demo PRIVATE nanovg_gl3)
    target_compile_definitions(demo PRIVATE NANOVG_GL3)
endif ()

target_include_directories(demo SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})

add_executable(example_sdl_gl3 src/example_sdl_gl3.c)
target_compile_definitions(example_sdl_gl3 PRIVATE NANOVG_USE_GLEW)
target_link_libraries(example_sdl_gl3 PRIVATE nanovg nanovg_gl3 ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL2_LIBRARIES})