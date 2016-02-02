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
#include "cmm_lex_util.h"
#include "cmm_lexer.h"
#include "cmm_value.h"

namespace cmm
{

// the state of context
struct SyntaxContextState
{
    Int16 context_type;  // current context type (loop / switch)
    bool  is_in_loop;    // is in loop block (while / for / if / loop)
    bool  is_in_switch;  // is in switch block

    SyntaxContextState() :
        context_type(0),
        is_in_loop(false),
        is_in_switch(false)
    {
    }
};

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

public:
    // Initialize/shutdown this module
    static bool init();
    static void shutdown();

public:
    // Constructor
    Lang();

public:
    // Do parse
    ErrorCode parse();

public:
    // Export to YACC
    static void         syntax_error(Lang* context, const char* msg);
    static void         syntax_errors(Lang* context, const char* format, ...);
    static void         syntax_warn(Lang* context, const char* msg);
    static void         syntax_warns(Lang* context, const char* format, ...);
    static void         syntax_stop(Lang* context, ErrorCode ret);
    static void         syntax_echo(Lang* context, const char* msg);

public:
    // Parse utility functions
    void                syntax_add_component(const String& component);
    SyntaxContextState* syntax_get_context_state();
    String              syntax_add_back_slash_for_quote(const String& str);
    void                set_current_attrib(Uint32 attrib);

public: // Exporse to yyparse
    // Lexer
    Lexer m_lexer;

    // number of errors
    Uint32 m_num_errors;

    // number of warnings
    Uint32 m_num_warnings;

    // is treat warnings as errors
    bool m_will_treat_warnings_as_errors;

    // context state
    SyntaxContextState m_context_state;

    // the depth of loop
    Uint32 m_loop_depth;

    // current compiling function
    AstPrototype* m_in_function;

    // object entry function
    AstFunction* m_entry_function;

    // AST root
    AstNode* m_root;

#if 0
    // all the members variables declaration
    AstDeclares m_member_vars;

    // all the member methods
    AstFunctions m_methods;

#endif
    // All components
    simple::vector<MMMString> m_components;

    // long jump buffer
    jmp_buf m_jmp_buf;

    // The attribute for the file being compiled, from #pragma
    Uint32 m_current_attrib;

private:
    static void collect_children(AstNode* node);
    static void print_ast(AstNode* node, int level);
};

// Internal routines
int cmm_lang_parse(Lang* context);

}
