// cmm_value.cpp

#include <stdio.h>

#include "cmm_domain.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

// Try to bind a reference value to a domain's values list first,
// Or bind to thread local values list IF no domain found
void ReferenceValue::bind_to_current_domain()
{
    Domain *domain = Thread::get_current_thread_domain();
    STD_ASSERT(("No found domain when bind_to_current_domain.", domain));
    domain->bind_value(this);
}

// Hash this buffer
size_t Buffer::hash_value() const
{
    if (this->hash)
        return this->hash;

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

    this->hash = h;
    return h;
}

// Compare two strings, return -1 means less, 1 means greater, 0 means equal
int String::compare(const String *a, const String *b)
{
    size_t len;

    len = a->m_len;
    if (len > b->m_len)
        len = b->m_len;

    return memcmp(a->m_buf, b->m_buf, (len + 1) * sizeof(char_t));
}

// Compare two buffers, return -1 means less, 1 means greater, 0 means equal
int Buffer::compare(const Buffer *a, const Buffer *b)
{
    // For buffer, compare len then data
    if (a->len < b->len)
        return -1;
    if (a->len > b->len)
        return 1;

    return memcmp(a->data, b->data, a->len);
}

// Duplicate string to local
ReferenceValue *String::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = String::new_string(this);
    thread->bind_value(v);
    return v;
}

// Duplicate buffer to local
ReferenceValue *Buffer::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = XNEW(Buffer, this->data, this->len);
    thread->bind_value(v);
    return v;
}

// Duplicate to local
ReferenceValue *FunctionPtr::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    ////---- To be added
    return 0;
}

// Duplicate to local
ReferenceValue *Array::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = XNEW(Array, this->a.size());
    thread->bind_value(v);
    // Bind before copy elements in case of exception when copying

    auto end = this->a.end();
    for (auto it = this->a.begin(); it != end; ++it)
        v->a.push_back(it->copy_to_local(thread));
    return v;
}

// Duplicate to local
ReferenceValue *Map::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = XNEW(Map, this->m.size());
    thread->bind_value(v);
    // Bind before copy elements in case of exception when copying

    auto end = this->m.end();
    for (auto it = this->m.begin(); it != end; ++it)
        v->m.put(it->first.copy_to_local(thread), it->second.copy_to_local(thread));
    return v;
}

// Mark all elements in function ptr value
void FunctionPtr::mark(MarkValueState& state)
{
    ////---- To be added
}

// Mark all elements in this container
void Array::mark(MarkValueState& state)
{
    auto end = a.end();
    for (auto it = a.begin(); it != end; ++it)
        if (it->m_type >= ValueType::REFERENCE_VALUE)
            Domain::mark_value(state, it->m_reference_value);
}

// Mark all elements in this container
void Map::mark(MarkValueState& state)
{
    auto end = m.end();
    for (auto it = m.begin(); it != end; ++it)
    {
        if (it->first.m_type >= ValueType::REFERENCE_VALUE)
            Domain::mark_value(state, it->first.m_reference_value);

        if (it->second.m_type >= ValueType::REFERENCE_VALUE)
            Domain::mark_value(state, it->second.m_reference_value);
    }
}

Value::Value(Object *ob) :
    Value(ob->get_oid())
{
}

Value::Value(const simple::char_t *c_str)
{
    auto *v = String::new_string(c_str);
    v->bind_to_current_domain();
    m_type = STRING;
    m_string = v;
}

Value::Value(const simple::string& str)
{
    auto *v = String::new_string(str);
    v->bind_to_current_domain();
    m_type = STRING;
    m_string = v;
}

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
Value Value::new_string(Domain *domain, const char_t *c_str)
{
    auto *v = String::new_string(c_str);
    STD_ASSERT(domain == Thread::get_current_thread_domain());
    domain->bind_value(v);
    return Value(v);
}

// Create a reference value belonged to this domain
Value Value::new_map(Domain *domain, size_t size_hint)
{
    auto *v = XNEW(Map, size_hint);
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

bool operator <(const Value& a, const Value& b)
{
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
            case ValueType::STRING: return String::compare(a.m_string, b.m_string) < 0;
            case ValueType::BUFFER: return Buffer::compare(a.m_buffer, b.m_buffer) < 0;
            default: break;
        }
    }

    // Compare intptr only
    return a.m_intptr < b.m_intptr;
}

bool operator ==(const Value& a, const Value& b)
{
    if (a.m_type != b.m_type)
        // Type is not same
        return false;

    if (a.m_type >= ValueType::REFERENCE_VALUE)
    {
        switch (a.m_type)
        {
            case ValueType::STRING: return String::compare(a.m_string, b.m_string) == 0;
            case ValueType::BUFFER: return Buffer::compare(a.m_buffer, b.m_buffer) == 0;
            default: break;
        }
    }    

    return a.m_intptr == b.m_intptr;
}

} // End of namespace: cmm
