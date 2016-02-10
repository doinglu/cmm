// std_port_unix_mmap.c
// Initial version Feb/10/2016 by doing
// Wrapper for VirtualAlloc (windows) or mmap (linux)

#include "std_port/std_port_platform.h"

#ifdef _UNIX

#include "std_port/std_port.h"
#include "std_port/std_port_mmap.h"
#include <sys/mman.h>

extern void* std_mmap(void* address, size_t size, int flags)
{
    int protect = 0;

    switch (flags & (STD_PAGE_READ | STD_PAGE_WRITE | STD_PAGE_EXECUTE))
    {
    case STD_PAGE_READ:
        protect = PROT_READ;
        break;

    case STD_PAGE_EXECUTE:
        protect= PROT_EXEC;
        break;

    case STD_PAGE_EXECUTE | STD_PAGE_READ:
        protect = PROT_EXEC | PROT_READ;
        break;

    case STD_PAGE_EXECUTE | STD_PAGE_READ | STD_PAGE_WRITE:
        protect = PROT_EXEC | PROT_READ | PROT_WRITE;
        break;

    case 0:
    case STD_PAGE_READ | STD_PAGE_WRITE:
        protect = PROT_READ | PROT_WRITE;
        break;

    default:
        STD_FATAL("Bad flags of access.");
    }

    void *ret_address = mmap(address, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (ret_address && (flags & STD_PAGE_COMMIT))
        mprotect(ret_address, size, protect);

    return ret_address;
}

#endif  /* End of _WINDOWS */
