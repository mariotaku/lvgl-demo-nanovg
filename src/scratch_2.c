void render() {

    driver_ctx.callbacks.set_offscreen(&driver_ctx, true);
    glViewport(0, 0, fbWidth, fbHeight);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    int x, y;
    SDL_GetMouseState(&x, &y);

    nvgBeginFrame(vg, winWidth, winHeight, fbRatio);
    nvgResetTransform(vg);
    nvgTranslate(vg, x - 100, y - 100);
    nvgBeginPath(vg);
    nvgRect(vg, 100, 100, 100, 100);
    nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRect(vg, 150, 150, 100, 100);
    nvgFillColor(vg, nvgRGBA(0, 255, 0, 255));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRect(vg, 200, 200, 100, 100);
    nvgFillColor(vg, nvgRGBA(0, 0, 255, 255));
    nvgFill(vg);
    nvgEndFrame(vg);

    // Reset to window
    driver_ctx.callbacks.set_offscreen(&driver_ctx, false);
    glViewport(0, 0, winWidth, winHeight);

    glClearColor(1.0f, 1.0f, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);


    driver_ctx.callbacks.submit_buffer(&driver_ctx);
    driver_ctx.callbacks.swap_window(&driver_ctx);
}