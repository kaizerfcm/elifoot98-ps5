/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "zip.h"

#include "util.h"

#include <stdio.h>
#include <string.h>

extern "C"
{
#include "puff.h"
}

namespace
{
const size_t kMaxFile = 16 * 1024 * 1024;
const size_t kMaxTotal = 64 * 1024 * 1024;

uint16_t ru16(const unsigned char *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}
uint32_t ru32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool inflate_raw(const unsigned char *src, size_t src_len, size_t dest_len,
                 std::vector<unsigned char> &out, std::string &error)
{
    out.assign(dest_len, 0);
    unsigned long dlen = (unsigned long)dest_len;
    unsigned long slen = (unsigned long)src_len;
    int rc = puff(out.data(), &dlen, src, &slen);
    if (rc != 0 || dlen != dest_len)
    {
        error = "Nao foi possivel descompactar o zip.";
        return false;
    }
    return true;
}
} // namespace

bool zip_read(const std::string &path, std::vector<ZipEntry> &entries, std::string &error)
{
    entries.clear();
    std::vector<unsigned char> file;
    if (!read_file(path, file))
    {
        error = "Nao foi possivel abrir o zip.";
        return false;
    }
    if (file.size() < 22)
    {
        error = "O arquivo zip esta vazio ou incompleto.";
        return false;
    }
    size_t eocd = std::string::npos;
    size_t scan = file.size() < (65535u + 22u) ? 0 : file.size() - (65535u + 22u);
    for (size_t i = file.size() - 22; i + 1 > scan; i--)
    {
        if (file[i] == 0x50 && file[i + 1] == 0x4b && file[i + 2] == 0x05 && file[i + 3] == 0x06)
        {
            eocd = i;
            break;
        }
        if (i == 0)
            break;
    }
    if (eocd == std::string::npos || eocd + 22 > file.size())
    {
        error = "O zip nao tem diretorio central.";
        return false;
    }
    uint16_t count = ru16(&file[eocd + 10]);
    uint32_t cd_off = ru32(&file[eocd + 16]);
    if ((size_t)cd_off >= file.size())
    {
        error = "O diretorio central do zip e invalido.";
        return false;
    }
    size_t cursor = cd_off;
    size_t total = 0;
    for (uint16_t n = 0; n < count; n++)
    {
        if (cursor + 46 > file.size() || ru32(&file[cursor]) != 0x02014b50u)
        {
            error = "O diretorio central do zip esta corrompido.";
            return false;
        }
        uint16_t flags = ru16(&file[cursor + 8]);
        uint16_t method = ru16(&file[cursor + 10]);
        uint32_t comp = ru32(&file[cursor + 20]);
        uint32_t uncomp = ru32(&file[cursor + 24]);
        uint16_t name_len = ru16(&file[cursor + 28]);
        uint16_t extra_len = ru16(&file[cursor + 30]);
        uint16_t comment_len = ru16(&file[cursor + 32]);
        uint32_t local_off = ru32(&file[cursor + 42]);
        if (cursor + 46 + name_len > file.size())
        {
            error = "Um nome de arquivo do zip e invalido.";
            return false;
        }
        std::string name((const char *)&file[cursor + 46], name_len);
        cursor += 46u + name_len + extra_len + comment_len;
        for (char &c : name)
        {
            if (c == '\\')
                c = '/';
        }
        if (name.empty() || name.back() == '/')
            continue;
        if (name.find("..") != std::string::npos)
        {
            error = "O zip contem um caminho invalido.";
            return false;
        }
        if (flags & 1u)
        {
            error = "O zip esta protegido por senha.";
            return false;
        }
        if (method != 0 && method != 8)
        {
            error = "O zip usa uma compressao nao suportada.";
            return false;
        }
        if ((size_t)uncomp > kMaxFile || total + uncomp > kMaxTotal)
        {
            error = "O zip e grande demais.";
            return false;
        }
        if ((size_t)local_off + 30 > file.size())
        {
            error = "Um arquivo do zip esta incompleto.";
            return false;
        }
        uint16_t local_name = ru16(&file[local_off + 26]);
        uint16_t local_extra = ru16(&file[local_off + 28]);
        size_t data_off = (size_t)local_off + 30u + local_name + local_extra;
        if (data_off + comp > file.size())
        {
            error = "Um arquivo do zip esta incompleto.";
            return false;
        }
        ZipEntry entry;
        entry.name = name;
        if (method == 0)
        {
            if (comp != uncomp)
            {
                error = "Um arquivo do zip esta incompleto.";
                return false;
            }
            entry.data.assign(file.begin() + (ptrdiff_t)data_off,
                              file.begin() + (ptrdiff_t)(data_off + comp));
        }
        else if (!inflate_raw(&file[data_off], comp, uncomp, entry.data, error))
        {
            return false;
        }
        total += entry.data.size();
        entries.push_back(std::move(entry));
    }
    return true;
}
