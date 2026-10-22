/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <string>

bool core_boot(const std::string &data_dir, std::string &error);
void core_shutdown();
void core_frame();
void core_frame_info(unsigned long &runs, unsigned long &frames);
bool core_reload(std::string &error);
bool core_ok();
uint8_t *core_ram(size_t *size);
const std::string &core_log();
void core_draw(SDL_Renderer *renderer, bool smooth);
