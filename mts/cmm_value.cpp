// cmm_value.cpp

#include <stdio.h>
#include <stdarg.h>

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
    STD_ASSERT(("Domain not found when bind_to_current_domain.", domain));
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

// Unbind me if already binded
void ReferenceImpl::unbind_when_free()
{
    if (owner == 0)
        // Not binded yet, do nothing
        return;

    owner->remove(this);
}

// Hash this buffer
size_t BufferImpl::hash_this() const
{
    size_t seed = 131; // 31 131 1313 13131 131313 etc..
    size_t h = 0;
    size_t maxn = 64;
    if (maxn > this->len)
        maxn = this->len;
    Uint8 *p = this->data();

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

    len = a->len;
    if (len > b->len)
        len = b->len;

    return memcmp(a->buf, b->buf, (len + 1) * sizeof(char_t));
}

// Compare two buffers, return -1 means less, 1 means greater, 0 means equal
int BufferImpl::compare(const BufferImpl *a, const BufferImpl *b)
{
    // For buffer, compare len then data
    if (a->len < b->len)
        return -1;
    if (a->len > b->len)
        return 1;

    return memcmp(a->data(), b->data(), a->len);
}

// Duplicate string to local
ReferenceImpl *StringImpl::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = STRING_ALLOC(this);
    thread->bind_value(v);
    return v;
}

// Duplicate buffer to local
ReferenceImpl *BufferImpl::copy_to_local(Thread *thread)
{
    if (is_constant())
        // Don't copy
        return this;

    auto *v = BufferImpl::alloc(__FILE__, __LINE__, this);
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

// Allocate a string with reserved size
StringImpl *StringImpl::alloc(const char *file, int line, size_t size)
{
    StringImpl *string;
    size_t total_size = sizeof(StringImpl) + sizeof(char_t) * size; // '\x0' is included
    string = (StringImpl *)std_allocate_memory(total_size, "cpp", file, line);
    // ATTENTION:
    // We can use XDELETE to free the string
    new (string)StringImpl(size);
    return string;
}

// Allocate a string & construct it
StringImpl *StringImpl::alloc(const char *file, int line, const char *c_str, size_t len)
{
    size_t str_length = strlen(c_str);
    if (len > str_length)
        len = str_length;
    auto *string = alloc(file, line, len);
    memcpy(string->buf, c_str, len * sizeof(char_t));
    string->buf[len] = 0; // Add terminator
    return string;
}

// Allocate a string & construct it
StringImpl *StringImpl::alloc(const char *file, int line, const simple::string& str)
{
    size_t len = str.length();
    auto *string = alloc(file, line, len);
    memcpy(string->buf, str.c_str(), (len + 1) * sizeof(char_t));
    return string;
}

// Allocate a string & construct it
StringImpl *StringImpl::alloc(const char *file, int line, const StringImpl *other)
{
    size_t len = other->length();
    auto *string = alloc(file, line, len);
    memcpy(string->buf, other->buf, (len + 1) * sizeof(char_t));
    return string;
}

// Free a string
void StringImpl::free(const char *file, int line, StringImpl *string)
{
    string->unbind_when_free();
    std_free_memory(string, "cpp", file, line);
}

// Destruct buffer
BufferImpl::~BufferImpl()
{
    // Destruct the class
    if (attrib & CONTAIN_1_CLASS)
    {
        // Destruct the single class
        STD_ASSERT(("Not found destructor for buffer class.", destructor != 0));
        destructor(data());
    } else
    if (attrib & CONTAIN_N_CLASS)
    {
        // Destruct the n class
        STD_ASSERT(("Not found destructor for buffer class.", destructor != 0));
        auto *info = (ArrInfo *)data();
        size_t n = info->n;
        size_t size = info->size;
        STD_ASSERT(("Bad stamp of buffer class[].", info->stamp == CLASS_ARR_STAMP));
        STD_ASSERT(("Bad n or size of buffer to contain class[].", size * n + RESERVE_FOR_CLASS_ARR == len));
        Uint8 *ptr_class = data() + RESERVE_FOR_CLASS_ARR;
        for (size_t i = 0; i < n; i++, ptr_class += size)
            destructor(ptr_class);
    }
}

// Allocate a buffer & construct it
BufferImpl *BufferImpl::alloc(const char *file, int line, size_t size)
{
    BufferImpl *buffer;
    size_t total_size = sizeof(BufferImpl) + size; // '\x0' is included
    buffer = (BufferImpl *)std_allocate_memory(total_size, "cpp", file, line);
    // ATTENTION:
    // We can use XDELETE to free the buffer
    new (buffer)BufferImpl(size);
    return buffer;
}

// Allocate a buffer & construct it
BufferImpl *BufferImpl::alloc(const char *file, int line, const void *p, size_t _len)
{
    auto *buffer = alloc(file, line, _len);
    memcpy(buffer->data(), p, _len);
    return buffer;
}

// Allocate a buffer & construct it
BufferImpl *BufferImpl::alloc(const char *file, int line, const BufferImpl *other)
{
    auto *buffer = alloc(file, line, other->len);
    buffer->attrib = other->attrib;
    buffer->constructor = other->constructor;
    buffer->destructor = other->destructor;
    if (buffer->attrib & CONTAIN_1_CLASS)
    {
        // Contruct the class
        STD_ASSERT(("Not found constructor for buffer class.", buffer->constructor != 0));
        buffer->constructor(buffer->data(), other->data());
    } else
    if (buffer->attrib & CONTAIN_N_CLASS)
    {
        // Copy reserved part
        memcpy(buffer->data(), other->data(), RESERVE_FOR_CLASS_ARR);
        STD_ASSERT(("Not found constructor for buffer class.", buffer->constructor != 0));
        auto *info = (ArrInfo *)buffer->data();
        size_t n = info->n;
        size_t size = info->size;
        STD_ASSERT(("Bad stamp of buffer class[].", info->stamp == CLASS_ARR_STAMP));
        STD_ASSERT(("Bad n or size of buffer to contain class[].",
                   size * n + RESERVE_FOR_CLASS_ARR == buffer->len));
        Uint8 *ptr_class = buffer->data() + RESERVE_FOR_CLASS_ARR;
        Uint8 *ptr_from = other->data() + RESERVE_FOR_CLASS_ARR;
        for (size_t i = 0; i < n; i++, ptr_class += size, ptr_from += size)
            buffer->constructor(ptr_class, ptr_from);
    } else
        // No contain class, just do copy
        memcpy(buffer->data(), other->data(), buffer->len);

    return buffer;
}

// Free a buffer
void BufferImpl::free(const char *file, int line, BufferImpl *buffer)
{
    buffer->unbind_when_free();

    if (buffer->attrib & CONTAIN_1_CLASS)
    {
        // Contruct the class
        STD_ASSERT(("Not found constructor for buffer class.", buffer->destructor != 0));
        buffer->destructor(buffer->data());
    } else
    if (buffer->attrib & CONTAIN_N_CLASS)
    {
        // Copy reserved part
        STD_ASSERT(("Not found constructor for buffer class.", buffer->destructor != 0));
        auto *info = (ArrInfo *)buffer->data();
        size_t n = info->n;
        size_t size = info->size;
        STD_ASSERT(("Bad stamp of buffer class[].", info->stamp == CLASS_ARR_STAMP));
        STD_ASSERT(("Bad n or size of buffer to contain class[].",
                   size * n + RESERVE_FOR_CLASS_ARR == buffer->len));
        Uint8 *ptr_class = buffer->data() + RESERVE_FOR_CLASS_ARR;
        for (size_t i = 0; i < n; i++, ptr_class += size)
            buffer->destructor(ptr_class);
    }

    std_free_memory(buffer, "cpp", file, line);
}

Value::Value(Object *ob) :
    Value(ob->get_oid())
{
}

Value::Value(const char *c_str, size_t len) :
    Value(STRING_ALLOC(c_str, len))
{
}

Value::Value(const simple::string& str) :
    Value(STRING_ALLOC(str))
{
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

// Bind this value to specified domain
Value Value::bind_to(Domain *domain)
{
    if (m_type < REFERENCE_VALUE)
        return *this;

    domain->bind_value(m_reference);
    return *this;
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
StringImpl *StringImpl::concat_string(const StringImpl *other) const
{
    size_t len1 = length();
    size_t len2 = other->length();
    size_t len = len1 + len2;
    auto *string = STRING_ALLOC(len);
    char_t *data = (char_t *)string->buf;
    memcpy(data, buf, len1 * sizeof(char_t));
    memcpy(data + len1, other->buf, len2 * sizeof(char_t));
    data[len] = 0;

    // Replace with new string
    return string;
}

// Get sub string
StringImpl *StringImpl::sub_string(size_t offset, size_t len) const
{
    if (offset >= length())
        return StringImpl::alloc(__FILE__, __LINE__);

    // Calculate the length of sub string
    if (len > length() - offset)
        len = length() - offset;

    return STRING_ALLOC(buf + offset, len);
}

// Generate string by snprintf
StringImpl *StringImpl::snprintf(const char *fmt, size_t n, ...)
{
    if (n == 0)
        return STRING_ALLOC("");

    if (n > 1024)
        // Limit size to 1K
        n = 1024;

    char *buf = (char *)STD_ALLOCA(n);
    if (!buf)
        return STRING_ALLOC("");

    va_list va;
    va_start(va, n);
    STD_VSNPRINTF(buf, n, fmt, va);
    va_end(va);

    buf[n - 1] = 0;
    return STRING_ALLOC(buf);
}

Array::Array(size_t size_hint) :
    TypedValue<ArrayImpl>(XNEW(ArrayImpl, size_hint))
{
}

Map::Map(size_t size_hint) :
    TypedValue<MapImpl>(XNEW(MapImpl, size_hint))
{
}

} // End of namespace: cmm
