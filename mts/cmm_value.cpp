// cmm_value.cpp

#include <stdio.h>

#include "cmm_domain.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

// Try to bind a reference value to a domain's values list.
// This value must be NEW one & never be binded
void ReferenceImpl::bind_to_current_domain()
{
    if (attrib & (ReferenceImpl::CONSTANT | ReferenceImpl::SHARED))
        // For constant/shared value, don't bind
        return;

    Domain *domain = Thread::get_current_thread_domain();
    STD_ASSERT(("No found domain when bind_to_current_domain.", domain));
    if (this->owner)
    {
        // Already binded?
        if (this->owner == domain->get_value_list())
            // Binded to current domain already, ignored
            return;

        throw "Reference value was already binded to other owner.";
    }
    domain->bind_value(this);
}

// Hash this buffer
size_t BufferImpl::hash_this() const
{
    size_t seed = 131; // 31 131 1313 13131 131313 etc..
    size_t h = 0;
    size_t maxn = 64;
    if (maxn > this->len)
        maxn = this->len;
    Uint8 *p = this->data;

    while (maxn--)
        h = h * seed + (*p++);
    if (!h)
        h = 1;
    return h;
}

// Compare two strings, return -1 means less, 1 means greater, 0 means equal
int StringImpl::compare(const StringImpl *a, const StringImpl *b)
{
    size_t len;

    len = a->m_len;
    if (len > b->m_len)
        len = b->m_len;

    return memcmp(a->m_buf, b->m_buf, (len + 1) * sizeof(char_t));
}

// Compare two buffers, return -1 means less, 1 means greater, 0 means equal
int BufferImpl::compare(const BufferImpl *a, const BufferImpl *b)
{
    // For buffer, compare len then data
    if (a->len < b->len)
        return -1;
    if (a->len > b->len)
        return 1;

    return memcmp(a->data, b->data, a->len);
}

// Duplicate string to local
ReferenceImpl *StringImpl::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = StringImpl::alloc_string(this);
    thread->bind_value(v);
    return v;
}

// Duplicate buffer to local
ReferenceImpl *BufferImpl::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = XNEW(BufferImpl, this->data, this->len);
    thread->bind_value(v);
    return v;
}

// Duplicate to local
ReferenceImpl *FunctionPtrImpl::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    ////---- To be added
    return 0;
}

// Duplicate to local
ReferenceImpl *ArrayImpl::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = XNEW(ArrayImpl, this->a.size());
    thread->bind_value(v);
    // Bind before copy elements in case of exception when copying

    auto end = this->a.end();
    for (auto it = this->a.begin(); it != end; ++it)
        v->a.push_back(it->copy_to_local(thread));
    return v;
}

// Duplicate to local
ReferenceImpl *MapImpl::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = XNEW(MapImpl, this->m.size());
    thread->bind_value(v);
    // Bind before copy elements in case of exception when copying

    auto end = this->m.end();
    for (auto it = this->m.begin(); it != end; ++it)
        v->m.put(it->first.copy_to_local(thread), it->second.copy_to_local(thread));
    return v;
}

// Mark all elements in function ptr value
void FunctionPtrImpl::mark(MarkValueState& state)
{
    ////---- To be added
}

// Mark all elements in this container
void ArrayImpl::mark(MarkValueState& state)
{
    auto end = a.end();
    for (auto it = a.begin(); it != end; ++it)
        if (it->m_type >= ValueType::REFERENCE_VALUE)
            Domain::mark_value(state, it->m_reference);
}

// Mark all elements in this container
void MapImpl::mark(MarkValueState& state)
{
    auto end = m.end();
    for (auto it = m.begin(); it != end; ++it)
    {
        if (it->first.m_type >= ValueType::REFERENCE_VALUE)
            Domain::mark_value(state, it->first.m_reference);

        if (it->second.m_type >= ValueType::REFERENCE_VALUE)
            Domain::mark_value(state, it->second.m_reference);
    }
}

Value::Value(Object *ob) :
    Value(ob->get_oid())
{
}

Value::Value(const char *c_str, size_t len)
{
    auto *v = StringImpl::alloc_string(c_str, len);
    v->bind_to_current_domain();
    m_type = STRING;
    m_string = v;
}

Value::Value(const simple::string& str)
{
    auto *v = StringImpl::alloc_string(str);
    v->bind_to_current_domain();
    m_type = STRING;
    m_string = v;
}

// Compare with other value
bool Value::operator <(const Value& b) const
{
    const Value& a = *this;

    // Compare type first
    if (a.m_type < b.m_type)
        return true;
    if (a.m_type > b.m_type)
        return false;

    // Type is same

    if (a.m_type >= ValueType::REFERENCE_VALUE)
    {
        switch (a.m_type)
        {
        case ValueType::STRING: return StringImpl::compare(a.m_string, b.m_string) < 0;
        case ValueType::BUFFER: return BufferImpl::compare(a.m_buffer, b.m_buffer) < 0;
        default: break;
        }
    }

    // Compare intptr only
    return a.m_intptr < b.m_intptr;
}

// Compare with other value
bool Value::operator ==(const Value& b) const
{
    const Value& a = *this;

    if (a.m_type != b.m_type)
        // Type is not same
        return false;

    if (a.m_type >= ValueType::REFERENCE_VALUE)
    {
        switch (a.m_type)
        {
        case ValueType::STRING: return StringImpl::compare(a.m_string, b.m_string) == 0;
        case ValueType::BUFFER: return BufferImpl::compare(a.m_buffer, b.m_buffer) == 0;
        default: break;
        }
    }

    return a.m_intptr == b.m_intptr;
}

// Calculate & cache hash number of the value
size_t Value::hash_value() const
{
    switch (m_type)
    {
    case STRING: return m_string->hash_value();
    case BUFFER: return m_buffer->hash_value();
    default:     return (size_t)m_intptr;
    }
}

// Create a reference value belonged to this domain
Value Value::new_string(Domain *domain, const char *c_str, size_t len)
{
    auto *v = StringImpl::alloc_string(c_str, len);
    STD_ASSERT(domain == Thread::get_current_thread_domain());
    domain->bind_value(v);
    return Value(v);
}

// Create a array value belonged to this domain
Value Value::new_array(Domain *domain, size_t size_hint)
{
    auto *v = XNEW(ArrayImpl, size_hint);
    STD_ASSERT(domain == Thread::get_current_thread_domain());
    domain->bind_value(v);
    return Value(v);
}

// Create a reference value belonged to this domain
Value Value::new_map(Domain *domain, size_t size_hint)
{
    auto *v = XNEW(MapImpl, size_hint);
    STD_ASSERT(domain == Thread::get_current_thread_domain());
    domain->bind_value(v);
    return Value(v);
}

Value& Value::operator [](const Value& value)
{
    switch (m_type)
    {
    case MAPPING: return (*m_map)[value];
    case ARRAY: return (*m_array)[value];
    default: throw "Bad type of value to index.\n";
    }
}

// Get index from container
Value Value::operator [](const Value& value) const
{
    switch (m_type)
    {
    case MAPPING: return (*m_map)[value];
    case ARRAY: return (*m_array)[value];
    default: throw "Bad type of value to index.\n";
    }
}

// Concat with other string
StringImpl *StringImpl::concat_string(const StringImpl *other)
{
    size_t len1 = length();
    size_t len2 = other->length();
    size_t len = len1 + len2;
    auto *string = StringImpl::alloc_string(len);
    char_t *data = (char_t *)string->m_buf;
    memcpy(data, m_buf, len1 * sizeof(char_t));
    memcpy(data + len1, other->m_buf, len2 * sizeof(char_t));
    data[len] = 0;

    // Replace with new string
    return string;
}

// Get sub string
StringImpl *StringImpl::sub_string(size_t offset, size_t len)
{
    if (offset >= length())
        return StringImpl::alloc_string();

    // Calculate the length of sub string
    if (len > length() - offset)
        len = length() - offset;

    return StringImpl::alloc_string(m_buf + offset, len);
}

ArrayPtr::ArrayPtr(size_t size_hint) :
    TypedValue<ArrayImpl>(Value::new_array(Thread::get_current_thread_domain(), size_hint))
{
}

MapPtr::MapPtr(size_t size_hint) :
    TypedValue<MapImpl>(Value::new_map(Thread::get_current_thread_domain(), size_hint))
{
}

} // End of namespace: cmm
