//
// Created by Mariotaku on 2021/11/30.
//

#include "nvg_driver.h"

#include <GL/gl.h>

#include <nanovg.h>

static void wait_cb(lv_disp_drv_t *disp_drv);

static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

void lv_nvg_disp_drv_init(lv_disp_drv_t *driver, lv_draw_nvg_context_t *drv_ctx) {
    lv_disp_drv_init(driver);
    lv_disp_draw_buf_t *draw_buf = lv_mem_alloc(sizeof(lv_disp_draw_buf_t));
    lv_disp_draw_buf_init(draw_buf, lv_mem_alloc(1), NULL, 0);
    // in order to make wait_cb to be called
    draw_buf->flushing = 1;
    driver->direct_mode = true;
    driver->draw_buf = draw_buf;
    driver->wait_cb = wait_cb;
    driver->flush_cb = flush_cb;
    driver->user_data = drv_ctx;
}

static void wait_cb(lv_disp_drv_t *disp_drv) {
    lv_draw_nvg_context_t *ctx = (lv_draw_nvg_context_t *) disp_drv->user_data;
    const lv_area_t *area = &disp_drv->draw_buf->area;

    // Set render target to texture buffer
    ctx->callbacks.set_offscreen(ctx, true);
    glViewport(0, 0, disp_drv->hor_res, disp_drv->ver_res);

    nvgBeginFrame(ctx->nvg, lv_area_get_width(area), lv_area_get_height(area), 1);
    nvgResetTransform(ctx->nvg);
    ctx->frame_begun = true;
    disp_drv->draw_buf->flushing = 0;
}

static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    LV_UNUSED(color_p);
    lv_draw_nvg_context_t *ctx = (lv_draw_nvg_context_t *) disp_drv->user_data;
    if (!ctx->frame_begun) {
        disp_drv->draw_buf->flushing = 1;
        return;
    }
    ctx->frame_begun = 0;
    nvgEndFrame(ctx->nvg);

    if (lv_area_get_width(area) <= 0 || lv_area_get_height(area) <= 0) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    // Set render target to window buffer
    ctx->callbacks.set_offscreen(ctx, false);
    glViewport(0, 0, disp_drv->hor_res, disp_drv->ver_res);

    // Wipe window buffer clean
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    ctx->callbacks.submit_buffer(ctx);

    // Swap window buffer
    ctx->callbacks.swap_window(ctx);

    lv_disp_flush_ready(disp_drv);
}