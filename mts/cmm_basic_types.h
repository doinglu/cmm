// cmm_basic_types.h

#pragma once

#include "std_port/std_port_type.h"
#include "std_template/simple_string.h"

namespace cmm
{

typedef simple::char_t char_t; // Character of string

#if 1
typedef Uint16 ArgNo;               // Argument count/no
typedef Uint32 MemberOffset;        // Offset of member in class object
typedef Uint16 ComponentNo;         // Number of component in a program
typedef Uint32 ComponentOffset;     // Offset of component in class Object
typedef Uint32 MapOffset;           // MapImpl offset for component no map
typedef Uint32 FunctionNo;          // Number of function in a program
#else
typedef size_t ArgNo;               // Argument count/no
typedef size_t MemberOffset;        // Offset of member in class object
typedef size_t ComponentNo;         // Number of component in a program
typedef size_t ComponentOffset;     // Offset of component in class Object
typedef size_t MapOffset;           // MapImpl offset for component no map
typedef size_t FunctionNo;          // Number of function in a program
#endif

typedef enum
{
    NIL = 0,            // Can be casted to any other type
    INTEGER = 1,        // Integer (int64)
    REAL = 2,           // Float (double)
    OBJECT = 3,         // Object
    REFERENCE_VALUE = 9,// Type >= ReferenceImpl is a ReferenceImpl type
    STRING = 9,         // StringImpl
    BUFFER = 10,        // Binary data
    FUNCTION = 11,      // Function pointer
    ARRAY = 12,         // ArrayImpl
    MAPPING = 13,       // Mapping
}  ValueType;

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
#if 1
            size_t  version : 32;
            size_t  index_page : 16;
            size_t  index_offset : 8;
            size_t  process_id : 8;
#else
            Uint32  version;
            Uint16  index_page;
            Uint8   index_offset;
            Uint8   process_id;
#endif
        };
        Uint64 i64;
    };
};

inline bool operator == (const GlobalId& left, const GlobalId& other)
{
    return left.i64 == other.i64;
}

enum { GLOBAL_ID_PER_PAGE = (1 << 8) };

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
