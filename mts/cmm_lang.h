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
#include "cmm_lang_cfg.h"
#include "cmm_lang_phi.h"
#include "cmm_lang_symbols.h"
#include "cmm_lex_util.h"
#include "cmm_lexer.h"
#include "cmm_mem_list.h"
#include "cmm_value.h"

namespace cmm
{

class Function;

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
    friend class LangCFG;
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
    // Constructor/Destructor
    Lang();
    ~Lang();

public:
    // Do parse
    ErrorCode parse();

    // Export to YACC
public:
    static void     syntax_error(Lang* lang_context, const char* msg);
    static void     syntax_errors(Lang* lang_context, const char* format, ...);
    static void     syntax_warn(Lang* lang_context, const char* msg);
    static void     syntax_warns(Lang* lang_context, const char* format, ...);
    static void     syntax_stop(Lang* lang_context, ErrorCode ret);
    static void     syntax_echo(Lang* lang_context, const char* msg);
    static void     print_ast(AstNode* node, int level);
    static void     print_node(AstNode* node);

    // Parse utility functions
public:
    simple::string  add_back_slash_for_quote(const simple::string& str);
    void            add_component(const char* component);
    int             get_frame_tag();
    AstBreakCont*   get_loop_switch(int level);
    AstBreakCont*   get_node_for_break();
    AstBreakCont*   get_node_for_continue();
    bool            is_in_entry_function();
    bool            is_in_top_frame();
    void            pop_loop_switch(int level);
    int             push_loop_switch(AstBreakCont* node);
    AstLabel*       new_label();
    void            set_current_attrib(Uint32 attrib);

    // Pass1 - Collect object var & functions
private:
    bool            pass1();
    void            init_symbol_table();

private:
    // Steps for pass
    void            lookup_and_collect_info(AstNode *node);
    void            lookup_expr_types_and_output(AstNode *node);
    void            lookup_and_remove_function_node(AstNode *node);

private:
    VirtualRegNo    alloc_virtual_reg(ValueType type, bool may_nil);
    void            free_virtual_reg(VirtualRegNo no);

private:
    void            alloc_expr_output(AstExpr* expr);
    void            free_expr_output(AstExpr* expr);

private:
    // Pass utilty functions
    bool            are_expr_list_constant(AstExpr* expr);
    bool            check_lvalue(AstExpr* node);
    Function*       create_function(AstFunction* ast_function);
    IdentInfo*      define_new_local(simple::string& name, AstNode* node);
    ValueType       derive_type_of_op(AstExprOp* node, Op op, ValueType operand_type1, ValueType operand_type2, ValueType operand_type3);
    bool            try_map_assign_op_to_op(Op assign_op, Op* out_op);

    // Pass2 - Analyse functions
private:
    bool            pass2(AstFunction* function);

private:
    // Steps for pass
    void            lookup_and_find_out_branch(AstNode* node);
    bool            lookup_and_gather_nodes(AstNode* node);
    void            lookup_and_build_ssa(AstNode* node);

private:
    // Pass utilty functions

    // Frame relative functions
private:
    void            create_new_frame();
    void            destruct_current_frame();

private:
    const simple::string& get_object_var_name(ObjectVarNo object_var_no) const;

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
    simple::vector<AstBreakCont*> m_loop_switches;

    // the depth of loop
    Uint32 m_loop_depth;

    // current compiling function
    AstFunction* m_in_ast_function;

    // object entry function
    AstFunction* m_entry_ast_function;

    // AST root
    AstNode* m_root;

    // Ast declaration of variables
    simple::vector<AstDeclaration*> m_object_vars;

    // Ast functions
    simple::vector<AstFunction*> m_ast_functions;

    // All components
    simple::vector<simple::string> m_components;

    // All anonymouse local information
    struct VirtualRegInfo
    {
        ValueType type;
        bool used;
    };
    typedef simple::vector<VirtualRegInfo> VirtualRegInfos;
    VirtualRegInfos m_virtual_regs;

    // CFG
    LangCFG m_cfg;

    // Phi
    LangPhi m_phi;

    // long jump buffer
    jmp_buf m_jmp_buf;

    // The attribute for the file being compiled, from #pragma
    Uint32 m_current_attrib;

private:
    // If set, stop compiling when necessary
    // (normally stop when entering next phase)
    ErrorCode m_stop_error_code;

    // Tag to declare new definition in symbol table
    int m_frame_tag;

    // class Function (used by class Program)
    simple::vector<Function*> m_program_functions;
};

// Internal routines
int cmm_lang_parse(Lang* lang_context);

#define LANG_NEW(lang_context, T, ...)  lang_context->mem_list.new1<T>(__FILE__, __LINE__, ##__VA_ARGS__)
#define LANG_NEWN(lang_context, T, n)   lang_context->mem_list.newn<T>(__FILE__, __LINE__, n)
#define LANG_DELETE(lang_context, p)    lang_context->mem_list.delete1(__FILE__, __LINE__, p)
#define LANG_DELETEN(lang_context, p)   lang_context->mem_list.deleten(__FILE__, __LINE__, p)
#define NEW_LABEL()                     lang_context->new_label()
#define NEW_NOP()                       LANG_NEW(lang_context, AstNop, lang_context)
#define SET_NULL_TO_NOP(node)           if (!(node)) (node) = NEW_NOP()

}
