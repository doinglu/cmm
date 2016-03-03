// cmm_cfg.h
// Initial version by doing Mar/1/2016
// Control-Flow-Graph

#pragma once

#include "std_template/simple_pair.h"
#include "std_template/simple_vector.h"

namespace cmm
{

struct AstNode;
class Lang;

// Node no (for build basic blocks)
typedef Uint16 AstNodeNo;

// Basic block
typedef Uint16 BasicBlockId;
typedef simple::vector<BasicBlockId> BasicBlockIds;
typedef simple::vector<AstNode*> AstNodes;
struct BasicBlock
{
    AstNodeNo     begin;    // Begin offset of ast nodes in m_nodes
    AstNodeNo     count;    // nodes in this block
    BasicBlockId  id;       // My ID
    BasicBlockId  idom;     // My immediate dominator, the closest dominator
    BasicBlockIds preds;    // Predecessors blocks of me
    BasicBlockIds branches; // Branch to other blocks
    BasicBlockIds df;       // Dominance Frontiers
};
typedef simple::vector<BasicBlock*> BasicBlocks;

class CFG
{
    friend Lang;

public:
    enum EdgeType
    {
        EDGE_GOTO,      // For AstGotoNode
        EDGE_NOT_GOTO,  // Generated edge by statements
    };

public:
    CFG(Lang* lang_context) :
        m_lang_context(lang_context)
    {
    }

    void clear();

public:
    void add_edge(AstNode* from, AstNode* to, EdgeType type);
    bool add_node(AstNode* node);
    void create_blocks();
    void generate_dom();

public:
    void print_blocks();

private:
    void create_new_block(AstNodeNo node_no);

private:
    Lang* m_lang_context;

    // All nodes & blocks
    AstNodes    m_nodes;
    BasicBlocks m_blocks;

    // All edges of jmp information
    typedef simple::pair<AstNode*, AstNode*> Edge;
    typedef simple::vector<Edge> Edges;
    Edges m_edges;
};

}