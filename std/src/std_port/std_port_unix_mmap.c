// std_port_unix_mmap.c
// Initial version Feb/10/2016 by doing
// Wrapper for VirtualAlloc (windows) or mmap (linux)

#include "std_port/std_port_platform.h"

#ifdef _UNIX

#include "std_port/std_port.h"
#include "std_port/std_port_mmap.h"
#include <sys/mman.h>

// Get privilege from flags
static int _std_get_page_privilege(int flags);

// Reserve memory, can not be allocated by others
// Argument address can be 0 or specified address
extern void* std_mem_reserve(void* address, size_t size)
{
    void *p = mmap(address, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == NULL)
        STD_TRACE("std_port_mem_reserve(mmap).Error = %d.\n", errno);
    return p;
}

// Free previously reserved memory
// Return 1 means OK
extern int std_mem_release(void* address, size_t size)
{
    if (munmap(address, size) == 0)
        return 1;

    STD_TRACE("std_port_mem_release(munmap).Error = %d.\n", errno);
    return 0;
}

// Commit memory to access
// The address can not be NULL
// Return 1 means OK
extern int std_mem_commit(void* address, size_t size, int flags)
{
    int protect = _std_get_page_privilege(flags);

    if (mprotect(address, size, protect) == 0)
        return 1;

    STD_TRACE("std_port_mem_commit(mprotect).Error = %d.\n", errno);
    return 0;
}

// Decommit memory, stop acessing
// The address can not be NULL
// Return 1 means OK
extern int std_mem_decommit(void* address, size_t size)
{
    if (mprotect(address, size, PROT_NONE) == 0)
        return 1;

    STD_TRACE("std_port_mem_decommit(mprotect).Error = %d.\n", errno);
    return 0;
}

// Change pages' privilege
extern int std_mem_protect(void* address, size_t size, int flags)
{
    int protect = _std_get_page_privilege(flags);

    if (mprotect(address, size, protect) == 0)
        return 1;

    STD_TRACE("std_port_mem_protect(mprotect).Error = %d.\n", errno);
    return 0;
}

// Get privilege from flags
static int _std_get_page_privilege(int flags)
{
    int protect;

    if (flags & STD_PAGE_NO_ACCESS)
        return PROT_NONE;

    switch (flags & (STD_PAGE_READ | STD_PAGE_WRITE | STD_PAGE_EXECUTE))
    {
    case STD_PAGE_READ:
        protect = PROT_READ;
        break;

    case STD_PAGE_EXECUTE:
        protect = PROT_EXEC;
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

    return protect;
}

#endif  /* End of _WINDOWS */
