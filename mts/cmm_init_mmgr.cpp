// cmm_init_mmgr.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std_memmgr/std_memmgr.h"
#include "cmm_init_mmgr.h"

namespace cmm
{

// Initialize the memory manager
AutoInitMemoryManager::AutoInitMemoryManager()
{
    std_init_mgr_para_t para;

    printf("Init memmgr.\n");
    memset(&para, 0, sizeof(para));
    para.options = STD_USE_BA_ALLOC;
#ifdef PLATFORM64
    para.ba_reserve_size = (size_t)1 * 1024 * 1024 * 1024 * 1024;   // 1T for 64bits
#else
    para.ba_reserve_size = (size_t)1 * 1024 * 1024 * 1024;          // 1G for 32bits
#endif
    std_init_mem_mgr(&para);
}

// Shutdown the memory manager
AutoInitMemoryManager::~AutoInitMemoryManager()
{
    printf("Shutdown memmgr.\n");
    std_shutdown_mem_mgr();
}

} // End of namespace: cmm
