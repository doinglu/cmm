// cmm_value_list.cpp

#include "cmm_value_list.h"
#include "cmm_value.h"

namespace cmm
{

// Append a new value to list
void ValueList::append_value(ReferenceImpl *value)
{
    STD_ASSERT(("The value was already owned by a list.", !value->owner));
    value->owner = this;
    value->next = m_list;
    m_list = value;
    ++m_count;
}

// Conact two memory list then clear one
void ValueList::concat_list(ValueList *list)
{
    if (!list->m_list)
        // Target list is empry
        return;

    // Update owner of the values in list
    auto *pp = &list->m_list;
    while (*pp)
    {
        STD_ASSERT(("Bad owner of value in list when concating.", (*pp)->owner == list));
        (*pp)->owner = this;
        pp = &(*pp)->next;
    }

    *pp = m_list;
    m_list = list->m_list;
    m_count += list->m_count;

    // Clear target list
    list->reset();
}

// Remove a value from list
void ValueList::remove(ReferenceImpl *value)
{
    STD_ASSERT(("Value is not in this list.", value->owner == this));
    auto *pp = &m_list;
    while (*pp != value)
        pp = &(*pp)->next;

    // Take off me from list
    (*pp)->owner = 0;
    *pp = value->next;
}

// Free all linked values in list
void ValueList::free()
{
    auto *p = m_list;
    reset();

    while (p)
    {
        auto *value = p;
        p = p->next;

        // Free it
        XDELETE(value);
    }
}

// Mark value
void MarkValueState::mark_value(ReferenceImpl *ptr_value)
{
    // Try remove from set
    if (!set.erase(ptr_value))
    {
        // Not in set, is this a class pointer?

        if (!class_ptrs.size())
            // Class pointer map is empty
            return;

        auto it = class_ptrs.find(ptr_value);
        if (it == class_ptrs.end())
            // Not found in class pointer map
            return;

        // Found raw BufferImpl contains this class
        class_ptrs.erase(ptr_value);
        if (!set.erase(it->second))
            // The BufferImpl was marked, ignored
            return;
    }

    // Put back to list
    ptr_value->owner = 0;
    list->append_value(ptr_value);

    // Let the ReferenceImpl mark
    ptr_value->mark(*this);
}

} // End of namespace: cmm
