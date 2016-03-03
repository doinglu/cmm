// simple_allocator.h
// Initial version by doing

#pragma once

#include <new>
#include "std_memmgr/std_memmgr.h"

namespace simple
{

class Allocator
{
private:
public:
    template <typename T, typename... Types>
    T* new1(const char *file, int line, Types&&... args)
    {
        T* p = (T*)alloc(file, line, sizeof(T));
        if (!p)
            return 0;
        new (p) T(simple::forward<Types>(args)...);
        return p;
    }

    template<typename T>
    void delete1(const char *file, int line, T *p)
    {
        p->~T();
        free(file, line, p);
    }

    template<typename T>
    T* newn(const char *file, int line, size_t n)
    {
        STD_ASSERT(("Invalid RESERVE_FOR_ARRAY, too small.", RESERVE_FOR_ARRAY >= sizeof(size_t) * 2));
        char* b = (char*)alloc(file, line, RESERVE_FOR_ARRAY + n * sizeof(T));
        if (!b)
            return 0;
        auto& count = ((size_t*)b)[0];
        auto& stamp = ((size_t*)b)[1];
        count = 0;
        stamp = RESERVE_STAMP;
        T* p = (T*)(b + RESERVE_FOR_ARRAY);
        init_class(p, n, count);
        return p;
    }

    template<typename T>
    void deleten(const char *file, int line, T *p)
    {
        char* b = ((char*)p) - RESERVE_FOR_ARRAY;
        STD_ASSERT(("Bad stamp of allocated array.", ((size_t*)b)[1] == RESERVE_STAMP));
        size_t n = ((size_t*)b)[0];
        for (size_t i = 0; i < n; i++, p++)
            p->~T();
        free(file, line, b);
    }

public:
    virtual void *alloc(const char *file, int line, size_t size);
    virtual void free(const char* file, int line, void *p);

public:
    // Default allocator
    static Allocator g;
};

} // End of namespace: simple
