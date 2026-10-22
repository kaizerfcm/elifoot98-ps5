/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "app.h"
#include "core.h"
#include "input.h"
#include "trace.h"
#include "ui.h"

#include <stdlib.h>

extern "C"
{
    int sceKernelAvailableFlexibleMemorySize(size_t *size);
    void *__real_malloc(size_t size);
    void __real_free(void *address);
}

namespace
{
void trace_memory()
{
    size_t flexible = 0;
    sceKernelAvailableFlexibleMemorySize(&flexible);
    size_t heap = 0;
    for (size_t step = 64u << 20; step >= (1u << 20); step /= 2)
    {
        void *probe = __real_malloc(heap + step);
        if (probe)
        {
            __real_free(probe);
            heap += step;
        }
    }
    trace("memoria flexivel livre %zu MB, maior bloco do heap ~%zu MB", flexible >> 20,
          heap >> 20);
}
} // namespace

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    trace("payload iniciado");
    trace_memory();
    std::string error;
    if (!ui_init(error))
    {
        trace("ui_init falhou: %s", error.c_str());
        return 1;
    }
    trace("loop principal, tela %d", (int)app().screen);
    unsigned long loops = 0;
    while (app().running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                app().running = false;
            if (event.type == SDL_CONTROLLERDEVICEADDED)
                input_open_pad(event.cdevice.which);
        }
        PadSnapshot pad;
        input_read(pad);
        ui_handle(pad);
        bool playing = app().screen == Screen::Playing;
        bool typing = app().screen == Screen::Keyboard && app().edit == EditTarget::Game;
        input_bind_game(pad, playing);
        if (app().core_ok && (playing || typing))
            core_frame();
        ui_draw();
        if (++loops == 1 || loops % 600 == 0)
            trace("loop %lu, tela %d", loops, (int)app().screen);
    }
    core_shutdown();
    ui_shutdown();
    trace("encerrado");
    return 0;
}
