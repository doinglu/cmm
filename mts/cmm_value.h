#pragma once

#include "std_template/simple_string.h"
#include "std_template/simple_vector.h"
#include "std_template/simple_hash_map.h"
#include "cmm_basic_types.h"

namespace cmm
{

// Type of values
typedef Int64   Integer;
typedef Int64   IntPtr;
typedef double  Real;

class Domain;
class Object;
class Thread;

class Value;
class ValueList;

struct Array;
struct Buffer;
struct FunctionPtr;
struct Map;
struct String;
struct MarkValueState;

// Base type of VM referenced  value
struct ReferenceValue
{
friend Value;

public:
    typedef enum
    {
        CONSTANT = 0x01,    // Unchanged/freed Referenced value
        SHARED = 0x80,      // Shared in a values pool
    } Attrib;

public:
	ReferenceValue() :
#ifdef _DEBUG
        type(0), // Value::NIL
        owner(0),
#endif
		attrib(0),
        next(0)
	{
	}

    virtual ~ReferenceValue()
    {
        // Let derived class do destroy work
    }

public:
    // Is this value a constant value?
    bool is_constant() const
    {
        return attrib & CONSTANT;
    }

public:
    // Append this reference value @ tail of the list
    void append(ReferenceValue **pp_list_tail)
    {
        *pp_list_tail = this;
    }

    // Copy to local value list
    virtual ReferenceValue *copy_to_local(Thread *thread) = 0;

    // Mark-sweep
    virtual void mark(MarkValueState& value_map) { }

private:
    // Bind the reference value to a domain
    inline void bind_to_current_domain();

public:
    Uint attrib;
    ReferenceValue *next;
#ifdef _DEBUG
    int type;           // Type of this reference value
    ValueList *owner;   // Owned by domain or thread
#endif
};

// VM value
// ATTENTION:
// This structure shouldn't has any virtual functions, either destuctor.
// We can simple use memcpy to duplicate this class (with updating reference
// correctly manually)
class Value
{
public:
    typedef enum
    {
        NIL = 0,            // Can be casted to any other type
        INTEGER = 1,        // Integer (int64)
        REAL = 2,           // Float (double)
        OBJECT = 3,         // Object
        REFERENCE_VALUE = 9,// Type >= ReferenceValue is a ReferenceValue type
        STRING = 9,         // String
        BUFFER = 10,        // Binary data
        FUNCTION = 11,      // Function pointer
        ARRAY = 12,         // Array
        MAPPING = 13,       // Mapping
    } Type;

public:
    Value()
    {
        m_type = NIL;
#ifdef _DEBUG
        m_intptr = 0;
#endif
    }

    // ATTENTION:
    // Don't add destructor, or it may cause the poor performance.
    // We will use this class in function freqencily and the destructor
    // will generated many Exception Handling relative code.

    Value(const Value& value)
    {
        *this = value;
    }

    // Construct for integer 
    Value(int v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    Value(Uint v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    Value(Int64 v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    Value(Uint64 v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    // Construct from real
    Value(Real v)
    {
        m_type = REAL;
        m_real = v;
    }

    // Construct from object
    Value(ObjectId oid)
    {
        m_type = OBJECT;
        m_oid = oid;
    }

    Value(Object *ob);

    // Contruct from reference values
    Value(Type type, ReferenceValue *v)
    {
        STD_ASSERT(("Construct reference value with incorrect type.", type == v->type));
        m_type = type;
        m_reference_value = v;
    }

    Value(String *v)
    {
        m_type = STRING;
        m_string = v;
    }

    Value(Buffer *v)
    {
        m_type = BUFFER;
        m_buffer = v;
    }

    Value(FunctionPtr *v)
    {
        m_type = FUNCTION;
        m_function = v;
    }

    Value(Array *v)
    {
        m_type = ARRAY;
        m_array = v;
    }

    Value(Map *v)
    {
        m_type = MAPPING;
        m_map = v;
    }

    // Construct for string
    Value(const simple::char_t *c_str);
	Value(const simple::string& str);

public:
    // Return name of this value
    static const char *get_type_name(Type type)
    {
        switch (type)
        {
        case NIL:      return "nil";
        case INTEGER:  return "int";
        case REAL:     return "real";
        case OBJECT:   return "object";
        case STRING:   return "string";
        case BUFFER:   return "buffer";
        case ARRAY:    return "array";
        case MAPPING:  return "mapping";
        case FUNCTION: return "function";
        default:       return "bad type";
        }
    }

    // Calculate & cache hash number of the value
    size_t hash_value() const;

    // Cast this value to integer
    Value& to_int()
    {
        if (m_type == INTEGER)
            return *this;

        throw "Failed to convert to integer.";
    }

	// Is this reference value?
	bool is_reference_value() const
	{
		return m_type >= REFERENCE_VALUE;
	}

    // Is this value zero?
    bool is_zero() const
    {
        return m_intptr == 0;
    }

    // Is this value non zero?
    bool is_non_zero() const
    {
        return !is_zero();
    }

    // Domain/local relative operations
public:
    // Copy this value to local value list if this is a reference type value
    Value copy_to_local(Thread *thread)
    {
        if (m_type < REFERENCE_VALUE)
            return *this;

        // Copy reference value
        m_reference_value = m_reference_value->copy_to_local(thread);
        return *this;
    }

    // New values & bind to current domain
    static Value new_string(Domain *domain, const char *c_str);
    static Value new_map(Domain *domain, size_t size_hint = 8);

public:
    Value& operator =(const Value& value)
    {
        m_type = value.m_type;
        m_intptr = value.m_intptr;
        return *this;
    }

    // Put value to container
    Value& operator [](const Value& value);

    // Get index from container
    Value operator [](const Value& value) const;

public:
    Type m_type;
    union
    {
        Integer          m_int;
        Real             m_real;
        IntPtr           m_intptr;
        ObjectId         m_oid;
        ReferenceValue  *m_reference_value;
        String          *m_string;
        Buffer          *m_buffer;
        FunctionPtr     *m_function;
        Array           *m_array;
        Map             *m_map;
    };
};

// VM value: string
struct String : ReferenceValue
{
public:
    String(size_t size) :
        s(size)
    {
#ifdef _DEBUG
        type = Value::STRING;
#endif
    }

    String(const simple::char_t *c_str) :
        s(c_str)
    {
#ifdef _DEBUG
        type = Value::STRING;
#endif
    }

    String(const simple::string& str) :
        s(str)
    {
#ifdef _DEBUG
        type = Value::STRING;
#endif
    }

    virtual ~String()
    {
    }

public:
    virtual ReferenceValue *copy_to_local(Thread *thread);

public:
    simple::string s;
};

// VM value: buffer
struct Buffer : ReferenceValue
{
public:
    Buffer(const void *bytes, size_t len)
    {
#ifdef _DEBUG
        type = Value::BUFFER;
#endif
        this->hash = 0;
        this->data = XNEWN(Uint8, len);
        this->len = len;
        memcpy(this->data, bytes, len);
    }

    virtual ~Buffer()
    {
        if (data)
            XDELETE(data);
    }

public:
    size_t hash_value() const;

public:
    virtual ReferenceValue *copy_to_local(Thread *thread);

public:
    mutable size_t hash;
    size_t len;
    Uint8 *data;
};

// VM value: function
struct FunctionPtr : ReferenceValue
{
public:
    FunctionPtr()
    {
#ifdef _DEBUG
        type = Value::FUNCTION;
#endif
    }

    virtual ~FunctionPtr()
    {
    }

public:
    virtual ReferenceValue *copy_to_local(Thread *thread);
    virtual void mark(MarkValueState& value_map);
};

// VM value: array
struct Array : ReferenceValue
{
public:
    typedef simple::unsafe_vector<Value> DataType;

public:
    Array(size_t size_hint = 8) :
        a(size_hint)
    {
#ifdef _DEBUG
        type = Value::ARRAY;
#endif
    }

    virtual ~Array()
    {
    }

public:
    virtual ReferenceValue *copy_to_local(Thread *thread);
    virtual void mark(MarkValueState& value_map);

public:
    Value& operator [](const Value& index)
    {
        if (index.m_type != Value::INTEGER)
            throw simple::string().snprintf("Bad index to array (expected integer got %s).", 128,
                                            Value::get_type_name(index.m_type));

        if (index.m_int < 0 || index.m_int >= (Integer)a.size())
            throw simple::string().snprintf("Index out of range to array, got %lld.", 128,
                                            (Int64)index.m_int);

        return a[index.m_int];
    }

public:
    DataType a;
};

// VM value: map
struct Map : ReferenceValue
{
    // Instantiated hash routine for simple::string
    struct hash_value
    {
        size_t operator()(const Value& value) const
        {
            return value.hash_value();
        }
    };

public:
    typedef simple::hash_map<Value, Value, hash_value> DataType;

public:
    Map(size_t size_hint = 4) :
        m(size_hint)
    {
#ifdef _DEBUG
        type = Value::MAPPING;
#endif
    }

    virtual ~Map()
    {
    }

public:
    virtual ReferenceValue *copy_to_local(Thread *thread);
    virtual void mark(MarkValueState& value_map);

public:
    Value& operator [](const Value& index)
    {
        return m[index];
    }

    Value operator [](const Value& index) const
    {
        Value value;
        m.try_get(index, &value);
        return value;
    }

public:
    DataType m;
};

bool operator <(const Value& a, const Value& b);
bool operator ==(const Value& a, const Value& b);

} // End namespace: cmm
