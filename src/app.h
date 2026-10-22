/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "omatic.h"
#include "patch.h"

#include <SDL.h>

#include <string>
#include <vector>

enum class Screen
{
    Boot,
    Playing,
    Menu,
    Formations,
    Keyboard,
    Files,
    Confirm,
    Text,
    Omatic,
    Cheats
};

enum class FileKind
{
    Save,
    Patch
};
enum class ConfirmKind
{
    None,
    Wipe,
    ClearPatch,
    Quit,
    Restart,
    ApplyPatch,
    ImportSave
};
enum class EditTarget
{
    Game,
    Omatic,
    CheatName
};

struct App
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    bool running = true;
    bool smooth = true;
    bool core_ok = false;
    std::string data_dir;
    std::string boot_error;
    Screen screen = Screen::Boot;
    int cursor = 0;
    int scroll = 0;
    std::string status;
    int status_ttl = 0;
    bool shift = false;
    int key_row = 0;
    int key_col = 0;
    EditTarget edit = EditTarget::Game;
    FileKind file_kind = FileKind::Save;
    std::vector<std::string> files;
    ConfirmKind confirm = ConfirmKind::None;
    std::string confirm_text;
    std::vector<PatchFile> pending_patch;
    std::string pending_import;
    std::string text_title;
    std::string text_body;
    int text_scroll = 0;
    std::string cheat_name;
    Omatic omatic;
};

App &app();
void app_status(const std::string &text);
bool app_reload(const std::string &ok_message);
