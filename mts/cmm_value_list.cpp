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

#if USE_LIST_IN_VALUE_LIST
    m_container.append_node(value);
#elif USE_VECTOR_IN_VALUE_LIST
    value->offset = get_count();
    m_container.push_back(value);
#else
    m_container.put(value);
#endif

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

    // Concat to my list
#if USE_LIST_IN_VALUE_LIST
    while (list->m_container.size() > 0)
    {
        auto* node = list->m_container.begin().get_node();
        STD_ASSERT(("The value was not owned by the list.", node->owner == list));

        // Take off
        list->m_container.remove_node(node);

        // Join me
        node->owner = this;
        if (m_low > node)
            m_low = node;
        if (m_high < node)
            m_high = node;
        m_container.append_node(node);
    }
#elif USE_VECTOR_IN_VALUE_LIST
    auto* head_address = list->get_head_address();
    size_t offset = this->get_count();
    for (size_t i = 0; i < list_count; i++)
    {
        STD_ASSERT(("Bad owner of value in list when concating.", (head_address[i])->owner == list));
        auto* p = head_address[i];
        p->offset = offset++;
        if (m_low > p)
            m_low = p;
        if (m_high < p)
            m_high = p;
    }
    m_container.push_back_array(head_address, list_count);

#else
    for (auto& it : list->m_container)
    {
        STD_ASSERT(("Bad owner of value in list when concating.", it->owner == list));
        it->owner = this;
        if (m_low > it)
            m_low = it;
        if (m_high < it)
            m_high = it;
        m_container.put(it);
    }
#endif

    // Clear target list
    list->reset();
}

// Remove a value from list
void ValueList::remove(ReferenceImpl* value)
{
    STD_ASSERT(("Value is not in this list.", value->owner == this));

#if USE_LIST_IN_VALUE_LIST
    ////----STD_ASSERT(("Value is not in this container.", m_container.find_node(value) != m_container.end()));
    m_container.remove_node(value);
#elif USE_VECTOR_IN_VALUE_LIST
    STD_ASSERT(("Value offset is invalid.", value->offset < get_count()));
    STD_ASSERT(("Value is not in the specified offset.", m_container[value->offset] == value));

    // Replace with tail element & shrink
    auto value_offset = value->offset;
    auto tail_offset = get_count() - 1;
    m_container[value_offset] = m_container[tail_offset];
    m_container[value_offset]->offset = value_offset;
    m_container[tail_offset] = 0;
    m_container.shrink(tail_offset);
#else
    // Replace with tail element & shrink
    m_container.erase(value);
#endif

    // Remove owner
    value->owner = 0;
}

// Free all linked values in list
void ValueList::free()
{
#if USE_LIST_IN_VALUE_LIST
    while (m_container.size() > 0)
    {
        auto* node = m_container.begin().get_node();
        node->unbind();
        XDELETE(node);
    }
#elif USE_VECTOR_IN_VALUE_LIST
    auto* p = get_head_address();
    auto list_count = get_count();
    for (size_t i = 0; i < list_count; i++)
    {
        // Set owner to 0 to prevent unbind when destructing
        p[i]->owner = 0;
        XDELETE(p[i]);
}
#else
    for (auto& it : m_container)
    {
        // Set owner to 0 to prevent unbind when destructing
        it->owner = 0;
        XDELETE(it);
    }
#endif

    // Clear
    reset();
}

// Constructor
MarkValueState::MarkValueState(ValueList* _value_list) :
    value_list(_value_list),
    container(&_value_list->get_container())
{
#if USE_LIST_IN_VALUE_LIST
    // Do nothing
#elif USE_VECTOR_IN_VALUE_LIST
    impl_ptrs_address = value_list->get_head_address();
#endif
    low = (void*)value_list->m_low;
    high = (void*)((char*)value_list->m_high + sizeof(BufferImpl) + BufferImpl::RESERVE_FOR_CLASS_ARR);
}

#if 0
// Mark value
void MarkValueState::mark_value(ReferenceImpl* ptr_value)
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

        // set owner to 0 means marked already
        buffer_impl->owner = 0;
        buffer_impl->mark(*this);
    }
}
#endif

} // End of namespace: cmm
