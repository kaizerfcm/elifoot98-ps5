/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "omatic.h"

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
const size_t kMaxResults = 10000;

std::string hex_addr(uint32_t address)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%X", address);
    return buf;
}
} // namespace

void Omatic::bind(uint8_t *ram, size_t size, const std::string &save_path)
{
    ram_ = ram;
    size_ = size;
    path_ = save_path;
    load();
    reset();
}

void Omatic::reset()
{
    results.clear();
    state = SearchState::New;
    inferred = DataByte;
    input.clear();
    refresh_labels();
}

void Omatic::refresh_labels()
{
    switch (state)
    {
    case SearchState::New:
        info = "Digite o valor atual que deseja modificar no jogo e escolha Buscar.";
        field_label = "Valor atual";
        primary_label = "Buscar";
        break;
    case SearchState::Ongoing:
        info = "Volte ao jogo, mude o valor e busque de novo. Filtrando " +
               std::to_string(results.size()) + " resultados.";
        field_label = "Valor atualizado";
        primary_label = "Buscar";
        break;
    case SearchState::Found:
        info = "Encontrado em " + hex_addr(results.empty() ? 0 : results[0]) +
               ". Digite o novo valor e escolha Alterar. Guarde o codigo para a proxima vez.";
        field_label = "Novo valor";
        primary_label = "Alterar";
        break;
    case SearchState::None:
        info = "Nenhum resultado foi encontrado para o valor informado.";
        field_label = "Valor";
        primary_label = "Buscar";
        break;
    case SearchState::Error:
        info = "Houve um erro na busca. Confira o valor informado e tente novamente.";
        field_label = "Valor";
        primary_label = "Buscar";
        break;
    }
}

void Omatic::note_type(const std::vector<uint8_t> &value)
{
    int kind = DataByte;
    if (value.size() == 1)
        kind = DataByte;
    else if (value.size() == 2)
        kind = DataWord;
    else if (value.size() < 5)
        kind = DataDword;
    else
        kind = DataString;
    if (kind > inferred)
        inferred = kind;
}

bool Omatic::parse(const std::string &term, std::vector<uint8_t> &out, std::string &err) const
{
    out.clear();
    if (term.empty())
    {
        err = "O valor esta vazio.";
        return false;
    }
    if (term.size() > 2 && term[0] == '0' && (term[1] == 'x' || term[1] == 'X'))
    {
        std::string hex = term.substr(2);
        if (hex.size() % 2)
            hex.insert(hex.begin(), '0');
        if (hex.empty())
        {
            err = "Valor hexadecimal invalido.";
            return false;
        }
        for (size_t i = 0; i < hex.size(); i += 2)
        {
            char byte[3] = {hex[i], hex[i + 1], 0};
            char *end = NULL;
            long v = strtol(byte, &end, 16);
            if (end != byte + 2)
            {
                err = "Valor hexadecimal invalido.";
                return false;
            }
            out.push_back((uint8_t)v);
        }
        return true;
    }
    bool numeric = true;
    size_t start = 0;
    if (term[0] == '-' && term.size() > 1)
        start = 1;
    for (size_t i = start; i < term.size(); i++)
    {
        if (term[i] < '0' || term[i] > '9')
            numeric = false;
    }
    if (numeric)
    {
        long v = strtol(term.c_str(), NULL, 10);
        unsigned long u = (unsigned long)v;
        int bytes = 1;
        unsigned long probe = u;
        while (probe > 0xFF)
        {
            probe >>= 8;
            bytes++;
        }
        out.resize((size_t)bytes);
        for (int i = 0; i < bytes; i++)
            out[(size_t)i] = (uint8_t)((u >> (8 * i)) & 0xFF);
        return true;
    }
    out.assign(term.begin(), term.end());
    return true;
}

std::vector<uint32_t> Omatic::scan(const std::vector<uint8_t> &value) const
{
    std::vector<uint32_t> found;
    if (!ram_ || value.empty() || value.size() > size_)
        return found;
    const size_t last = size_ - value.size();
    for (size_t address = 0; address <= last; address++)
    {
        if (memcmp(ram_ + address, value.data(), value.size()) != 0)
            continue;
        found.push_back((uint32_t)address);
        if (found.size() >= kMaxResults)
            break;
    }
    return found;
}

std::string Omatic::search()
{
    if (!ram_)
        return "A memoria do jogo ainda nao esta disponivel.";
    if (input.size() > 2 && input[0] == '0' && (input[1] == 'x' || input[1] == 'X') &&
        state == SearchState::New)
    {
        char *end = NULL;
        unsigned long address = strtoul(input.c_str(), &end, 16);
        if (end == input.c_str() || address >= size_)
        {
            state = SearchState::None;
            refresh_labels();
            return "Endereco invalido.";
        }
        results.clear();
        results.push_back((uint32_t)address);
        state = SearchState::Found;
        input.clear();
        refresh_labels();
        return "";
    }
    std::vector<uint8_t> value;
    std::string err;
    if (!parse(input, value, err))
    {
        state = SearchState::Error;
        refresh_labels();
        return err;
    }
    note_type(value);
    std::vector<uint32_t> found = scan(value);
    if (state == SearchState::Ongoing)
    {
        std::vector<uint32_t> kept;
        for (uint32_t address : results)
        {
            for (uint32_t hit : found)
            {
                if (hit == address)
                    kept.push_back(address);
            }
        }
        results.swap(kept);
    }
    else
    {
        results.swap(found);
    }
    if (results.empty())
        state = SearchState::None;
    else if (results.size() == 1)
        state = SearchState::Found;
    else
        state = SearchState::Ongoing;
    refresh_labels();
    return "";
}

std::string Omatic::set_value()
{
    if (!ram_ || state != SearchState::Found || results.size() != 1)
        return "Nao ha um unico endereco para alterar.";
    std::vector<uint8_t> value;
    std::string err;
    if (!parse(input, value, err))
        return err;
    note_type(value);
    if (inferred < DataString && value.size() < (size_t)inferred)
    {
        std::vector<uint8_t> padded((size_t)inferred, 0);
        memcpy(padded.data(), value.data(), value.size());
        value.swap(padded);
    }
    uint32_t address = results[0];
    if ((size_t)address + value.size() > size_)
        return "O valor nao cabe nesse endereco.";
    memcpy(ram_ + address, value.data(), value.size());
    return "";
}

std::string Omatic::save_cheat(const std::string &name)
{
    if (state != SearchState::Found || results.size() != 1)
        return "Busque ate restar um endereco.";
    if (name.empty())
        return "O cheat precisa de um nome.";
    SavedCheat cheat;
    cheat.name = name;
    cheat.address = results[0];
    cheat.data_type = inferred;
    bool replaced = false;
    for (SavedCheat &existing : cheats)
    {
        if (existing.address == cheat.address && existing.data_type == cheat.data_type)
        {
            existing = cheat;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        cheats.push_back(cheat);
    store();
    return "";
}

void Omatic::select(size_t index)
{
    if (index >= cheats.size() || !ram_)
        return;
    const SavedCheat &cheat = cheats[index];
    inferred = cheat.data_type;
    results.clear();
    if (cheat.address < size_)
        results.push_back(cheat.address);
    state = results.empty() ? SearchState::None : SearchState::Found;
    input.clear();
    refresh_labels();
}

void Omatic::remove(size_t index)
{
    if (index >= cheats.size())
        return;
    cheats.erase(cheats.begin() + (ptrdiff_t)index);
    store();
}

void Omatic::load()
{
    cheats.clear();
    std::vector<unsigned char> raw;
    if (!read_file(path_, raw))
        return;
    std::string text(raw.begin(), raw.end());
    size_t line_start = 0;
    while (line_start < text.size())
    {
        size_t line_end = text.find('\n', line_start);
        if (line_end == std::string::npos)
            line_end = text.size();
        std::string line = text.substr(line_start, line_end - line_start);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        size_t tab1 = line.find('\t');
        size_t tab2 = tab1 == std::string::npos ? std::string::npos : line.find('\t', tab1 + 1);
        if (tab1 != std::string::npos && tab2 != std::string::npos)
        {
            SavedCheat cheat;
            cheat.name = line.substr(0, tab1);
            cheat.address = (uint32_t)strtoul(line.c_str() + tab1 + 1, NULL, 16);
            cheat.data_type = atoi(line.c_str() + tab2 + 1);
            if (!cheat.name.empty())
                cheats.push_back(cheat);
        }
        line_start = line_end + 1;
    }
}

void Omatic::store() const
{
    std::string text;
    for (const SavedCheat &cheat : cheats)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "\t%X\t%d\n", cheat.address, cheat.data_type);
        text += cheat.name;
        text += buf;
    }
    write_text(path_, text);
}
