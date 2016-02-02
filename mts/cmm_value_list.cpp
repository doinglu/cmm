// cmm_value_list.cpp

#include "cmm.h"
#include "cmm_value_list.h"
#include "cmm_value.h"

namespace cmm
{

// Append a new value to list
void ValueList::append_value(ReferenceImpl* value)
{
    STD_ASSERT(("The value was already owned by a list.", !value->owner));
    value->owner = this;
    value->offset = get_count();
    m_list.push_back(value);

    if (m_high < value)
        m_high = value;
    if (m_low > value)
        m_low = value;
}

// Conact two memory list then clear one
void ValueList::concat_list(ValueList* list)
{
    size_t list_count = list->get_count();
    if (!list_count)
        // Target list is empry
        return;

    // Concat list to my list
    auto* head_address = list->get_head_address();
    size_t offset = this->get_count();
    for (size_t i = 0; i < list_count; i++)
    {
        STD_ASSERT(("Bad owner of value in list when concating.", (head_address[i])->owner == list));
        head_address[i]->offset = offset++;
    }
    m_list.push_back_array(head_address, list_count);

    // Clear target list
    list->reset();
}

// Remove a value from list
void ValueList::remove(ReferenceImpl* value)
{
    STD_ASSERT(("Value is not in this list.", value->owner == this));
    STD_ASSERT(("Value offset is invalid.", value->offset < get_count()));
    STD_ASSERT(("Value is not in the specified offset.", m_list[value->offset] == value));

    // Replace with tail element & shrink
    auto value_offset = value->offset;
    auto tail_offset = get_count() - 1;
    m_list[value_offset] = m_list[tail_offset];
    m_list[value_offset]->offset = value_offset;
    m_list[tail_offset] = 0;
    m_list.shrink(tail_offset);

    // Remove owner
    value->owner = 0;
}

// Free all linked values in list
void ValueList::free()
{
    auto* p = get_head_address();
    auto list_count = get_count();
    for (size_t i = 0; i < list_count; i++)
        XDELETE(p[i]);
    m_list.clear();
}

// Constructor
MarkValueState::MarkValueState(ValueList* _list) :
    list(_list),
    impl_ptrs_address(_list->get_head_address()),
    impl_ptrs_count(_list->get_count())
{
    low = (void*)list->m_low;
    high = (void*)((char*)list->m_high + sizeof(BufferImpl) + BufferImpl::RESERVE_FOR_CLASS_ARR);
}

// Mark value
void MarkValueState::mark_value(ReferenceImpl* ptr_value)
{
    // Try remove from set
    size_t offset = ptr_value->offset;
    if (offset < impl_ptrs_count && impl_ptrs_address[offset] == ptr_value)
    {
        // Got the valid pointer
        if (!ptr_value->owner)
            // Already marked
            return;

        // set owner to 0 means marked already
        ptr_value->owner = 0;
        ptr_value->mark(*this);
        return;
    }

    // Not valid pointer, is this a class pointer?
    auto* buffer_impl = (BufferImpl *)(((char*)ptr_value) - BufferImpl::RESERVE_FOR_CLASS_ARR - sizeof(BufferImpl));
    offset = buffer_impl->offset;
    if (offset < impl_ptrs_count && impl_ptrs_address[offset] == buffer_impl)
    {
        // Got the valid pointer
        if (!buffer_impl->owner)
            // Already marked
            return;

        // set owner to 0 means marked already
        buffer_impl->owner = 0;
        buffer_impl->mark(*this);
    }
}

} // End of namespace: cmm
