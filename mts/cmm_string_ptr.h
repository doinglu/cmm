// cmm_string_ptr.h

// ATTENTION: WHY StringPtr?
// Since String is not valid for containers such as hash_map, vector
// because we can't define the class String but use String * instead of.
// So we define the type StringPtr for using in container.

#pragma once

#include "std_template/simple.h"
#include "cmm_basic_types.h"
#include "cmm_value.h"

namespace cmm
{

class StringPtr
{
public:
    StringPtr(const String *string = 0)
    {
        // Remove the const
        s = (String*)string;
    }

    String *get_string()
    {
        return s;
    }

    const String *get_string() const
    {
        return s;
    }

    const String *operator ->() const
    {
        return s;
    }

    const String& operator *() const
    {
        return *s;
    }

public:
    String *s;
};

// Hash function
struct hash_string_ptr_func
{
    size_t operator ()(const StringPtr& string_ptr) const
    {
        return string_ptr->hash_value();
    }
};

// Operator for StringPtr
inline bool operator ==(const StringPtr& a, const StringPtr& b)
{
    return *a == *b;
}

// Operator for StringPtr
inline bool operator <(const StringPtr& a, const StringPtr &b)
{
    return *a < *b;
}

}
