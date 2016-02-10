// cmm_value_list.h

#pragma once

#include "cmm.h"
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
#if USE_VECTOR_IN_VALUE_LIST
    typedef simple::unsafe_vector<ReferenceImpl*> ContainerType;
#else
    typedef simple::hash_set<ReferenceImpl*> ContainerType;
#endif

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
    size_t get_count() { return m_container.size(); }

    // Return the address of values
#if USE_VECTOR_IN_VALUE_LIST
    ReferenceImpl** get_head_address() { return m_container.get_array_address(0); }
#endif

    // Return the head of list
    ContainerType& get_container() { return m_container; }

    // Remove a value
    void remove(ReferenceImpl* value);

private:
    // Reset without free
    void reset()
    {
        m_container.clear();
        m_high = 0;
        m_low = (ReferenceImpl*)(size_t)-1;
    }

private:
    // List of all reference values
    ContainerType m_container;
    ReferenceImpl* m_high;
    ReferenceImpl* m_low;
};

// Strcuture using by GC
struct MarkValueState
{
public:
    ValueList* value_list;
    ValueList::ContainerType* container;
#if USE_VECTOR_IN_VALUE_LIST
    ReferenceImpl** impl_ptrs_address;
#endif
    void* low;      // Low bound of all pointers
    void* high;     // High bound of all pointers 

public:
    MarkValueState(ValueList* value_list);

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
