/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "app.h"
#include "core.h"
#include "input.h"
#include "ui.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("elifoot98: payload iniciado\n");
    fflush(stdout);
    std::string error;
    if (!ui_init(error))
    {
        printf("elifoot98: %s\n", error.c_str());
        fflush(stdout);
        return 1;
    }
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
    }
    core_shutdown();
    ui_shutdown();
    printf("elifoot98: encerrado\n");
    fflush(stdout);
    return 0;
}
