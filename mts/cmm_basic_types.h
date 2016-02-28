// cmm_basic_types.h

#pragma once

#include "std_port/std_port_type.h"
#include "std_template/simple_string.h"

namespace cmm
{

typedef simple::char_t char_t; // Character of string
typedef simple::uchar_t uchar_t; // Unsigned character

typedef IntR Handle;                // Handle

#if 1
typedef Uint16 ArgNo;               // Argument count/no
typedef Uint16 ComponentNo;         // Number of component in a program
typedef Uint16 ConstantIndex;       // Constant index in a program
typedef Uint32 ComponentOffset;     // Offset of component in class Object
typedef Uint32 FunctionNo;          // Number of function in a program
typedef Uint16 LocalNo;             // Local variables count/no
typedef Uint32 MapOffset;           // MapImpl offset for component no map
typedef Uint32 VariableNo;          // Index of member in AbstractClass object

#else
typedef size_t ArgNo;               // Argument count/no
typedef size_t ConstantIndex;       // Constant index in a program
typedef size_t ComponentNo;         // Number of component in a program
typedef size_t ComponentOffset;     // Offset of component in class Object
typedef size_t FunctionNo;          // Number of function in a program
typedef size_t LocalNo;             // Local variables count/no
typedef size_t MapOffset;           // MapImpl offset for component no map
typedef size_t VariableNo;          // Index of member in AbstractClass object
#endif

enum ValueType
{
    NIL = 0,            // Can be casted to any other type
    INTEGER = 1,        // Integer (int64)
    REAL = 2,           // Float (double)
    OBJECT = 3,         // Object
    STRING = 9,         // StringImpl
    REFERENCE_VALUE = 9,// Type >= it is a ReferenceImpl type
    BUFFER = 10,        // Binary data
    FUNCTION = 11,      // Function pointer
    ARRAY = 12,         // ArrayImpl
    MAPPING = 13,       // Mapping
    TVOID = 95,         // Void
    MIXED = 96,         // Mixed
    PRIMITIVE_TYPE = 97,// Primitive - Not for value, for compile use only
    ANY_TYPE = 98,      // Any type  - Not for value, for compile use only
    BAD_TYPE = 99,      // Bad type  - Not for value, for compile use only
};

enum
{
    GLOBAL_ID_PER_PAGE = (1 << 8)
};

// Global Id is a 64bits, cross multi-process Id.
// The Id.process_id indicates the process;
// index_page:index_offset indicates the index;
// version indicates the allocation times for this index
struct GlobalId
{
    union
    {
        struct
        {
            size_t  version : 32;
            size_t  index_page : 16;
            size_t  index_offset : 8;
            size_t  process_id : 8;
        };
        Uint64 i64;
    };

    // Get index of the id
    size_t index() const
    {
        return index_page * GLOBAL_ID_PER_PAGE + index_offset;
    }

    // Print id with prefix
    void print(char *buf, size_t size, const char *prefix) const;
};

inline bool operator == (const GlobalId& left, const GlobalId& other)
{
    return left.i64 == other.i64;
}

struct global_id_hash_func
{
    size_t operator()(const GlobalId& key) const
    {
        return (size_t)key.index_page * GLOBAL_ID_PER_PAGE + key.index_offset;
    }
};

typedef GlobalId ObjectId;
typedef GlobalId DomainId;

}
