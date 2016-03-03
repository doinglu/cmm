// cmm_value.cpp
// Initial version by doing

#include <stdio.h>
#include <stdarg.h>

#include "cmm_domain.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

// Const value
StringImpl* EMPTY_STRING = 0;
BufferImpl* EMPTY_BUFFER = 0;

bool Value::init()
{
    EMPTY_STRING = STRING_ALLOC("");
    EMPTY_BUFFER = BUFFER_ALLOC((size_t)0);

    EMPTY_STRING->attrib |= REFERENCE_CONSTANT;
    EMPTY_BUFFER->attrib |= REFERENCE_CONSTANT;
    return true;
}

void Value::shutdown()
{
    BUFFER_FREE(EMPTY_BUFFER);
    STRING_FREE(EMPTY_STRING);
}
    
// Try to bind a reference value to a domain's values list.
// This value must be NEW one & never be binded
void ReferenceImpl::bind_to_current_domain()
{
    if (attrib & (REFERENCE_CONSTANT | REFERENCE_SHARED))
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

        throw_error("Reference value was already binded to other owner.\n");
    }
    domain->bind_value(this);
}

// Unbind me if already binded
void ReferenceImpl::unbind()
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

// Compare two strings, return -1 means less, 1 means greater, 0 means equal
int StringImpl::compare(const StringImpl *a, const char *b)
{
    size_t len;

    len = strlen(b);
    if (len > a->len)
        len = a->len;

    return memcmp(a->buf, b, (len + 1) * sizeof(char));
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

    for (auto &it: this->a)
        v->a.push_back((ValueInContainer&&)it.copy_to_local(thread));
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

    for (auto &it: this->m)
        v->m.put((ValueInContainer&&)it.first.copy_to_local(thread),
                 (ValueInContainer&&)it.second.copy_to_local(thread));
    return v;
}

// Mark all elements in buffer value
void BufferImpl::mark(MarkValueState& state)
{
    if (!(buffer_attrib & CONTAIN_CLASS))
        // Not contains classes, ignored
        return;

    // Lookup all elements in classes in buffer
    auto **p = (ReferenceImpl **)data();
    size_t count = length() / sizeof(void *);
    for (size_t i = 0; i < count; i++, p++)
        if (state.is_possible_pointer(*p))
            state.mark_value(*p);
}

// Mark all elements in function ptr value
void FunctionPtrImpl::mark(MarkValueState& state)
{
    ////---- To be added
}

// Mark all elements in this container
void ArrayImpl::mark(MarkValueState& state)
{
    for (auto &it: this->a)
        if (it.m_type >= ValueType::REFERENCE_VALUE)
            state.mark_value(it.m_reference);
}

// Mark all elements in this container
void MapImpl::mark(MarkValueState& state)
{
    for (auto &it: this->m)
    {
        if (it.first.m_type >= ValueType::REFERENCE_VALUE)
            state.mark_value(it.first.m_reference);

        if (it.second.m_type >= ValueType::REFERENCE_VALUE)
            state.mark_value(it.second.m_reference);
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
    string->~StringImpl();
    std_free_memory(string, "cpp", file, line);
}

// Destruct buffer
BufferImpl::~BufferImpl()
{
    // Destruct the class
    if (buffer_attrib & CONTAIN_CLASS)
    {
        // Destruct the n class
        STD_ASSERT(("Not found destructor for buffer class.", destructor != 0));
        auto *info = (ArrInfo *)data();
        size_t n = info->n;
        size_t size = info->size;
        STD_ASSERT(("Bad stamp of buffer class[].", info->stamp == CLASS_1_STAMP || info->stamp == CLASS_N_STAMP));
        STD_ASSERT(("Bad n or size of buffer to contain class[].", size * n + RESERVE_FOR_CLASS_ARR <= len));
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
    if (buffer->buffer_attrib & CONTAIN_CLASS)
    {
        // Copy reserved part
        memcpy(buffer->data(), other->data(), RESERVE_FOR_CLASS_ARR);
        STD_ASSERT(("Not found constructor for buffer class.", buffer->constructor != 0));
        auto *info = (ArrInfo *)buffer->data();
        size_t n = info->n;
        size_t size = info->size;
        STD_ASSERT(("Bad stamp of buffer class[].", info->stamp == CLASS_1_STAMP || info->stamp == CLASS_N_STAMP));
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
    buffer->~BufferImpl();

    std_free_memory(buffer, "cpp", file, line);
}

Value& Value::operator =(Object *ob)
{
    *this = ob->get_oid();
    return *this;
}

Value& Value::operator =(const char *c_str)
{
    *this = STRING_ALLOC(c_str);
    return *this;
}

Value& Value::operator =(const simple::string& str)
{
    *this = STRING_ALLOC(str);
    return *this;
}

Value::Value(const char *c_str)
{
    *this = STRING_ALLOC(c_str);
}

Value::Value(const simple::string& str)
{
    *this = STRING_ALLOC(str);
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
Value& Value::bind_to(Domain *domain)
{
    if (m_type < REFERENCE_VALUE)
        return *this;

    m_reference->bind_to_current_domain();
    return *this;
}

Value& Value::set(const Value& key, const Value& value)
{
    switch (m_type)
    {
    case MAPPING: return (*m_map).set(key, value);
    case ARRAY: return (*m_array).set(key, value);
    default: throw_error("Bad type of value to index.\n");
    }
}

// Get index from container
const Value& Value::get(const Value& key, Value* ptr_value) const
{
    switch (m_type)
    {
    case STRING: return *ptr_value = (*m_string).get(key);
    case MAPPING: return (*m_map).get(key, ptr_value);
    case ARRAY: return (*m_array).get(key, ptr_value);
    default: throw_error("Bad type of value to index.\n");
    }
}

// Concat with other string
StringImpl *StringImpl::concat(const StringImpl *other) const
{
    size_t len1 = this->length();
    size_t len2 = other->length();
    size_t len = len1 + len2;
    auto *string = STRING_ALLOC(len);
    char_t *p = (char_t *)string->buf;
    memcpy(p, this->buf, len1 * sizeof(char_t));
    memcpy(p + len1, other->buf, len2 * sizeof(char_t));
    p[len] = 0;

    // Replace with new string
    return string;
}

// Concat with other c string
StringImpl *StringImpl::concat(const char *c_str) const
{
    size_t len1 = this->length();
    size_t len2 = strlen(c_str);
    size_t len = len1 + len2;
    auto *string = STRING_ALLOC(len);
    char_t *p = (char_t *)string->buf;
    memcpy(p, this->buf, len1 * sizeof(char_t));
    memcpy(p + len1, c_str, len2 * sizeof(char_t));
    p[len] = 0;

    // Replace with new string
    return string;
}

// Get sub of me
StringImpl *StringImpl::sub_of(size_t offset, size_t len) const
{
    if (offset >= length())
        return StringImpl::alloc(__FILE__, __LINE__);

    // Calculate the length of sub
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

// Concat with other buffer
BufferImpl *BufferImpl::concat(const BufferImpl *other) const
{
    size_t len1 = this->length();
    size_t len2 = other->length();
    size_t len = len1 + len2;
    auto *buffer = BUFFER_ALLOC(len);
    Uint8 *p = buffer->data();
    memcpy(p, this->data(), len1 * sizeof(Uint8));
    memcpy(p + len1, other->data(), len2 * sizeof(Uint8));
    p[len] = 0;

    // Replace with new string
    return buffer;
}

// Get sub of me
BufferImpl *BufferImpl::sub_of(size_t offset, size_t len) const
{
    if (offset >= length())
        return BufferImpl::alloc(__FILE__, __LINE__, (size_t)0);

    // Calculate the length of sub
    if (len > length() - offset)
        len = length() - offset;

    // Return part of me
    return BufferImpl::alloc(__FILE__, __LINE__, data() + offset, len);
}

// Concat with other buffer
ArrayImpl *ArrayImpl::concat(const ArrayImpl *other) const
{
    size_t size1 = this->size();
    size_t size2 = other->size();
    size_t size = size1 + size2;
    auto *array = XNEW(ArrayImpl, size);
    array->a.push_back_array(this->a.get_array_address(0), this->a.size());
    array->a.push_back_array(other->a.get_array_address(0), other->a.size());
    return array;
}

// Get sub of me
ArrayImpl *ArrayImpl::sub_of(size_t offset, size_t len) const
{
    if (offset >= size())
        return XNEW(ArrayImpl, (size_t)0);

    // Calculate the length of sub
    if (len > size() - offset)
        len = size() - offset;

    // Return part of me
    auto *array = XNEW(ArrayImpl, (size_t)len);
    array->a.push_back_array(a.get_array_address(offset), len);
    return array;
}

// Concat with other
MapImpl *MapImpl::concat(const MapImpl *other) const
{
    size_t size1 = this->size();
    size_t size2 = other->size();
    size_t size = size1 + size2;
    auto *map = XNEW(MapImpl, size);
    for (auto& it : (DataType&)this->m)
        map->m.put(it.first, it.second);
    for (auto& it : (DataType&)other->m)
        map->m.put(it.first, it.second);
    return map;
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
