// cmm_memory.cpp

#include <stdio.h>
#include <stdlib.h>
#include "std_memmgr/std_memmgr.h"
#include "cmm_memory.h"

#if 1
void *operator new(size_t size)
{
    void *p;
    if (std_is_mem_mgr_installed())
    {
        p = STD_MEM_ALLOC(size);
    }
    else
    {
        p = malloc(size);
    }
    return p;
}

void operator delete(void *p) throw()
{
    if (std_is_mem_mgr_installed())
    {
        STD_MEM_FREE(p);
    }
    else
    {
        free(p);
    }
}
#endif

namespace cmm
{

// Initialize the memory manager
AutoInitMemoryManager::AutoInitMemoryManager()
{
    printf("Init memmgr.\n");
    std_init_mem_mgr();
}

// Shutdown the memory manager
AutoInitMemoryManager::~AutoInitMemoryManager()
{
    printf("Shutdown memmgr.\n");
    std_shutdown_mem_mgr();
}

} // End of namespace: cmm
