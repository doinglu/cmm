// cmm_operate.h
// ATTENTION:
// All value derived from refereneced value should be bind to current
// domain for GC.

#pragma once

#include "std_template/simple.h"
#include "cmm_basic_types.h"
#include "cmm_domain.h"
#include "cmm_thread.h"
#include "cmm_value.h"

#if 0
namespace cmm
{

template <typename T>
class ValuePtr
{
public:
    // Hash this reference value
    struct hash_func
    {
        size_t operator ()(const ValuePtr& ptr) const
        {
            return ptr.m_ptr->hash_value();
        }
    };

public:
    // Other constructor
    template <typename... Types>
    ValuePtr(Types&&... args)
    {
        Value value(simple::forward<Types>(args)...);
        if (value.m_type != T::this_type)
            // Bad type
            throw simple::string().snprintf("Expect %s for value type, got %s.",
                                            128,
                                            Value::get_type_name(T::this_type),
                                            Value::get_type_name(value.m_type));
        m_ptr = (T *)value.m_reference;
        m_ptr->bind_to_current_domain();
    }

    ValuePtr& operator =(const T *other)
    {
        m_ptr = (T *)other;
        m_ptr->bind_to_current_domain();
    }

    bool operator ==(const ValuePtr& other) const
    {
        return *m_ptr == *other.m_ptr;
    }

    bool operator <(const ValuePtr& other) const
    {
        return *m_ptr < *other.m_ptr;
    }

    operator T *()
    {
        return m_ptr;
    }

    operator const T *() const
    {
        return m_ptr;
    }

    T *operator ->()
    {
        return m_ptr;
    }

    const T *operator ->() const
    {
        return m_ptr;
    }

    T& operator *()
    {
        return *m_ptr;
    }

    const T& operator *() const
    {
        return *m_ptr;
    }

public:
    T *get_ptr()
    {
        return m_ptr;
    }

protected:
    T *m_ptr;
};

class String : public ValuePtr<StringImpl>
{
public:
    String() :
        ValuePtr("")
    {
    }

    template <typename... Types>
    String(Types&&... args) :
        ValuePtr(simple::forward<Types>(args)...)
    {
    }
        
    // Get sub string
    String sub_string(size_t offset, size_t len = SIZE_MAX)
    {
        if (offset >= m_ptr->length())
            return String();

        // Calculate the length of sub string
        if (len > m_ptr->length() - offset)
            len = m_ptr->length() - offset;

        return String(m_ptr->c_str() + offset, len);
    }

public:
    // Concat with another string
    String operator +(const String& other)
    {
        size_t len1 = m_ptr->length();
        size_t len2 = other.m_ptr->length();
        size_t len = len1 + len2;
        auto *string = StringImpl::alloc_string(len);
        string->bind_to_current_domain();
        char_t *data = (char_t *)string->c_str();
        memcpy(data, m_ptr->c_str(), len1 * sizeof(char_t));
        memcpy(data + len1, other.m_ptr->c_str(), len2 * sizeof(char_t));
        data[len] = 0;

        // Replace with new string
        return String(string);
    }
};

class Array : public ValuePtr<ArrayImpl>
{
public:
    Array(size_t size_hint = 8) :
        ValuePtr(Value::new_array(Thread::get_current_thread_domain(), size_hint))
    {
    }
};

class Map : public ValuePtr<MapImpl>
{
public:
    Map(size_t size_hint = 8) :
        ValuePtr(Value::new_map(Thread::get_current_thread_domain(), size_hint))
    {
    }
};

}
#endif