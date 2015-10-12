// cmm_value_list.cpp

#include "cmm_value_list.h"
#include "cmm_value.h"

namespace cmm
{

// Append a new value to list
void ValueList::append_value(ReferenceValue *value)
{
#ifdef _DEBUG
    STD_ASSERT(("The value was already owned by a list.", !value->owner));
    value->owner = this;
#endif
    *m_pp_last = value;
    m_pp_last = &value->next;
    ++m_count;
}

// Conact two memory list then clear one
void ValueList::concat_list(ValueList *list)
{
    if (!list->m_list)
        // Target list is empry
        return;

#ifdef _DEBUG
    // Update owner of the values in list
    auto *p = list->m_list;
    while (p)
    {
        STD_ASSERT(("Bad owner of value in list when concating.", p->owner == list));
        p->owner = this;
        p = p->next;
    }
#endif

    *m_pp_last = list->m_list;
    m_pp_last = list->m_pp_last;
    m_count += list->m_count;

    // Clear target list
    list->reset();
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

} // End of namespace: cmm
