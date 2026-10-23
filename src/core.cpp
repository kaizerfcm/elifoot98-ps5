/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "core.h"

#include "input.h"
#include "trace.h"

#include <libretro.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <vector>

namespace
{
std::string g_dir;
std::string g_conf;
std::string g_log;
bool g_ready = false;
bool g_inited = false;
bool g_loaded = false;
SDL_AudioDeviceID g_audio = 0;
int g_audio_freq = 44100;
SDL_Texture *g_tex = nullptr;
int g_tex_w = 0;
int g_tex_h = 0;
bool g_tex_smooth = true;
std::vector<uint32_t> g_frame;
unsigned g_frame_w = 0;
unsigned g_frame_h = 0;
bool g_dirty = false;
float g_aspect = 4.0f / 3.0f;
uint8_t *g_ram = nullptr;
size_t g_ram_size = 0;
unsigned long g_runs = 0;
unsigned long g_frames = 0;
retro_keyboard_callback g_keys = {};

void forward_key(bool down, unsigned key)
{
    if (g_keys.callback)
        g_keys.callback(down, key, 0, 0);
}

void log_line(const char *text)
{
    trace("dosbox: %s", text);
    g_log += text;
    g_log += "\n";
    if (g_log.size() > 4000)
        g_log.erase(0, g_log.size() - 4000);
}

void logf(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    log_line(buf);
}

void RETRO_CALLCONV log_cb(enum retro_log_level level, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    (void)level;
    log_line(buf);
}

void RETRO_CALLCONV video_cb(const void *data, unsigned width, unsigned height, size_t pitch)
{
    if (!data || data == RETRO_HW_FRAME_BUFFER_VALID || width == 0 || height == 0)
        return;
    if (width > 2048 || height > 2048)
        return;
    if (g_frames == 0 || width != g_frame_w || height != g_frame_h)
        trace("dosbox: quadro %ux%u", width, height);
    g_frames++;
    g_frame_w = width;
    g_frame_h = height;
    g_frame.resize((size_t)width * height);
    const uint8_t *src = (const uint8_t *)data;
    for (unsigned y = 0; y < height; y++)
    {
        memcpy(&g_frame[(size_t)y * width], src + (size_t)y * pitch, (size_t)width * 4);
    }
    g_dirty = true;
}

size_t RETRO_CALLCONV audio_batch_cb(const int16_t *data, size_t frames)
{
    if (g_audio)
    {
        Uint32 queued = SDL_GetQueuedAudioSize(g_audio);
        Uint32 limit = (Uint32)g_audio_freq;
        if (queued < limit)
            SDL_QueueAudio(g_audio, data, (Uint32)(frames * 4));
    }
    return frames;
}

void RETRO_CALLCONV audio_sample_cb(int16_t left, int16_t right)
{
    int16_t frame[2] = {left, right};
    audio_batch_cb(frame, 1);
}

void RETRO_CALLCONV input_poll_cb(void)
{
    input_on_poll();
}

int16_t RETRO_CALLCONV input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)index;
    if (port != 0)
        return 0;
    if (device == RETRO_DEVICE_KEYBOARD)
        return input_key_state(id);
    if (device == RETRO_DEVICE_MOUSE)
        return input_mouse_state(id);
    return 0;
}

bool env_cb(unsigned cmd, void *data)
{
    switch (cmd)
    {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
    {
        auto *cb = (struct retro_log_callback *)data;
        cb->log = log_cb;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return data && *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_XRGB8888;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = g_dir.c_str();
        return !g_dir.empty();
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        if (data)
            *(bool *)data = false;
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE:
    {
        auto *var = (struct retro_variable *)data;
        if (!var || !var->key)
            return false;
        if (!strcmp(var->key, "dosbox_pure_memory_size"))
        {
            var->value = "8";
            return true;
        }
        if (!strcmp(var->key, "dosbox_pure_machine"))
        {
            var->value = "svga";
            return true;
        }
        if (!strcmp(var->key, "dosbox_pure_cpu_type"))
        {
            // pentium_slow checks every memory access. That turns the Windows
            // 3.1 menus into slow motion, which feels like input lag.
            var->value = "auto";
            return true;
        }
        if (!strcmp(var->key, "dosbox_pure_cpu_core"))
        {
            // The dynamic core needs an executable mapping. Keep the
            // interpreter so a rejected mprotect cannot stall a frame.
            var->value = "normal";
            return true;
        }
        if (!strcmp(var->key, "dosbox_pure_cycles"))
        {
            // Fixed 486-class speed, the same cap the game's own conf uses.
            // "auto" with no maximum grows until one frame fills the measured
            // time, so a slow present turns into a long emulated slice.
            var->value = "24000";
            return true;
        }
        if (!strcmp(var->key, "dosbox_pure_voodoo"))
        {
            var->value = "off";
            return true;
        }
        if (!strcmp(var->key, "dosbox_pure_voodoo_perf"))
        {
            var->value = "0";
            return true;
        }
        return false;
    }
    case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
        if (!data)
            return false;
        g_keys = *(retro_keyboard_callback *)data;
        return true;
    case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
        if (data)
            *(bool *)data = false;
        return true;
    default:
        return false;
    }
}

void open_audio(double rate)
{
    if (g_audio)
        return;
    if (rate < 8000 || rate > 96000)
        rate = 44100;
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = (int)rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    SDL_AudioSpec have;
    SDL_zero(have);
    g_audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!g_audio)
    {
        logf("Audio indisponivel: %s", SDL_GetError());
        return;
    }
    g_audio_freq = have.freq > 0 ? have.freq : want.freq;
    SDL_PauseAudioDevice(g_audio, 0);
}

void refresh_ram()
{
    g_ram = (uint8_t *)retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    g_ram_size = retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    logf("RAM do jogo: %u bytes", (unsigned)g_ram_size);
}

bool load_game(std::string &error)
{
    g_conf = g_dir + "/dosbox.conf";
    struct retro_game_info info;
    memset(&info, 0, sizeof(info));
    info.path = g_conf.c_str();
    bool ok = retro_load_game(&info);
    if (!ok)
    {
        error = "O DOSBox Pure nao abriu dosbox.conf.";
        return false;
    }
    g_loaded = true;
    struct retro_system_av_info av;
    memset(&av, 0, sizeof(av));
    retro_get_system_av_info(&av);
    if (av.geometry.aspect_ratio > 0.1f)
        g_aspect = av.geometry.aspect_ratio;
    else if (av.geometry.base_height)
        g_aspect = (float)av.geometry.base_width / (float)av.geometry.base_height;
    open_audio(av.timing.sample_rate);
    refresh_ram();
    input_set_key_edge(forward_key);
    return true;
}
} // namespace

bool core_boot(const std::string &data_dir, std::string &error)
{
    g_dir = data_dir;
    g_log.clear();
    retro_set_environment(env_cb);
    retro_set_video_refresh(video_cb);
    retro_set_audio_sample(audio_sample_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);
    trace("retro_init");
    retro_init();
    g_inited = true;
    trace("retro_load_game %s/dosbox.conf", g_dir.c_str());
    if (!load_game(error))
        return false;
    g_ready = true;
    log_line("Elifoot 98 iniciado.");
    return true;
}

void core_shutdown()
{
    if (g_loaded)
    {
        retro_unload_game();
        g_loaded = false;
    }
    if (g_inited)
    {
        retro_deinit();
        g_inited = false;
    }
    g_ready = false;
    if (g_audio)
    {
        SDL_CloseAudioDevice(g_audio);
        g_audio = 0;
    }
    if (g_tex)
    {
        SDL_DestroyTexture(g_tex);
        g_tex = nullptr;
    }
}

void core_frame()
{
    if (!g_ready)
        return;
    if (g_runs == 0)
        trace("primeiro retro_run");
    retro_run();
    g_runs++;
    if (g_runs == 1 || g_runs % 600 == 0)
        trace("retro_run %lu, quadros %lu (%ux%u)", g_runs, g_frames, g_frame_w, g_frame_h);
}

void core_frame_info(unsigned long &runs, unsigned long &frames)
{
    runs = g_runs;
    frames = g_frames;
}

bool core_reload(std::string &error)
{
    if (g_loaded)
    {
        retro_unload_game();
        g_loaded = false;
    }
    g_ready = false;
    if (!g_inited)
        return core_boot(g_dir, error);
    if (!load_game(error))
        return false;
    g_ready = true;
    return true;
}

bool core_ok()
{
    return g_ready;
}

uint8_t *core_ram(size_t *size)
{
    if (size)
        *size = g_ram_size;
    return g_ram;
}

const std::string &core_log()
{
    return g_log;
}

void core_draw(SDL_Renderer *renderer, bool smooth)
{
    if (!renderer || g_frame.empty() || g_frame_w == 0)
        return;
    if (!g_tex || g_tex_w != (int)g_frame_w || g_tex_h != (int)g_frame_h || g_tex_smooth != smooth)
    {
        if (g_tex)
            SDL_DestroyTexture(g_tex);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, smooth ? "1" : "0");
        g_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                  (int)g_frame_w, (int)g_frame_h);
        g_tex_w = (int)g_frame_w;
        g_tex_h = (int)g_frame_h;
        g_tex_smooth = smooth;
        g_dirty = true;
        if (g_tex)
            SDL_SetTextureScaleMode(g_tex, smooth ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
    }
    if (!g_tex)
        return;
    if (g_dirty)
    {
        SDL_UpdateTexture(g_tex, NULL, g_frame.data(), (int)g_frame_w * 4);
        g_dirty = false;
    }
    int out_w = 0, out_h = 0;
    SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
    if (out_w < 1 || out_h < 1)
        return;
    float screen_aspect = (float)out_w / (float)out_h;
    SDL_Rect dest;
    if (screen_aspect > g_aspect)
    {
        dest.h = out_h;
        dest.w = (int)(out_h * g_aspect);
        dest.x = (out_w - dest.w) / 2;
        dest.y = 0;
    }
    else
    {
        dest.w = out_w;
        dest.h = (int)(out_w / g_aspect);
        dest.x = 0;
        dest.y = (out_h - dest.h) / 2;
    }
    SDL_RenderCopy(renderer, g_tex, NULL, &dest);
}
