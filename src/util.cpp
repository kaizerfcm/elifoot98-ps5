/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "util.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

std::string path_join(const std::string &a, const std::string &b)
{
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    if (a.back() == '/')
        return a + b;
    return a + "/" + b;
}

static bool stat_kind(const std::string &path, mode_t kind)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return (st.st_mode & S_IFMT) == kind;
}

bool path_is_dir(const std::string &path)
{
    return stat_kind(path, S_IFDIR);
}
bool path_is_file(const std::string &path)
{
    return stat_kind(path, S_IFREG);
}

bool mkdir_p(const std::string &path)
{
    if (path.empty() || path_is_dir(path))
        return true;
    std::string cur;
    for (size_t i = 0; i < path.size(); i++)
    {
        cur.push_back(path[i]);
        if (path[i] != '/' || cur.size() == 1)
            continue;
        if (!path_is_dir(cur) && mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST)
            return false;
    }
    if (!path_is_dir(path) && mkdir(path.c_str(), 0755) != 0 && errno != EEXIST)
        return false;
    return path_is_dir(path);
}

bool copy_file(const std::string &from, const std::string &to)
{
    FILE *in = fopen(from.c_str(), "rb");
    if (!in)
        return false;
    mkdir_p(parent_dir(to));
    FILE *out = fopen(to.c_str(), "wb");
    if (!out)
    {
        fclose(in);
        return false;
    }
    char buf[64 * 1024];
    bool ok = true;
    while (ok)
    {
        size_t n = fread(buf, 1, sizeof(buf), in);
        if (n && fwrite(buf, 1, n, out) != n)
            ok = false;
        if (n < sizeof(buf))
        {
            if (ferror(in))
                ok = false;
            break;
        }
    }
    if (fclose(out) != 0)
        ok = false;
    fclose(in);
    return ok;
}

bool copy_tree(const std::string &from, const std::string &to)
{
    if (!path_is_dir(from))
        return copy_file(from, to);
    if (!mkdir_p(to))
        return false;
    DIR *dir = opendir(from.c_str());
    if (!dir)
        return false;
    bool ok = true;
    while (dirent *ent = readdir(dir))
    {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;
        std::string src = path_join(from, ent->d_name);
        std::string dst = path_join(to, ent->d_name);
        if (path_is_dir(src))
        {
            if (!copy_tree(src, dst))
                ok = false;
        }
        else if (!copy_file(src, dst))
        {
            ok = false;
        }
    }
    closedir(dir);
    return ok;
}

bool write_file(const std::string &path, const void *data, size_t size)
{
    mkdir_p(parent_dir(path));
    FILE *out = fopen(path.c_str(), "wb");
    if (!out)
        return false;
    bool ok = fwrite(data, 1, size, out) == size;
    if (fclose(out) != 0)
        ok = false;
    return ok;
}

bool write_text(const std::string &path, const std::string &text)
{
    return write_file(path, text.data(), text.size());
}

bool read_file(const std::string &path, std::vector<unsigned char> &out)
{
    FILE *in = fopen(path.c_str(), "rb");
    if (!in)
        return false;
    if (fseek(in, 0, SEEK_END) != 0)
    {
        fclose(in);
        return false;
    }
    long n = ftell(in);
    if (n < 0)
    {
        fclose(in);
        return false;
    }
    rewind(in);
    out.resize((size_t)n);
    bool ok = n == 0 || fread(out.data(), 1, out.size(), in) == out.size();
    fclose(in);
    return ok;
}

bool remove_tree(const std::string &path)
{
    if (!path_is_dir(path))
        return unlink(path.c_str()) == 0 || errno == ENOENT;
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return false;
    struct dirent *ent;
    bool ok = true;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        if (!remove_tree(path_join(path, ent->d_name)))
            ok = false;
    }
    closedir(dir);
    if (rmdir(path.c_str()) != 0)
        ok = false;
    return ok;
}

std::string parent_dir(const std::string &path)
{
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return "";
    if (slash == 0)
        return "/";
    return path.substr(0, slash);
}

std::string base_name(const std::string &path)
{
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos)
        return path;
    return path.substr(slash + 1);
}

std::string lower_ascii(std::string s)
{
    for (char &c : s)
        c = (char)tolower((unsigned char)c);
    return s;
}

std::string upper_ascii(std::string s)
{
    for (char &c : s)
        c = (char)toupper((unsigned char)c);
    return s;
}

static bool ends_with_ext(const std::string &name, const char *ext)
{
    std::string low = lower_ascii(name);
    size_t n = strlen(ext);
    return low.size() >= n && low.compare(low.size() - n, n, ext) == 0;
}

static void list_dir(const std::string &dir, const char *ext, std::vector<std::string> &out)
{
    DIR *d = opendir(dir.c_str());
    if (!d)
        return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        if (ent->d_name[0] == '.')
            continue;
        std::string path = path_join(dir, ent->d_name);
        if (path_is_file(path) && ends_with_ext(ent->d_name, ext))
            out.push_back(path);
    }
    closedir(d);
}

void list_files(const std::string &dir, const char *ext, bool one_level,
                std::vector<std::string> &out)
{
    list_dir(dir, ext, out);
    if (!one_level)
        return;
    DIR *d = opendir(dir.c_str());
    if (!d)
        return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        if (ent->d_name[0] == '.')
            continue;
        std::string path = path_join(dir, ent->d_name);
        if (path_is_dir(path))
            list_dir(path, ext, out);
    }
    closedir(d);
}
