// std_port_mmap.h
// Initial version Feb/7/2016 by doing
// Wrapper for VirtualAlloc (windows) or mmap (linux)

#ifndef _STD_PORT_MMAP_H_
#define _STD_PORT_MMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define STD_PAGE_COMMIT         0x0001
#define STD_PAGE_RESERVE        0x0002
#define STD_PAGE_RESET          0x0004
#define STD_PAGE_READ           0x0010
#define STD_PAGE_WRITE          0x0020
#define STD_PAGE_EXECUTE        0x0040
#define STD_PAGE_ALL_ACCESS     (STD_PAGE_READ | STD_PAGE_WRITE | STD_PAGE_EXECUTE)

extern void* std_mmap(void* address, size_t size, int flags);

#ifdef __cplusplus
}
#endif

#endif
