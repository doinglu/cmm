// simple_xnew.h

#pragma once

#ifndef XNEW
// Other may replace the XNEW with themselves' definition

#include "std_memmgr/std_memmgr.h"
#include "simple_util.h"

namespace simple
{

#define REQUIRE_LOCATION

// new
template <typename T, typename... Types>
inline T* xnew(const char* file, int line, Types&&... args)
{
    T* p = (T*)std_allocate_memory(sizeof(T), "simple", file, line);
    if (!p)
        return 0;
    new (p) T(simple::forward<Types>(args)...);
    return p;
}

// delete
template <typename T>
inline void xdelete(const char* file, int line, T* p)
{
    p->~T();
    std_free_memory(p, "x", file, line);
}

enum { RESERVE_FOR_ARRAY = STD_BEST_ALIGN_SIZE };
enum { RESERVE_STAMP = 0x19770531 };

// new[]
template <typename T>
T* xnew_arr(const char* file, int line, size_t n)
{
    STD_ASSERT(("Invalid RESERVE_FOR_ARRAY, too small.", RESERVE_FOR_ARRAY >= sizeof(size_t) * 2));
    char* b = (char*)std_allocate_memory(RESERVE_FOR_ARRAY + n * sizeof(T), "simple", file, line);
    if (!b)
        return 0;
    auto& count = ((size_t*) b)[0];
    auto& stamp = ((size_t*) b)[1];
    count = 0;
    stamp = RESERVE_STAMP;
    T* p = (T*)(b + RESERVE_FOR_ARRAY);
    init_class(p, n, count);
    return p;
}

// delete[]
template <typename T>
void xdelete_arr(const char* file, int line, T* p)
{
    char* b = ((char*)p) - RESERVE_FOR_ARRAY;
    STD_ASSERT(("Bad stamp of allocated array.", ((size_t*)b)[1] == RESERVE_STAMP));
    size_t n = ((size_t*)b)[0];
    for (size_t i = 0; i < n; i++, p++)
        p->~T();
    std_free_memory(b, "simple", file, line);
}

// Macro as operators
#define XNEW(T, ...)    simple::xnew<T>(__FILE__, __LINE__, ##__VA_ARGS__)
#define XDELETE(p)      { simple::xdelete(__FILE__, __LINE__, p); (p) = 0; }
#define XNEWN(T, n)     simple::xnew_arr<T>(__FILE__, __LINE__, n)
#define XDELETEN(p)     { simple::xdelete_arr(__FILE__, __LINE__, p); (p) = 0; }

} // End of namespace: simple

#endif // XNEW
