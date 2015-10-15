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

struct ArrayImpl;
struct BufferImpl;
struct FunctionPtrImpl;
struct MapImpl;
struct StringImpl;
struct MarkValueState;

// Base type of VM referenced  value
struct ReferenceImpl
{
friend Value;

public:
    typedef enum
    {
        CONSTANT = 0x01,    // Unchanged/freed Referenced value
        SHARED = 0x80,      // Shared in a values pool
    } Attrib;

public:
	ReferenceImpl() :
        attrib(0),
        hash_cache(0),
        owner(0),
        next(0)
	{
	}

    virtual ~ReferenceImpl()
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
    void append(ReferenceImpl **pp_list_tail)
    {
        *pp_list_tail = this;
    }

    // Bind the reference value to a domain
    void bind_to_current_domain();

    // Return hash value of me
    size_t hash_value() const
    {
        if (!hash_cache)
            hash_cache = (Uint) hash_this() + 1;

        return hash_cache;
    }

public:
    // Copy to local value list
    virtual ReferenceImpl *copy_to_local(Thread *thread) = 0;

    // Return type of this referneced value
    virtual ValueType get_type() = 0;

    // Hash this value (hash this pointer)
    virtual size_t hash_this() const { return ((size_t)this) / sizeof(void *); }

    // Mark-sweep
    virtual void mark(MarkValueState& value_map) { }

public:
    Uint attrib;
    mutable Uint hash_cache; // Cache of hash value
    ValueList *owner;        // Owned by domain or thread
    ReferenceImpl *next;     // Next value in list
};

// VM value
// ATTENTION:
// This structure shouldn't has any virtual functions, either destuctor.
// We can simple use memcpy to duplicate this class (with updating reference
// correctly manually)
class Value
{
public:
    // Instantiated hash routine for simple::string
    struct hash_func
    {
        size_t operator()(const Value& value) const
        {
            return value.hash_value();
        }
    };

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
    Value(ValueType type, ReferenceImpl *v)
    {
        STD_ASSERT(("Construct reference value with incorrect type.", type == v->get_type()));
        m_type = type;
        m_reference = v;
    }

    Value(const StringImpl *v)
    {
        m_type = STRING;
        m_string = (StringImpl *)v; // Remove "const"
    }

    Value(BufferImpl *v)
    {
        m_type = BUFFER;
        m_buffer = v;
    }

    Value(FunctionPtrImpl *v)
    {
        m_type = FUNCTION;
        m_function = v;
    }

    Value(ArrayImpl *v)
    {
        m_type = ARRAY;
        m_array = v;
    }

    Value(MapImpl *v)
    {
        m_type = MAPPING;
        m_map = v;
    }

    // Construct for string
    Value(const char *c_str, size_t len = SIZE_MAX);
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

    const StringImpl      *get_string() const { return as_string().m_string; }
    const BufferImpl      *get_buffer() const { return as_buffer().m_buffer; }
    const FunctionPtrImpl *get_function() const { return as_function().m_function; }
    const ArrayImpl       *get_array() const { return as_array().m_array; }
    const MapImpl         *get_map() const { return as_map().m_map; }

    StringImpl      *get_string() { return as_string().m_string; }
    BufferImpl      *get_buffer() { return as_buffer().m_buffer; }
    FunctionPtrImpl *get_function() { return as_function().m_function; }
    ArrayImpl       *get_array() { return as_array().m_array; }
    MapImpl         *get_map() { return as_map().m_map; }

public:
    bool operator <(const Value& b) const;
    bool operator ==(const Value& b) const;

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
        m_reference = m_reference->copy_to_local(thread);
        return *this;
    }

    // New values & bind to current domain
    static Value new_string(Domain *domain, const char *c_str, size_t len = SIZE_MAX);
    static Value new_array(Domain *domain, size_t size_hint = 8);
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
        ReferenceImpl   *m_reference;
        StringImpl      *m_string;
        BufferImpl      *m_buffer;
        FunctionPtrImpl *m_function;
        ArrayImpl       *m_array;
        MapImpl         *m_map;
    };
};

// VM value: string
struct StringImpl : ReferenceImpl
{
public:
    typedef simple::string::string_hash_t string_hash_t;
    typedef simple::string::string_size_t string_size_t;
    static const ValueType this_type = ValueType::STRING;

private:
    // The string can be constructed by StringImpl::new_string() only,
    // since the string data should be allocated at same time. That
    // means the operation: new StringImpl(), StringImpl xxx is invalid. We
    // must use StringImpl * to access it.
    StringImpl(size_t size)
    {
        m_len = (string_size_t)size;
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() { return this_type; }
    virtual size_t hash_this() const { return simple::string::hash_string(m_buf); }

public:
    size_t length() const { return m_len; }
    const char_t *c_str() const { return m_buf; }

public:
    static StringImpl *alloc_string(size_t size = 0)
    {
        StringImpl *string;
        size_t total_size = sizeof(StringImpl) + sizeof(char_t) * size; // '\x0' is included
        string = (StringImpl *)STD_MEM_ALLOC(total_size);
        // ATTENTION:
        // We can use XDELETE to free the string
        new (string)StringImpl(size);
        return string;
    }

    static StringImpl *alloc_string(const char *c_str, size_t len = SIZE_MAX)
    {
        size_t str_length = strlen(c_str);
        if (len > str_length)
            len = str_length;
        auto *string = alloc_string(len);
        memcpy(string->m_buf, c_str, len * sizeof(char_t));
        string->m_buf[len] = 0; // Add terminator
        return string;
    }

    static StringImpl *alloc_string(const simple::string& str)
    {
        size_t len = str.length();
        auto *string = alloc_string(len);
        memcpy(string->m_buf, str.c_str(), (len + 1) * sizeof(char_t));
        return string;
    }

    static StringImpl *alloc_string(const StringImpl *other_string)
    {
        size_t len = other_string->length();
        auto *string = alloc_string(len);
        memcpy(string->m_buf, other_string->m_buf, (len + 1) * sizeof(char_t));
        return string;
    }

    static void free_string(StringImpl *string)
    {
        XDELETE(string);
    }

    static int compare(const StringImpl *a, const StringImpl *b);

public:
    // Concat with other string
    StringImpl *concat_string(const StringImpl *other);

    // Get sub string
    StringImpl *sub_string(size_t offset, size_t len = SIZE_MAX);

public:
    inline bool operator <(const StringImpl& b)
    {
        return StringImpl::compare(this, &b) < 0;
    }

    inline bool operator ==(const StringImpl& b)
    {
        return StringImpl::compare(this, &b) == 0;
    }

private:
    mutable string_hash_t m_hash_value = 0;
    string_size_t m_len;
    char_t m_buf[1]; // For count size of terminator '\x0'
};

// VM value: buffer
struct BufferImpl : ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::BUFFER;

public:
    BufferImpl(const void *bytes, size_t len)
    {
        this->hash = 0;
        this->data = XNEWN(Uint8, len);
        this->len = len;
        memcpy(this->data, bytes, len);
    }

    virtual ~BufferImpl()
    {
        if (data)
            XDELETE(data);
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() { return this_type; }
    virtual size_t hash_this() const;

public:
    static int compare(const BufferImpl *a, const BufferImpl *b);

public:
    mutable size_t hash;
    size_t len;
    Uint8 *data;
};

// VM value: function
struct FunctionPtrImpl : ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::FUNCTION;

public:
    FunctionPtrImpl()
    {
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() { return this_type; }
    virtual void mark(MarkValueState& value_map);
};

// VM value: array
struct ArrayImpl : ReferenceImpl
{
public:
    typedef simple::unsafe_vector<Value> DataType;
    static const ValueType this_type = ValueType::ARRAY;

public:
    ArrayImpl(size_t size_hint = 8) :
        a(size_hint)
    {
    }

    ArrayImpl(const DataType& data)
    {
        a = data;
    }

    ArrayImpl(DataType&& data)
    {
        a = data;
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() { return this_type; }
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

        if (sizeof(index.m_int) > sizeof(size_t))
        {
            // m_int is longer than size_t
            if (index.m_int > SIZE_MAX)
                throw simple::string().snprintf("Index out of range to array, got %lld.", 128,
                                                (Int64)index.m_int);
        }

        return a[(size_t)index.m_int];
    }

    // Append an element
    void append(const Value& value)
    {
        a.push_back(value);
    }

    // Get size
    size_t size() { return a.size(); }

public:
    DataType a;
};

// VM value: map
struct MapImpl : ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::MAPPING;

public:
    typedef simple::hash_map<Value, Value, Value::hash_func> DataType;

public:
    MapImpl(size_t size_hint = 4) :
        m(size_hint)
    {
    }

    MapImpl(const DataType& data)
    {
        m = data;
    }

    MapImpl(DataType&& data)
    {
        m = data;
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() { return this_type; }
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

    // Contains key?
    bool contains_key(const Value& key)
    {
        return m.contains_key(key);
    }

    // Get keys
    Value keys()
    {
        auto vec = m.keys();
        return XNEW(ArrayImpl, (ArrayImpl::DataType &&)simple::move(vec));
    }

    // Get size
    size_t size() { return m.size(); }

    // Get values
    Value values()
    {
        auto vec = m.values();
        return XNEW(ArrayImpl, (ArrayImpl::DataType &&)simple::move(vec));
    }

public:
    DataType m;
};

template <typename T>
class TypedValue : public Value
{
public:
    // Other constructor
    template <typename... Types>
    TypedValue(Types&&... args) :
        Value()
    {
        Value value(simple::forward<Types>(args)...);
        m_type = T::this_type;
        *this = value;
    }

    TypedValue& operator =(const Value& other)
    {
        if (other.m_type != T::this_type)
            // Bad type
            throw simple::string().snprintf("Expect %s for value type, got %s.",
                                            128,
                                            Value::get_type_name(T::this_type),
                                            Value::get_type_name(other.m_type));
        m_reference = other.m_reference;
        m_reference->bind_to_current_domain();
        return *this;
    }

    TypedValue& operator =(const TypedValue& other)
    {
        m_reference = other.m_reference;
        // The value should be already binded
        return *this;
    }

    TypedValue& operator =(const T *other_impl)
    {
        m_reference = (T *)other_impl;
        m_reference->bind_to_current_domain();
        return *this;
    }

    bool operator ==(const TypedValue& other) const
    {
        return *(T *)m_reference == *(const T *)other.m_reference;
    }

    bool operator <(const TypedValue& other) const
    {
        return *(T *)m_reference < *(const T *)other.m_reference;
    }

    operator T *()
    {
        return (T *)m_reference;
    }

    operator T *const() const
    {
        return (T *)m_reference;
    }

public:
    T &impl()
    {
        return *(T *)m_reference;
    }

    const T &impl() const
    {
        return *(T *)m_reference;
    }
};

class StringPtr : public TypedValue<StringImpl>
{
public:
    StringPtr() :
        TypedValue("")
    {
    }

    template <typename... Types>
    StringPtr(Types&&... args) :
        TypedValue(simple::forward<Types>(args)...)
    {
    }

    // Redirect function to impl()
    // ATTENTION:
    // Why not override -> or * (return T *) for redirection call?
    // Because the override may cause confusing, I would rather to write
    // more codes to make them easy to understand.
public:
    StringPtr operator +(const StringPtr& other)
        { return impl().concat_string(&other.impl()); }

    const char *c_str()
        { return impl().c_str(); }

    size_t length()
        { return impl().length(); }

    StringPtr sub_string(size_t offset, size_t len = SIZE_MAX)
        { return impl().sub_string(offset, len); }
};

class ArrayPtr : public TypedValue<ArrayImpl>
{
public:
    ArrayPtr(size_t size_hint = 8);

public:
    Value& operator [](const Value& index)
        { return impl()[index]; }

    Value operator [](const Value& index) const
        { return ((ArrayImpl &)impl())[index]; }

    void append(const Value& value)
        { impl().append(value); }

    size_t size() { return impl().size(); }
};

class MapPtr : public TypedValue<MapImpl>
{
public:
    MapPtr(size_t size_hint = 8);

public:
    Value& operator [](const Value& index)
        { return impl()[index]; }

    Value operator [](const Value& index) const
        { return impl()[index]; }

    Value keys()
        { return impl().keys(); }

    size_t size()
        { return impl().size(); }

    Value values()
        { return impl().values(); }
};

} // End namespace: cmm
