//
// Created by Mariotaku on 2021/11/30.
//

#include "lv_nvg_disp_drv.h"

#include <nanovg.h>
#include <src/draw/nvg/lv_draw_nvg_priv.h>


static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

void lv_nvg_disp_drv_init(lv_disp_drv_t *driver, lv_draw_nvg_context_t *drv_ctx) {
    lv_disp_drv_init(driver);
    lv_disp_draw_buf_t *draw_buf = lv_mem_alloc(sizeof(lv_disp_draw_buf_t));
    lv_disp_draw_buf_init(draw_buf, lv_mem_alloc(1), NULL, 0);
    driver->direct_mode = true;
    driver->draw_buf = draw_buf;
    driver->flush_cb = flush_cb;
    driver->user_data = drv_ctx;
}

void lv_nvg_disp_drv_deinit(lv_disp_drv_t *driver) {
    lv_mem_free(driver->draw_buf->buf1);
    lv_mem_free(driver->draw_buf);
}

static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    LV_UNUSED(color_p);
    lv_draw_nvg_context_t *ctx = (lv_draw_nvg_context_t *) disp_drv->user_data;
    lv_draw_nvg_end_frame(ctx);

    if (lv_area_get_width(area) <= 0 || lv_area_get_height(area) <= 0) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    // Set render target to window buffer
    ctx->callbacks.set_render_buffer(ctx, LV_DRAW_NVG_BUFFER_SCREEN, true);

    nvgBeginFrame(ctx->nvg, lv_area_get_width(area), lv_area_get_height(area), 1);
    ctx->callbacks.fill_buffer(ctx, LV_DRAW_NVG_BUFFER_FRAME, NULL);

    nvgEndFrame(ctx->nvg);

    // Swap window buffer
    ctx->callbacks.swap_window(ctx);

    lv_disp_flush_ready(disp_drv);
}