// std_port_mmap.h
// Initial version Feb/7/2016 by doing
// Wrapper for VirtualAlloc (windows) or mmap (linux)

#ifndef _STD_PORT_MMAP_H_
#define _STD_PORT_MMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define STD_PAGE_READ           0x0010
#define STD_PAGE_WRITE          0x0020
#define STD_PAGE_EXECUTE        0x0040
#define STD_PAGE_ALL_ACCESS     (STD_PAGE_READ | STD_PAGE_WRITE | STD_PAGE_EXECUTE)

#define STD_PAGE_SIZE           4096

extern void* std_mem_reserve(void* address, size_t size);
extern int   std_mem_release(void* address, size_t size);
extern int   std_mem_commit(void* address, size_t size, int flags);
extern int   std_mem_decommit(void* address, size_t size);

// Align size
inline size_t std_align_size(size_t size)
{
    return (size + STD_PAGE_SIZE - 1) & ~(STD_PAGE_SIZE - 1);
}

// Align pointer
inline void* std_align_ptr(void *ptr)
{
    return (void*)std_align_size((size_t)ptr & ~(STD_PAGE_SIZE - 1));
}

#ifdef __cplusplus
}
#endif

#endif
