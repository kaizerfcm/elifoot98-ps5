/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <string>
#include <utility>
#include <vector>

struct PatchFile
{
    std::string relative;
    std::vector<unsigned char> data;
};

std::vector<std::string> find_patch_zips();
std::string prepare_patch(const std::string &zip_path, std::vector<PatchFile> &files);
std::string apply_patch(const std::string &data_dir, const std::vector<PatchFile> &files);
std::string clear_patch(const std::string &data_dir);
void install_boot_bats(const std::string &data_dir);
