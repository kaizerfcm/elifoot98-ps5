/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <stddef.h>
#include <string>
#include <vector>

std::string path_join(const std::string &a, const std::string &b);
bool path_is_dir(const std::string &path);
bool path_is_file(const std::string &path);
bool mkdir_p(const std::string &path);
bool copy_file(const std::string &from, const std::string &to);
bool copy_tree(const std::string &from, const std::string &to);
bool write_file(const std::string &path, const void *data, size_t size);
bool write_text(const std::string &path, const std::string &text);
bool read_file(const std::string &path, std::vector<unsigned char> &out);
bool remove_tree(const std::string &path);
std::string parent_dir(const std::string &path);
std::string base_name(const std::string &path);
std::string lower_ascii(std::string s);
std::string upper_ascii(std::string s);
void list_files(const std::string &dir, const char *ext, bool one_level,
                std::vector<std::string> &out);
int uppercase_tree(const std::string &root);
