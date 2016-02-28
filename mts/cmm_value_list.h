// cmm_value_list.h

#pragma once

#include "std_template/simple_hash_set.h"
#include "std_template/simple_list.h"
#include "cmm.h"
#include "cmm_value.h"

namespace cmm
{

struct BufferImpl;
struct ReferenceImpl;

// Values list for thread/domain, using by GC
class ValueList
{
friend struct MarkValueState;

public:
#if USE_LIST_IN_VALUE_LIST
    typedef simple::manual_list<char, ReferenceImpl> ContainerType;
#elif USE_VECTOR_IN_VALUE_LIST
    typedef simple::unsafe_vector<ReferenceImpl*> ContainerType;
#else
    typedef simple::hash_set<ReferenceImpl*> ContainerType;
#endif

public:
    // Constructor
    // name must point to a non-free memory
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

    // Set name of me
    void set_name(const char* name)
    {
        m_name = name;
    }

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
    const char*    m_name;
    ContainerType  m_container;
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
    // Mark value
    inline void mark_value(ReferenceImpl* ptr_value)
    {
        // Try remove from set
#if USE_LIST_IN_VALUE_LIST
        if (true)
#elif USE_VECTOR_IN_VALUE_LIST
        size_t offset = ptr_value->offset;
        if (offset < container->size() && impl_ptrs_address[offset] == ptr_value)
#else
        if (container->contains(ptr_value))
#endif
        {
            // Got the valid pointer
            if (!ptr_value->owner)
                // Already marked
                return;

            STD_ASSERT(("The value will be marked is not belonged to this domain.", ptr_value->owner == value_list));

            // set owner to 0 means marked already
            ptr_value->owner = 0;
            if (ptr_value->need_mark_for_domain_gc())
               ptr_value->mark(*this);
            return;
        }

        // Not valid pointer, is this a class pointer?
        auto* buffer_impl = (BufferImpl *)(((char*)ptr_value) - BufferImpl::RESERVE_FOR_CLASS_ARR - sizeof(BufferImpl));
#if USE_LIST_IN_VALUE_LIST
        // Shouldn't be here
        STD_FATAL("No here.\n");
#elif USE_VECTOR_IN_VALUE_LIST
        offset = buffer_impl->offset;
        if (offset < container->size() && impl_ptrs_address[offset] == buffer_impl)
#else
        if (container->contains(buffer_impl))
#endif
        {
            // Got the valid pointer
            if (!buffer_impl->owner)
                // Already marked
                return;

            STD_ASSERT(("The value will be marked is not belonged to this domain.", buffer_impl->owner == value_list));

            // set owner to 0 means marked already
            buffer_impl->owner = 0;
            buffer_impl->mark(*this);
        }
    }
};

} // End of namespace: cmm
