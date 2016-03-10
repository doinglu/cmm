// cmm_lang_symbols.h
// Initial version Feb/2/2016 by doing
// To organize symbols

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm.h"
#include "cmm_lang_component.h"
#include "cmm_mmm_value.h"

namespace cmm
{

class Function;
class Lang;

struct AstNode;
struct AstDeclaration;
struct AstFunction;
struct AstFunctionArg;
struct AstLabel;

// To speed up cleaning the hash table, and identify the union
enum IdentType : Uint16
{
    IDENT_RESWORD       = 0x0001,
    IDENT_EFUN          = 0x0002,
    IDENT_OS_FUN        = 0x0004,
    IDENT_OBJECT_FUN    = 0x0008,
    IDENT_OBJECT_VAR    = 0x0010,
    IDENT_LOCAL_VAR     = 0x0020,
    IDENT_ARGUMENT      = 0x0040,

    IDENT_FUN           = (IDENT_EFUN | IDENT_OS_FUN | IDENT_OBJECT_FUN),
    IDENT_VAR           = (IDENT_OBJECT_VAR | IDENT_LOCAL_VAR | IDENT_ARGUMENT),
    IDENT_ALL           = 0xFFFF,
};

// Mixed identifier entry
struct IdentInfo
{
    union
    {
        FunctionNo  function_no;      // Function no
        ArgNo       arg_no;           // Argument no
        LocalNo     local_var_no;     // Local variable
        ObjectVarNo object_var_no;    // Object variable no
        VariableNo  var_no;           // Common variable no
    };
    union
    {
        AstFunction*    ast_function; // AstNode of function definition
        AstFunctionArg* arg;          // AstNode of function argument
        AstDeclaration* decl;         // AstNode of variable declaration
    };
    IdentType  type;                  // Ident type
    int        tag;                   // Tag (level) for this identifier 
    IdentInfo* next;

    IdentInfo(Lang* lang_context);
};

class LangSymbols : LangComponent
{
public:
    LangSymbols(Lang* lang_context);

    // Ident symbols manager
public:
    bool        add_ident_info(const simple::string& name, IdentInfo* info, AstNode* node);
    IdentInfo*  get_ident_info(const simple::string& name, Uint32 expect_types);
    void        remove_ident_info_by_tag(int tag);

public:
    void        add_label_info(AstLabel* label);
    AstLabel*   get_label_info(const simple::string& label_name);

public:
    Function*   get_function(const simple::string& name, AstNode* node);

    // Utilities
public:

private:
    typedef simple::hash_map<simple::string, IdentInfo*> IdentTable;
    IdentTable m_ident_table;

    typedef simple::hash_map<simple::string, AstLabel*> LabelTable;
    LabelTable m_label_table;
};

}
