// cmm_mmm_value.h
// Initial version 2016.1.31 by doing
// Manual-Memory-Management values/string

#pragma once

#include "cmm_value.h"

namespace cmm
{

class MMMValue : public Value
{
public:
    MMMValue() :
        Value(NIL)
    {
    }

    MMMValue(const Value& value) :
        Value(value)
    {
    }

    // Construct for integer 
    MMMValue(Integer v) :
        Value(v)
    {
    }

    // Construct from real
    MMMValue(Real v) :
        Value(v)
    {
        m_type = REAL;
        m_real = v;
    }

    // Construct from StringImpl
    MMMValue(StringImpl* const v)
    {
        m_string = v;
        m_type = STRING;
    }

    // ATTENTION:
    // Don't define following constructors since the StringImp
    // should be created by caller to maintain the memory
    // MMMValue(const char* v)

};

class MMMString : public MMMValue
{
public:
    MMMString() :
        MMMValue(EMPTY_STRING)
    {
    }

    MMMString(StringImpl* const impl) :
        MMMValue(impl)
    {
    }

    MMMString(const String& str) :
        MMMValue(str)
    {
    }

    // ATTENTION:
    // Don't define following constructors since the StringImpl
    // should be created by caller to maintain the memory
    // MMMString(const char *c_str, size_t len = SIZE_MAX);
    // MMMString(simple::string str);

public:
public:
    StringImpl &impl()
    {
        return *(StringImpl *)m_reference;
    }

    const StringImpl &impl() const
    {
        return *(StringImpl *)m_reference;
    }

    StringImpl *ptr()
    {
        return (StringImpl *)m_reference;
    }

    StringImpl *const ptr() const
    {
        return (StringImpl *)m_reference;
    }

public:
    bool operator ==(const MMMString& other)
        { return this->impl() == other.impl(); }

    bool operator !=(const MMMString& other)
        { return !(this->impl() == other.impl()); }

    const char *c_str() const
        { return impl().c_str(); }

    size_t length() const
        { return impl().length(); }

    // Generate string by snprintf
    template <typename... Types>
    static MMMString snprintf(const char *fmt, size_t n, Types&&... args)
        { return StringImpl::snprintf(fmt, n, simple::forward<Types>(args)...); }

    MMMString sub_string(size_t offset, size_t len = SIZE_MAX) const
        { return impl().sub_of(offset, len); }

};

}
