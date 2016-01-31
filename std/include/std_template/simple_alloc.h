// simple_alloc.h

#pragma once

#include "simple_memory.h"

namespace simple
{

struct XAlloc
{
    template <typename T, typename... Types>
    static T* new1(const char *file, int line, Types&&... args)
    {
        return xnew<T>(file, line, simple::forward<Types>(args)...);
    }

    template<typename T>
    static void delete1(const char *file, int line, T *p)
    {
        xdelete<T>(file, line, p);
    }

    template<typename T>
    static T* newn(const char *file, int line, size_t n)
    {
        return xnew_arr<T>(file, line, n);
    }

    template<typename T>
    static void deleten(const char *file, int line, T *p)
    {
        xdelete_arr<T>(file, line, p);
    }
};

} // End of namespace: simple
