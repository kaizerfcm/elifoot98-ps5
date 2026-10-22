/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "trace.h"

#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern "C" int sceKernelDebugOutText(int channel, const char *text);

namespace
{
FILE *g_file = nullptr;
bool g_file_tried = false;

void open_file()
{
    g_file_tried = true;
    if (!mkdir_p("/data/elifoot98"))
        return;
    g_file = fopen("/data/elifoot98/elifoot98.log", "w");
}
} // namespace

void trace(const char *fmt, ...)
{
    char buf[1024];
    int at = snprintf(buf, sizeof(buf), "[elifoot98] ");
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + at, sizeof(buf) - at - 1, fmt, ap);
    va_end(ap);
    size_t len = strlen(buf);
    if (len == 0 || buf[len - 1] != '\n')
    {
        buf[len++] = '\n';
        buf[len] = 0;
    }
    sceKernelDebugOutText(0, buf);
    if (!g_file_tried)
        open_file();
    if (g_file)
    {
        fputs(buf, g_file);
        fflush(g_file);
    }
}
