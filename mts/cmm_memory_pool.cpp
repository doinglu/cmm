// cmm_memory_pool.cpp
// Initial version Feb/7/2016 by doing
// Static & float memory pool

#include <string.h>
#include "std_port/std_port.h"
#include "std_port/std_port_mmap.h"
#include "std_memmgr/std_memmgr.h"
#include "cmm_memory_pool.h"

namespace cmm
{

MemoryPool::MemoryPool()
{
    m_head = 0;
    m_size = 0;
    m_reserve = 0;
}

MemoryPool::~MemoryPool()
{
    STD_ASSERT(("Memory pool is not freed.", !m_head));
}

// Allocate memory from pool, extend size when necessary
void* MemoryPool::allocate(size_t size)
{
    void* p = m_head + m_size;
    auto new_size = m_size + size;
    if (new_size > m_reserve)
    {
        // Extend the reserved memory to double size
        auto suggest_size = m_reserve * 2;
        if (suggest_size < PAGE_SIZE)
            suggest_size = PAGE_SIZE;
        if (! extend_pool(suggest_size))
            return 0;
    }

    // Got the memory
    m_size = new_size;
    return p;
}

// Allocate memory, don't extend if failed
void* MemoryPool::try_allocate(size_t size)
{
    void* p = m_head + m_size;
    auto new_size = m_size + size;
    if (new_size > m_reserve)
        return 0;

    // Got the memory
    m_size = new_size;
    return p;
}

// Free the memory after begin of pool
void MemoryPool::free(size_t begin)
{
    if (begin >= m_size)
    {
        STD_ASSERT(("Free out of range.", begin == m_size));
        return;
    }

    // Resize the size
    m_size = begin;

    // Shrink when necessary
    if (m_size <= m_reserve / 2)
        shrink_pool(m_size);
}

StaticMemoryPool::StaticMemoryPool(size_t max_reserve) :
    MemoryPool()
{
    m_head = (char*)std_mmap(0, max_reserve, STD_PAGE_RESERVE);
    if (!m_head)
        STD_FATAL("Failed to reserve memory for static memory pool.");
    m_max_reserve = max_reserve;
}

StaticMemoryPool::~StaticMemoryPool()
{
    shrink_pool(0);
    m_head = 0;
}

// Commit more memory no more than m_max_reserve
bool StaticMemoryPool::extend_pool(size_t new_reserve)
{
    // Align to PAGE_SIZE
    new_reserve = (new_reserve + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (new_reserve > m_max_reserve)
        m_reserve = m_max_reserve;

    if (new_reserve <= m_reserve)
    {
        STD_ASSERT(("Extend-size is smaller than current reserved size.", new_reserve == m_reserve));
        return false;
    }

    // Commit new memory
    size_t delta_size = new_reserve - m_reserve;
    std_mmap(m_head + m_reserve, delta_size, STD_PAGE_COMMIT);
    m_reserve = new_reserve;
}

// Reset memory
void StaticMemoryPool::shrink_pool(size_t new_reserve)
{
    // Align to PAGE_SIZE
    new_reserve = (new_reserve + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (new_reserve >= m_reserve)
    {
        STD_ASSERT(("Shrink-size is larger than current reserved size.", new_reserve == m_reserve));
        return;
    }

    // Reset committed memory
    size_t delta_size = m_reserve - new_reserve;
    std_mmap(m_head + new_reserve, delta_size, STD_PAGE_RESET);
    m_reserve = new_reserve;
}

DynamicMemoryPool::DynamicMemoryPool(size_t max_reserve, size_t init_size = 0) :
    MemoryPool()
{
    if (!init_size)
        init_size = max_reserve;

    m_head = (char*)STD_MEM_ALLOC(init_size);
    if (!m_head)
        STD_FATAL("Failed to allocate memory for dynamic memory pool.");
    m_size = init_size;
    m_max_reserve = max_reserve;
}

DynamicMemoryPool::~DynamicMemoryPool()
{
    STD_MEM_FREE(m_head);
    m_head = 0;
}

// Commit more memory no more than m_max_reserve
bool DynamicMemoryPool::extend_pool(size_t new_reserve)
{
    // Align to PAGE_SIZE
    new_reserve = (new_reserve + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (new_reserve > m_max_reserve)
        m_reserve = m_max_reserve;

    if (new_reserve <= m_reserve)
    {
        STD_ASSERT(("Extend-size is smaller than current reserved size.", new_reserve == m_reserve));
        return false;
    }

    // Allocate new memory pool
    auto* new_pool = (char*)STD_MEM_ALLOC(new_reserve);
    if (!new_pool)
        // Failed to allocate new memory pool
        return false;
    memcpy(new_pool, m_head, m_reserve);
    STD_MEM_FREE(m_head);
    m_head = new_pool;
    m_reserve = new_reserve;
}

// Reset memory
void DynamicMemoryPool::shrink_pool(size_t new_reserve)
{
    // Align to PAGE_SIZE
    new_reserve = (new_reserve + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (new_reserve >= m_reserve)
    {
        STD_ASSERT(("Shrink-size is larger than current reserved size.", new_reserve == m_reserve));
        return;
    }

    // Allocate new memory pool
    auto* new_pool = (char*)STD_MEM_ALLOC(new_reserve);
    if (!new_pool)
        // Failed to allocate new memory pool
        return;
    memcpy(new_pool, m_head, new_reserve);
    STD_MEM_FREE(m_head);
    m_head = new_pool;
    m_reserve = new_reserve;
}

}
