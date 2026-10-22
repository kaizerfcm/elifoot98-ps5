/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "app.h"
#include "core.h"
#include "input.h"
#include "trace.h"
#include "ui.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    trace("payload iniciado");
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
