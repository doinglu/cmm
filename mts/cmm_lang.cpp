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

/* When the num of error exceeds MAX_COMPILE_ERRORS, stop compile */
enum { MAX_COMPILER_ERRORS  = 64 };

// Initialize the context
LangContext::LangContext()
{
    m_num_errors = 0;
    m_num_warnings = 0;
    m_will_treat_warnings_as_errors = false;
    m_unique_count = 0;
    m_loop_depth = 0;
    m_in_function = NULL;
    m_entry_function = NULL;
    m_current_attrib = 0;
}

String LangContext::syntax_get_current_file_name()
{
    return m_lexer.m_current_file_string;
}

/* Get current line */
LineNo LangContext::syntax_get_current_line()
{
    return m_lexer.get_current_line();
}

/* Compiler's error function */
void LangContext::syntax_error(const char *msg)
{
    m_num_errors++;

    cmm_errprintf("Error:   %s:%d: %s\n",
                  syntax_get_current_file_name().c_str(),
                  syntax_get_current_line(), msg);

    m_num_errors++;
    if (m_num_errors >= MAX_COMPILER_ERRORS)
    {
        cmm_errprintf("Max errors(%d) exceeded.\nCompiler terminated\n",
                      MAX_COMPILER_ERRORS);
        syntax_stop(COMPILE_ERROR);
    }
}

/* Compiler's error function */
void LangContext::syntax_errors(const char *format, ...)
{
    m_num_errors++;

    va_list args;
    cmm_errprintf("Error:   %s: ",
                  syntax_get_current_file_name().c_str());
    va_start(args, format);
    cmm_vprintf(format, args);
    va_end(args);

    if (m_num_errors >= MAX_COMPILER_ERRORS)
    {
        cmm_errprintf("Max errors(%d) exceeded.\nCompiler terminated\n",
                      MAX_COMPILER_ERRORS);
        syntax_stop(COMPILE_ERROR);
    }
}


/* Compiler's warning function */
void LangContext::syntax_warn(const char *msg)
{
    /* If treat-warning-as-error flag is on */
    if (m_will_treat_warnings_as_errors)
    {
        syntax_error(msg);
    }
    /* Traditional way */
    else
    {
#ifndef DISABLE_WARNING
        cmm_printf("Warning: %s:%d: %s\n",
                   syntax_get_current_file_name().c_str(),
                   syntax_get_current_line(), msg);
#endif        

        m_num_warnings++;
    }
}

/* Compiler's warning function */
void LangContext::syntax_warns(const char *format, ...)
{
    va_list args;

    /* If treat-warning-as-error flag is on */
    if (m_will_treat_warnings_as_errors)
    {
        m_num_errors++;

        va_list args;
        cmm_errprintf("Error:   %s: ",
                      syntax_get_current_file_name().c_str());
        va_start(args, format);
        cmm_vprintf(format, args);
        va_end(args);

        if (m_num_errors >= MAX_COMPILER_ERRORS)
        {
            cmm_errprintf("Max errors(%d) exceeded.\nCompiler terminated\n",
                          MAX_COMPILER_ERRORS);
            syntax_stop(COMPILE_ERROR);
        }
    }
    /* Traditional way */
    else
    {
#ifndef DISABLE_WARNING
        va_list args;

        cmm_printf("Warning: %s: ", syntax_get_current_file_name().c_str());

        va_start(args, format);
        cmm_vprintf(format, args);
        va_end(args);
#endif

        m_num_warnings++;
    }
}

/* Stop compile */
void LangContext::syntax_stop(IntR ret)
{
    /* long jump */
    longjmp(m_jmp_buf, (int) ret);
}

/* Echo when compiling */
void LangContext::syntax_echo(const char *msg)
{
    cmm_printf(msg);
}

/* Create context structure */
LangContext* LangContext::syntax_create_lang_context()
{
    LangContext *context = BUFFER_NEW(LangContext);
    return context;
}

/* Get current context state */
SyntaxContextState* LangContext::syntax_get_context_state()
{
    return &m_context_state;
}

/* Destroy context */
void LangContext::syntax_destroy_context()
{
    syntax_destroy_function(m_entry_function);
    BUFFER_DELETE(this);
}

/* Generate a unique count */
IntR LangContext::syntax_generate_unique_count()
{
    return m_unique_count++;
}

/* Create a new function */
AstFunction* LangContext::syntax_create_function(const String& name)
{
    AstFunction *func = BUFFER_NEW(AstFunction);
    func->name = name;
    func->has_ret = false;
    func->is_declaration = false;
    func->file = syntax_get_current_file_name();
    func->line = syntax_get_current_line();
    return func;
}

/* Destroy a function */
void LangContext::syntax_destroy_function(AstFunction *func)
{
    BUFFER_DELETE(func);
}

/* Get current function */
AstFunction* LangContext::syntax_get_current_function()
{
    if (m_in_function)

        return m_in_function;

    STD_ASSERT(m_entry_function);
    return m_entry_function;
}

/* Create a declaration */
AstDeclare* LangContext::syntax_create_declare(const String& name)
{
    auto* decl = BUFFER_NEW(AstDeclare);
    decl->name = name;
    decl->is_used = false;
    decl->type.basic_type = MIXED;
    decl->type.var_attrib = 0;
    decl->line = 0;
    return decl;
}

/* Destroy a declaration */
void LangContext::syntax_destroy_declare(AstDeclare* decl)
{
    BUFFER_DELETE(decl);
}

/* Create a new expression */
AstExpression* LangContext::syntax_create_expression(AstElement* element)
{
    auto* expression = BUFFER_NEW(AstExpression);
    expression->element = element;
    expression->lvalue = NULL;
    expression->left = NULL;
    expression->right = NULL;
    expression->op = 0;
    expression->is_constant = false;
    expression->type = MIXED;
    expression->reg_index = -1;
    return expression;
}

/* Destroy expression */
void LangContext::syntax_destroy_expression(AstExpression* expression)
{
    if (expression->left != NULL)
        syntax_destroy_expression(expression->left);

    if (expression->right != NULL)
        syntax_destroy_expression(expression->right);

    BUFFER_DELETE(expression);
}

/* Create new element */
AstElement* LangContext::syntax_create_element()
{
    auto* element = BUFFER_NEW(AstElement);
    element->is_constant = false;
    element->type = MIXED;
    element->reg_index = -1;
    element->is_ref = false;
    return element;
}

/* Destroy element */
void LangContext::syntax_destroy_element(AstElement* element)
{
    BUFFER_DELETE(element);
}

/* Create left value information */
AstLValueInfo* LangContext::syntax_create_lvalue_info()
{
    auto* lvalue_info = BUFFER_NEW(AstLValueInfo);
    lvalue_info->assigner_value = NULL;
    lvalue_info->element_value = NULL;
    lvalue_info->index_from = NULL;
    lvalue_info->index_to = NULL;
    lvalue_info->is_reverse_from = false;
    lvalue_info->is_reverse_to = false;
    return lvalue_info;
}

/* Destroy left value information */
void LangContext::syntax_destroy_lvalue_info(AstLValueInfo *lvalue_info)
{
    if (lvalue_info->index_from)
        BUFFER_DELETE(lvalue_info->index_from);

    if (lvalue_info->index_to)
        BUFFER_DELETE(lvalue_info->index_to);

    if (lvalue_info->element_value)
        BUFFER_DELETE(lvalue_info->element_value);

    BUFFER_DELETE(lvalue_info);
}

/* Create case */
AstCase* LangContext::syntax_create_case()
{
    auto* a_case = BUFFER_NEW(AstCase);
    a_case->is_default = false;
    a_case->label_id = -1;
    a_case->line = 0;
    return a_case;
}

/* Destroy case */
void LangContext::syntax_destroy_case(AstCase* a_case)
{
    BUFFER_DELETE(a_case);
}

/* Add a local variable declaration */
void LangContext::syntax_add_local_variable(AstDeclare* var_decl)
{
    auto* func = syntax_get_current_function();
    STD_ASSERT(func);
    if (func->local_vars.contains_key(var_decl->name))
    {
        syntax_errors("Variable \"%s\" was already existed.\n",
                      var_decl->name.c_str());
        return;
    }

    func->local_vars.put(var_decl->name, var_decl);
}

/* Add member variable */
void LangContext::syntax_add_member_variable(AstDeclare* var_decl)
{
    if (m_member_vars.contains_key(var_decl->name))
    {
        syntax_errors("Variable \"%s\" was already existed.\n",
                      var_decl->name.c_str());
        return;
    }
        
    m_member_vars.put(var_decl->name, var_decl);
}

/* Add member function */
void LangContext::syntax_add_member_function(AstFunction* func)
{
    if (m_methods.contains_key(func->name))
    {
        syntax_errors("Function \"%s\" was already existed.\n",
                      func->name.c_str());
        return;
    }

    m_methods.put(func->name, func);
}

/* Get local variable */
AstDeclare* LangContext::syntax_get_local_variable(const String& name)
{
    AstFunction* func = syntax_get_current_function();
    AstDeclare* decl = NULL;
    func->local_vars.try_get(name, &decl);
    return decl;
}

/* Get member function */
AstFunction* LangContext::syntax_get_member_function(const String& name)
{
    AstFunction* function = NULL;
    m_methods.try_get(name, &function);
    return function;
}

/* Get member variable */
AstDeclare* LangContext::syntax_get_member_variable(const String& name)
{
    AstDeclare* var_decl = NULL;
    m_member_vars.try_get(name, &var_decl);
    return var_decl;
}

/* lookup efun in current using packages */
Function* LangContext::syntax_lookup_efun(const String& short_name)
{
    Function *function = Efun::get_efun(short_name);
    return function;
}

/* Add inherit name */
void LangContext::syntax_add_component(const String& name)
{
    auto* component = BUFFER_NEW(AstComponent);
    component->name = name;

    if (m_components.contains_key(name))
    {
        syntax_warns("Component \"%s\" was already added.\n",
                     name.c_str());
        return;
    }

    m_components.put(name, component);
}

/* Convert string */
String LangContext::syntax_add_back_slash_for_quote(const String& str)
{
    // " ->\"
    // \ -> \\

    // Count size after conversion
    size_t counter = 0;
    for (size_t i = 0; i < str.length(); i++)
        if (str[i] == '\"' || str[i] == '\\')
            counter++;

    // Create new string
    StringImpl* new_string = STRING_ALLOC((str.length() + counter) * sizeof(char_t));
    char_t *buf = new_string->ptr();
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

}
