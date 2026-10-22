/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "input.h"

#include <string>

bool ui_init(std::string &error);
void ui_handle(const PadSnapshot &pad);
void ui_draw();
void ui_shutdown();
