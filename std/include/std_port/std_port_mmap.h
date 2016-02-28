// std_port_mmap.h
// Initial version Feb/7/2016 by doing
// Wrapper for VirtualAlloc (windows) or mmap (linux)

#ifndef _STD_PORT_MMAP_H_
#define _STD_PORT_MMAP_H_

#include "std_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STD_PAGE_READ           0x0010
#define STD_PAGE_WRITE          0x0020
#define STD_PAGE_EXECUTE        0x0040
#define STD_PAGE_NO_ACCESS      0x8000
#define STD_PAGE_ALL_ACCESS     (STD_PAGE_READ | STD_PAGE_WRITE | STD_PAGE_EXECUTE)

extern void* std_mem_reserve(void* address, size_t size);
extern int   std_mem_release(void* address, size_t size);
extern int   std_mem_commit(void* address, size_t size, int flags);
extern int   std_mem_decommit(void* address, size_t size);
extern int   std_mem_protect(void* address, size_t size, int flags);

// Align size
#define std_align_size(size) \
    (((size) + STD_MEM_PAGE_SIZE - 1) & ~(STD_MEM_PAGE_SIZE - 1))

// Align pointer
#define std_align_ptr(ptr) \
    ((void*)((size_t)(ptr) & ~(STD_MEM_PAGE_SIZE - 1)))

#ifdef __cplusplus
}
#endif

#endif
