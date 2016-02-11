// simple_allocator.h
// Initial version by doing

#pragma once

namespace simple
{

class Allocator
{
public:
    template <typename T, typename... Types>
    T* new1(const char *file, int line, Types&&... args)
    {
        return xnew<T>(file, line, simple::forward<Types>(args)...);
    }

    template<typename T>
    void delete1(const char *file, int line, T *p)
    {
        xdelete<T>(file, line, p);
    }

    template<typename T>
    T* newn(const char *file, int line, size_t n)
    {
        return xnew_arr<T>(file, line, n);
    }

    template<typename T>
    void deleten(const char *file, int line, T *p)
    {
        xdelete_arr<T>(file, line, p);
    }

public:
    // Default allocator
    static Allocator g;
};

} // End of namespace: simple
