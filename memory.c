/*
 * Memory manager reimplementation using SDL_malloc.
 * Replaces MEMORY.ASM for builds where fragmentation is not a concern.
 * Same API semantics; no coalescing or priority-based eviction.
 * Compacting / cache can be added later.
 */

#include "memory.h"
#include <SDL3/SDL_stdinc.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_MEMBLOCKS  1000
#define INITIAL_FREE   (16 * 1024 * 1024)   /* reported "free" for Size_of_free_memory */
#define MEMLIST_ENTRY  12

/* Per-block descriptor (internal). */
typedef struct {
    void    *ptr;
    uint32_t size;
    uint8_t  type;
    uint8_t  priority;
    uint16_t subfile;
    uint8_t  in_use;
} mem_entry_t;

/* File type -> priority (from MEMORY.ASM .Priority table). */
static const uint8_t priority_tab[64] = {
    0, 0, 7, 1, 4, 0, 3, 3, 1, 0, 4, 2, 1, 1, 7, 2,
    3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static mem_entry_t blocks[MAX_MEMBLOCKS];
static uint8_t  memory_list_buf[MEMLIST_ENTRY * MAX_MEMBLOCKS];

void *Memory_list   = memory_list_buf;
void *Memlist_end   = memory_list_buf;   /* first byte past last entry */
void *Start_of_free_memory = 0;          /* unused in malloc impl */

/* ASM references this as .l; must be 4-byte aligned. */
uint32_t Size_of_free_memory;

/* Write big-endian for 68k Show_memory if it reads this buffer. */
static void write_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}
static void write_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v);
}

static void rebuild_memory_list(void)
{
    uint8_t *out = memory_list_buf;
    int n = 0;
    for (int i = 0; i < MAX_MEMBLOCKS && n < MAX_MEMBLOCKS; i++) {
        if (!blocks[i].in_use)
            continue;
        write_be32(out + 0, (uint32_t)(uintptr_t)blocks[i].ptr);
        write_be32(out + 4, blocks[i].size);
        out[8] = blocks[i].type & 0x7f;   /* bit 7 = 0 (allocated) */
        out[9] = blocks[i].priority;
        write_be16(out + 10, blocks[i].subfile);
        out += MEMLIST_ENTRY;
        n++;
    }
    Memlist_end = memory_list_buf + (size_t)n * MEMLIST_ENTRY;
}

static mem_entry_t *find_by_ptr(void *ptr)
{
    for (int i = 0; i < MAX_MEMBLOCKS; i++)
        if (blocks[i].in_use && blocks[i].ptr == ptr)
            return &blocks[i];
    return NULL;
}

static mem_entry_t *find_by_type_subfile(uint8_t type, uint16_t subfile)
{
    for (int i = 0; i < MAX_MEMBLOCKS; i++)
        if (blocks[i].in_use && blocks[i].type == type && blocks[i].subfile == subfile)
            return &blocks[i];
    return NULL;
}

static mem_entry_t *alloc_slot(void)
{
    for (int i = 0; i < MAX_MEMBLOCKS; i++)
        if (!blocks[i].in_use)
            return &blocks[i];
    return NULL;
}

static uint32_t align_size(uint32_t size)
{
    return (size + 1u) & ~1u;
}

/* Init_memory: a0 = basepage (ignored in C). */
void Init_memory(void *basepage)
{
    (void)basepage;
    for (int i = 0; i < MAX_MEMBLOCKS; i++)
        blocks[i].in_use = 0;
    Size_of_free_memory = INITIAL_FREE;
    Memlist_end = memory_list_buf;
}

/* Next_generation: no-op; can later decay priorities for a compactor. */
void Next_generation(void)
{
}

/* Allocate_memory: d0 = size (.l). Returns a1 = pointer. */
void *Allocate_memory(uint32_t size)
{
    size = align_size(size);
    if (size > Size_of_free_memory)
        return NULL;   /* caller expects error; DI_Error can be called by wrapper */
    void *ptr = SDL_malloc(size);
    if (!ptr)
        return NULL;
    Size_of_free_memory -= size;
    mem_entry_t *e = alloc_slot();
    if (!e) {
        SDL_free(ptr);
        Size_of_free_memory += size;
        return NULL;
    }
    e->ptr = ptr;
    e->size = size;
    e->type = 0;
    e->priority = 0;
    e->subfile = 0;
    e->in_use = 1;
    rebuild_memory_list();
    return ptr;
}

/* Reallocate_memory: d0 = file type (.b), d1 = subfile (.w). Returns a0 = ptr or 0 (cache miss). */
void *Reallocate_memory(uint8_t file_type, uint16_t subfile)
{
    mem_entry_t *e = find_by_type_subfile(file_type, subfile);
    if (!e)
        return NULL;
    return e->ptr;
}

/* File_allocate: d0 = type (.b), d1 = size (.l), d2 = subfile (.w). Returns a1 = pointer. */
void *File_allocate(uint8_t file_type, uint32_t size, uint16_t subfile)
{
    size = align_size(size);
    if (size > Size_of_free_memory)
        return NULL;
    void *ptr = SDL_malloc(size);
    if (!ptr)
        return NULL;
    Size_of_free_memory -= size;
    mem_entry_t *e = alloc_slot();
    if (!e) {
        SDL_free(ptr);
        Size_of_free_memory += size;
        return NULL;
    }
    e->ptr = ptr;
    e->size = size;
    e->type = file_type;
    e->priority = (uint8_t)(7 * 8);   /* top generation; table used for eviction order when compactor added */
    e->subfile = subfile;
    e->in_use = 1;
    rebuild_memory_list();
    return ptr;
}

/* Kill_memory: a0 = pointer. */
void Kill_memory(void *ptr)
{
    mem_entry_t *e = find_by_ptr(ptr);
    if (!e)
        return;   /* or call DI_Error in wrapper */
    Size_of_free_memory += e->size;
    SDL_free(e->ptr);
    e->in_use = 0;
    rebuild_memory_list();
}

/* Free_memory: a0 = pointer. */
void Free_memory(void *ptr)
{
    Kill_memory(ptr);
}

/* Shrink_memory: d0 = new size (.w), a0 = pointer. */
void Shrink_memory(uint32_t new_size, void *ptr)
{
    mem_entry_t *e = find_by_ptr(ptr);
    if (!e)
        return;
    new_size = align_size(new_size);
    if (new_size >= e->size)
        return;
    void *nptr = SDL_realloc(e->ptr, new_size);
    if (!nptr)
        return;
    uint32_t freed = e->size - new_size;
    Size_of_free_memory += freed;
    e->ptr = nptr;
    e->size = new_size;
    rebuild_memory_list();
}

