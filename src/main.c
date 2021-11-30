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
#include <SDL_image.h>

#include <GL/glew.h>

#include <nanovg.h>
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>

#include <lvgl.h>
#include <src/draw/nvg/lv_draw_nvg.h>
#include "lv_demo.h"

#include "nvg_driver.h"

typedef struct lv_draw_nvg_context_userdata_t {
    SDL_Window *window;
    GLuint framebuffer;
    int framebuffer_img;
    int width, height;
} nvg_sdl_userdata;

static void nvg_sdl_set_offscreen(lv_draw_nvg_context_t *context, bool offscreen);

static void nvg_sdl_submit_buffer(lv_draw_nvg_context_t *context);

static void nvg_sdl_swap_window(lv_draw_nvg_context_t *context);

int main(int argc, char **argv) {
    int flags = SDL_INIT_EVERYTHING & ~(SDL_INIT_TIMER | SDL_INIT_HAPTIC);
    if (SDL_Init(flags) < 0) {
        printf("ERROR: SDL_Init failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    lv_init();

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Try with these GL attributes
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    SDL_Window *window = SDL_CreateWindow("NanoVG Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 800,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN |
                                          SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window) { // If it fails, try with more conservative options
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

        window = SDL_CreateWindow("Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 800,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN |
                                  SDL_WINDOW_ALLOW_HIGHDPI);

        if (!window) { // We were not able to create the window
            printf("ERROR: SDL_CreateWindow failed: %s", SDL_GetError());
            return EXIT_FAILURE;
        }
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    NVGcontext *vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
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


    GLuint framebuffers[1], textures[1];
    SDL_memset(framebuffers, 0, sizeof(framebuffers));
    SDL_memset(textures, 0, sizeof(textures));
    glGenFramebuffers(1, framebuffers);
    glGenTextures(1, textures);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    glBindTexture(GL_TEXTURE_2D, textures[0]);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbWidth, fbHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    int fbimg = nvglCreateImageFromHandleGL3(vg, textures[0], fbWidth, fbHeight, NVG_ANTIALIAS);

    nvg_sdl_userdata sdl_ctx = {
            .window = window,
            .framebuffer = framebuffers[0],
            .framebuffer_img = fbimg,
            .width = winWidth,
            .height = winHeight,
    };
    lv_draw_nvg_context_t driver_ctx = {
            .nvg = vg,
            .callbacks = {
                    .set_offscreen = nvg_sdl_set_offscreen,
                    .submit_buffer = nvg_sdl_submit_buffer,
                    .swap_window = nvg_sdl_swap_window
            },
            .userdata = &sdl_ctx,
    };

    lv_nvg_disp_drv_init(&driver, &driver_ctx);
    driver.hor_res = winWidth;
    driver.ver_res = winHeight;
    lv_disp_t *disp = lv_disp_drv_register(&driver);

    lv_theme_t *th = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                           LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);

    lv_draw_backend_t backend;
    lv_draw_nvg_init(&backend);
    lv_draw_backend_add(&backend);

    lv_demo_widgets();

    int quit = 0;
    SDL_Event event;

    while (!quit) {
        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
        }
        lv_timer_handler();
        SDL_Delay(1);
    }

    return EXIT_SUCCESS;
}

static void nvg_sdl_set_offscreen(lv_draw_nvg_context_t *context, bool offscreen) {
    glBindFramebuffer(GL_FRAMEBUFFER, offscreen ? context->userdata->framebuffer : 0);
}

static void nvg_sdl_submit_buffer(lv_draw_nvg_context_t *context) {
    NVGcontext *vg = context->nvg;
    int winWidth = context->userdata->width, winHeight = context->userdata->height, fbRatio = 1;
    nvgBeginFrame(vg, winWidth, winHeight, fbRatio);
    nvgResetTransform(vg);
    nvgScale(vg, 1, -1);
    nvgTranslate(vg, 0, -winHeight);

    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, winWidth, winHeight);
    NVGpaint paint = nvgImagePattern(vg, 0, 0, winWidth, winHeight, 0, context->userdata->framebuffer_img, 1.0f);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
    nvgEndFrame(vg);
}

static void nvg_sdl_swap_window(lv_draw_nvg_context_t *context) {
    SDL_GL_SwapWindow(context->userdata->window);
}