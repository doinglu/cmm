// cmm_lex_predefines.cpp:
// Initial version 2011.07.11 by shenyq
// Immigrated 2015.11.3 by doing

#include "cmm_file_path.h"
#include "cmm_util.h"
#include "cmm_lang.h"
#include "cmm_lexer.h"

namespace cmm
{

// Macro expansion function
Lexer::ExpandFuncMap *Lexer::expand_builtin_macro_funcs = 0;
    
// Add all predefines when initialized
int Lexer::init_predefines()
{
    // Create macro funcs map
    expand_builtin_macro_funcs = XNEW(ExpandFuncMap);

    Lexer::add_predefine("__FILE__",      expand_file_name);
    Lexer::add_predefine("__PURE_FILE__", expand_pure_file_name);
    Lexer::add_predefine("__DIR__",       expand_dir_name);
    Lexer::add_predefine("__LINE__",      expand_line_no);
    Lexer::add_predefine("__FUN__",       expand_function_name);
    Lexer::add_predefine("__COUNTER__",   expand_counter);
    return 0;
}

void Lexer::shutdown_predefines()
{
    XDELETE(expand_builtin_macro_funcs);
}

// Register func by macro name
void Lexer::add_predefine(String macro, ExpandFunc func)
{

}

// Get file name
String Lexer::expand_file_name(Lang* context)
{
    return context->m_lexer.m_current_file_string;
}

// Get pure file name
String Lexer::expand_pure_file_name(Lang* context)
{
    return context->m_lexer.m_current_pure_file_string;
}

// Get dir name
String Lexer::expand_dir_name(Lang* context)
{
    return context->m_lexer.m_current_dir_string;
}

// Get current line number
String Lexer::expand_line_no(Lang* context)
{
    char temp[16];
    snprintf(temp, sizeof(temp), "%zu", (size_t)context->m_lexer.m_current_line);
    return temp;
}

// Get current function name
String Lexer::expand_function_name(Lang* context)
{
    const char* UNKNOWN_FUN_NAME = "\"Unknown Function\"";
    auto* prototype = context->m_in_function;
    
    if (prototype)
    {
        return prototype->name;
    } else
    {
        return UNKNOWN_FUN_NAME;
    }
}

// Get global counter
String Lexer::expand_counter(Lang* context)
{
    char temp[16];
    snprintf(temp, sizeof(temp), "%zu", (size_t)context->m_lexer.m_unique_counter++);
    return temp;
}

}
