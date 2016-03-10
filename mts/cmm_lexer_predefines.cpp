// cmm_lex_predefines.cpp:
// Initial version 2011.07.11 by shenyq
// Immigrated 2015.11.3 by doing

#include "cmm_file_path.h"
#include "cmm_lang.h"
#include "cmm_lang_lexer.h"

namespace cmm
{

// Macro expansion function
LangLexer::ExpandFuncMap *LangLexer::expand_builtin_macro_funcs = 0;
    
// Add all predefines when initialized
int LangLexer::init_predefines()
{
    // Create macro funcs map
    expand_builtin_macro_funcs = XNEW(ExpandFuncMap);

    LangLexer::add_predefine("__FILE__",      expand_file_name);
    LangLexer::add_predefine("__PURE_FILE__", expand_pure_file_name);
    LangLexer::add_predefine("__DIR__",       expand_dir_name);
    LangLexer::add_predefine("__LINE__",      expand_line_no);
    LangLexer::add_predefine("__FUN__",       expand_function_name);
    LangLexer::add_predefine("__COUNTER__",   expand_counter);
    return 0;
}

void LangLexer::shutdown_predefines()
{
    XDELETE(expand_builtin_macro_funcs);
}

// Register func by macro name
void LangLexer::add_predefine(const simple::string& macro, ExpandFunc func)
{

}

// Get file name
simple::string LangLexer::expand_file_name(Lang* lang_context)
{
    return lang_context->m_lexer->m_current_file_string;
}

// Get pure file name
simple::string LangLexer::expand_pure_file_name(Lang* lang_context)
{
    return lang_context->m_lexer->m_current_pure_file_string;
}

// Get dir name
simple::string LangLexer::expand_dir_name(Lang* lang_context)
{
    return lang_context->m_lexer->m_current_dir_string;
}

// Get current line number
simple::string LangLexer::expand_line_no(Lang* lang_context)
{
    char temp[16];
    snprintf(temp, sizeof(temp), "%zu", (size_t)lang_context->m_lexer->m_current_line);
    return temp;
}

// Get current function name
simple::string LangLexer::expand_function_name(Lang* lang_context)
{
    const char* UNKNOWN_FUN_NAME = "\"Unknown Function\"";
    auto* ast_function = lang_context->m_in_ast_function;
    
    if (ast_function)
    {
        return ast_function->prototype->name;
    } else
    {
        return UNKNOWN_FUN_NAME;
    }
}

// Get global counter
simple::string LangLexer::expand_counter(Lang* lang_context)
{
    char temp[16];
    snprintf(temp, sizeof(temp), "%zu", (size_t)lang_context->m_lexer->m_unique_counter++);
    return temp;
}

}
