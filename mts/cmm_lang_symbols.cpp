// cmm_lang_symbols.cpp
// Initial version Feb/2/2016 by doing
// To organize symbols

#include "cmm.h"
#include "cmm_buffer_new.h"
#include "cmm_efun.h"
#include "cmm_lang.h"
#include "cmm_lang_symbols.h"
#include "cmm_thread.h"

namespace cmm
{

IdentInfo::IdentInfo(Lang* context) :
    tag(context->get_frame_tag())
{
}

// Add a new ident unit to ident table
// ATTENTION:
// tag of the ident unit must be larger or equal to previous existed one
bool LangSymbols::add_ident_info(const simple::string& name, IdentInfo* info, AstNode* node)
{
    IdentInfo* next = 0;

    // Put this ident unit to the head of list
    m_ident_table.try_get(name, &next);
    STD_ASSERT(("Tag of ident unit is smaller than previous one.",
                !next || info->tag >= next->tag));
    if (next && info->tag <= next->tag)
    {
        // Redefinition
        if (info->type & (IDENT_LOCAL_VAR | IDENT_OBJECT_VAR))
        {
            auto* decl = (AstDeclaration*)node;
            m_lang_context->syntax_errors(m_lang_context,
                "%s(%d): error %d: '%s %s': redefinition\n",
                node->location.file->c_str(), node->location.line,
                C_REDEFINITION,
                ast_var_type_to_string(decl->var_type).c_str(), decl->name.c_str());
            m_lang_context->m_stop_error_code = PASS1_ERROR;
        } else
        if (info->type & (IDENT_OBJECT_FUN))
        {
            auto* ast_function = (AstFunction*)node;
            auto* prototype = ast_function->prototype;
            m_lang_context->syntax_errors(m_lang_context,
                "%s(%d): error %d: '%s %s()': redefinition\n",
                node->location.file->c_str(), node->location.line,
                C_REDEFINITION,
                ast_var_type_to_string(prototype->ret_var_type).c_str(), prototype->name.c_str());
            m_lang_context->m_stop_error_code = PASS1_ERROR;
        } else
            STD_ASSERT(("Unknown ident type.", 0));
        return false;
    }
    info->next = next;
    m_ident_table.put(name, info);
    return true;
}

// Get a ident unit that matched expected type
IdentInfo* LangSymbols::get_ident_info(const simple::string& name, Uint32 expect_types)
{
    IdentInfo* p;

    if (!m_ident_table.try_get(name, &p))
        // Not found the symbol
        return 0;

    while (p)
    {
        if (p->type & expect_types)
            // Got expected ident unit
            return p;

        p = p->next;
    }

    // Not found
    return 0;
}

// Remove all ident information tag 
void LangSymbols::remove_ident_info_by_tag(int tag)
{
    auto it = m_ident_table.begin();
    while (it != m_ident_table.end())
    {
        IdentInfo* p;
        p = it->second;
        while (p && p->tag >= tag)
        {
            // Remove & free the ident unit
            auto* next = p->next; 
            LANG_DELETE(m_lang_context, p);
            p = next;
        }
        if (!p)
            // Remove this entry of hash map
            m_ident_table.erase(it);
        else
            it++;
    }
}

// Add a label
void LangSymbols::add_label_info(AstLabel* label)
{
    if (m_label_table.contains_key(label->name))
    {
        m_lang_context->syntax_errors(m_lang_context,
            "%s(%d): error %d: '%s': label redefined\n",
            label->location.file->c_str(), label->location.line,
            C_LABEL_REDEFINED,
            label->name.c_str());
        m_lang_context->m_stop_error_code = PASS1_ERROR;
    }
    m_label_table.put(label->name, label);
}

// Get a label
AstLabel* LangSymbols::get_label_info(const simple::string& label_name)
{
    AstLabel* label;
    if (!m_label_table.try_get(label_name, &label))
        // No such label
        return 0;

    return label;
}

// Get function
Function* LangSymbols::get_function(const simple::string& name, AstNode* node)
{
    // Try lookup symbole table first
    auto* info = get_ident_info(name, IDENT_ALL);
    if (!info)
    {
        // Not found function in symbol table, try efun
        ReserveStack r(1);
        String& temp = (String&)r[0];

        auto* function = Efun::get_efun(temp = name);
        if (!function)
        {
            m_lang_context->syntax_errors(m_lang_context,
                "%s(%d): error %d: '%s': identifier not found\n",
                node->location.file->c_str(), node->location.line,
                C_IDENTIFIER_NOT_FOUND,
                name.c_str());
            return 0;
        }
        return function;
    }

    if (!(info->type & IDENT_FUN))
    {
        // Not found function
        m_lang_context->syntax_errors(m_lang_context,
            "%s(%d): error %d: '%s' does not evaluate to a function\n",
            node->location.file->c_str(), node->location.line,
            C_NOT_FUNCTION,
            name.c_str());
        return 0;
    }

    STD_ASSERT(("Expected object function only.", info->type & IDENT_OBJECT_FUN));

    return m_lang_context->m_program_functions[info->function_no];
}

}
