/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "input.h"

#include <libretro.h>

#include <SDL.h>

#include <string.h>

#include <deque>

namespace
{
SDL_GameController *g_pad = nullptr;
bool g_prev[PadCount];
bool g_held[512];
bool g_pulsed[512];
bool g_pulse_shift = false;
int g_mouse_x = 0;
int g_mouse_y = 0;
bool g_mouse_left = false;
bool g_mouse_right = false;

struct Pulse
{
    unsigned key;
    bool shift;
    bool releasing;
};

std::deque<Pulse> g_pulses;

int clamp_mouse(int v)
{
    if (v > 24)
        return 24;
    if (v < -24)
        return -24;
    return v;
}

bool trigger(SDL_GameControllerAxis axis)
{
    return g_pad && SDL_GameControllerGetAxis(g_pad, axis) > 8000;
}

bool button(SDL_GameControllerButton id)
{
    return g_pad && SDL_GameControllerGetButton(g_pad, id) != 0;
}
} // namespace

void input_open_pad(int which)
{
    if (g_pad)
        return;
    if (!SDL_IsGameController(which))
        return;
    g_pad = SDL_GameControllerOpen(which);
}

void input_read(PadSnapshot &pad)
{
    memset(&pad, 0, sizeof(pad));
    g_mouse_x = 0;
    g_mouse_y = 0;
    g_mouse_left = false;
    g_mouse_right = false;
    if (!g_pad)
    {
        for (int i = 0; i < SDL_NumJoysticks(); i++)
        {
            input_open_pad(i);
            if (g_pad)
                break;
        }
    }
    bool down[PadCount];
    memset(down, 0, sizeof(down));
    if (g_pad)
    {
        down[PadCross] = button(SDL_CONTROLLER_BUTTON_A);
        down[PadCircle] = button(SDL_CONTROLLER_BUTTON_B);
        down[PadSquare] = button(SDL_CONTROLLER_BUTTON_X);
        down[PadTriangle] = button(SDL_CONTROLLER_BUTTON_Y);
        down[PadL1] = button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        down[PadR1] = button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        down[PadL2] = trigger(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        down[PadR2] = trigger(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        down[PadCreate] = button(SDL_CONTROLLER_BUTTON_BACK);
        down[PadOptions] = button(SDL_CONTROLLER_BUTTON_START);
        down[PadUp] = button(SDL_CONTROLLER_BUTTON_DPAD_UP);
        down[PadDown] = button(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        down[PadLeft] = button(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        down[PadRight] = button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        int lx = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_LEFTX);
        int ly = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_LEFTY);
        if (ly < -16000)
            down[PadUp] = true;
        if (ly > 16000)
            down[PadDown] = true;
        if (lx < -16000)
            down[PadLeft] = true;
        if (lx > 16000)
            down[PadRight] = true;
        int rx = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_RIGHTX);
        int ry = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_RIGHTY);
        if (rx > 7000 || rx < -7000)
            g_mouse_x = clamp_mouse(rx / 1800);
        if (ry > 7000 || ry < -7000)
            g_mouse_y = clamp_mouse(ry / 1800);
        g_mouse_left = down[PadR2];
        g_mouse_right = down[PadL2];
    }
    for (int i = 0; i < PadCount; i++)
    {
        pad.down[i] = down[i];
        pad.pressed[i] = down[i] && !g_prev[i];
        g_prev[i] = down[i];
    }
    pad.mouse_dx = g_mouse_x;
    pad.mouse_dy = g_mouse_y;
    pad.mouse_left = g_mouse_left;
    pad.mouse_right = g_mouse_right;
}

void input_bind_game(const PadSnapshot &pad, bool gameplay)
{
    memset(g_held, 0, sizeof(g_held));
    if (!gameplay)
    {
        g_mouse_left = false;
        g_mouse_right = false;
        return;
    }
    if (pad.down[PadUp])
        g_held[RETROK_UP] = true;
    if (pad.down[PadDown])
        g_held[RETROK_DOWN] = true;
    if (pad.down[PadLeft])
        g_held[RETROK_LEFT] = true;
    if (pad.down[PadRight])
        g_held[RETROK_RIGHT] = true;
    if (pad.down[PadCross])
        g_held[RETROK_RETURN] = true;
    if (pad.down[PadCircle])
        g_held[RETROK_ESCAPE] = true;
    if (pad.down[PadSquare])
        g_held[RETROK_a] = true;
    if (pad.down[PadTriangle])
        g_held[RETROK_m] = true;
    if (pad.down[PadL1])
        g_held[RETROK_KP_MULTIPLY] = true;
    if (pad.down[PadR1])
        g_held[RETROK_KP_MINUS] = true;
}

void input_clear()
{
    memset(g_held, 0, sizeof(g_held));
    memset(g_pulsed, 0, sizeof(g_pulsed));
    g_pulses.clear();
    g_pulse_shift = false;
    g_mouse_x = 0;
    g_mouse_y = 0;
    g_mouse_left = false;
    g_mouse_right = false;
}

void input_pulse(unsigned retrok, bool shift)
{
    if (retrok >= 512)
        return;
    Pulse pulse;
    pulse.key = retrok;
    pulse.shift = shift;
    pulse.releasing = false;
    g_pulses.push_back(pulse);
}

void input_on_poll()
{
    memset(g_pulsed, 0, sizeof(g_pulsed));
    g_pulse_shift = false;
    if (!g_pulses.empty() && g_pulses.front().releasing)
        g_pulses.pop_front();
    if (!g_pulses.empty())
    {
        g_pulsed[g_pulses.front().key] = true;
        g_pulse_shift = g_pulses.front().shift;
        g_pulses.front().releasing = true;
    }
}

int16_t input_key_state(unsigned id)
{
    if (id >= 512)
        return 0;
    if (id == RETROK_LSHIFT && g_pulse_shift)
        return 1;
    if (g_held[id] || g_pulsed[id])
        return 1;
    return 0;
}

int16_t input_mouse_state(unsigned id)
{
    if (id == RETRO_DEVICE_ID_MOUSE_X)
    {
        int v = g_mouse_x;
        g_mouse_x = 0;
        return (int16_t)v;
    }
    if (id == RETRO_DEVICE_ID_MOUSE_Y)
    {
        int v = g_mouse_y;
        g_mouse_y = 0;
        return (int16_t)v;
    }
    if (id == RETRO_DEVICE_ID_MOUSE_LEFT)
        return g_mouse_left ? 1 : 0;
    if (id == RETRO_DEVICE_ID_MOUSE_RIGHT)
        return g_mouse_right ? 1 : 0;
    return 0;
}
