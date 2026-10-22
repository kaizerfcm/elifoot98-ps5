/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

enum class SearchState
{
    New,
    Ongoing,
    Found,
    None,
    Error
};

enum DataType
{
    DataByte = 1,
    DataWord = 2,
    DataDword = 4,
    DataString = 5
};

struct SavedCheat
{
    std::string name;
    uint32_t address;
    int data_type;
};

class Omatic
{
  public:
    void bind(uint8_t *ram, size_t size, const std::string &save_path);
    void reset();
    std::string search();
    std::string set_value();
    std::string save_cheat(const std::string &name);
    void select(size_t index);
    void remove(size_t index);
    void refresh_labels();

    SearchState state = SearchState::New;
    std::string input;
    std::string info;
    std::string field_label;
    std::string primary_label;
    std::vector<uint32_t> results;
    std::vector<SavedCheat> cheats;
    int inferred = DataByte;

  private:
    uint8_t *ram_ = nullptr;
    size_t size_ = 0;
    std::string path_;
    bool parse(const std::string &term, std::vector<uint8_t> &out, std::string &err) const;
    std::vector<uint32_t> scan(const std::vector<uint8_t> &value) const;
    void note_type(const std::vector<uint8_t> &value);
    void load();
    void store() const;
};
