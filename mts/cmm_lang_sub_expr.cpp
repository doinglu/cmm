// cmm_lang_sub_expr.cpp
// Initial version by doing Mar/10/2016
// Optimization: Merge common sub expression

#include "cmm_lang.h"
#include "cmm_lang_cfg.h"
#include "cmm_lang_sub_expr.h"

namespace cmm
{

LangSubExpr::LangSubExpr(Lang* lang_context) :
    LangComponent(lang_context, "Subexpr")
{
    m_cfg = m_lang_context->m_cfg;
}

}
