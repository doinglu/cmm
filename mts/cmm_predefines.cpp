// cmm_predefines.cpp:
// Initial version 2011.07.11 by shenyq
// Immigrated 2015.11.3 by doing

#include "cmm_file_path.h"
#include "cmm_util.h"
#include "cmm_lang.h"
#include "cmm_lexer.h"
#include "cmm_predefines.h"

namespace cmm
{

/* Predefine functions */
String get_file_name(LangContext* context);
String get_pure_file_name(LangContext* context);
String get_dir_name(LangContext* context);
String get_line_no(LangContext* context);
String get_function_name(LangContext* context);
String get_counter(LangContext* context);

/* Add all predefines when initialized */
void add_predefines()
{
    LangContext::add_predefine("__FILE__",      get_file_name);
    LangContext::add_predefine("__PURE_FILE__", get_pure_file_name);
    LangContext::add_predefine("__DIR__",       get_dir_name);
    LangContext::add_predefine("__LINE__",      get_line_no);
    LangContext::add_predefine("__FUN__",       get_function_name);
    LangContext::add_predefine("__COUNTER__",   get_counter);
}

/* Get file name */
String get_file_name(LangContext* context)
{
    return context->syntax_get_current_file();
}

/* Get pure file name */
String get_pure_file_name(LangContext* context)
{
    return context->syntax_get_current_pure_file();
}

/* Get dir name */
String get_dir_name(LangContext* context)
{
    return context->syntax_get_current_dir();
}

/* Get current line number */
String get_line_no(LangContext* context)
{
    char temp[16];
    snprintf(temp, sizeof(temp), "%u", (Uint32)context->syntax_get_current_line());
    return temp;
}

/* Get current function name */
String get_function_name(LangContext* context)
{
    const char *UNKNOWN_FUN_NAME = "\"Unknown Function\"";
    
    if (context->is_in_function())
    {
        return context->syntax_get_current_function()->name;
    } else
    {
        return UNKNOWN_FUN_NAME;
    }
}

/* Get global counter */
String get_counter(LangContext* context)
{
    char temp[16];
    snprintf(temp, sizeof(temp), "%u", (Uint32)context->syntax_get_inc_counter());
    return temp;
}

}
