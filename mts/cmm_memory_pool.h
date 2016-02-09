// cmm_memory_pool.h
// Initial version Feb/7/2016 by doing
// Static & float memory pool

#pragma once

namespace cmm
{

class MemoryPool
{
public:
    MemoryPool();
    virtual ~MemoryPool();

public:
    void* allocate(size_t size);
    void  free(size_t begin);
    void* try_allocate(size_t size);

public:
    char* get_head()
    {
        return m_head;
    }

    char* get_tail()
    {
        return m_head + m_size;
    }

    size_t get_size()
    {
        return m_size;
    }

    size_t get_reserve()
    {
        return m_reserve;
    }

public:
    // Extend/Shrink pointers memory
    virtual bool extend_pool(size_t new_reserve) = 0;
    virtual void shrink_pool(size_t new_reserve) = 0;

protected:
    const size_t PAGE_SIZE = 8192;
    char*  m_head;
    size_t m_size;
    size_t m_reserve;
};

// Memory without change head address
// Use std_mmap to reserve/free memory
class StaticMemoryPool : public MemoryPool
{
public:
    StaticMemoryPool(size_t max_reserve);
    ~StaticMemoryPool();

public:
    virtual bool extend_pool(size_t new_reserve);
    virtual void shrink_pool(size_t new_reserve);

private:
    size_t m_max_reserve;
};

// Memory may change head address during extend/shrink
// Use std_allocate_memory/free_memory to reserve/free memory
class DynamicMemoryPool : public MemoryPool
{
public:
    DynamicMemoryPool(size_t max_reserve, size_t init_reserve = 0);
    ~DynamicMemoryPool();

public:
    virtual bool extend_pool(size_t new_reserve);
    virtual void shrink_pool(size_t new_reserve);

private:
    size_t m_max_reserve;
};

}