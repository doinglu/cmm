// std_port_windows_mmap.c
// Initial version Feb/7/2016 by doing
// Wrapper for VirtualAlloc (windows) or mmap (linux)

#include "std_port/std_port_platform.h"

#ifdef _WINDOWS

#include "std_port/std_port.h"
#include "std_port/std_port_mmap.h"
#include <windows.h>

// Get privilege from flags
static int _std_get_page_privilege(int flags);

// Reserve memory, can not be allocated by others
// Argument address can be 0 or specified address
extern void* std_mem_reserve(void* address, size_t size)
{
    void* p = VirtualAlloc(address, size, MEM_RESERVE, PAGE_NOACCESS);
    if (p == NULL)
        STD_TRACE("std_port_mem_reserve(VirtualAlloc).Error = %d.\n", (int)GetLastError());
    return p;
}

// Free previously reserved memory
// Return 1 means OK
extern int std_mem_release(void* address, size_t size)
{
    if (VirtualFree(address, 0, MEM_RELEASE))
        return 1;

    STD_TRACE("std_port_mem_release(VirtualFree).Error = %d.\n", (int)GetLastError());
    return 0;
}

// Commit memory to access
// The address can not be NULL
// Return 1 means OK
extern int std_mem_commit(void* address, size_t size, int flags)
{
    int protect = _std_get_page_privilege(flags);
    void *p;


    p = VirtualAlloc(address, size, MEM_COMMIT, protect);
    if (p != NULL)
        return 1;

    STD_TRACE("std_port_mem_commit(VirtualAlloc).Error = %d.\n", (int)GetLastError());
    return 0;
}

// Decommit memory, stop acessing
// The address can not be NULL
// Return 1 means OK
extern int std_mem_decommit(void* address, size_t size)
{
    if (VirtualFree(address, size, MEM_DECOMMIT))
        return 1;

    STD_TRACE("std_port_mem_decommit(VirtualFree).Error = %d.\n", (int)GetLastError());
    return 0;
}

// Change pages' privilege
extern int std_mem_protect(void* address, size_t size, int flags)
{
    DWORD old;
    DWORD protect = (DWORD)_std_get_page_privilege(flags);

    if (VirtualProtect(address, size, protect, &old))
        return 1;

    STD_TRACE("std_port_mem_protect(mprotect).Error = %d.\n", (int)GetLastError());
    return 0;
}

// Get privilege from flags
static int _std_get_page_privilege(int flags)
{
    DWORD protect = 0;

    if (flags & STD_PAGE_NO_ACCESS)
        return PAGE_NOACCESS;

    switch (flags & (STD_PAGE_READ | STD_PAGE_WRITE | STD_PAGE_EXECUTE))
    {
    case STD_PAGE_READ:
        protect = PAGE_READONLY;
        break;

    case STD_PAGE_EXECUTE:
        protect = PAGE_EXECUTE;
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

    return protect;
}

#endif  /* End of _WINDOWS */
