// cmm_cfg.h
// Initial version by doing Mar/1/2016
// Control-Flow-Graph

#include "std_template/simple_algorithm.h"
#include "cmm.h"
#include "cmm_ast.h"
#include "cmm_bitset.h"
#include "cmm_cfg.h"
#include "cmm_lang.h"

namespace cmm
{

// Clear data to build new CFG
void CFG::clear()
{
    m_nodes.clear();

    // Free all blocks
    for (auto block : m_blocks)
        LANG_DELETE(m_lang_context, block);
    m_blocks.clear();

    // Create Block#0
    auto* block = LANG_NEW(m_lang_context, BasicBlock);
    block->begin = 0;
    block->count = 0;
    block->id = 0;
    block->idom = 0;
    m_blocks.push_back(block);
}

// Add an edge: node->node
// CFG::EDGE_NOT_GOTO means this edge won't be added if from is an ast goto node.
void CFG::add_edge(AstNode* from, AstNode* to, EdgeType type)
{
    from->set_branch_node();
    auto join_type = (type == EDGE_GOTO) ? AST_JOIN_POINT_GOTO
                                         : AST_JOIN_POINT_STRUCT_ENTRY;
    to->set_join_point(join_type);

    // Get last effect node of statements node
    auto* effect_node = AstNode::get_last_effect_node(from);

    if (type == CFG::EDGE_NOT_GOTO && effect_node->get_node_type() == AST_GOTO)
        // This node will create a force jmp, don't add branch
        // by condition
        return;

    m_edges.push_back(Edge(from, to));
}

// Add new node
bool CFG::add_node(AstNode* node)
{
    node->node_no = (AstNodeNo)m_nodes.size();
    if (m_nodes.size() >= (AstNodeNo)~0)
    {
        // Overflow!
        m_lang_context->syntax_errors(m_lang_context,
            "%s(%d): error %d: function body is too big (over %zu nodes)\n",
            node->location.file->c_str(), node->location.line,
            C_FUNCTION_BODY_TOO_BIG,
            (size_t)(AstNodeNo)~0);
        return false;
    }
    m_nodes.push_back(node);
    return true;
}

// Create blocks by branch/join points
void CFG::create_blocks()
{
    for (auto node : m_nodes)
    {
        if (node->is_join_point())
        {
            // Create the new block before me
            create_new_block(node->node_no);
            if (!node->is_struct_entry())
            {
                // Create path from previous block to this
                // For example:
                // s1;
                // label:
                // s2;
                // goto label;
                // If we break (s1), (label, s2) to two blocks (block#1 & #2).
                // The block#1 should go through the block#2
                // How ever, if the code like these:
                // if (cond)
                // {
                //    label:
                //    s2;
                // }
                // goto label;
                // Because the label was also an entry of struct code. Don't
                // add edge from previous block to the label block.
                if (node->node_no > 0)
                    add_edge(m_nodes[node->node_no - 1], node, EDGE_NOT_GOTO);
            }
        } else
        if (node->is_branch_node())
            // Previous one is a branch node
            // ATTENTION: node->node_no + 1 won't be overflow
            create_new_block(node->node_no + 1);
    }
    create_new_block((AstNodeNo)m_nodes.size());

    // Apply edges to predecessors and branches of each block
    for (auto& it : m_edges)
    {
        auto* from = m_blocks[it.first->block_id];
        auto* to = m_blocks[it.second->block_id];
        from->branches.push_back(it.second->block_id);
        to->preds.push_back(it.first->block_id);
    }
}

// Debug options
#define DEBUG_DOM_ALGORITHM         0
#define DEBUG_ADD_NODES             1
#define DEBUG_ADD_EDGES             1

// Output information when enable DEBUG_DOM_ALGORITHM
#if DEBUG_DOM_ALGORITHM
#undef STD_TRACE
#define STD_TRACE   printf
#endif

// Generate DOM
void CFG::generate_dom()
{
    // Empty nodes
    if (m_blocks.size() == 0)
        return;

    // Link Block#0 to Block#1
    m_blocks[0]->branches.push_back(1);
    m_blocks[1]->preds.push_back(0);

    // Mark all unreachable blocks
    mark_unreachable_blocks();

    size_t times;   // Total iterating times
    bool changed;   // Flag to stop when finished

#if DEBUG_DOM_ALGORITHM && DEBUG_ADD_NODES
    // Add debug data

    // Add extra nodes that not reached block#0
    // (a)<->(c) (d)<->(e)
    //   \   /     \   /
    //    v v       v v
    //    (b)       (f)
    //      \       /
    //       \     /
    //        \   /
    //         v v
    //         (g)
    // Allocate new block
#define _ADD_BLOCK(x) \
    m_nodes.push_back(LANG_NEW(m_lang_context, AstNop, m_lang_context)); \
    auto* x = create_new_block((AstNodeNo)m_nodes.size());

#define _ADD_EDGE(x, y) \
    x->branches.push_back(y->id); y->preds.push_back(x->id);

    _ADD_BLOCK(a);
    _ADD_BLOCK(b);
    _ADD_BLOCK(c);
    _ADD_BLOCK(d);
    _ADD_BLOCK(e);
    _ADD_BLOCK(f);
    _ADD_BLOCK(g);
    _ADD_EDGE(a, b);
    _ADD_EDGE(a, c);
    _ADD_EDGE(c, a);
    _ADD_EDGE(c, b);
    _ADD_EDGE(d, e);
    _ADD_EDGE(d, f);
    _ADD_EDGE(e, d);
    _ADD_EDGE(e, f);
    _ADD_EDGE(b, g);
    _ADD_EDGE(f, g);
#endif // End of add debug data

#if DEBUG_DOM_ALGORITHM && DEBUG_ADD_EDGES
    // Create random edges
    for (size_t i = 0; i < 10000; i++)
    {
        int src = (rand() << 16) + rand();
        int dst = (rand() << 16) + rand();
        src = 1 + (src % (m_blocks.size() - 1));
        dst = 1 + (dst % (m_blocks.size() - 1));
        m_blocks[src]->branches.push_back(dst);
        m_blocks[dst]->preds.push_back(src);
    }
#endif // End of create random edges

#if DEBUG_DOM_ALGORITHM
    // Basic algorithm
    auto b1 = std_get_current_us_counter();

    // Allocate temporary DOMs
    simple::vector<BasicBlockIds*> doms(m_blocks.size());
    doms.resize(m_blocks.size());

    // DOM[n] <- { 0...N-1 }
    for (auto block : m_blocks)
    {
        doms[block->id] = LANG_NEW(m_lang_context, BasicBlockIds);
        doms[block->id]->push_back(block->id);
    }

    times = 0;
    do
    {
        times++;
        changed = false;
        for (auto block : m_blocks)
        {
            auto preds_size = (BasicBlockId)block->preds.size();
            if (preds_size < 1)
                // Unreached - Dead block
                continue;

            // set = set_intersection preds.DOM
            BasicBlockIds set = *doms[block->preds[0]];
            for (BasicBlockId i = 1; i < preds_size; i++)
            {
                BasicBlockIds& other_set = *doms[block->preds[i]];
                auto set_size = simple::set_intersection(set.begin(), set.end(),
                                    other_set.begin(), other_set.end(), set.begin() /* out */)
                                - set.begin();
                set.resize(set_size);
            }
            
            // Insert with { N } & keep ordered
            auto it = simple::lower_bound(set.begin(), set.end(), block->id);
            if (it == set.end())
                set.push_back(block->id);
            else
            if (*it != block->id)
                set.insert(it, block->id);

            if (simple::equal(set.begin(), set.end(), doms[block->id]->begin(), doms[block->id]->end()))
                // Not changed
                continue;

            // Update dom
            *doms[block->id] = simple::move(set);
            changed = true;
        }
    } while (changed);

    // Find out IDOM
    for (auto block : m_blocks)
    {
        BasicBlockId idom = 0;
        auto& dom = *doms[block->id];
        size_t dom_size = dom.size();
        if (dom_size == 1)
            // Only 1 node
            idom = 0;
        else
        {
            // The DOM[IDOM] should have n-1 elements of mine
            // (As predecessor of me in the DOM tree)
            for (auto id : dom)
            {
                if (doms[id]->size() == dom_size - 1)
                {
                    idom = id;
                    break;
                }
            }
        }

        block->idom = idom;
    }

    // Free temporary DOMs
    for (auto dom : doms)
        LANG_DELETE(m_lang_context, dom);

    auto e1 = std_get_current_us_counter();
                                     
    // Save all ids
    simple::vector<BasicBlockId> r1(m_blocks.size());
    for (auto block : m_blocks)
        r1.push_back(block->idom);
    STD_TRACE("Raw DOM algorihtm: %zuus (times = %zu).\n", (size_t)(e1 - b1), times);
#endif

#if DEBUG_DOM_ALGORITHM
    // Advanced algorithm
    auto b2 = std_get_current_us_counter();
#endif
                                     
    // BLOCK(N).DOM = { N } + BLOCK(IDOM).DOM
    // IDOM represent a linked-list (last one must be block 0).
    // The IDOM list + { N } is DOM of block N.
    // Initial all idom to 0
    for (auto block : m_blocks)
        block->idom = 0;

    // Buffer to get set intersection
    simple::vector<BasicBlockId> ordered_id(m_blocks.size()); // IDs sorted by level of IDOM tree
    simple::vector<BasicBlockId> id_index(m_blocks.size());   // ID index in IDOM tree
    for (BasicBlockId i = 0; i < (BasicBlockId)m_blocks.size(); i++)
    {
        id_index.push_back(i);
        ordered_id.push_back(i);
    }

    times = 0;
    do
    {
        times++;
        changed = false;
        for (auto block : m_blocks)
        {
            auto preds_size = (BasicBlockId)block->preds.size();
            if (preds_size < 1)
                // Unreached - Dead block
                continue;

            if (preds_size == 1)
            {
                // From only 1 block, this is my IDOM
                if (block->idom == block->preds[0])
                    continue;
                block->idom = block->preds[0];
                changed = true;
                continue;
            }

            // set = set_intersection preds.DOM
            // Create cache for one set, then search other set in the cache to
            // get intersection list.
            auto set = block->preds[0];
            for (BasicBlockId i = 1; set != 0 && i < preds_size; i++)
            {
                auto other_set = block->preds[i];
                while (set != other_set)
                {
                    if (id_index[set] > id_index[other_set])
                    {
                        auto new_set = m_blocks[set]->idom;
                        if (id_index[new_set] > id_index[set])
                        {
                            // Reserve? It must be a loop
                            set = 0;
                            break;
                        }
                        set = new_set;
                    } else
                    {
                        auto new_set = m_blocks[other_set]->idom;
                        if (id_index[new_set] > id_index[other_set])
                        {
                            // Reserve? It must be a loop
                            set = 0;
                            break;
                        }
                        other_set = new_set;
                    }
                }
            }

            if (block->idom != set)
            {
                block->idom = set;
                auto block_idom_index = id_index[block->idom];
                auto this_block_index = id_index[block->id];
                if (block_idom_index > this_block_index)
                {
                    // Move block->idom in front of this block
                    for (auto i = block_idom_index; i > this_block_index; i--)
                    {
                        ordered_id[i] = ordered_id[i - 1];
                        id_index[i]++;
                    }
                    ordered_id[this_block_index] = block->idom;
                    id_index[block->idom] = this_block_index;
                }

                // DOM of this block is changed
                changed = true;
            }
            // Finish of this block
        }
        // Do more iterate until nothing changed
    } while (changed);

    // Erase loop IDOM
    // If we got CFG like this:
    // 0->1
    // 2<->3
    // That means no edge to isolated (2, 3). The algorithm
    // will generate 2.IDOM == 3, 3.IDOM == 2. Check & erase
    // the 2/3.IDOM to 0
    for (auto block : m_blocks)
    {
        auto idom = block->idom;
        if (id_index[block->idom] > id_index[block->id])
        {
            // Resever order? That means there is a loop and can't reach
            // block#0, erase idom in the loop-ring
            auto* p = block;
            while (p->idom)
            {
                auto* next = m_blocks[p->idom];
                p->idom = 0;
                p = next;
            }
        }
    }

#if DEBUG_DOM_ALGORITHM
    auto e2 = std_get_current_us_counter();

    // Save all ids
    simple::vector<BasicBlockId> r2(m_blocks.size());
    for (auto block : m_blocks)
        r2.push_back(block->idom);
    STD_TRACE("New DOM algorihtm: %zuus (times = %zu).\n", (size_t)(e2 - b2), times);

    size_t not_matched = 0;
    for (size_t i = 0; i < m_blocks.size(); i++)
        if (r1[i] != r2[i])
        {
            not_matched++;
            // Use r1 as IDOM
            for (size_t k = 0; k < m_blocks.size(); k++)
                m_blocks[k]->idom = r1[k];
            printf("****** RAW ******\n");
            print_block(m_blocks[i]);
            // Use r2 as IDOM
            for (size_t k = 0; k < m_blocks.size(); k++)
                m_blocks[k]->idom = r2[k];
            printf("****** NEW ******\n");
            print_block(m_blocks[i]);
        }
    if (not_matched)
        printf("Not matched: %zu blocks.\n", not_matched);
    else
        printf("Matched!.\n");
#endif

    // Workout DF
    for (auto block : m_blocks)
    {
        auto preds_size = (BasicBlockId)block->preds.size();
        if (preds_size < 2)
            // No multiple predecessors
            continue;

        for (auto pred : block->preds)
        {
            auto idom = pred;
            while (idom && idom != block->idom)
            {
                // idom is not Block#0
                // idom is not strictly dominate this block
                m_blocks[idom]->df.push_back(block->id);

                // Travel in dominator tree
                idom = m_blocks[idom]->idom;
            }
        }
    }
}

// Print a single block
void CFG::print_block(BasicBlock* block)
{
    printf("---------- Block %zu (nodes: %zu-%zu) ----------\n",
           (size_t)block->id,
           (size_t)block->begin,
           block->count ? (size_t)(block->begin + block->count - 1) : 0);
    for (auto i = block->begin; i < block->begin + block->count; i++)
    {
        auto* node = m_nodes[i];
        Lang::print_node(node);
    }
    printf("From: ");
    for (auto& id : block->preds)
        printf("%zu ", (size_t)id);
    printf("\n");

    printf("Dest: ");
    for (auto& id : block->branches)
        printf("%zu ", (size_t)id);
    printf("\n");

#if 0
    printf("DOM : ");
    size_t dom_count = 1; // For block 0
    auto idom = block->id;
    while (idom != 0)
    {
        dom_count++;
        idom = m_blocks[idom]->idom;
    }
        
    if (dom_count > 10)
    {
        // Print first n elements
        printf("[%zu] ", dom_count);
        idom = block->id;
        for (auto i = 0; i < 10; i++)
        {
            printf("%zu ", (size_t)idom);
            idom = m_blocks[idom]->idom;
        }
        printf("...\n");
    } else
    {
        // Print all
        idom = block->id;
        while (idom != 0)
        {
            printf("%zu ", (size_t)idom);
            idom = m_blocks[idom]->idom;
        }
        printf("0\n");
    }
#endif
    printf("IDOM: %zu\n", (size_t)block->idom);

    printf("DF  : ");
    for (auto& id : block->df)
        printf("%zu ", (size_t)id);
    printf("\n");
}

// Print the blocks
void CFG::print_blocks()
{
    size_t id = 0;
    for (auto& it : m_blocks)
        print_block(it);
}

// New block is created before specified node_no
// Block = [last free node..node_no)
BasicBlock* CFG::create_new_block(AstNodeNo node_no)
{
    AstNodeNo begin_no = 0;
    if (m_blocks.size() > 0)
    {
        // There is existed block, get last block for begin_no
        auto* last_block = m_blocks[m_blocks.size() - 1];
        begin_no = last_block->begin + last_block->count;
    }

    if (node_no == begin_no)
        // Don't create empty block
        return 0;

    STD_ASSERT(("Bad branch node to create new block.", node_no > begin_no));

    // Generate new block id
    auto block_id = (BasicBlockId)m_blocks.size();

    // Allocate new block
    auto* block = LANG_NEW(m_lang_context, BasicBlock);
    block->id = block_id;
    block->attrib = (BasicBlockAttrib)0;
    block->begin = begin_no;
    block->count = node_no - begin_no;
    block->idom = 0;
    // Set nodes to this block
    for (auto i = begin_no; i < node_no; i++)
        m_nodes[i]->block_id = block_id;
    m_blocks.push_back(block);
    return block;
}

// Scan & mark all unreachable blocks attrib
void CFG::mark_unreachable_blocks()
{
    simple::vector<BasicBlockId> scan_blocks;

    // Mark all blocks to UNREACHABLE first
    for (auto block : m_blocks)
        block->attrib |= BASIC_BLOCK_NOT_REACH_0;

    // Put block#0 (start block) to scan list
    scan_blocks.push_back(0);
    m_blocks[0]->attrib &= ~BASIC_BLOCK_NOT_REACH_0;
    while (scan_blocks.size())
    {
        // Pick last one from scan list
        auto it = scan_blocks.end() - 1;
        auto block_id = *it;
        scan_blocks.remove(it);

        auto* block = m_blocks[block_id];
        for (;;)
        {
            // Mark branches of this block
            auto branches_size = block->branches.size();
            if (!branches_size)
                // No more branch blocks from this block
                break;

            if (branches_size == 1)
            {
                // Only 1 branch, go through
                auto branch_id = block->branches[0];
                auto* branch_block = m_blocks[branch_id];
                if (!(branch_block->attrib & BASIC_BLOCK_NOT_REACH_0))
                    // Already marked
                    break;

                // Clear flag & continue go through the branch block
                branch_block->attrib &= ~BASIC_BLOCK_NOT_REACH_0;
                block = branch_block;
                continue;
            }

            // Put the branches to scan_blocks
            for (auto branch_id : block->branches)
            {
                auto* branch_block = m_blocks[branch_id];
                if (!(branch_block->attrib & BASIC_BLOCK_NOT_REACH_0))
                    // Already marked
                    continue;

                // Clear flag & add this branch block to scan list
                branch_block->attrib &= ~BASIC_BLOCK_NOT_REACH_0;
                scan_blocks.push_back(branch_id);
            }
            // Do next scan
            break;
        }
    }

#ifdef _DEBUG
    size_t not_reached = 0;
    for (auto block : m_blocks)
        if (block->is_not_reached_0())
            not_reached++;
    STD_TRACE("There is %zu/%zu blocks not reached block#0.\n",
              not_reached, m_blocks.size());
#endif
}

}