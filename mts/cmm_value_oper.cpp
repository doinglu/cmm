// cmm_value_oper.cpp
// To operate values

#include <math.h>
#include "cmm.h"
#include "cmm_common_util.h"
#include "cmm_output.h"
#include "cmm_value.h"

namespace cmm
{
    
Value operator +(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int + b.m_int;

        if (b.m_type == REAL)
            return (Real)a.m_int + b.m_real;

        break;

    case REAL:
        if (b.m_type == REAL)
            return a.m_real + b.m_real;

        if (b.m_type == INTEGER)
            return a.m_real + (Real)b.m_int;

        break;

    case STRING:
        if (b.m_type == STRING)
        {
            return a.m_string->concat(b.m_string);
        } else
        {
            Output output;
            return a.m_string->concat(output.type_value(&b).ptr());
        }
        break;

    case BUFFER:
        if (b.m_type == BUFFER)
            return a.m_buffer->concat(b.m_buffer);

        break;

    case ARRAY:
        if (b.m_type == ARRAY)
            return a.m_array->concat(b.m_array);

        break;

    case MAPPING:
        if (b.m_type == MAPPING)
            return a.m_map->concat(b.m_map);

        break;

    default:
        break;
    }

    throw_error("Failed to do %s + %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator -(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int - b.m_int;

        if (b.m_type == REAL)
            return (Real)a.m_int - b.m_real;

        break;

    case REAL:
        if (b.m_type == REAL)
            return a.m_real - b.m_real;

        if (b.m_type == INTEGER)
            return a.m_real - (Real)b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s - %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator *(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int * b.m_int;

        if (b.m_type == REAL)
            return (Real)a.m_int * b.m_real;

        break;

    case REAL:
        if (b.m_type == REAL)
            return a.m_real * b.m_real;

        if (b.m_type == INTEGER)
            return a.m_real * (Real)b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s * %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator /(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int / b.m_int;

        if (b.m_type == REAL)
            return (Real)a.m_int / b.m_real;

        break;

    case REAL:
        if (b.m_type == REAL)
            return a.m_real / b.m_real;

        if (b.m_type == INTEGER)
            return a.m_real / (Real)b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s / %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator %(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int % b.m_int;

        if (b.m_type == REAL)
            return fmod((Real)a.m_int, b.m_real);

        break;

    case REAL:
        if (b.m_type == REAL)
            return fmod(a.m_real, b.m_real);

        if (b.m_type == INTEGER)
            return fmod(a.m_real, (Real)b.m_int);

        break;

    default:
        break;
    }

    throw_error("Failed to do %s / %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator &(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int & b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s & %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator |(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int | b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s | %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator ^(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int ^ b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s ^ %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator ~(const Value& a)
{
    switch (a.m_type)
    {
    case INTEGER:
        return ~a.m_int;

    default:
        break;
    }

    throw_error("Failed to do ~%s.\n",
                Value::type_to_name(a.m_type));
}

Value operator -(const Value& a)
{
    switch (a.m_type)
    {
    case INTEGER:
        return -a.m_int;
        
    case REAL:
        return -a.m_real;

    default:
        break;
    }

    throw_error("Failed to do -%s.\n",
                Value::type_to_name(a.m_type));
}

Value operator <<(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int << b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s << %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

Value operator >>(const Value& a, const Value& b)
{
    switch (a.m_type)
    {
    case INTEGER:
        if (b.m_type == INTEGER)
            return a.m_int >> b.m_int;

        break;

    default:
        break;
    }

    throw_error("Failed to do %s >> %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(b.m_type));
}

// Not a value
bool operator !(const Value& a)
{
    return a.is_zero();
}

// Compare with other value
bool operator ==(const Value& a, const Value& b)
{
    if (a.m_type != b.m_type)
        // Type is not same
        return false;

    if (a.m_type >= REFERENCE_VALUE)
    {
        switch (a.m_type)
        {
        case STRING: return StringImpl::compare(a.m_string, b.m_string) == 0;
        case BUFFER: return BufferImpl::compare(a.m_buffer, b.m_buffer) == 0;
        default: break;
        }
    }

    return a.m_intptr == b.m_intptr;
}

// Compare with other value
bool operator !=(const Value& a, const Value& b)
{
    return !(a == b);
}

// Compare with other value
bool operator <(const Value& a, const Value& b)
{
    // Compare type first
    if (a.m_type < b.m_type)
        return true;
    if (a.m_type > b.m_type)
        return false;

    // Type is same

    if (a.m_type >= REFERENCE_VALUE)
    {
        switch (a.m_type)
        {
        case STRING: return StringImpl::compare(a.m_string, b.m_string) < 0;
        case BUFFER: return BufferImpl::compare(a.m_buffer, b.m_buffer) < 0;
        default: break;
        }
    }

    // Compare intptr only
    return a.m_intptr < b.m_intptr;
}

// Compare with other value
bool operator >(const Value& a, const Value& b)
{
    return b < a;
}

// Compare with other value
bool operator >=(const Value& a, const Value& b)
{
    return !(a < b);
}

// Compare with other value
bool operator <=(const Value& a, const Value& b)
{
    return !(b < a);
}

Value::operator bool()
{
    const Value& a = *this;
    return !a.is_zero();
}

Value::operator Integer()
{
    const Value& a = *this;

    switch (a.m_type)
    {
    case NIL:
        return 0;

    case INTEGER:
        return a.m_int;

    case REAL:
        return (Integer)a.m_real;

    case STRING:
        return (Integer)strtol(a.m_string->c_str(), 0, 10, 1);

    default:
        break;
    }

    throw_error("Failed to cast %s to %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(INTEGER));
}

Value::operator Real()
{
    const Value& a = *this;

    switch (a.m_type)
    {
    case NIL:
        return (Real)0;

    case INTEGER:
        return (Real)a.m_int;

    case REAL:
        return a.m_real;

    case STRING:
        return (Real)strtof(a.m_string->c_str(), 0, 1);

    default:
        break;
    }

    throw_error("Failed to cast %s to %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(REAL));
}

Value::operator const char*()
{
    const Value& a = *this;
    return ((String)a).c_str();
}

Value::operator String()
{
    const Value& a = *this;

    switch (a.m_type)
    {
    case NIL:
        return STRING_ALLOC("nil");

    case INTEGER:
    {
        char temp[64];
        snprintf(temp, sizeof(temp), "%lld", (Int64)a.m_int);
        return STRING_ALLOC(temp);
    }

    case REAL:
    {
        char temp[64];
        snprintf(temp, sizeof(temp), "%g", (double)a.m_real);
        return STRING_ALLOC(temp);
    }

    case STRING:
        return a.m_string;

    case BUFFER:
    {
        // Find length to string
        size_t len = a.m_buffer->length();
        auto *p = a.m_buffer->data();
        for (size_t i = 0; i < len; i++)
        {
            if (p[i] == 0)
            {
                len = i;
                break;
            }
        }
        return STRING_ALLOC((char *)p, len);
    }

    default:
        break;
    }

    throw_error("Failed to cast %s to %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(STRING));
}

Value::operator Buffer()
{
    const Value& a = *this;

    switch (a.m_type)
    {
    case NIL:
        return BUFFER_ALLOC((size_t)0);

    case INTEGER:
    {
        // Make to network order
        Uint8 temp[sizeof(Integer)];
        Integer val = a.m_int;
        for (int i = sizeof(Integer) - 1; i >= 0; i--)
        {
            temp[i] = (Uint8)(val & 0xFF);
            val >>= 8;
        }
        return BUFFER_ALLOC(temp, sizeof(Integer));
    }

    case REAL:
    {
        // Make to network order
        Uint8 temp[sizeof(double)];
        Int64 val = *(Int64 *)&a.m_real;
        for (int i = sizeof(Int64) - 1; i >= 0; i--)
        {
            temp[i] = (Uint8)(val & 0xFF);
            val >>= 8;
        }
        return BUFFER_ALLOC(temp, sizeof(Int64));
    }

    case STRING:
        return BUFFER_ALLOC((Uint8 *)a.m_string->c_str(), a.m_string->length());

    case BUFFER:
        return a.m_buffer;

    default:
        break;
    }

    throw_error("Failed to cast %s to %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(BUFFER));
}

Value::operator Array()
{
    const Value& a = *this;

    switch (a.m_type)
    {
    case NIL:
        return XNEW(ArrayImpl, 0);

    case ARRAY:
        return a.m_array;

    default:
        break;
    }

    throw_error("Failed to cast %s to %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(ARRAY));
}

Value::operator Map()
{
    const Value& a = *this;

    switch (a.m_type)
    {
    case NIL:
        return XNEW(MapImpl, 0);

    case MAPPING:
        return a.m_map;

    default:
        break;
    }

    throw_error("Failed to cast %s to %s.\n",
                Value::type_to_name(a.m_type),
                Value::type_to_name(MAPPING));
}

}
