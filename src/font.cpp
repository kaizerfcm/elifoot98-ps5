/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "font.h"

#include <string.h>

#include "font8x8_basic.h"
#include "font8x8_ext_latin.h"

namespace
{
uint32_t utf8_next(const char *&s)
{
    const unsigned char *p = (const unsigned char *)s;
    if (*p == 0)
        return 0;
    if (*p < 0x80)
    {
        s += 1;
        return *p;
    }
    if ((*p & 0xE0) == 0xC0 && p[1])
    {
        uint32_t cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        s += 2;
        return cp;
    }
    if ((*p & 0xF0) == 0xE0 && p[1] && p[2])
    {
        uint32_t cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        s += 3;
        return cp;
    }
    s += 1;
    return '?';
}

const unsigned char *glyph(uint32_t cp)
{
    static const unsigned char empty[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    if (cp < 128)
        return (const unsigned char *)font8x8_basic[cp];
    if (cp >= 0xA0 && cp <= 0xFF)
        return (const unsigned char *)font8x8_ext_latin[cp - 0xA0];
    return empty;
}

void blit(SDL_Renderer *renderer, int x, int y, uint32_t cp, int scale)
{
    const unsigned char *rows = glyph(cp);
    for (int row = 0; row < 8; row++)
    {
        unsigned char bits = rows[row];
        for (int col = 0; col < 8; col++)
        {
            if ((bits & (1u << col)) == 0)
                continue;
            SDL_Rect rect{x + col * scale, y + row * scale, scale, scale};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}
} // namespace

int font_height(int scale)
{
    return 8 * scale;
}

int font_width(const char *utf8, int scale)
{
    int n = 0;
    const char *s = utf8;
    while (utf8_next(s))
        n++;
    return n * 8 * scale;
}

void font_draw(SDL_Renderer *renderer, int x, int y, const char *utf8, SDL_Color color, int scale)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const char *s = utf8;
    int pen = x;
    for (;;)
    {
        uint32_t cp = utf8_next(s);
        if (!cp)
            break;
        blit(renderer, pen, y, cp, scale);
        pen += 8 * scale;
    }
}

static int take_line(const std::string &text, size_t &pos, int max_width, int scale,
                     std::string &line)
{
    line.clear();
    if (pos >= text.size())
        return 0;
    if (text[pos] == '\n')
    {
        pos++;
        return 1;
    }
    size_t start = pos;
    size_t last_space = std::string::npos;
    const char *s = text.c_str() + pos;
    const char *begin = s;
    std::string trial;
    while (*s)
    {
        if (*s == '\n')
            break;
        const char *next = s;
        uint32_t cp = 0;
        {
            const char *tmp = s;
            cp = utf8_next(tmp);
            next = tmp;
        }
        (void)cp;
        trial.append(s, next);
        if (font_width(trial.c_str(), scale) > max_width && !line.empty())
            break;
        line.append(s, next);
        if (*s == ' ')
            last_space = line.size();
        s = next;
        if (font_width(line.c_str(), scale) > max_width)
            break;
    }
    if (*s && *s != '\n' && last_space != std::string::npos && last_space > 0)
    {
        line.resize(last_space);
        pos = start + line.size();
        if (pos < text.size() && text[pos] == ' ')
            pos++;
    }
    else
    {
        pos = start + (size_t)(s - begin);
        if (pos < text.size() && text[pos] == '\n')
            pos++;
    }
    return 1;
}

void font_draw_wrapped(SDL_Renderer *renderer, int x, int y, int max_width, const std::string &utf8,
                       SDL_Color color, int scale, int scroll)
{
    size_t pos = 0;
    int row = 0;
    int line_h = font_height(scale) + scale;
    std::string line;
    while (take_line(utf8, pos, max_width, scale, line))
    {
        if (row >= scroll)
            font_draw(renderer, x, y + (row - scroll) * line_h, line.c_str(), color, scale);
        row++;
        if (y + (row - scroll) * line_h > 1080)
            break;
    }
}

int font_wrapped_height(int max_width, const std::string &utf8, int scale)
{
    size_t pos = 0;
    int row = 0;
    std::string line;
    while (take_line(utf8, pos, max_width, scale, line))
        row++;
    return row;
}
