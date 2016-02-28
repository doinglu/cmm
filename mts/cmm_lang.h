// cmm_lang.h:
// Initial version 2011.06.30 by shenyq
// Immigrated 2015.10.28 by doing

#pragma once

#include <setjmp.h>
#include "std_template/simple_hash_map.h"
#include "std_template/simple_hash_set.h"
#include "std_template/simple_list.h"
#include "cmm.h"
#include "cmm_ast.h"
#include "cmm_lang_symbols.h"
#include "cmm_lex_util.h"
#include "cmm_lexer.h"
#include "cmm_mem_list.h"
#include "cmm_value.h"

namespace cmm
{

// the state of context
// #if state
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

class Lang
{
    friend class Lexer;
    friend class LangSymbols;

public:
    // Initialize/shutdown this module
    static bool init();
    static void shutdown();

private:
    // Sub initialization routines of this module
    static bool init_expr_op_type();
    static void destruct_expr_op_type();

public:
    // Constructor
    Lang();

public:
    // Do parse
    ErrorCode parse();

    // Export to YACC
public:
    static void     syntax_error(Lang* context, const char* msg);
    static void     syntax_errors(Lang* context, const char* format, ...);
    static void     syntax_warn(Lang* context, const char* msg);
    static void     syntax_warns(Lang* context, const char* format, ...);
    static void     syntax_stop(Lang* context, ErrorCode ret);
    static void     syntax_echo(Lang* context, const char* msg);
    static void     print_ast(AstNode* node, int level);

    // Parse utility functions
public:
    simple::string  add_back_slash_for_quote(const simple::string& str);
    void            add_component(const char* component);
    int             get_frame_tag();
    AstNode*        get_loop_switch(int level);
    AstNode*        get_node_for_break();
    AstNode*        get_node_for_continue();
    bool            is_in_entry_function();
    bool            is_in_top_frame();
    void            pop_loop_switch(int level);
    int             push_loop_switch(AstNode* node);
    void            set_current_attrib(Uint32 attrib);

    // Pass1 - Collect object var & functions
private:
    bool            pass1();
    void            init_symbol_table();
    void            lookup_and_map_identifiers(AstNode *node);
    void            lookup_expr_types(AstNode *node);
    LocalNo         alloc_anonymouse_local(ValueType type);
    void            free_anonymouse_local(LocalNo no);

    // Pass2 - Create final result
private:
    bool pass2();

private:
    // Pass utilty functions
    bool       are_expr_list_constant(AstExpr* expr);
    ValueType  derive_type_of_op(AstExprOp* node, Op op, ValueType operand_type1, ValueType operand_type2, ValueType operand_type3);
    bool       check_lvalue(AstExpr* node);
    bool       try_map_assign_op_to_op(Op assign_op, Op* out_op);

    // Frame relative functions
private:
    void create_new_frame();
    void destruct_current_frame();

private:
    static void collect_children(AstNode* node);

public:
    // Memory list
    MemList mem_list;

    // Lexer
    Lexer m_lexer;

    // Symbol manager
    LangSymbols m_symbols;

    // number of errors
    Uint32 m_num_errors;

    // number of warnings
    Uint32 m_num_warnings;

    // is treat warnings as errors
    bool m_will_treat_warnings_as_errors;

    // context state
    simple::vector<AstNode*> m_loop_switches;

    // the depth of loop
    Uint32 m_loop_depth;

    // current compiling function
    AstFunction* m_in_function;

    // object entry function
    AstFunction* m_entry_function;

    // AST root
    AstNode* m_root;

    // Program's object var variables
    simple::vector<AstDeclaration*> m_object_vars;

    // Program's method function
    simple::vector<AstFunction*> m_functions;

    // All components
    simple::vector<simple::string> m_components;

    // All anonymouse local information
    struct LocalInfo
    {
        ValueType type;
        bool used;
    };
    typedef simple::vector<LocalInfo> LocalInfos;
    LocalInfos m_anon_locals;

    // long jump buffer
    jmp_buf m_jmp_buf;

    // The attribute for the file being compiled, from #pragma
    Uint32 m_current_attrib;

private:
    // Error code during compiling
    ErrorCode m_error_code;

    // Tag to declare new definition in symbol table
    int m_frame_tag;
};

// Internal routines
int cmm_lang_parse(Lang* context);

#define LANG_NEW(context, T, ...)   context->mem_list.new1<T>(__FILE__, __LINE__, ##__VA_ARGS__)
#define LANG_NEWN(context, T, n)    context->mem_list.newn<T>(__FILE__, __LINE__, n)
#define LANG_DELETE(context, p)     context->mem_list.delete1(__FILE__, __LINE__, p)
#define LANG_DELETEN(context, p)    context->mem_list.deleten(__FILE__, __LINE__, p)

}
