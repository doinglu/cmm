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

    // Return type of this referneced value
    virtual ValueType get_type() = 0;

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
    Value(ValueType type, ReferenceValue *v)
    {
        STD_ASSERT(("Construct reference value with incorrect type.", type == v->get_type()));
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

    // Safe get values (check type)
public:
    // Return this value if they're expected type
    const Value& as_nil() const { return expect_type(NIL); }
    const Value& as_int() const { return expect_type(INTEGER); }
    const Value& as_real() const { return expect_type(REAL); }
    const Value& as_object() const { return expect_type(OBJECT); }
    const Value& as_string() const { return expect_type(STRING); }
    const Value& as_buffer() const { return expect_type(BUFFER); }
    const Value& as_function() const { return expect_type(FUNCTION); }
    const Value& as_array() const { return expect_type(ARRAY); }
    const Value& as_map() const { return expect_type(MAPPING); }

    Value& as_nil() { return expect_type(NIL); }
    Value& as_int() { return expect_type(INTEGER); }
    Value& as_real() { return expect_type(REAL); }
    Value& as_object() { return expect_type(OBJECT); }
    Value& as_string() { return expect_type(STRING); }
    Value& as_buffer() { return expect_type(BUFFER); }
    Value& as_function() { return expect_type(FUNCTION); }
    Value& as_array() { return expect_type(ARRAY); }
    Value& as_map() { return expect_type(MAPPING); }

    Integer      get_int() const { return as_int().m_int; }
    Real         gets_real() const { return as_real().m_real; }
    ObjectId     get_object() const { return as_object().m_oid; }

    const String      *get_string() const { return as_string().m_string; }
    const Buffer      *get_buffer() const { return as_buffer().m_buffer; }
    const FunctionPtr *get_function() const { return as_function().m_function; }
    const Array       *get_array() const { return as_array().m_array; }
    const Map         *get_map() const { return as_map().m_map; }

    String      *get_string() { return as_string().m_string; }
    Buffer      *get_buffer() { return as_buffer().m_buffer; }
    FunctionPtr *get_function() { return as_function().m_function; }
    Array       *get_array() { return as_array().m_array; }
    Map         *get_map() { return as_map().m_map; }

public:
    // Return name of this value
    static const char *get_type_name(ValueType type)
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

private:
    // Check type & return this value
    const Value& expect_type(ValueType type) const
    {
        if (m_type != type)
            throw simple::string().snprintf("Value type is not %s.", 128, get_type_name(type));
        return *this;
    }

    Value& expect_type(ValueType type)
    {
        if (m_type != type)
            throw simple::string().snprintf("Value type is not %s.", 128, get_type_name(type));
        return (Value &)*this;
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
    static Value new_string(Domain *domain, const char_t *c_str);
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
    ValueType m_type;
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
    typedef simple::string::string_hash_t string_hash_t;
    typedef simple::string::string_size_t string_size_t;

private:
    // The string can be constructed by String::new_string() only,
    // since the string data should be allocated at same time. That
    // means the operation: new String(), String xxx is invalid. We
    // must use String * to access it.
    String(size_t size) :
        m_hash_value(0)
    {
        m_len = (string_size_t)size;
    }

public:
    virtual ValueType get_type() { return ValueType::STRING; }
    virtual ReferenceValue *copy_to_local(Thread *thread);

public:
    size_t length() const { return m_len; }
    const char_t *c_str() const { return m_buf; }

    // Calculate hash value
    size_t hash_value() const
    {
        if (!m_hash_value)
            m_hash_value = simple::string::hash_string(m_buf) + 1;
        
        return (size_t) m_hash_value;
    }

public:
    static String *new_string(size_t size)
    {
        String *string;
        size_t total_size = sizeof(String) + sizeof(char_t) * size; // '\x0' is included
        string = (String *)STD_MEM_ALLOC(total_size);
        // ATTENTION:
        // We can use XDELETE to free the string
        new (string)String(size);
        return string;
    }

    static String *new_string(const char_t *c_str)
    {
        size_t len = strlen(c_str);
        auto *string = new_string(len);
        memcpy(string->m_buf, c_str, (len + 1) * sizeof(char_t));
        return string;
    }

    static String *new_string(const simple::string str)
    {
        size_t len = str.length();
        auto *string = new_string(len);
        memcpy(string->m_buf, str.c_str(), (len + 1) * sizeof(char_t));
        return string;
    }

    static String *new_string(const String *other_string)
    {
        size_t len = other_string->length();
        auto *string = new_string(len);
        memcpy(string->m_buf, other_string->m_buf, (len + 1) * sizeof(char_t));
        return string;
    }

    static void delete_string(String *string)
    {
        XDELETE(string);
    }

    static int compare(const String *a, const String *b);

private:
    mutable string_hash_t m_hash_value = 0;
    string_size_t m_len;
    char_t m_buf[1]; // For count size of terminator '\x0'
};

// VM value: buffer
struct Buffer : ReferenceValue
{
public:
    Buffer(const void *bytes, size_t len)
    {
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
    virtual ValueType get_type() { return ValueType::BUFFER; }
    virtual ReferenceValue *copy_to_local(Thread *thread);

public:
    static int compare(const Buffer *a, const Buffer *b);

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
    }

public:
    virtual ValueType get_type() { return ValueType::FUNCTION; }
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
    }

public:
    virtual ValueType get_type() { return ValueType::ARRAY; }
    virtual ReferenceValue *copy_to_local(Thread *thread);
    virtual void mark(MarkValueState& value_map);

public:
    Value& operator [](const Value& index)
    {
        if (index.m_type != ValueType::INTEGER)
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
    }

public:
    virtual ValueType get_type() { return ValueType::MAPPING; }
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

inline bool operator <(const String&a, const String& b)
{
    return String::compare(&a, &b) < 0;
}

inline bool operator ==(const String&a, const String& b)
{
    return String::compare(&a, &b) == 0;
}

bool operator <(const Value& a, const Value& b);
bool operator ==(const Value& a, const Value& b);

} // End namespace: cmm
