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
    to->set_join_point();

    if (type == CFG::EDGE_NOT_GOTO && from->get_node_type() == AST_GOTO)
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
            // Create the new block before me
            create_new_block(node->node_no);
        else
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

// Generate DOM
void CFG::generate_dom()
{
    // Empty nodes
    if (m_blocks.size() == 0)
        return;

    // Link Block#0 to Block#1
    m_blocks[0]->branches.push_back(1);
    m_blocks[1]->preds.push_back(0);
    auto b = std_get_current_us_counter();////----

#if 0
    ////----
    // Allocate new block
    for (size_t i = 0; i < 10000; i++)
    {
        int src = (rand() << 16) + rand();
        int dst = (rand() << 16) + rand();
        src = 1 + (src % (m_blocks.size() - 1));
        dst = 1 + (dst % (m_blocks.size() - 1));
        m_blocks[src]->branches.push_back(dst);
        m_blocks[dst]->preds.push_back(src);
    }
    ////----
#endif

#if false
    // Basic algorithm

    // Allocate temporary DOMs
    simple::vector<BasicBlockIds*> doms(m_blocks.size());
    doms.resize(m_blocks.size());

    // DOM[n] <- { 0...N-1 }
    for (auto block : m_blocks)
    {
        doms[block->id] = LANG_NEW(m_lang_context, BasicBlockIds);
        doms[block->id]->push_back(block->id);
    }

    size_t times = 0;
    bool changed;
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
#else
    // Advanced algorithm
    // BLOCK(N).DOM = { N } + BLOCK(IDOM).DOM
    // IDOM represent a linked-list (last one must be block 0).
    // The IDOM list + { N } is DOM of block N.
    // Initial all idom to 0
    for (auto block : m_blocks)
        block->idom = 0;

    size_t times = 0;
    simple::vector<Bitset*> dom_cache(m_blocks.size());
    dom_cache.resize(m_blocks.size());
    bool changed;
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
                block->idom = block->preds[0];
                continue;
            }

            // set = set_intersection preds.DOM
            // Create cache for one set, then search other set in the cache to
            // get intersection list.
            auto set = m_blocks[block->preds[0]]->idom;
            for (BasicBlockId i = 1; set != 0 && i < preds_size; i++)
            {
                auto& other_set = m_blocks[block->preds[i]]->idom;
                if (set != other_set)
                {
                    if (other_set == 0)
                    {
                        // Intersecion with DOM(0) must be { 0 }
                        set = 0;
                        break;
                    }
                    // Load cache
                    auto* bitset = dom_cache[other_set];
                    if (!bitset)
                    {
                        // Create cache
                        bitset = LANG_NEW(m_lang_context, Bitset);
                        bitset->set_size(m_blocks.size());
                        dom_cache[other_set] = bitset;
                        bitset->set(0);
                    }
                    // Mark the ancestors of set
                    // When encounter a marked one, stop since the ancestor should be
                    // already marked
                    for (auto ancestor = other_set; ancestor != 0; ancestor = m_blocks[ancestor]->idom)
                        // Mark this one
                        if (bitset->set(ancestor))
                            // Bit was marked
                            break;

                    // Get intersection list
                    while (!bitset->is_marked(set))
                        set = m_blocks[set]->idom;
                }
            }

            if (block->idom != set)
            {
                block->idom = set;

                // DOM of this block is changed
                changed = true;
            }
            // Finish of this block
        }
        // Do more iterate until nothing changed
    } while (changed);

    // Free dom_cache
    size_t cached = 0;
    for (auto bitset : dom_cache)
    {
        if (!bitset)
            continue;
        LANG_DELETE(m_lang_context, bitset);
        cached++;
    }

    STD_TRACE("Cached %zu bitmap when generating DOM.\n", cached);
#endif

    STD_TRACE("Iterating times = %zu when generating DOM.\n", times);
    auto e = std_get_current_us_counter();////----
    printf("Generating DOM cost: %zuus\n", (size_t)e - b);

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

    getchar();
}

// Print the blocks
void CFG::print_blocks()
{
    size_t id = 0;
    for (auto& it : m_blocks)
    {
        printf("---------- Block %zu (nodes: %zu-%zu) ----------\n",
            id++,
            (size_t)it->begin,
            it->count ? (size_t)(it->begin + it->count - 1) : 0);
        for (auto i = it->begin; i < it->begin + it->count; i++)
        {
            auto* node = m_nodes[i];
            Lang::print_node(node);
        }
        printf("From: ");
        for (auto& id : it->preds)
            printf("%zu ", (size_t)id);
        printf("\n");

        printf("Dest: ");
        for (auto& id : it->branches)
            printf("%zu ", (size_t)id);
        printf("\n");

        printf("DOM : ");
        size_t dom_count = 1; // For block 0
        auto idom = it->id;
        while (idom != 0)
        {
            dom_count++;
            idom = m_blocks[idom]->idom;
        }
        
        if (dom_count > 10)
        {
            // Print first n elements
            printf("[%zu] ", dom_count);
            idom = it->id;
            for (auto i = 0; i < 10; i++)
            {
                printf("%zu ", (size_t)idom);
                idom = m_blocks[idom]->idom;
            }
            printf("...\n");
        } else
        {
            // Print all
            idom = it->id;
            while (idom != 0)
            {
                printf("%zu ", (size_t)idom);
                idom = m_blocks[idom]->idom;
            }
            printf("0\n");
        }
        printf("IDOM: %zu\n", (size_t)it->idom);

        printf("DF  : ");
        for (auto& id : it->df)
            printf("%zu ", (size_t)id);
        printf("\n");
    }
}

// New block is created before specified node_no
// Block = [last free node..node_no)
void CFG::create_new_block(AstNodeNo node_no)
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
        return;

    STD_ASSERT(("Bad branch node to create new block.", node_no > begin_no));

    // Generate new block id
    auto block_id = (BasicBlockId)m_blocks.size();

    // Allocate new block
    auto* block = LANG_NEW(m_lang_context, BasicBlock);
    block->begin = begin_no;
    block->count = node_no - begin_no;
    block->id = block_id;
    block->idom = 0;
    // Set nodes to this block
    for (auto i = begin_no; i < node_no; i++)
        m_nodes[i]->block_id = block_id;
    m_blocks.push_back(block);
}

}