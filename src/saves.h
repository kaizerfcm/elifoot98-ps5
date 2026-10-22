/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <string>
#include <vector>

std::string export_saves(const std::string &data_dir);
std::vector<std::string> find_import_saves();
std::string import_save(const std::string &data_dir, const std::string &file);
std::string wipe_data(const std::string &data_dir);
std::string saves_folder(const std::string &data_dir);
