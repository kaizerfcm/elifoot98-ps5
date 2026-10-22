/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

// Writes one line to the kernel log (what klogsrv streams) and to
// /data/elifoot98/elifoot98.log. stdout from a title never reaches klog.
void trace(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
