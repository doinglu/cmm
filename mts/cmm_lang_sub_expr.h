// cmm_lang_sub_expr.h
// Initial version by doing Mar/10/2016
// Optimization: Merge common sub expression

#pragma once

#include "cmm_lang_component.h"

namespace cmm
{

class Lang;
class LangCFG;

class LangSubExpr : LangComponent
{
    friend Lang;
    typedef BasicBlock::VarInfo VarInfo;
    typedef BasicBlock::VarInfos VarInfos;
    typedef BasicBlock::PhiNode PhiNode;
    typedef BasicBlock::PhiNodes PhiNodes;
    typedef BasicBlock::VersionNo VersionNo;
    typedef BasicBlock::Incoming Incoming;
    typedef BasicBlock::VarInfoLess VarInfoLess;
    typedef BasicBlock::PhiNodeVarInfoLess PhiNodeVarInfoLess;

public:
    LangSubExpr(Lang* lang_context);

private:
    LangCFG* m_cfg;
};

}
