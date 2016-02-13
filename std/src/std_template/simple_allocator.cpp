// simple_allocator.c
// Initial version Feb/10/2016 by doing

#include "std_template/simple.h"
#include "std_template/simple_allocator.h"
#include "std_memmgr/std_memmgr.h"

namespace simple
{

void *Allocator::alloc(const char *file, int line, size_t size)
{
    return std_allocate_memory(size, "allocator", file, line);
}

void Allocator::free(const char* file, int line, void *p)
{
    std_free_memory(p, "allocator", file, line);
}

// Global allocator
Allocator Allocator::g;

} // End of namespace: simple
