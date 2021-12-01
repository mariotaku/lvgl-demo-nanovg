#pragma once

#include <lvgl.h>

lv_indev_t *lv_sdl_init_pointer();

void lv_sdl_deinit_pointer(lv_indev_t *dev);