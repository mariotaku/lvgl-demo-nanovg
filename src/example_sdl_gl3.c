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

#include <GL/glew.h>
#include <SDL2/SDL.h>

#define NANOVG_GL3

#include <nanovg.h>
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>

typedef enum buf_index {
    BUF_INDEX_WINDOW = 0,
    BUF_INDEX_COMPOSITE,
    BUF_INDEX_BLEND_SOURCE,
    BUF_INDEX_BLEND_DESTINATION,
    BUF_INDEX_COUNT,
} buf_index;

int main(int argc, char **argv) {
    int flags = SDL_INIT_EVERYTHING & ~(SDL_INIT_TIMER | SDL_INIT_HAPTIC);
    if (SDL_Init(flags) < 0) {
        printf("ERROR: SDL_Init failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

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

    GLuint textures[BUF_INDEX_COUNT], framebuffers[BUF_INDEX_COUNT];
    int framebuffer_imgs[BUF_INDEX_COUNT];
    SDL_memset(framebuffers, 0, sizeof(framebuffers));
    SDL_memset(textures, 0, sizeof(textures));
    SDL_memset(framebuffer_imgs, 0, sizeof(framebuffer_imgs));
    glGenFramebuffers(BUF_INDEX_COUNT - BUF_INDEX_COMPOSITE, &framebuffers[BUF_INDEX_COMPOSITE]);
    glGenTextures(BUF_INDEX_COUNT - BUF_INDEX_COMPOSITE, &textures[BUF_INDEX_COMPOSITE]);

    for (int i = BUF_INDEX_COMPOSITE; i < BUF_INDEX_COUNT; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbWidth, fbHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        framebuffer_imgs[i] = nvglCreateImageFromHandleGL3(vg, textures[i], fbWidth, fbHeight, NVG_ANTIALIAS);
    }

    int quit = 0;
    SDL_Event event;

    while (!quit) {
        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
            default:
                break;
        };


        {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[BUF_INDEX_WINDOW]);
            glViewport(0, 0, winWidth, winHeight);

            nvgBeginFrame(vg, winWidth, winHeight, fbRatio);

            nvgBeginPath(vg);
            nvgRect(vg, 0, 0, winWidth, winHeight);
            nvgFillColor(vg, nvgRGBA(255, 255, 0, 255));
            nvgFill(vg);
            nvgReset(vg);

            nvgEndFrame(vg);
        };

        {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[BUF_INDEX_BLEND_DESTINATION]);
            glViewport(0, 0, winWidth, winHeight);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            nvgBeginFrame(vg, winWidth, winHeight, fbRatio);
            // Dest circle
            nvgBeginPath(vg);
            nvgCircle(vg, 250, 150, 100);
            nvgFillColor(vg, nvgRGB(232, 28, 100));
            nvgFill(vg);
            nvgEndFrame(vg);

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[BUF_INDEX_BLEND_SOURCE]);
            glViewport(0, 0, winWidth, winHeight);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            nvgBeginFrame(vg, winWidth, winHeight, fbRatio);
            // Source rect
            nvgBeginPath(vg);
            nvgRect(vg, 50, 150, 200, 200);
            nvgFillColor(vg, nvgRGB(42, 151, 240));
            nvgFill(vg);
            nvgEndFrame(vg);


            glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[BUF_INDEX_COMPOSITE]);
            glViewport(0, 0, winWidth, winHeight);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            nvgBeginFrame(vg, winWidth, winHeight, fbRatio);

            nvgScale(vg, 1, -1);
            nvgTranslate(vg, 0, -winHeight);

            nvgBeginPath(vg);
            nvgRect(vg, 0, 0, winWidth, winHeight);
            nvgFillPaint(vg, nvgImagePattern(vg, 0, 0, winWidth, winHeight, 0, framebuffer_imgs[BUF_INDEX_BLEND_DESTINATION], 1));
            nvgFill(vg);

            nvgGlobalCompositeOperation(vg, NVG_XOR);
            nvgBeginPath(vg);
            nvgRect(vg, 0, 0, winWidth, winHeight);
            nvgFillPaint(vg, nvgImagePattern(vg, 0, 0, winWidth, winHeight, 0, framebuffer_imgs[BUF_INDEX_BLEND_SOURCE], 1));
            nvgFill(vg);

            nvgEndFrame(vg);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[BUF_INDEX_WINDOW]);
        glViewport(0, 0, winWidth, winHeight);

        nvgBeginFrame(vg, winWidth, winHeight, fbRatio);

        nvgScale(vg, 1, -1);
        nvgTranslate(vg, 0, -winHeight);

        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, winWidth, winHeight);
        nvgFillPaint(vg, nvgImagePattern(vg, 0, 0, winWidth, winHeight, 0, framebuffer_imgs[BUF_INDEX_COMPOSITE], 1));
        nvgFill(vg);

        nvgEndFrame(vg);

        SDL_GL_SwapWindow(window);
    }

    return EXIT_SUCCESS;
}
