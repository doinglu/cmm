// std_bin_alloc.h
// Inital version Feb/14/2016 by doing
// Binary allocator

#ifndef _STD_BIN_ALLOC_H_
#define _STD_BIN_ALLOC_H_

#include "std_port/std_port.h"
#include "std_memmgr/std_memmgr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Per page size for binary allocation
// Attention: WHY add 64?
// Because the size is offen 2^n when do large allocation. And some routines may add
// header to the raw data. Such as std_allocate_memory adds sizeof(...) + STD_BLOCK_RESERVED
// to the raw size. Reserve 64 more bytes here will do bettre for those cases.
#define STD_BIN_PAGE_SIZE   (65536 + 64)

typedef IntR  scan_type_t;
typedef struct std_bin_alloc
{
    size_t total_reserved;  // Total size of reserved memory,    *NOT CHANGE AFTER CREATE
    size_t mem_reserved;    // Max reserved size for allocation, *NOT CHANGE AFTER CREATE
    size_t current_reserved;// Current reserved size
    size_t current_used;    // Current used size
    size_t free_start;      // Pointer to first free position 
    size_t bitmap_count;    // Size of bitmap in scan_type_t
    std_spin_lock_t lock;   // Lock
    scan_type_t* bitmap;    // Status of all binary pages
    char* mem;
} std_bin_alloc_t;

extern int     std_ba_create(std_bin_alloc_t* pool, size_t reserved);
extern void    std_ba_destruct(std_bin_alloc_t* pool);
extern void*   std_ba_alloc(std_bin_alloc_t* pool, size_t size);
extern int     std_ba_free(std_bin_alloc_t* pool, void *p, size_t size);

#ifdef __cplusplus
}
#endif

#endif
