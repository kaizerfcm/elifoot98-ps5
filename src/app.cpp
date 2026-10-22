/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "app.h"

#include "core.h"
#include "patch.h"
#include "util.h"

namespace
{
App g_app;
}

App &app()
{
    return g_app;
}

void app_status(const std::string &text)
{
    g_app.status = text;
    g_app.status_ttl = 240;
}

bool app_reload(const std::string &ok_message)
{
    std::string error;
    if (!core_reload(error))
    {
        g_app.core_ok = false;
        g_app.boot_error = error;
        g_app.screen = Screen::Boot;
        return false;
    }
    size_t bytes = 0;
    uint8_t *ram = core_ram(&bytes);
    g_app.omatic.bind(ram, bytes, path_join(g_app.data_dir, "cheats.txt"));
    g_app.core_ok = true;
    g_app.screen = Screen::Playing;
    app_status(ok_message);
    return true;
}
