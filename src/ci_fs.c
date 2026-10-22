/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * The game folder is a normal host directory, and the PS5 filesystem is
 * case-sensitive. DOS always looks names up in uppercase, so D:\ELI98 misses
 * the real directory eli98 and the boot batch stops before Windows. These
 * wrappers retry a failed lookup by matching each path component without
 * regard to case.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int __real_stat(const char *path, struct stat *st);
int __real_access(const char *path, int mode);
FILE *__real_fopen(const char *path, const char *mode);
DIR *__real_opendir(const char *path);

static int resolve_case(const char *path, char *out, size_t outlen)
{
    if (!path || path[0] != '/' || outlen < 2)
        return -1;
    snprintf(out, outlen, "/");
    const char *src = path + 1;
    while (*src)
    {
        while (*src == '/')
            src++;
        if (!*src)
            break;
        const char *end = src;
        while (*end && *end != '/')
            end++;
        size_t length = (size_t)(end - src);
        if (length == 0 || length >= 256)
            return -1;
        char wanted[256];
        memcpy(wanted, src, length);
        wanted[length] = 0;

        char trial[1024];
        int wrote = snprintf(trial, sizeof(trial), "%s%s%s", out, out[1] ? "/" : "", wanted);
        struct stat st;
        if (wrote > 0 && (size_t)wrote < sizeof(trial) && __real_stat(trial, &st) == 0)
        {
            snprintf(out, outlen, "%s", trial);
        }
        else
        {
            DIR *dir = __real_opendir(out);
            if (!dir)
                return -1;
            char found[256];
            found[0] = 0;
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL)
            {
                if (strcasecmp(entry->d_name, wanted) == 0)
                {
                    snprintf(found, sizeof(found), "%s", entry->d_name);
                    break;
                }
            }
            closedir(dir);
            if (!found[0])
                return -1;
            wrote = snprintf(trial, sizeof(trial), "%s%s%s", out, out[1] ? "/" : "", found);
            if (wrote < 0 || (size_t)wrote >= sizeof(trial))
                return -1;
            snprintf(out, outlen, "%s", trial);
        }
        src = end;
    }
    return out[1] ? 0 : -1;
}

int __wrap_stat(const char *path, struct stat *st)
{
    if (__real_stat(path, st) == 0)
        return 0;
    int saved = errno;
    char resolved[1024];
    if (resolve_case(path, resolved, sizeof(resolved)) != 0)
    {
        errno = saved;
        return -1;
    }
    return __real_stat(resolved, st);
}

int __wrap_access(const char *path, int mode)
{
    if (__real_access(path, mode) == 0)
        return 0;
    int saved = errno;
    char resolved[1024];
    if (resolve_case(path, resolved, sizeof(resolved)) != 0)
    {
        errno = saved;
        return -1;
    }
    return __real_access(resolved, mode);
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
    FILE *file = __real_fopen(path, mode);
    if (file)
        return file;
    int saved = errno;
    char resolved[1024];
    if (resolve_case(path, resolved, sizeof(resolved)) != 0)
    {
        errno = saved;
        return NULL;
    }
    return __real_fopen(resolved, mode);
}

DIR *__wrap_opendir(const char *path)
{
    DIR *dir = __real_opendir(path);
    if (dir)
        return dir;
    int saved = errno;
    char resolved[1024];
    if (resolve_case(path, resolved, sizeof(resolved)) != 0)
    {
        errno = saved;
        return NULL;
    }
    return __real_opendir(resolved);
}
