// cmm_lang_component.h
// Initial version by doing Mar/10/2016
// Basic class of lang component

#pragma once

#include "cmm_basic_block.h"

namespace cmm
{

class Lang;

class LangComponent
{
protected:
    typedef BasicBlock::VarInfo VarInfo;
    typedef BasicBlock::VarInfos VarInfos;
    typedef BasicBlock::PhiNode PhiNode;
    typedef BasicBlock::PhiNodes PhiNodes;
    typedef BasicBlock::VersionNo VersionNo;
    typedef BasicBlock::Incoming Incoming;
    typedef BasicBlock::VarInfoLess VarInfoLess;
    typedef BasicBlock::PhiNodeVarInfoLess PhiNodeVarInfoLess;

public:
    LangComponent(Lang* lang_context, const char* name) :
        m_lang_context(lang_context)
    {
    }

protected:
    const char* m_name; // Component name
    Lang* m_lang_context;
};

}
