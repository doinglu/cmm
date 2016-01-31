// cmm_value_list.h

#pragma once

#include "std_template/simple_hash_set.h"

namespace cmm
{

struct BufferImpl;
struct ReferenceImpl;

// Values list for thread/domain, using by GC
class ValueList
{
public:
    ValueList()
    {
        reset();
    }

public:
    // Bind value to list
    void append_value(ReferenceImpl* value);

    // Concat two lists
    void concat_list(ValueList *list);

    // Free all linked values in list
    void free();

    // Return the count of total values
    size_t get_count() { return m_count; }

    // Return the head of list
    ReferenceImpl* get_list() { return m_list; }

    // Remove a value
    void remove(ReferenceImpl* value);

    // Reset the list (don't free linked values)
    void reset()
    {
        m_list = 0;
        m_count = 0;
    }

private:
    // List of all reference values
    ReferenceImpl* m_list;
    size_t m_count;
};

// Strcuture using by GC
struct MarkValueState
{
public:
    simple::unsafe_vector<ReferenceImpl*> impl_ptrs;
    simple::hash_map<void*, BufferImpl*> class_ptrs;
    ValueList *list;
    void *low;      // Low bound of all pointers
    void *high;     // High bound of all pointers 

public:
    MarkValueState(ValueList *_list) :
        impl_ptrs(_list->get_count()),
        class_ptrs(_list->get_count())
    {
        list = _list;
    }

public:
    // Is the pointer possible be a valid ReferenceImpl* ?
    bool is_possible_pointer(void* p)
    {
        // For all valid pointer, the last N bits should be zero
        const IntR mask = sizeof(void*) - 1;
        return (((IntR)p & mask) == 0 && p >= low && p <= high);
    }

    // Mark the possible pointer
    void mark_value(ReferenceImpl* ptr_value);
};

} // End of namespace: cmm
