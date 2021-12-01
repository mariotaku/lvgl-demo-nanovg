#pragma once

#include <lvgl.h>
#include "src/draw/nvg/lv_draw_nvg.h"

void lv_nvg_disp_drv_init(lv_disp_drv_t *driver, lv_draw_nvg_context_t *drv_ctx);

void lv_nvg_disp_drv_deinit(lv_disp_drv_t *driver);
