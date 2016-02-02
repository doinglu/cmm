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
friend struct MarkValueState;

public:
    typedef simple::unsafe_vector<ReferenceImpl*> ListType;

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
    size_t get_count() { return m_list.size(); }

    // Return the address of values
    ReferenceImpl** get_head_address() { return m_list.get_array_address(0); }

    // Return the head of list
    ListType& get_list() { return m_list; }

    // Remove a value
    void remove(ReferenceImpl* value);

private:
    // Reset without free
    void reset()
    {
        m_list.clear();
        m_high = 0;
        m_low = (ReferenceImpl*)(size_t)-1;
    }

private:
    // List of all reference values
    ListType m_list;
    ReferenceImpl* m_high;
    ReferenceImpl* m_low;
};

// Strcuture using by GC
struct MarkValueState
{
public:
    ReferenceImpl** impl_ptrs_address;
    size_t impl_ptrs_count;
    simple::hash_map<void*, BufferImpl*> class_ptrs;
    ValueList* list;
    void* low;      // Low bound of all pointers
    void* high;     // High bound of all pointers 

public:
    MarkValueState(ValueList* _list);

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
