// cmm_memory.cpp

#include <stdio.h>
#include <stdlib.h>
#include "std_memmgr/std_memmgr.h"
#include "cmm_memory.h"

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
