// cmm_lang_symbols.cpp
// Initial version Feb/2/2016 by doing
// To organize symbols

#include "cmm.h"
#include "cmm_buffer_new.h"
#include "cmm_lang.h"
#include "cmm_lang_symbols.h"

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
            m_lang_context->m_error_code = PASS1_ERROR;
        } else
        if (info->type & (IDENT_OBJECT_FUN))
        {
            auto* function = (AstFunction*)node;
            auto* prototype = function->prototype;
            m_lang_context->syntax_errors(m_lang_context,
                "%s(%d): error %d: '%s %s()': redefinition\n",
                node->location.file->c_str(), node->location.line,
                C_REDEFINITION,
                ast_var_type_to_string(prototype->ret_var_type).c_str(), prototype->name.c_str());
            m_lang_context->m_error_code = PASS1_ERROR;
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
            BUFFER_DELETE(p);
            p = next;
        }
        if (!p)
            // Remove this entry of hash map
            m_ident_table.erase(it);
        else
            it++;
    }
}

}
