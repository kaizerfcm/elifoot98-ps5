/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <string>
#include <vector>

struct ZipEntry
{
    std::string name;
    std::vector<unsigned char> data;
};

bool zip_read(const std::string &path, std::vector<ZipEntry> &entries, std::string &error);
