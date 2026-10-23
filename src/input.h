/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <stdint.h>

enum PadId
{
    PadCross,
    PadCircle,
    PadSquare,
    PadTriangle,
    PadL1,
    PadR1,
    PadL2,
    PadR2,
    PadCreate,
    PadOptions,
    PadUp,
    PadDown,
    PadLeft,
    PadRight,
    PadCount
};

struct PadSnapshot
{
    bool down[PadCount];
    bool pressed[PadCount];
    int mouse_dx;
    int mouse_dy;
    bool mouse_left;
    bool mouse_right;
};

void input_open_pad(int which);
void input_read(PadSnapshot &pad);
void input_bind_game(const PadSnapshot &pad, bool gameplay);
void input_clear();
void input_pulse(unsigned retrok, bool shift);
int16_t input_key_state(unsigned id);
int16_t input_mouse_state(unsigned id);
void input_on_poll();
typedef void (*InputKeyEdge)(bool down, unsigned retrok);
void input_set_key_edge(InputKeyEdge edge);
