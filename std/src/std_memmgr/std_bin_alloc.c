// std_bin_alloc.c
// Inital version Feb/14/2016 by doing
// Binary allocator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std_port/std_port.h"
#include "std_port/std_port_mmap.h"
#include "std_memmgr/std_bin_alloc.h"

static void std_ba_extend(std_bin_alloc_t* pool, size_t size);
static void std_ba_shrink(std_bin_alloc_t* pool, size_t size);

// Create the pool
extern int std_ba_create(std_bin_alloc_t* pool, size_t reserved)
{
    size_t bitmap_size;
    size_t total_reserved;
    char*  m_ptr;
    size_t pages;

    pages = reserved / STD_BIN_PAGE_SIZE;
    if (pages < 1)
        pages = 1;
    if (reserved & (reserved - 1))
    {
        // reserved size must be 2^n * pages
        reserved |= reserved >> 1;
        reserved |= reserved >> 2;
        reserved |= reserved >> 4;
        reserved |= reserved >> 8;
        reserved |= reserved >> 16;
        if (sizeof(size_t) > 4)
            reserved |= reserved >> 32;
        reserved++;
    }

    // Get bitmap size
    bitmap_size = (pages / 8 + 15) & ~15;
    STD_ASSERT(("bitmap_size is not aligned.", bitmap_size % sizeof(scan_type_t) == 0));
    if (pages < bitmap_size * 8)
        // Minimal memory pages to reserved
        pages = bitmap_size * 8;

    reserved = pages * STD_BIN_PAGE_SIZE;
    total_reserved = std_align_size(bitmap_size) + std_align_size(reserved);
    m_ptr = (char*)std_mem_reserve(NULL, total_reserved);
    if (m_ptr == NULL)
        // Failed to reserve memory
        return 0;

    memset(pool, 0, sizeof(*pool));
    std_init_spin_lock(&pool->lock);
    pool->total_reserved = total_reserved;
    pool->mem_reserved = reserved;
    pool->bitmap_count = bitmap_size / sizeof(scan_type_t);
    pool->bitmap = (scan_type_t*)m_ptr;
    pool->mem = m_ptr + std_align_size(bitmap_size);
    std_mem_commit(pool->bitmap, std_align_size(bitmap_size), STD_PAGE_READ | STD_PAGE_WRITE);
    memset(pool->bitmap, 0, bitmap_size);
    return 1;
}

// Destruct the pool
extern void std_ba_destruct(std_bin_alloc_t* pool)
{
    std_destroy_spin_lock(lock);
    std_mem_release(pool->bitmap, pool->total_reserved);
}

// Allocate memory from pool
extern void* std_ba_alloc(std_bin_alloc_t* pool, size_t size)
{
    size_t bit_len;
    size_t word_count;
    size_t i, k;
    size_t offset;
    scan_type_t word;

    // Get page count of size
    bit_len = (size + STD_BIN_PAGE_SIZE - 1) / STD_BIN_PAGE_SIZE;
    // Make bit_len to nearest 2^n
    if (bit_len & (bit_len - 1))
    {
        bit_len |= bit_len >> 1;
        bit_len |= bit_len >> 2;
        bit_len |= bit_len >> 4;
        bit_len |= bit_len >> 8;
        bit_len |= bit_len >> 16;
        if (sizeof(size_t) > 4)
            bit_len |= bit_len >> 32;
        bit_len++;
    }
    word_count = bit_len / (8 * sizeof(scan_type_t));
    // Align the size
    size = bit_len * STD_BIN_PAGE_SIZE;

    // Find for free page
    std_get_spin_lock(&pool->lock);
    if (!word_count)
    {
        // The length of pages to be allocated is < one word bitmap
        for (i = 0; i < pool->bitmap_count; i++)
        {
            word = pool->bitmap[i];
            if (word != ~(scan_type_t)0)
            {
                // There is free page here, check for length

                // Create bit mask: bit_lenx1 like "0...00001111"
                scan_type_t bit_mask = ((((scan_type_t)1) << bit_len) - 1);
                for (k = 0; k < sizeof(scan_type_t) * 8; k += bit_len, bit_mask <<= bit_len)
                    if ((word & bit_mask) == 0)
                    {
                        // Got free page @ (i, k)
                        // Mark bits to 1...111 indicating used
                        pool->bitmap[i] = word | bit_mask;
                        // Return address of memory
                        offset = i * (sizeof(scan_type_t) * 8);
                        offset += k;
                        offset *= STD_BIN_PAGE_SIZE;
                        if (pool->current_used < offset + size)
                            std_ba_extend(pool, offset + size);
                        std_release_spin_lock(&pool->lock);
                        return pool->mem + offset;
                    }
                // Not found free, continue
            }
            // Check next word in bitmap
        }
        // Failed to allocate
    } else
    {
        // The length of pages to be allocated is >= one word bitmap
        for (i = 0; i < pool->bitmap_count; i += word_count)
        {
            word = pool->bitmap[i];
            if (word)
                continue;

            // Got a free word, check for continous word
            int not_freed = 0;
            for (k = 1; k < word_count; k++)
                if (pool->bitmap[i + k])
                {
                    not_freed = 1;
                    break;;
                }

            if (not_freed)
                // Not found, continue
                continue;

            // Got free pages @ i
            // Mark words to 1...111 indicating used
            for (k = 0; k < word_count; k++)
                pool->bitmap[i + k] = ~(scan_type_t)0;
            offset = i * (sizeof(scan_type_t) * 8 * STD_BIN_PAGE_SIZE);
            if (pool->current_used < offset + size)
                std_ba_extend(pool, offset + size);
            std_release_spin_lock(&pool->lock);
            return pool->mem + offset;
        }
        // Failed to allocate
    }

    // Failed to allocate
    std_release_spin_lock(&pool->lock);
    return NULL;
}

// Free memory to pool
extern int std_ba_free(std_bin_alloc_t* pool, void *p, size_t size)
{
    size_t bit_len;
    size_t word_count;
    size_t i, k;
    size_t offset;

    // Get page count of size
    bit_len = (size + STD_BIN_PAGE_SIZE - 1) / STD_BIN_PAGE_SIZE;
    // Make bit_len to nearest 2^n
    if (bit_len & (bit_len - 1))
    {
        bit_len |= bit_len >> 1;
        bit_len |= bit_len >> 2;
        bit_len |= bit_len >> 4;
        bit_len |= bit_len >> 8;
        bit_len |= bit_len >> 16;
        if (sizeof(size_t) > 4)
            bit_len |= bit_len >> 32;
        bit_len++;
    }
    word_count = bit_len / (8 * sizeof(scan_type_t));
    // Align the size
    size = bit_len * STD_BIN_PAGE_SIZE;

    offset = (char*)p - pool->mem;
    if (offset % (STD_BIN_PAGE_SIZE * bit_len) != 0)
        // Bad pointer to free, should be alignment to block size
        return 0;

    if (offset + size > pool->current_used)
        // Failed, out of range
        return 0;

    // Free the pages
    std_get_spin_lock(&pool->lock);
    if (!word_count)
    {
        // Unmark word
        scan_type_t bit_mask;
        i = offset / STD_BIN_PAGE_SIZE;
        k = i % (8 * sizeof(scan_type_t));
        i = i / (8 * sizeof(scan_type_t));
        bit_mask = ((((scan_type_t)1) << bit_len) - 1);
        bit_mask <<= k;
        if ((pool->bitmap[i] & bit_mask) != bit_mask)
        {
            // Memory was not allocated
            std_release_spin_lock(&pool->lock);
            return 0;
        }

        // Erase bit flags
        pool->bitmap[i] &= ~bit_mask;
    } else
    {
        i = offset / (STD_BIN_PAGE_SIZE * 8 * sizeof(scan_type_t));
        for (k = 0; k < word_count; k++)
            if (pool->bitmap[i + k] != ~(scan_type_t)0)
            {
                // Memory was not allocated
                std_release_spin_lock(&pool->lock);
                return 0;
            }

        // Erase bit flags
        for (k = 0; k < word_count; k++)
            pool->bitmap[i + k] = 0;
    }

    if (offset + size >= pool->current_used)
        std_ba_shrink(pool, offset);
    std_release_spin_lock(&pool->lock);
    return 1;
}

// Hit for commit/demmit
#define COMMIT_HINT_SIZE        (8 * 1024 * 1024)

// Extend used memory
static void std_ba_extend(std_bin_alloc_t* pool, size_t size)
{
    pool->current_used = size;

    if (pool->current_used > pool->current_reserved)
    {
        // Commit new pages to access
        size_t to;
        if (pool->current_used < pool->current_reserved + COMMIT_HINT_SIZE)
            // Commit more
            to = pool->current_reserved + COMMIT_HINT_SIZE;
        else
            // Commit exactly to unused
            to = pool->current_used;
        to = std_align_size(to);

        std_mem_commit(pool->mem + pool->current_reserved, to - pool->current_reserved, STD_PAGE_READ | STD_PAGE_WRITE);
        pool->current_reserved = to;
    }
}

// Shrink used memory
static void std_ba_shrink(std_bin_alloc_t* pool, size_t size)
{
    int i, k;
    // Find first unsed from specified size
    i = (int)(size / (8 * sizeof(scan_type_t) * STD_BIN_PAGE_SIZE));
    k = 0;
    while (i >= 0)
    {
        if (pool->bitmap[i])
        {
            scan_type_t word = pool->bitmap[i];
            for (k = 0; k < 8 * sizeof(scan_type_t) && word; k++, word >>= 1);
            break;
        }
        i--;
    }
    // Got used range
    if (i < 0)
        size = 0;
    else
        size = (size_t)(i * 8 * sizeof(scan_type_t) + k) * STD_BIN_PAGE_SIZE;
    pool->current_used = size;

    if (pool->current_reserved >= COMMIT_HINT_SIZE &&
        pool->current_used <= pool->current_reserved - COMMIT_HINT_SIZE)
    {
        // May decommit more than 16M

        // Decommit shrinked memory after aligned page
        size_t to = std_align_size(pool->current_used);
        char *p_unused = pool->mem + to;
        if (pool->mem + pool->current_reserved > p_unused)
        {
            // Decommit unused memory
            size_t size_unused = pool->mem + pool->current_reserved - p_unused;
            std_mem_decommit(p_unused, size_unused);
            pool->current_reserved = to;
        }
    }    
}
