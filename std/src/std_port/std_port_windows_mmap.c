// std_port_windows_mmap.c
// Initial version Feb/7/2016 by doing
// Wrapper for VirtualAlloc (windows) or mmap (linux)

#include "std_port/std_port_platform.h"

#ifdef _WINDOWS

#include "std_port/std_port.h"
#include "std_port/std_port_mmap.h"
#include <windows.h>

void* std_mmap(void* address, size_t size, int flags)
{
    DWORD allocation_type = 0;
    DWORD protect = 0;

    if (flags & STD_PAGE_COMMIT)
        allocation_type |= MEM_COMMIT;

    if (flags & STD_PAGE_RESERVE)
        allocation_type |= MEM_RESERVE;

    if (flags & STD_PAGE_RESET)
        allocation_type |= MEM_RESET;

    if (!allocation_type)
        STD_FATAL("Bad flags, expect STD_PAGE_COMMIT|RESERVE|RESET.");

    switch (flags & (STD_PAGE_READ | STD_PAGE_WRITE | STD_PAGE_EXECUTE))
    {
    case STD_PAGE_READ:
        protect = PAGE_READONLY;
        break;

    case STD_PAGE_EXECUTE:
        protect= PAGE_EXECUTE;
        break;

    case STD_PAGE_EXECUTE | STD_PAGE_READ:
        protect = PAGE_EXECUTE_READ;
        break;

    case STD_PAGE_EXECUTE | STD_PAGE_READ | STD_PAGE_WRITE:
        protect = PAGE_EXECUTE_READWRITE;
        break;

    case 0:
    case STD_PAGE_READ | STD_PAGE_WRITE:
        protect = PAGE_READWRITE;
        break;

    default:
        STD_FATAL("Bad flags of access.");
    }

    return VirtualAlloc(address, size, allocation_type, protect);
}

#endif  /* End of _WINDOWS */
