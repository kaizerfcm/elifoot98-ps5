/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * The game sandbox caps the system libc heap at a few megabytes no matter
 * what the process parameters ask for, so a single 1920x1080 SDL surface
 * already fails with "Out of memory". Every malloc in the title is routed
 * here with ld --wrap. Small requests stay on the system heap. When the
 * system heap refuses a request, the block comes from direct memory instead,
 * the same kind SDL's PS5 video driver maps for its 64 MB scanout buffers.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *address, size_t size);
void __real_free(void *address);
int __real_posix_memalign(void **address, size_t alignment, size_t size);
void *__real_memalign(size_t alignment, size_t size);
void *__real_aligned_alloc(size_t alignment, size_t size);
size_t malloc_usable_size(void *address);

int sceKernelAllocateMainDirectMemory(size_t length, size_t alignment, int type, int64_t *physical);
int sceKernelMapDirectMemory(void **address, size_t length, int protection, int flags,
                             int64_t physical, size_t alignment);
int sceKernelMunmap(void *address, size_t length);
int sceKernelReleaseDirectMemory(int64_t physical, size_t length);

#define BIG_ALIGN 0x10000u
#define BIG_SLOTS 1024

struct Block
{
    void *address;
    size_t length;
    size_t requested;
    int64_t physical;
};

static struct Block g_blocks[BIG_SLOTS];
static int g_lock;
static size_t g_mapped;
static unsigned g_count;

static void lock(void)
{
    while (__atomic_exchange_n(&g_lock, 1, __ATOMIC_ACQUIRE))
        ;
}

static void unlock(void)
{
    __atomic_store_n(&g_lock, 0, __ATOMIC_RELEASE);
}

static struct Block *find(const void *address)
{
    for (int i = 0; i < BIG_SLOTS; i++)
        if (g_blocks[i].address == address)
            return &g_blocks[i];
    return NULL;
}

static void *direct_alloc(size_t size, size_t alignment)
{
    if (size == 0)
        size = 1;
    if (alignment > BIG_ALIGN)
        return NULL;
    size_t length = (size + BIG_ALIGN - 1) & ~(size_t)(BIG_ALIGN - 1);
    int64_t physical = 0;
    if (sceKernelAllocateMainDirectMemory(length, BIG_ALIGN, 3, &physical) != 0)
        return NULL;
    void *address = NULL;
    if (sceKernelMapDirectMemory(&address, length, 0x33, 0, physical, BIG_ALIGN) != 0)
    {
        sceKernelReleaseDirectMemory(physical, length);
        return NULL;
    }
    lock();
    struct Block *slot = find(NULL);
    if (slot)
    {
        slot->address = address;
        slot->length = length;
        slot->requested = size;
        slot->physical = physical;
        g_mapped += length;
        g_count++;
    }
    unlock();
    if (!slot)
    {
        sceKernelMunmap(address, length);
        sceKernelReleaseDirectMemory(physical, length);
        return NULL;
    }
    return address;
}

static int direct_free(void *address)
{
    lock();
    struct Block *slot = find(address);
    struct Block block = {0};
    if (slot)
    {
        block = *slot;
        memset(slot, 0, sizeof(*slot));
        g_mapped -= block.length;
        g_count--;
    }
    unlock();
    if (!slot)
        return 0;
    sceKernelMunmap(block.address, block.length);
    sceKernelReleaseDirectMemory(block.physical, block.length);
    return 1;
}

static size_t direct_size(void *address)
{
    lock();
    struct Block *slot = find(address);
    size_t size = slot ? slot->requested : 0;
    unlock();
    return size;
}

void bigalloc_stats(size_t *mapped, unsigned *count)
{
    lock();
    *mapped = g_mapped;
    *count = g_count;
    unlock();
}

void *__wrap_malloc(size_t size)
{
    void *address = __real_malloc(size);
    return address ? address : direct_alloc(size, 0);
}

void *__wrap_calloc(size_t count, size_t size)
{
    void *address = __real_calloc(count, size);
    if (address || count == 0 || size == 0)
        return address;
    if (size > SIZE_MAX / count)
        return NULL;
    address = direct_alloc(count * size, 0);
    if (address)
        memset(address, 0, count * size);
    return address;
}

void __wrap_free(void *address)
{
    if (!address || direct_free(address))
        return;
    __real_free(address);
}

void *__wrap_realloc(void *address, size_t size)
{
    if (!address)
        return __wrap_malloc(size);
    size_t old = direct_size(address);
    if (old)
    {
        void *grown = __wrap_malloc(size);
        if (!grown)
            return NULL;
        memcpy(grown, address, old < size ? old : size);
        direct_free(address);
        return grown;
    }
    void *grown = __real_realloc(address, size);
    if (grown || size == 0)
        return grown;
    size_t usable = malloc_usable_size(address);
    grown = direct_alloc(size, 0);
    if (!grown)
        return NULL;
    memcpy(grown, address, usable < size ? usable : size);
    __real_free(address);
    return grown;
}

int __wrap_posix_memalign(void **address, size_t alignment, size_t size)
{
    int result = __real_posix_memalign(address, alignment, size);
    if (result == 0)
        return 0;
    void *block = direct_alloc(size, alignment);
    if (!block)
        return result;
    *address = block;
    return 0;
}

void *__wrap_memalign(size_t alignment, size_t size)
{
    void *address = __real_memalign(alignment, size);
    return address ? address : direct_alloc(size, alignment);
}

void *__wrap_aligned_alloc(size_t alignment, size_t size)
{
    void *address = __real_aligned_alloc(alignment, size);
    return address ? address : direct_alloc(size, alignment);
}
