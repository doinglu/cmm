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
    return true;
}

void Lang::shutdown()
{
}

// Initialize the context
Lang::Lang() :
    m_components((size_t)0),
    m_lexer(this)
{
    m_num_errors = 0;
    m_num_warnings = 0;
    m_will_treat_warnings_as_errors = false;
    m_loop_depth = 0;
    m_in_function = NULL;
    m_entry_function = BUFFER_NEW(AstFunction);
    m_entry_function->prototype = BUFFER_NEW(AstPrototype);
    m_current_attrib = 0;
}

void Lang::set_current_attrib(Uint32 attrib)
{
    m_current_attrib = attrib;
}

// Compiler's error function 
void Lang::syntax_error(Lang* context, const char *msg)
{
    context->m_num_errors++;

    cmm_errprintf("Error:   %s:%d: %s\n",
                  context->m_lexer.m_current_file_string.c_str(),
                  (int)context->m_lexer.m_current_line, msg);

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
    cmm_errprintf("Error:   %s: ",
                  context->m_lexer.m_current_file_string.c_str());
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
    // If treat-warning-as-error flag is on 
    if (context->m_will_treat_warnings_as_errors)
    {
        syntax_error(context, msg);
    }
    // Traditional way 
    else
    {
#ifndef DISABLE_WARNING
        cmm_printf("Warning: %s:%d: %s\n",
                   context->m_lexer.m_current_file_string.c_str(),
                   (int)context->m_lexer.m_current_line, msg);
#endif        

        context->m_num_warnings++;
    }
}

// Compiler's warning function 
void Lang::syntax_warns(Lang* context, const char *format, ...)
{
    // If treat-warning-as-error flag is on 
    if (context->m_will_treat_warnings_as_errors)
    {
        context->m_num_errors++;

        va_list args;
        cmm_errprintf("Error:   %s: ",
                      context->m_lexer.m_current_file_string.c_str());
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
    // Traditional way 
    else
    {
#ifndef DISABLE_WARNING
        va_list args;

        cmm_printf("Warning: %s: ", context->m_lexer.m_current_file_string.c_str());

        va_start(args, format);
        cmm_vprintf(format, args);
        va_end(args);
#endif

        context->m_num_warnings++;
    }
}

// Stop compile 
void Lang::syntax_stop(Lang* context, IntR ret)
{
    // long jump 
    longjmp(context->m_jmp_buf, (int) ret);
}

// Echo when compiling 
void Lang::syntax_echo(Lang* context, const char *msg)
{
    cmm_printf(msg);
}

// Add a component
void Lang::syntax_add_component(const String& component)
{
    m_components.push_back(component);
}

// Get current context state 
SyntaxContextState* Lang::syntax_get_context_state()
{
    return &m_context_state;
}

// Convert string 
String Lang::syntax_add_back_slash_for_quote(const String& str)
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
