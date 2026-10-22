/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include <errno.h>
#include <stdio.h>

/*
 * The PS5 libc headers declare funopen(), and DOSBox Pure calls it because
 * the toolchain defines __FreeBSD__. The payload SDK's libc stub does not
 * export the symbol. DOSBox only uses it for libretro VFS paths of the form
 * "C:/...", which this frontend never passes (host files are fopen paths).
 * Returning NULL makes that unused path fail cleanly instead of aborting
 * the link.
 */
FILE *funopen(const void *cookie, int (*readfn)(void *, char *, int),
              int (*writefn)(void *, const char *, int), fpos_t (*seekfn)(void *, fpos_t, int),
              int (*closefn)(void *))
{
    (void)cookie;
    (void)readfn;
    (void)writefn;
    (void)seekfn;
    (void)closefn;
    errno = ENOSYS;
    return NULL;
}
