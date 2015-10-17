#pragma once

#include "std_port/std_port.h"
#include "std_port/std_port_type.h"
#include "std_port/std_port_compiler.h"
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

    // This value will be freed manually, unbind it if necessary
    void unbind_when_free();

public:
    // Copy to local value list
    virtual ReferenceImpl *copy_to_local(Thread *thread) = 0;

    // Return type of this referneced value
    virtual ValueType get_type() const = 0;

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

    Value(Int64 v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    Value(size_t v)
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
    Value(ReferenceImpl *v, ValueType type = NIL)
    {
        if (type == NIL)
            type = v->get_type();
        m_type = type;
        m_reference = v;
        m_reference->bind_to_current_domain();
    }

    Value(const StringImpl *v) :
        Value((ReferenceImpl *)v, STRING) {}

    Value(BufferImpl *v) :
        Value((ReferenceImpl *)v, BUFFER) {}

    Value(FunctionPtrImpl *v) :
        Value((ReferenceImpl *)v, FUNCTION) {}

    Value(ArrayImpl *v) :
        Value((ReferenceImpl *)v, ARRAY) {}

    Value(MapImpl *v) :
        Value((ReferenceImpl *)v, MAPPING) {}

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

    StringImpl      *get_string() const { return as_string().m_string; }
    BufferImpl      *get_buffer() const { return as_buffer().m_buffer; }
    FunctionPtrImpl *get_function() const { return as_function().m_function; }
    ArrayImpl       *get_array() const { return as_array().m_array; }
    MapImpl         *get_map() const { return as_map().m_map; }

public:
    bool operator <(const Value& b) const;
    bool operator ==(const Value& b) const;

public:
    // Type to name
    static const char *type_to_name(ValueType type)
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
        case MIXED:    return "mixed";
        default:       return "bad type";
        }
    }

    // Name to type
    static ValueType name_to_type(const char *name)
    {
        switch (name[0])
        {
            case 'n': return (strcmp(name, "nil") == 0)      ? NIL      : BAD_TYPE;
            case 'i': return (strcmp(name, "int") == 0)      ? INTEGER  : BAD_TYPE;
            case 'r': return (strcmp(name, "real") == 0)     ? REAL     : BAD_TYPE;
            case 'o': return (strcmp(name, "object") == 0)   ? OBJECT   : BAD_TYPE;
            case 's': return (strcmp(name, "string") == 0)   ? STRING   : BAD_TYPE;
            case 'b': return (strcmp(name, "buffer") == 0)   ? BUFFER   : BAD_TYPE;
            case 'f': return (strcmp(name, "function") == 0) ? FUNCTION : BAD_TYPE;
            case 'a': return (strcmp(name, "array") == 0)    ? ARRAY    : BAD_TYPE;
            case 'm': return (strcmp(name, "mapping") == 0)  ? MAPPING  :
                             (strcmp(name, "mixed") == 0)    ? MIXED    : BAD_TYPE;
            case 'v': return (strcmp(name, "void") == 0)     ? TVOID    : BAD_TYPE;
            default:  return BAD_TYPE;
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
            throw simple::string().snprintf("Value type is not %s.", 128, type_to_name(type));
        return *this;
    }

    Value& expect_type(ValueType type)
    {
        if (m_type != type)
            throw simple::string().snprintf("Value type is not %s.", 128, type_to_name(type));
        return (Value &)*this;
    }

    // Domain/local relative operations
public:
    // Bind this value to specified domain
    Value& bind_to(Domain *domain);

    // Copy this value to local value list if this is a reference type value
    // After copy, caller should enter another domain & transfer these values
    // to it
    Value copy_to_local(Thread *thread)
    {
        if (m_type < REFERENCE_VALUE)
            return *this;

        // Copy reference value
        m_reference = m_reference->copy_to_local(thread);
        return *this;
    }

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

// Macro as functions
#define STRING_ALLOC(...)   StringImpl::alloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define STRING_FREE(string) StringImpl::free(__FILE__, __LINE__, string)

// VM value: string
struct StringImpl : public ReferenceImpl
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
        len = (string_size_t)size;
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual size_t hash_this() const { return simple::string::hash_string(buf); }

public:
    size_t length() const { return len; }
    const char_t *c_str() const { return buf; }

public:
    // Allocate a string with reserved size
    static StringImpl *alloc(const char *file, int line, size_t size = 0);

    // Allocate a string & construct it
    static StringImpl *alloc(const char *file, int line, const char *c_str, size_t len = SIZE_MAX);
    static StringImpl *alloc(const char *file, int line, const simple::string& str);
    static StringImpl *alloc(const char *file, int line, const StringImpl *other);

    // Free a string
    static void free(const char *file, int line, StringImpl *string);

    // Compare two strings
    static int compare(const StringImpl *a, const StringImpl *b);

public:
    // Concat with other string
    StringImpl *concat_string(const StringImpl *other) const;

    // Get sub string
    StringImpl *sub_string(size_t offset, size_t len = SIZE_MAX) const;

    // Generate string by snprintf
    static StringImpl *snprintf(const char *fmt, size_t n, ...);

public:
    inline int operator [](size_t index) const
    {
        if (index > length())
            throw "Index of string is out of range.";
        return (int)(uchar_t)buf[index];
    }

    inline bool operator <(const StringImpl& b) const
    {
        return StringImpl::compare(this, &b) < 0;
    }

    inline bool operator ==(const StringImpl& b) const
    {
        return StringImpl::compare(this, &b) == 0;
    }

private:
    string_size_t len;
    char_t buf[1]; // For count size of terminator '\x0'
};

// Macro as functions
#define BUFFER_ALLOC(...)   BufferImpl::alloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define BUFFER_FREE(string) BufferImpl::free(__FILE__, __LINE__, string)

// VM value: buffer
STD_BEGIN_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE)
struct BufferImpl : public ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::BUFFER;
    typedef enum
    {
        // The buffer contains 1 or N class
        CONTAIN_1_CLASS = 0x0001,
        CONTAIN_N_CLASS = 0x0002,
        CONTAIN_CLASS = CONTAIN_1_CLASS | CONTAIN_N_CLASS,
    } Attrib;

    enum { CLASS_ARR_STAMP = 0x19801126 };

    // Head structure for buffer to store class[] 
    STD_BEGIN_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE)
    struct ArrInfo
    {
        Uint32 n;
        Uint32 size;
        Uint32 stamp;
    }
    STD_END_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE);
    static const int RESERVE_FOR_CLASS_ARR = sizeof(ArrInfo);

    // Constructor (copy) function entry
    typedef void (*ConstructFunc)(void *ptr_class, const void *from_class);

    // Destructor function entry
    typedef void (*DestructFunc)(void *ptr_class);

private:
    BufferImpl(size_t _len)
    {
        attrib = (Attrib)0;
        constructor = 0;
        destructor = 0;
        len = _len;
    }

    virtual ~BufferImpl();

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual size_t hash_this() const;

public:
    // Get raw data pointer
    Uint8 *data() const { return (Uint8 *) (this + 1); }

    // Get single or class array pointer
    void *class_ptr(size_t index = 0) const
    {
        if (attrib & CONTAIN_1_CLASS)
        {
            STD_ASSERT(("Index out of range of class_ptr.", index == 0));
            return data();
        }
        if (attrib & CONTAIN_N_CLASS)
        {
            auto info = (ArrInfo *)(this + 1);
            STD_ASSERT(("Index out of range of class_ptr.", index < info->n));
            return data() + RESERVE_FOR_CLASS_ARR + index * info->size;
        }
        throw "This buffer doesn't not contain any class.";
    }

public:
    // Allocate a string with reserved size
    static BufferImpl *alloc(const char *file, int line, size_t size);

    // Allocate a string & construct it
    static BufferImpl *alloc(const char *file, int line, const void *p, size_t len);
    static BufferImpl *alloc(const char *file, int line, const BufferImpl *other);

    // Free a buffer
    static void free(const char *file, int line, BufferImpl *buffer);

public:
    // Compare two buffers
    static int compare(const BufferImpl *a, const BufferImpl *b);

public:
    Attrib attrib;
    ConstructFunc constructor;
    DestructFunc destructor;
    size_t len;
}
STD_END_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE);

// VM value: function
struct FunctionPtrImpl : public ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::FUNCTION;

public:
    FunctionPtrImpl()
    {
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual void mark(MarkValueState& value_map);
};

// VM value: array
struct ArrayImpl : public ReferenceImpl
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
    virtual ValueType get_type() const { return this_type; }
    virtual void mark(MarkValueState& value_map);

public:
    Value& operator [](const Value& index)
    {
        if (index.m_type != ValueType::INTEGER)
            throw simple::string().snprintf("Bad index to array (expected integer got %s).", 128,
                                            Value::type_to_name(index.m_type));

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

    // Get R-value
    Value operator[](const Value& index) const
    {
        return ((ArrayImpl *)this)->operator[](index);
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
struct MapImpl : public ReferenceImpl
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
    virtual ValueType get_type() const { return this_type; }
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
                                            Value::type_to_name(T::this_type),
                                            Value::type_to_name(other.m_type));
        m_reference = other.m_reference;
        return *this;
    }

    TypedValue& operator =(const TypedValue& other)
    {
        m_reference = other.m_reference;
        return *this;
    }

    TypedValue& operator =(const T *other_impl)
    {
        m_reference = (T *)other_impl;
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

    bool operator !=(const TypedValue& other) const
    {
        return !(*(T *)m_reference == *(const T *)other.m_reference);
    }

    bool operator >(const TypedValue& other) const
    {
        return *(const T *)other.m_reference < *(T *)m_reference;
    }

    bool operator <=(const TypedValue& other) const
    {
        return !(*(T *)m_reference > *(const T *)other.m_reference);
    }

    bool operator >=(const TypedValue& other) const
    {
        return !(*(T *)m_reference < *(const T *)other.m_reference);
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

    T *ptr()
    {
        return (T *)m_reference;
    }

    T *const ptr() const
    {
        return (T *)m_reference;
    }

};

class String : public TypedValue<StringImpl>
{
public:
    String() :
        TypedValue("")
    {
    }

    String(const Value& value) :
        TypedValue(value)
    {
    }

    String(StringImpl *const impl) :
        TypedValue(impl)
    {
    }

    String(const char *c_str, size_t len = SIZE_MAX) :
        TypedValue(c_str, len)
    {
    }

    String(const simple::string& str) :
        TypedValue(str)
    {
    }

    // Redirect function to impl()
    // ATTENTION:
    // Why not override -> or * (return T *) for redirection call?
    // Because the override may cause confusing, I would rather to write
    // more codes to make them easy to understand.
public:
    String operator +(const String& other) const
        { return impl().concat_string(&other.impl()); }

    String operator +=(const String& other)
        { return (*this = impl().concat_string(&other.impl())); }

    int operator[](size_t index) const
        { return impl()[index]; }

    const char *c_str() const
        { return impl().c_str(); }

    size_t length() const
        { return impl().length(); }

    // Generate string by snprintf
    template <typename... Types>
    static String snprintf(const char *fmt, size_t n, Types&&... args)
        { return StringImpl::snprintf(fmt, n, simple::forward<Types>(args)...); }

    String sub_string(size_t offset, size_t len = SIZE_MAX) const
        { return impl().sub_string(offset, len); }
};

class Buffer : public TypedValue<BufferImpl>
{
public:
    Buffer(const Value& value) :
        TypedValue(value)
    {
    }

    Buffer(BufferImpl *impl) :
        TypedValue(impl)
    {
    }
};

class Array : public TypedValue<ArrayImpl>
{
public:
    Array(const Value& value) :
        TypedValue(value)
    {
    }

    Array(ArrayImpl *impl) :
        TypedValue(impl)
    {
    }

    Array(size_t size_hint = 8);

public:
    Value& operator [](const Value& index)
        { return impl()[index]; }

    Value operator [](const Value& index) const
        { return impl()[index]; }

    Value& operator [](size_t index)
        { return impl()[Value(index)]; }

    Value operator [](size_t index) const
        { return impl()[Value(index)]; }

    void append(const Value& value)
        { impl().append(value); }

    auto begin() -> decltype(impl().a.begin())
        { return impl().a.begin(); }

    auto end() -> decltype(impl().a.begin())
        { return impl().a.end(); }

    size_t size() { return impl().size(); }
};

class Map : public TypedValue<MapImpl>
{
public:
    Map(const Value& value) :
        TypedValue(value)
    {
    }

    Map(MapImpl *impl) :
        TypedValue(impl)
    {
    }

    Map(size_t size_hint = 8);

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
