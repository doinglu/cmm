// cmm_lang_symbols.h
// Initial version Feb/2/2016 by doing
// To organize symbols

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm_mmm_value.h"

namespace cmm
{

class  Lang;
struct AstNode;

// To speed up cleaning the hash table, and identify the union
enum IdentType
{
    IDENT_RESWORD       = 0x0001,
    IDENT_EFUN          = 0x0002,
    IDENT_OS_FUN        = 0x0004,
    IDENT_OBJECT_FUN    = 0x0008,
    IDENT_OBJECT_VAR    = 0x0010,
    IDENT_LOCAL_VAR     = 0x0020,

    IDENT_FUN           = (IDENT_EFUN | IDENT_OS_FUN | IDENT_OBJECT_FUN),
    IDENT_VAR           = (IDENT_OBJECT_VAR | IDENT_LOCAL_VAR),
    IDENT_ALL           = 0xFFFF,
};

// Mixed identifier entry
struct IdentInfo
{
    union
    {
        FunctionNo  function_no;    // Function number
        LocalNo     local_var_no;   // local variable number
        VariableNo  object_var_no;  // Object variable number
        VariableNo  var_no;         // Common variable number
    };
    union
    {
        AstFunction*    function;   // AstNode of function definition
        AstDeclaration* decl;       // AstNode of variable declaration
    };
    Uint32 type;                    // Ident type
    int    tag;                     // Tag (level) for this identifier 
    IdentInfo* next;

    IdentInfo(Lang* context);
};

class LangSymbols
{
public:
    LangSymbols(Lang* lang_context) :
        m_lang_context(lang_context)
    {
    }

    // Ident symbols manager
public:
    bool        add_ident_info(const simple::string& name, IdentInfo* info, AstNode* node);
    IdentInfo*  get_ident_info(const simple::string& name, Uint32 expect_types);
    void        remove_ident_info_by_tag(int tag);

    // Utilities
public:

private:
    typedef simple::hash_map<simple::string, IdentInfo*> IdentTable;
    IdentTable m_ident_table;

    Lang* m_lang_context;
};

}
