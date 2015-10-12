// simple_shared_ptr.h

#pragma once

#include "std_port/std_port.h"
#include "simple.h"

namespace simple
{

// Object include reference counter & class instance
template <typename T>
class ref_obj
{
public:
    template<typename... Types>
    ref_obj(Types&&... args) :
        m_obj(simple::forward<Types>(args)...),
        m_ref_counter(0)
    {
    }

public:
    int m_ref_counter;
    T   m_obj;
};

// Shared pointer to a class
template <typename T>
class shared_ptr
{
public:
    shared_ptr() :
        shared_ptr(NULL)
    {
    }

    shared_ptr(ref_obj<T> *ref_obj)
    {
        m_ref_obj = ref_obj;
        inc_ref();
    }

    shared_ptr(const shared_ptr& from)
    {
        m_ref_obj = from.m_ref_obj;
        inc_ref();
    }

    // Construct from R value
    shared_ptr(shared_ptr&& from)
    {
        m_ref_obj = from.m_ref_obj;
        from.m_ref_obj = NULL;
    }

    ~shared_ptr()
    {
        dec_ref();
        STD_DEBUG_SET_NULL(m_ref_obj);
    }

    int ref_counter() const
    {
        if (! m_ref_obj)
            return 0;

        return m_ref_obj->counter;
    }

    T *ptr() const
    {
        if (! m_ref_obj)
            return NULL;

        return &m_ref_obj->m_obj;
    }

    shared_ptr<T>& operator =(const shared_ptr<T>& s)
    {
        dec_ref();
        m_ref_obj = s.m_ref_obj;
        inc_ref();
        return *this;
    }

    shared_ptr<T>& operator =(shared_ptr<T>&& s)
    {
        dec_ref();
        m_ref_obj = s.m_ref_obj; // Peek the referenced object directly
        s.m_ref_obj = 0;
        return *this;
    }

private:
    // Increase reference counter to object
    void inc_ref()
    {
        if (m_ref_obj)
            ++m_ref_obj->m_ref_counter;
    }

    // Decrease reference counter to object, free when counter down to 0
    void dec_ref()
    {
        if (m_ref_obj)
        {
            if (--m_ref_obj->m_ref_counter <= 0)
                XDELETE(m_ref_obj);
        }
    }

private:
    ref_obj<T> *m_ref_obj;
};

// Create a shared pointer
template<class T, class... Types>
inline shared_ptr<T> make_shared(Types&&... args)
{
    // Create T & make a shared ptr to it
    ref_obj<T> *obj = XNEW(ref_obj<T>, simple::forward<Types>(args)...);
    shared_ptr<T> ret(obj);
    return ret;
}

} // End of namespace: simple
