// cmm_string_ptr.h

// ATTENTION: WHY StringPtr?
// Since StringImpl is not valid for containers such as hash_map, vector
// because we can't define the class StringImpl but use StringImpl * instead of.
// So we define the type StringPtr for using in container.

// How to use the 6 types of string?
// 1. const char *
// 2. const char_t *
// 3. simple::string
// 4. StringImpl *
// 5. StringPtr
// 6. Value (ValueType::STRING)
// See following rules:
// 1 - NEVER USE EXCEPT as function argument (will be constructed to other
//     types)
// 2 - Internal storage pointer type only.
// 3 - DO NOT USE IN THIS PROJECT
// 4 - Implementation of StringImpl in CMM, CAN NOT BE NEW/CONSTRUCT, use
//     StringImpl::new_string and StringImpl:delete_string instead of. 
// 5 - Only for containers. (hash_map/set/vector ...) The StringImpl can't be
//     used for any containers because it can't be constructed by StringImpl().
//     Sometimes we need to use StringImpl as key, use StringPtr. Attention:
//     if we use StringImpl * as key it means we lookup the pointer address only
//     but not the string content.
// 6 - Value can carry a string.
//     a. When we want to modify string which "StringImpl" can't be modified,
//     using the Value will be a good idea.
//     b. If we need caller use const char_t * as argument, we should use
//     "Value" for callee function prototype.
//     Sample: Value invoke(... const Value& function_name_value, ...
//     The value can be constructed from const char_t *. Why not use the
//     StringPtr? Because the StringPtr has not any memory management
//     functionality but only a wrapper for StringImpl *. DON'T USE StringPtr
//     unless in a container with requirement to compare the content.

#pragma once

#include "std_template/simple.h"
#include "cmm_basic_types.h"
#include "cmm_value.h"

#include "cmm_operate.h"////----

#if 0
namespace cmm
{

class StringPtr
{
public:
    StringPtr(const StringImpl *string = 0)
    {
        // Remove the const
        s = (StringImpl*)string;
    }

    StringImpl *get_string()
    {
        return s;
    }

    const StringImpl *get_string() const
    {
        return s;
    }

    const StringImpl *operator ->() const
    {
        return s;
    }

    const StringImpl& operator *() const
    {
        return *s;
    }

public:
    StringImpl *s;
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
#endif
