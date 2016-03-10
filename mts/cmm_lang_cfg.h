// cmm_lang_cfg.h
// Initial version by doing Mar/1/2016
// Control-Flow-Graph

#pragma once

#include "std_template/simple_pair.h"
#include "std_template/simple_vector.h"
#include "cmm.h"
#include "cmm_basic_block.h"
#include "cmm_lang_component.h"

namespace cmm
{

class Lang;
class LangPhi;

class LangCFG : LangComponent
{
    friend Lang;
    friend LangPhi;

public:
    enum EdgeType
    {
        EDGE_GOTO,      // For AstGotoNode
        EDGE_NOT_GOTO,  // Generated edge by statements
    };

public:
    LangCFG(Lang* lang_context);

    void init(AstFunction* in_functino);

public:
    void add_edge(AstNode* from, AstNode* to, EdgeType type);
    bool add_node(AstNode* node);
    void create_blocks();
    void generate_dom();

public:
    void print_block(BasicBlock* block);
    void print_blocks();

private:
    BasicBlock*     create_new_block(AstNodeNo node_no);
    simple::string  get_var_name(BasicBlock::VarInfo var_info);
    simple::string  get_var_name(VarStorage storage, VariableNo var_no);
    void            mark_unreachable_blocks();
    int             version_no_to_int(BasicBlock::VersionNo version_no);

private:
    // All nodes & blocks
    AstFunction*  m_in_ast_function;
    AstNodes      m_nodes;
    BasicBlocks   m_blocks;
    BasicBlockIds m_idom_ordered_ids;

    // All edges of jmp information
    typedef simple::pair<AstNode*, AstNode*> Edge;
    typedef simple::vector<Edge> Edges;
    Edges m_edges;
};

}