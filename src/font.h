/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <SDL.h>
#include <stdint.h>
#include <string>

void font_draw(SDL_Renderer *renderer, int x, int y, const char *utf8, SDL_Color color, int scale);
int font_width(const char *utf8, int scale);
int font_height(int scale);
void font_draw_wrapped(SDL_Renderer *renderer, int x, int y, int max_width, const std::string &utf8,
                       SDL_Color color, int scale, int scroll);
int font_wrapped_height(int max_width, const std::string &utf8, int scale);
