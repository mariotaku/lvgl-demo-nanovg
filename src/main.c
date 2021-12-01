//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#if NANOVG_GLES2
#include <GLES2/gl2.h>
#else

#include <GL/glew.h>

#endif

#include <nanovg.h>
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>

#include <lvgl.h>
#include <src/draw/nvg/lv_draw_nvg.h>
#include <src/draw/nvg/lv_draw_nvg_priv.h>
#include <src/draw/nvg/lv_draw_nvg_img_cache.h>
#include "lv_demo.h"

#include "lv_nvg_disp_drv.h"
#include "lv_sdl_drv_input.h"

static bool quit = false;

typedef struct lv_draw_nvg_context_userdata_t {
    SDL_Window *window;
    GLuint framebuffers[LV_DRAW_NVG_BUFFER_COUNT];
    int framebuffer_imgs[LV_DRAW_NVG_BUFFER_COUNT];
} nvg_sdl_userdata;

#define LV_DRAW_NVG_BUFFER_BEGIN LV_DRAW_NVG_BUFFER_FRAME

static int sdl_handle_event(void *userdata, SDL_Event *event);

static void nvg_sdl_set_render_buffer(lv_draw_nvg_context_t *context, lv_draw_nvg_buffer_index index, bool clear);

static void
nvg_sdl_submit_buffer(lv_draw_nvg_context_t *context, lv_draw_nvg_buffer_index index, const lv_area_t *a);

static void nvg_sdl_swap_window(lv_draw_nvg_context_t *context);

static SDL_Window *create_gl_window();

static NVGcontext *create_nvg();

static void nvg_sdl_userdata_init(nvg_sdl_userdata *userdata, SDL_Window *window, NVGcontext *nvg);

int main(int argc, char **argv) {
    int flags = SDL_INIT_EVERYTHING & ~(SDL_INIT_TIMER | SDL_INIT_HAPTIC);
    if (SDL_Init(flags) < 0) {
        printf("ERROR: SDL_Init failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    lv_init();

    SDL_Window *window = create_gl_window();

    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    SDL_GL_SetSwapInterval(0);

    NVGcontext *vg = create_nvg();
    if (vg == NULL) {
        printf("ERROR: NanoVG init failed");
        return EXIT_FAILURE;
    }

    int winWidth = 0, winHeight = 0;
    SDL_GetWindowSize(window, &winWidth, &winHeight);

    int fbWidth = winWidth;
    int fbHeight = winHeight;
    float fbRatio = (float) fbWidth / (float) winWidth;

    lv_disp_drv_t driver;

    nvg_sdl_userdata sdl_ctx;
    nvg_sdl_userdata_init(&sdl_ctx, window, vg);

    lv_draw_nvg_callbacks_t callbacks = {
            .set_render_buffer = nvg_sdl_set_render_buffer,
            .fill_buffer = nvg_sdl_submit_buffer,
            .swap_window = nvg_sdl_swap_window
    };
    lv_draw_nvg_context_t driver_ctx;
    lv_draw_nvg_context_init(&driver_ctx, vg, &callbacks);
    driver_ctx.userdata = &sdl_ctx;

    lv_nvg_disp_drv_init(&driver, &driver_ctx);
    driver.hor_res = winWidth;
    driver.ver_res = winHeight;
    lv_disp_drv_register(&driver);

    lv_nvg_draw_cache_init(driver_ctx.nvg);

    lv_draw_backend_t backend;
    lv_draw_nvg_init(&backend);
    lv_draw_backend_add(&backend);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    lv_indev_t *indev_pointer = lv_sdl_init_pointer();

    lv_demo_widgets();
//    lv_demo_music();
//    lv_demo_benchmark();

    while (!quit) {
        SDL_PumpEvents();
        SDL_FilterEvents(sdl_handle_event, NULL);

        lv_timer_handler();
        SDL_Delay(1);
    }

    lv_nvg_draw_cache_deinit();

    lv_nvg_disp_drv_deinit(&driver);

    lv_sdl_deinit_pointer(indev_pointer);
    lv_deinit();

    SDL_GL_DeleteContext(context);


    SDL_Quit();

    return EXIT_SUCCESS;
}


static SDL_Window *create_gl_window() {
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#if NANOVG_GLES2
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#elif NANOVG_GL3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
#error "No GL version epcified"
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Try with these GL attributes
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    SDL_Window *window = SDL_CreateWindow("NanoVG Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 480,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN |
                                          SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window) { // If it fails, try with more conservative options
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

        window = SDL_CreateWindow("Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 480,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN |
                                  SDL_WINDOW_ALLOW_HIGHDPI);

        if (!window) { // We were not able to create the window
            printf("ERROR: SDL_CreateWindow failed: %s", SDL_GetError());
            window = NULL;
        }
    }
    return window;
}

static NVGcontext *create_nvg() {
    NVGcontext *vg;
#if NANOVG_GLES2
    vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#elif NANOVG_GL3
    vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
#error "No GL version epcified"
#endif
    return vg;
}

static void nvg_sdl_set_render_buffer(lv_draw_nvg_context_t *context, lv_draw_nvg_buffer_index index, bool clear) {
    glBindFramebuffer(GL_FRAMEBUFFER, context->userdata->framebuffers[index]);
    int w, h;
    SDL_GL_GetDrawableSize(context->userdata->window, &w, &h);
    glViewport(0, 0, w, h);
    if (clear) {
        glClearColor(1, 1, 1, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

static void nvg_sdl_submit_buffer(lv_draw_nvg_context_t *context, lv_draw_nvg_buffer_index index, const lv_area_t *a) {
    NVGcontext *vg = context->nvg;
    int w, h;
    SDL_GL_GetDrawableSize(context->userdata->window, &w, &h);
    int winWidth = w, winHeight = h, fbRatio = 1;

    int image = context->userdata->framebuffer_imgs[index];
    if (!image) return;

    nvgScale(vg, 1, -1);
    nvgTranslate(vg, 0, -winHeight);

    nvgBeginPath(vg);
    if (a) {
        nvgRect(vg, a->x1, a->y1, lv_area_get_width(a), lv_area_get_height(a));
    } else {
        nvgRect(vg, 0, 0, winWidth, winHeight);
    }

    NVGpaint paint = nvgImagePattern(vg, 0, 0, winWidth, winHeight, 0,
                                     image, 1.0f);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
}

static void nvg_sdl_swap_window(lv_draw_nvg_context_t *context) {
    SDL_GL_SwapWindow(context->userdata->window);
}

static int sdl_handle_event(void *userdata, SDL_Event *event) {
    switch (event->type) {
        case SDL_QUIT:
            quit = true;
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            return 1;
        default:
            return 0;
    }
    return 0;
}

static void nvg_sdl_userdata_init(nvg_sdl_userdata *userdata, SDL_Window *window, NVGcontext *nvg) {
    userdata->window = window;
    int fbWidth, fbHeight;
    SDL_GL_GetDrawableSize(window, &fbWidth, &fbHeight);

    GLuint textures[LV_DRAW_NVG_BUFFER_COUNT];
    SDL_memset(userdata->framebuffers, 0, sizeof(userdata->framebuffers));
    SDL_memset(textures, 0, sizeof(textures));
    SDL_memset(userdata->framebuffer_imgs, 0, sizeof(userdata->framebuffer_imgs));
    glGenFramebuffers(LV_DRAW_NVG_BUFFER_COUNT - LV_DRAW_NVG_BUFFER_BEGIN,
                      &userdata->framebuffers[LV_DRAW_NVG_BUFFER_BEGIN]);
    glGenTextures(LV_DRAW_NVG_BUFFER_COUNT - LV_DRAW_NVG_BUFFER_BEGIN, &textures[LV_DRAW_NVG_BUFFER_BEGIN]);

    for (int i = LV_DRAW_NVG_BUFFER_BEGIN; i < LV_DRAW_NVG_BUFFER_COUNT; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, userdata->framebuffers[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        if (i == LV_DRAW_NVG_BUFFER_MASK) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, fbWidth, fbHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbWidth, fbHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
#if NANOVG_GLES2
        userdata->framebuffer_imgs[i] = nvglCreateImageFromHandleGLES2(nvg, textures[i], fbWidth, fbHeight, NVG_ANTIALIAS);
#elif NANOVG_GL3
        userdata->framebuffer_imgs[i] = nvglCreateImageFromHandleGL3(nvg, textures[i], fbWidth, fbHeight,
                                                                     NVG_ANTIALIAS);
#else
#error "No GL version epcified"
#endif
    }

}