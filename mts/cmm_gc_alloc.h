// cmm_gc_alloc.h
// GC allocator for simple template

#pragma once

#include "cmm_buffer_new.h"

namespace cmm
{

struct GCAlloc
{
    template <typename T, typename... Types>
    static T* new1(const char *file, int line, Types&&... args)
    {
        return BUFFER_NEW(T, simple::forward<Types>(args)...);
    }

    template <typename T>
    static void delete1(const char *file, int line, T *p)
    {
        BUFFER_DELETE(p);
    }

    template <typename T>
    static T* newn(const char *file, int line, size_t n)
    {
        return BUFFER_NEWN(T, n);
    }

    template <typename T>
    static void deleten(const char *file, int line, T *p)
    {
        BUFFER_DELETEN(p);
    }
};

} // End of namespace: simple
