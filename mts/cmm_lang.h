// cmm_lang.h:
// Initial version 2011.06.30 by shenyq
// Immigrated 2015.10.28 by doing

#pragma once

#include <setjmp.h>
#include "std_template/simple_hash_map.h"
#include "cmm.h"
#include "cmm_lex_util.h"
#include "cmm_lexer.h"
#include "cmm_value.h"

namespace cmm
{

class Function;

/* Function type */
enum FunctionType
{
    SYN_FT_UNKNOWN    = 0,
    SYN_FT_NEAR_FUN   = 1,
    SYN_FT_FAR_FUN    = 2,
    SYN_FT_EFUN       = 4,
    SYN_FT_OBJECT_FUN = 5,
};

/* Context type */
enum ContextType
{
    CONTEXT_MAIN   = 0,
    CONTEXT_LOOP   = 1,
    CONTEXT_FOR    = 2,
    CONTEXT_WHILE  = 3,
    CONTEXT_SWITCH = 4,
};

enum AstNodeType
{
    AST_ROOT = 1,
    AST_COMPONENT = 10,
    AST_COMPONENTS = 11,
    AST_DECLARE_MEMBER = 20,
    AST_DECLARE_FUNCTION = 21,
    AST_SWITCH = 31,
    AST_CASE = 32,
    AST_ELEMENT = 41,
    AST_LVALUE_INFO = 42,
    AST_VAR_TYPE = 43,
    AST_EXPRESSION = 44,
    AST_EXPRESSIONS = 45,
    AST_FUNCTION_CALL = 46,
    AST_FUNCTION_ARG = 47,
    AST_FUNCTION_ARGS = 48,
    AST_REGISTER = 49,
    AST_REGISTERS = 50,
    AST_DECLARE = 51,
    AST_DECLARES = 52,
    AST_FUNCTION = 60,
    AST_FUNCTIONS = 61,
    AST_FUNCTION_DESC = 62,
    AST_VARIABLE_DESC = 70,
    AST_VARAIBLE_INFO = 71,
};

// The AST nodes
// AST node: abstract type */
struct AstNode
{
    virtual AstNodeType get_type() = 0;
    AstNode* next;

    AstNode() :
        next(0)
    {
    }
};

/* AST node: root */
struct AstRoot : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_ROOT; }
};

// AST node: component
struct AstComponent : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_COMPONENT; }
    String name;
};

struct AstComponents : AstNode, simple::hash_map<String, AstComponent*>
{
    virtual AstNodeType get_type() { return AstNodeType::AST_COMPONENTS; }
};

/* AST node: declare_member */
struct AstDeclareMember : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_DECLARE_MEMBER; }
    AstDeclare member;
};

/* AST node: declare_function */
struct AstDeclareFunction : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_DECLARE_FUNCTION; }
    AstFunction function;
};

/* Switch - case */
struct AstCase : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_CASE; }
    Value  case_value;
    bool   is_default;
    IntR   label_id;
    LineNo line;
    AstNode* statements;
    
    AstCase() :
        is_default(false),
        label_id(0),
        line(0),
        statements(0)
    {
    }
};

/* Switch - case */
struct AstSwitch : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_SWITCH; }
    simple::list<AstCase*> cases;
};

/* Variable information */
struct AstVariableInfo : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_VARAIBLE_INFO; }
    String name;
};

/* Element of expression */
struct AstElement : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_ELEMENT; }
    ValueType type;         /* element type */
    bool      is_constant;  /* is this element a constant */
    String    output;       /* element output */
    Int32     reg_index;    /* output register index */
    bool      is_ref;       /* is reference */

    AstElement() :
        type(MIXED),
        is_constant(false),
        reg_index(0),
        is_ref(false)
    {
    }
};

/* Left-value information */
struct AstLValueInfo : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_LVALUE_INFO; }
    AstLValueInfo* assigner_value; /* assignable value */
    AstElement*    element_value;  /* element value */
    AstElement*    index_from;     /* from index */
    AstElement*    index_to;       /* to index */
    bool           is_reverse_from;/* is from index reverse */
    bool           is_reverse_to;  /* is to index reverse */

    AstLValueInfo() :
        assigner_value(0),
        element_value(0),
        index_from(0),
        index_to(0),
        is_reverse_from(false),
        is_reverse_to(0)
    {
    }
};

/* Variable type */
struct AstVarType : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_VAR_TYPE; }
    ValueType basic_type;           /* basic variable type */
    Uint8     var_attrib;           /* variable attribute */

    AstVarType() :
        basic_type(MIXED),
        var_attrib(0)
    {
    }
};

/* Expression */
struct AstExpression : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_EXPRESSION; }
    ValueType type;          /* expression return type */
    Uint32 op;               /* expression operation */
    Uint8  attrib;           /* Attribute of expression */
    bool   is_constant;      /* if this is a constant expression */
    Int32  reg_index;        /* expression output register index */
    AstExpression* left;     /* right expression */
    AstExpression* right;    /* left expression */
    AstElement*    element;  /* only leaf node has elem */
    AstLValueInfo* lvalue;   /* only used when assign */

    AstExpression() :
        type(MIXED),
        op(0),
        attrib(0),
        is_constant(false),
        reg_index(0),
        left(0),
        right(0),
        element(0),
        lvalue(0)
    {
    }
};

/* Expression list */
struct AstExpressions : AstNode, simple::list<AstExpression*>
{
    virtual AstNodeType get_type() { return AstNodeType::AST_EXPRESSIONS; }
};

/* Function call */
struct AstFunctionCall : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_FUNCTION_CALL; }
    String function_name;        /* function name */
    AstExpressions arguments;    /* arguments */
};

/* Function argument */
struct AstFunctionArg : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_FUNCTION_ARG; }
    String          name;            /* function argument name */
    AstVarType      var_type;        /* argument variable type */
    AstExpression*  default_value;   /* default argument value */

    AstFunctionArg() :
        default_value(0)
    {
    }
};

/* Function arguments */
struct AstFunctionArgs : AstNode, simple::list<AstFunctionArg*>
{
    virtual AstNodeType get_type() { return AstNodeType::AST_FUNCTION_ARGS; }
    bool random_args; /* has var args */

    AstFunctionArgs() :
        random_args(0)
    {
    }
};

/* Register */
struct AstRegister : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_REGISTER; }
    ValueType type;                        /* register type */
    Uint16    index;                       /* register index */
    bool      is_using;                    /* is this register using currently */
    bool      is_fixed_reg;                /* is fixed register */

    AstRegister() :
        type(MIXED),
        index(0),
        is_using(false),
        is_fixed_reg(false)
    {
    }
};

/* Registers */
struct AstRegisters : AstNode, simple::list<AstRegister*>
{
    virtual AstNodeType get_type() { return AstNodeType::AST_REGISTERS; }
};

/* Variable declaration */
struct AstDeclare : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_DECLARE; }
    String         name;     /* variable name */
    AstVarType     type;     /* variable type */
    bool           is_used;  /* is used */
    LineNo         line;     /* declare line */
    AstExpression* expr;     /* assign expression */

    AstDeclare() :
        is_used(false),
        line(0),
        expr(0)
    {
    }
};

/* Delcares */
struct AstDeclares : AstNode, simple::hash_map<String, AstDeclare *>
{
    virtual AstNodeType get_type() { return AstNodeType::AST_DECLARES; }
};

/* Function */
struct AstFunction : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_FUNCTION; }

    /* function name */
    String name;

    /* function attrib */
    Uint16 attrib;

    /* return type */
    AstVarType ret_var_type;

    /* argument list */
    AstFunctionArgs arg_list;

    /* function body */
    AstNode *body;

    /* all the local variable declaration */
    AstDeclares local_vars;

    /* all the registers */
    AstRegisters registers;

    /* has return value */
    bool has_ret;

    /* is declaration */
    bool is_declaration;

    /* the function's first line */
    LineNo line;

    /* the function's real file name */
    String file;

    AstFunction() :
        body(0),
        has_ret(false),
        is_declaration(false),
        line(0)
    {
    }
};

struct AstFunctions : AstNode, simple::hash_map<String, AstFunction *>
{
    virtual AstNodeType get_type() { return AstNodeType::AST_FUNCTIONS; }
};

/* function description */
struct AstFunctionDesc : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_FUNCTION_DESC; }
    ValueType    type; /* type of function */
    ValueType    ret_type; /* type of return value */
    AstElement*  object;
    String       function_name;
    char*        info;
    FunctionNo   fun_offset;

    AstFunctionDesc() :
        type(MIXED),
        ret_type(MIXED),
        obejct(0),
        info(0),
        fun_offset(0)
    {
    }
};

/* Variable description */
struct AstVariableDesc : AstNode
{
    virtual AstNodeType get_type() { return AstNodeType::AST_VARIABLE_DESC; }
    String       name;
    AstVarType   type;
};

/* the state of context */
struct SyntaxContextState
{
    Int16 context_type;  /* current context type (loop / switch) */
    bool  is_in_block;   /* current in block (while / for / if / switch ) flag */
    bool  is_in_switch;  /* is in switch block */
    Int32 break_id;      /* break id */
    Int32 continue_id;   /* continue id */

    SyntaxContextState() :
        context_type(0),
        is_in_block(false),
        is_in_switch(false),
        break_id(0),
        continue_id(0)
    {
    }
};

/* #if state */
struct IfStatement
{
    enum IfState
    {
        EXPECT_ELSE = 1,
        EXPECT_ENDIF = 2,
    };

    IfStatement* next;
    IfState state;

    IfStatement() :
        next(0),
        state((IfState)0)
    {
    }
};

/* This file must be included after syntax structure definitions */
#include "cmm_grammar.h"

class LangContext
{
    friend class Lexer;

public:
    // This class is created by 
    LangContext();

public:
    void                syntax_error(const char* msg);
    void                syntax_errors(const char* format, ...);
    void                syntax_warn(const char* msg);
    void                syntax_warns(const char* format, ...);
    void                syntax_stop(IntR ret);
    void                syntax_echo(const char* msg);
    LangContext*        syntax_create_lang_context();
    SyntaxContextState* syntax_get_context_state();
    void                syntax_destroy_context();
    IntR                syntax_generate_unique_count();
    AstFunction*        syntax_create_function(const String& name);
    String              syntax_get_current_file_name();
    AstFunction*        syntax_get_current_function();
    void                syntax_destroy_function(AstFunction* func);
    AstDeclare*         syntax_create_declare(const String& name);
    void                syntax_destroy_declare(AstDeclare* decl);
    AstExpression*      syntax_create_expression(AstElement* element);
    void                syntax_destroy_expression(AstExpression* expression);
    AstElement*         syntax_create_element();
    void                syntax_destroy_element(AstElement* elem);
    AstLValueInfo*      syntax_create_lvalue_info();
    void                syntax_destroy_lvalue_info(AstLValueInfo* lvInfo);
    AstCase*            syntax_create_case();
    void                syntax_destroy_case(AstCase* ca);
    Function*           syntax_lookup_efun(const String& short_name);
    void                syntax_add_local_variable(AstDeclare* var_decl);
    void                syntax_add_member_variable(AstDeclare* var_decl);
    void                syntax_add_member_function(AstFunction* func);
    AstDeclare*         syntax_get_local_variable(const String& name);
    AstFunction*        syntax_get_member_function(const String& name);
    AstDeclare*         syntax_get_member_variable(const String& name);
    void                syntax_add_component(const String& name);
    void                syntax_into_block();
    void                syntax_out_block();
    void                syntax_push_label_id();
    void                syntax_pop_label_id();
    void                syntax_push_context_state(IntR context_state);
    void                syntax_pop_context_state();
    String              syntax_add_back_slash_for_quote(const String& str);
    LineNo              syntax_get_current_line();
    void                set_current_attrib(Uint32 attrib);

public:
    AstExpression*   gen_op_const(Uint32 op, AstExpression *left, AstExpression *right);
    AstExpression*   gen_cast_const(AstExpression *exp, ValueType type);

public: // Exporse to yyparse
    /* Lexer */
    Lexer m_lexer;

    /* number of errors */
    Uint32 m_num_errors;

    /* number of warnings */
    Uint32 m_num_warnings;

    /* is treat warnings as errors */
    bool m_will_treat_warnings_as_errors;

    /* unique count, used to generate unique name */
    Uint32 m_unique_count;

    /* context state */
    SyntaxContextState m_context_state;

    /* the depth of loop */
    Uint32 m_loop_depth;

    /* current compiling function */
    AstFunction* m_in_function;

    /* object entry function */
    AstFunction* m_entry_function;

    /* all the members variables declaration */
    AstDeclares m_member_vars;

    /* all the member methods */
    AstFunctions m_methods;

    // All components
    AstComponents m_components;

    /* all the components */
//    Vector_T m_components;

    /* entry source */
    char* m_entry;

    /* asm source of current compiling program */
    char* m_source;

    /* long jump buffer */
    jmp_buf m_jmp_buf;

    /* The attribute for the file being compiled, from #pragma */
    Uint32 m_current_attrib;
};

// Utilities for single linked list
template <typename T>
void list_append(T** pplist, T* node)
{
    T* p;
    while ((p = *pplist))
        pplist = &(T*)p->next;
    *pplist = node;
}

template <typename T>
bool list_remove(T** pplist, T* node)
{
    T* p;
    while ((p = *pplist))
    {
        if (p == node)
        {
            *pplist = p->next;
            return true;
        }
        pplist = &(T*)p->next;
    }
    return false;
}

template <typename T>
void list_free(T** pplist)
{
    T* p;
    while ((p = *pplist))
    {
        *pplist = (T*)p->next;
        BUFFER_DELETE(p);
    }
}

}
