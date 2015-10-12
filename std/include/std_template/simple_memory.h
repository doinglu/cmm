// simple_memory.h

#pragma once

#include <new>
#include "std_memmgr/std_memmgr.h"

namespace simple
{

template <typename T, typename... Types>
T *xnew(const char *file, int line, Types&&... args)
{
    T *p = (T *)std_allocate_memory(sizeof(T), "x", file, line);
    new (p) T(args...);
    return p;
}

template <typename T>
void xdelete(T *p, const char *file, int line)
{
    p->~T();
    std_free_memory(p, "x", file, line);
}

enum { RESERVE_FOR_ARRAY = 16 };

template <typename T>
T *xnew_arr(size_t n, const char *file, int line)
{
    char *b = (char *)std_allocate_memory(RESERVE_FOR_ARRAY + n * sizeof(T), "x", file, line);
    *(size_t *) b = n;
    T *p = (T *)(b + RESERVE_FOR_ARRAY);
    for (size_t i = 0; i < n; i++, p++)
        new (p) T();
    return (T *)(b + RESERVE_FOR_ARRAY);
}

template <typename T>
void xdelete_arr(T *p, const char *file, int line)
{
    char *b = ((char *)p) - RESERVE_FOR_ARRAY;
    size_t n = *(size_t *)b;
    for (size_t i = 0; i < n; i++, p++)
        p->~T();
    std_free_memory(b, "x", file, line);
}

#define XNEW(T, ...)    simple::xnew<T>(__FILE__, __LINE__, __VA_ARGS__)
#define XDELETE(p)      { simple::xdelete(p, __FILE__, __LINE__); (p) = 0; }

#define XNEWN(T, n)     simple::xnew_arr<T>(n, __FILE__, __LINE__)
#define XDELETEN(p)     simple::xdelete_arr(p, __FILE__, __LINE__);


} // End of namespace: simple
