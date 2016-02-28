// cmm_lang.cpp:
// Initial version 2011.06.30 by shenyq
// Immigrated 2015.10.29 by doing

#include <stdarg.h>
#include "cmm.h"
#include "cmm_buffer_new.h"
#include "cmm_efun.h"
#include "cmm_lang.h"
#include "cmm_shell.h"

namespace cmm
{

// When the num of error exceeds MAX_COMPILE_ERRORS, stop compile 
enum { MAX_COMPILER_ERRORS  = 64 };

bool Lang::init()
{
    if (!init_expr_op_type())
        // Failed to initialize the sub module
        return false;

    return true;
}

void Lang::shutdown()
{
    destruct_expr_op_type();
}

// Initialize the context
Lang::Lang() :
    m_components((size_t)0),
    m_lexer(this),
    m_symbols(this)
{
    // Initialize before any node created
    m_in_function = 0;

    // Create the entry function @ no:0
    m_entry_function = LANG_NEW(this, AstFunction, this);
    m_entry_function->prototype = LANG_NEW(this, AstPrototype, this);
    m_entry_function->no = 0;
    m_functions.push_back(m_entry_function);

    m_num_errors = 0;
    m_num_warnings = 0;
    m_will_treat_warnings_as_errors = false;
    m_loop_depth = 0;
    m_root = 0;
    m_current_attrib = 0;
    m_error_code = ErrorCode::OK;
    m_frame_tag = 0;
}

// Parse & generate AST
ErrorCode Lang::parse()
{
    // Set m_in_function to m_entry_function means start parse()
    m_in_function = m_entry_function;
    m_error_code = (ErrorCode)cmm_lang_parse(this);
    if (m_num_errors)
        // Got error
        m_error_code = COMPILE_ERROR;
    if (!m_root)
        return m_error_code;

    // Set m_in_function to 0 means parse() is finished, start pass()
    m_in_function = 0;

    // Collect children
    collect_children(m_root);

    // Pass1
    if (!pass1())
        return ErrorCode::PASS1_ERROR;

    if (!pass2())
        return ErrorCode::PASS2_ERROR;

    return ErrorCode::OK;
}

// Compiler's error function 
void Lang::syntax_error(Lang* context, const char *msg)
{
    context->m_num_errors++;

    cmm_errprintf("%s(%d): error %d: %s\n",
                  context->m_lexer.m_current_file_name->c_str(),
                  (int)context->m_lexer.m_current_line,
                  C_PARSER, msg);

    context->m_num_errors++;
    if (context->m_num_errors >= MAX_COMPILER_ERRORS)
    {
        cmm_errprintf("Max errors(%d) exceeded.\nCompiler terminated\n",
                      MAX_COMPILER_ERRORS);
        syntax_stop(context, COMPILE_ERROR);
    }
}

// Compiler's error function 
void Lang::syntax_errors(Lang* context, const char *format, ...)
{
    context->m_num_errors++;

    va_list args;
    va_start(args, format);
    cmm_vprintf(format, args);
    va_end(args);

    if (context->m_num_errors >= MAX_COMPILER_ERRORS)
    {
        cmm_errprintf("Max errors(%d) exceeded.\nCompiler terminated\n",
                      MAX_COMPILER_ERRORS);
        syntax_stop(context, COMPILE_ERROR);
    }
}


// Compiler's warning function 
void Lang::syntax_warn(Lang* context, const char *msg)
{
#ifndef DISABLE_WARNING
    // If treat-warning-as-error flag is on 
    if (context->m_will_treat_warnings_as_errors)
        context->m_num_errors++;

    cmm_printf("%s(%d): warning %d: %s\n",
                context->m_lexer.m_current_file_name->c_str(),
                (int)context->m_lexer.m_current_line,
                C_PARSER, msg);
#endif        

    context->m_num_warnings++;
}

// Compiler's warning function 
void Lang::syntax_warns(Lang* context, const char *format, ...)
{
#ifndef DISABLE_WARNING
    // If treat-warning-as-error flag is on 
    if (context->m_will_treat_warnings_as_errors)
        context->m_num_errors++;

    va_list args;
    va_start(args, format);
    cmm_vprintf(format, args);
    va_end(args);
#endif

    context->m_num_warnings++;
}

// Stop compile 
void Lang::syntax_stop(Lang* context, ErrorCode ret)
{
    // long jump 
    longjmp(context->m_jmp_buf, (int)ret);
}

// Echo when compiling 
void Lang::syntax_echo(Lang* context, const char *msg)
{
    cmm_printf(msg);
}

// Print the tree
void Lang::print_ast(AstNode* node, int level)
{
    if (!node)
        return;

    for (auto i = 0; i < level; i++)
        printf("  ");
    printf("%s: %s\n", ast_node_type_to_c_str(node->get_node_type()), node->to_string().c_str());

    // Collect recursive in children
    level++;
    for (auto p = node->children; p != 0; p = p->sibling)
        print_ast(p, level);
}

// Convert string 
simple::string Lang::add_back_slash_for_quote(const simple::string& str)
{
    // " ->\"
    // \ -> \\

    // Count size after conversion
    size_t counter = 0;
    for (size_t i = 0; i < str.length(); i++)
        if (str[i] == '\"' || str[i] == '\\')
            counter++;

    // Create new string
    simple::string new_string(simple::reserve_space::EMPTY, (str.length() + counter));
    char_t *buf = (char_t*)new_string.c_str();
    size_t k = 0;
    for (size_t i = 0; i < str.length(); i++)
    {
        char_t ch = buf[i];
        if (ch == '\"' || ch == '\\')
            buf[k++] = '\\';
        buf[k++] = ch;
    }
    STD_ASSERT(buf[k] == 0);
    return new_string;
}

// Add a component
void Lang::add_component(const char* component)
{
    m_components.push_back(component);
}

// Return the current frame tag
int Lang::get_frame_tag()
{
    return m_frame_tag;
}

// Return the nearest loop switch node
AstNode* Lang::get_loop_switch(int level)
{
    return m_loop_switches[level];
}

// Return any block
AstNode* Lang::get_node_for_break()
{
    return m_loop_switches[m_loop_switches.size() - 1];
}

// Return nearest loop block (do / while / for / loop)
AstNode* Lang::get_node_for_continue()
{
    auto i = m_loop_switches.size();
    while (i > 0)
    {
        i--;
        auto* node = m_loop_switches[i];
        auto type = node->get_node_type();
        if (type == AST_DO_WHILE ||
            type == AST_WHILE_LOOP ||
            type == AST_FOR_LOOP ||
            type == AST_LOOP_EACH ||
            type == AST_LOOP_RANGE)
            return node;
    }

    return 0;
}

// Is the parser processing in entry function?
// eg.
/*
Entry-Function?   Top-Frame?
int a;              -       Y               Y
{                   -       Y               N
int x;           -       Y               N
}                   -       Y               N
void func()         -       N               N
{                   -       N               N
...              -       N               N
}                   -       N               N
int b;              -       Y               Y
*/
// Valid when in parse()
bool Lang::is_in_entry_function()
{
    STD_ASSERT(("is_in_entry_function() valid for parse() only.", m_in_function != 0));
    return m_in_function == m_entry_function;
}

// Is the parser in top frame?
// See comment of Lang::is_in_entry_function
// Valid when in pass()
bool Lang::is_in_top_frame()
{
    STD_ASSERT(("is_in_top_frame() valid for pass() only.", m_in_function == 0));
    return m_frame_tag == 1;
}

// Pop the nearest loop-switch node
void Lang::pop_loop_switch(int level)
{
    STD_ASSERT(("No loop-switch node to pop.", m_loop_switches.size() > 0));
    while ((int)m_loop_switches.size() > level)
        m_loop_switches.remove(m_loop_switches.size() - 1);
}

// Push a new loop-switch node
int Lang::push_loop_switch(AstNode* node)
{
    m_loop_switches.push_back(node);
    return (int)m_loop_switches.size() - 1;
}

void Lang::set_current_attrib(Uint32 attrib)
{
    m_current_attrib = attrib;
}

// Create new frame to contain definitions
void Lang::create_new_frame()
{
    m_frame_tag++;
}

// Destruct current frame & release definitions
void Lang::destruct_current_frame()
{
    m_symbols.remove_ident_info_by_tag(m_frame_tag);
    m_frame_tag--;
}

// Collect children to create tree
void Lang::collect_children(AstNode* node)
{
    if (!node)
        return;

    // Collect children of this node
    node->collect_children();

    // Collect recursive in children
    for (auto p = node->children; p != 0; p = p->sibling)
        collect_children(p);
}

}
