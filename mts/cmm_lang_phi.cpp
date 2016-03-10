// cmm_lang_phi.cpp
// Initial version by doing Mar/4/2016
// Figure out input/output & add phi nodes

#include "std_template/simple_algorithm.h"
#include "cmm.h"
#include "cmm_ast.h"
#include "cmm_lang.h"
#include "cmm_lang_phi.h"
#include "cmm_util.h"

namespace cmm
{

// Create Phi info
void LangPhi::create_phi_info()
{
#ifdef _DEBUG
    // Assure the VarInfoLess() & VarInfoEuqalTo() would be worked properly
    STD_ASSERT(("Bad layout of struct BasicBlock::VarInfo, please modify VarInfoLess/VarInfoEqualTo.",
               offsetof(BasicBlock::VarInfo, unused) == offsetof(BasicBlock::VarInfo, cmp_val) + 3));
#endif

    // Generate input/ouput var for each block
    generate_block_var_info();

    // Create phi node for each block
    // Incoming var name only, no source, no version
    generate_block_var_name_of_phi();

    // Workout all output versions
    generate_block_output_var_version();

    // Workout all phi incoming after output versions are existed
    generate_block_phi_incoming();

    // Lookup var in check list to find out those partial initialized phi node
    generate_block_uninitilized_var_info();

    // Workout all input versions at last
    generate_block_input_var_version();
}

// Generate variable definition & reference information of each block
void LangPhi::generate_block_var_info()
{
    for (auto block : m_cfg->m_blocks)
    {
        if (block->is_not_reachable())
            // Skip
            continue;

        for (AstNodeNo index = 0; index < block->count; index++)
        {
            // Get node
            auto* node = m_cfg->m_nodes[block->begin + index];
            if (node->get_node_type() != AST_EXPR_VARIABLE)
                // Skip non AST_EXPR_VARIABLE
                continue;

            auto* expr = (AstExprVariable*)node;
            VarInfo var_info(expr->var_storage, expr->var_no);

            if (expr->is_right_value())
                // Load value
                add_to_input(block, var_info);

            if (expr->is_left_value())
                // Assigned to value
                add_to_output(block, var_info);
        }
        // Done this block
    }
}

// Generate variable name of phi node for each block
void LangPhi::generate_block_var_name_of_phi()
{
    for (auto block : m_cfg->m_blocks)
    {
        // Propagate all outputs to every dominator frontier
        for (auto& output : block->outputs)
            for (auto df_id : block->df)
            {
                auto* df_block = m_cfg->m_blocks[df_id];
                PhiNode phi_node(VarInfo(output.storage, output.var_no));
                if (!found_in_phi_nodes(df_block->phi_nodes, phi_node.var_info))
                    insert_sorted(df_block->phi_nodes, phi_node, PhiNodeVarInfoLess());
            }
    }
}

// Lookup all outputs & create versions
void LangPhi::generate_block_output_var_version()
{
    // Update version no of phi nodes & outputs
    VarInfos current_versions;
    for (auto block : m_cfg->m_blocks)
    {
        // Update version no of phi node
        for (auto& phi_node : block->phi_nodes)
        {
            // Update phi node output version no
            reserve_output_var_version(&phi_node.var_info.version_no, 1, phi_node.var_info, current_versions);
        }

        // Loop backward to get echo node output version no
        for (size_t i = 0; i < block->count; i++)
        {
            auto* node = m_cfg->m_nodes[block->begin + i];
            if (node->get_node_type() != AST_EXPR_VARIABLE)
                // Skip non AST_EXPR_VARIABLE
                continue;

            auto* expr = (AstExprVariable*)node;
            VarInfo var_info(expr->var_storage, expr->var_no);

            if (expr->is_left_value())
                // Assigned to value
                // Reserve 1 version no for this assignment
                reserve_output_var_version(&expr->var_output_version_no, 1, var_info, current_versions);
        }

        // Update version no of outputs
        for (auto& var_info : block->outputs)
            // Get output version no
            reserve_output_var_version(&var_info.version_no, 0, var_info, current_versions);
    }
}

// Lookup var in check list to find out those partial initialized phi node
// For example:
/*
CFG:
            0
            |
            v
            1-->3 (x=3)
           / \ /
          v   v
   (x=2) 2     5
          \   /
           \ /
            v
            4 (print x)
Flow:
    0->1
    1->2, 3, 5
    2->4
    3->5
    5->4
    
    B2, B3: initialize local var: "x"
    B4    : refer to var "x"

  * There is a path from 0->1->5->4 that cause x uninitialized.

    This routine will detect all pathes from 0 & mark block phi node
    with UNINITIALIZED flag. So when we set var version of inputs, we
    can figure out the var may be uninitialized.

    HOW DOES THIS ROUTINE WORK?
    For each var in check list, the routine does scan from block#0 (u-set).
    Mark all phi nodes of blocks that reached from u-set directly to
    PARTIAL_INITIALIZED. Then put the blocks that doesn't output specified
    var in u-set & go on.
*/            
void LangPhi::generate_block_uninitilized_var_info()
{
    size_t blocks_count = m_cfg->m_blocks.size();
    simple::vector<BasicBlockId> uset(blocks_count);
    simple::vector<bool> processed(blocks_count);
    processed.push_backs(false, blocks_count);

    for (auto& var_info : m_check_list)
    {
        // Clear flags & set
        memset(&*processed.begin(), 0, sizeof(bool) * blocks_count);
        uset.clear();

        // Put block#0 into set
        uset.push_back(0);
        processed[0] = true;
        while (uset.size())
        {
            // Peek one block from u-set
            auto id_it = uset.end() - 1;
            auto id = *id_it;
            uset.remove(id_it);
            auto* block = m_cfg->m_blocks[id];
            for (auto branch_id : block->branches)
            {
                auto* branch_block = m_cfg->m_blocks[branch_id];

                auto& phi_nodes = branch_block->phi_nodes;
                auto phi_it = simple::lower_bound(phi_nodes.begin(), phi_nodes.end(), var_info,
                                                  PhiNodeVarInfoLess());
                if (phi_it != phi_nodes.end() &&
                    !PhiNodeVarInfoLess()(var_info, *phi_it))
                {
                    // This block has phi node for this var, mark it be
                    // partial initialized
                    phi_it->var_info.set_partial_initialized();
                }

                if (!found_in_var_infos(branch_block->outputs, var_info))
                {
                    // Put this block to u-set since it won't output the
                    // specified variable
                    if (!processed[branch_id])
                    {
                        // Put id into u-set
                        processed[branch_id] = true;
                        uset.push_back(branch_id);
                    }
                }

                // Check next branch
            }
            // Finished all branchs of this block

            // Peek next in u-set
        }
        // Finished work of this var

        // Peek next var in check list
    }
}

// Lookup all phi nodes, work out the incoming
// ATTENTION:
// Because var of phi node may be an uninitialized or partially initialized
// version, I should work out these before generating input version 
void LangPhi::generate_block_phi_incoming()
{
    // Ordered BY IDOM tree, because incoming informaton depends on previous
    // IDOM node
    for (auto id : m_cfg->m_idom_ordered_ids)
    {
        auto* block = m_cfg->m_blocks[id];

        // Work out incoming information for block
        for (auto& phi_node : block->phi_nodes)
        {
            // Get first node that refer to the variable
            // MAYBE null if no node refer to the variable
            for (auto pred_id : block->preds)
            {
                auto* def_block = find_phi_incoming_block(block, pred_id, phi_node.var_info);
                if (!def_block)
                    // Not found in this branch
                    continue;
                auto incoming = get_input_var_incoming(phi_node.var_info, def_block, 0);
                phi_node.incoming.push_back(incoming);
                if (BasicBlock::maybe_uninitialized(incoming.version_no))
                    add_to_check_list(phi_node.var_info);
            }
            if (phi_node.incoming.size() < block->preds.size())
            {
                // At least 1 predecessor's branch has not output
                // Find output in idom tree
                auto* last_idom_block = m_cfg->m_blocks[block->idom];
                auto incoming = get_input_var_incoming(phi_node.var_info, last_idom_block, 0);
                phi_node.incoming.push_back(incoming);
                if (BasicBlock::maybe_uninitialized(incoming.version_no))
                    // At least 1 branch not fullly initialized the var
                    add_to_check_list(phi_node.var_info);
            }
        }
    }
}

// Lookup all phi nodes, work out the incoming
void LangPhi::generate_block_input_var_version()
{
    // Update version no  of input
    for (auto block : m_cfg->m_blocks)
    {
        // ATTENTION:
        // RESET outputs of current block to generate version of all input nodes
        block->outputs.clear();
        // Initialize output with phi nodes
        for (auto& phi_node : block->phi_nodes)
        {
            // Put phi_node var_info to outputs
            add_to_output(block, phi_node.var_info);
        }

        // Loop backward to get echo node output version no
        for (size_t i = 0; i < block->count; i++)
        {
            auto* node = m_cfg->m_nodes[block->begin + i];
            if (node->get_node_type() != AST_EXPR_VARIABLE)
                // Skip non AST_EXPR_VARIABLE
                continue;

            auto* expr = (AstExprVariable*)node;
            VarInfo var_info(expr->var_storage, expr->var_no);

            if (expr->is_right_value())
            {
                // Load value
                // Get version no from inputs of this block
                auto incoming = get_input_var_incoming(var_info, block, node);
                if (BasicBlock::is_uninitialized(incoming.version_no))
                {
                    // Raise error
                    m_lang_context->syntax_errors(m_lang_context,
                        "%s(%d): error %d: uninitialized local variable '%s' used\n",
                        node->location.file->c_str(), node->location.line,
                        C_UNINITIALIZED_LOCAL_VAR,
                        m_cfg->get_var_name(var_info).c_str());
                    // Clear unitialized flag (set version_no to 0)
                    update_input_var_version(var_info, block->outputs);
                }
                if (BasicBlock::is_partial_initialized(incoming.version_no))
                {
                    // Raise error
                    m_lang_context->syntax_errors(m_lang_context,
                        "%s(%d): error %d: possible uninitialized local variable '%s' used\n",
                        node->location.file->c_str(), node->location.line,
                        C_UNINITIALIZED_LOCAL_VAR,
                        m_cfg->get_var_name(var_info).c_str());
                    // Clear unitialized flag (set version_no to 0)
                    update_input_var_version(var_info, block->outputs);
                }
            }

            if (expr->is_left_value())
            {
                // Assigned to value
                // Update inputs of this block
                var_info.version_no = expr->var_output_version_no;
                update_input_var_version(var_info, block->outputs);
            }
        }
    }
}

// Should check 
void LangPhi::add_to_check_list(const VarInfo& var_info)
{
    STD_ASSERT(("Only local var could be uninitialized.",
                var_info.storage == VAR_LOCAL_VAR));
    auto it = simple::lower_bound(m_check_list.begin(), m_check_list.end(),
                                  var_info, VarInfoLess());
    if (it == m_check_list.end() || VarInfoLess()(var_info, *it))
    {
        // Not found, add new var info
        m_check_list.insert(it, var_info);
    }
}

// Add new var_info into input
void LangPhi::add_to_input(BasicBlock* block, const VarInfo& var_info)
{
    auto it = simple::lower_bound(block->inputs.begin(), block->inputs.end(),
                                  var_info, VarInfoLess());
    if (it == block->inputs.end() || VarInfoLess()(var_info, *it))
    {
        // Not found, add new var info
        block->inputs.insert(it, var_info);
    }
}

// Add new var_info into output
void LangPhi::add_to_output(BasicBlock* block, const VarInfo& var_info)
{
    auto it = simple::lower_bound(block->outputs.begin(), block->outputs.end(),
        var_info, VarInfoLess());
    if (it == block->outputs.end() || VarInfoLess()(var_info, *it))
    {
        // Not found, add new var info
        block->outputs.insert(it, var_info);
    }
}

// Find block that defined var (as output) that used in block
BasicBlock* LangPhi::find_phi_incoming_block(BasicBlock* block, BasicBlockId pred_id, const VarInfo& var_info)
{
    BasicBlocks from;
    auto idom = block->idom;

    while (pred_id != idom)
    {
        auto* pred_block = m_cfg->m_blocks[pred_id];
        if (found_in_var_infos(pred_block->outputs, var_info))
            // Got!
            return pred_block;

        if (found_in_phi_nodes(pred_block->phi_nodes, var_info))
            // Got!
            return pred_block;

        // Back trace the IDOM
        pred_id = pred_block->idom;
    }

    // Not found in this predecessor branch
    return 0;
}

// Is the var_info in the array? (version is ignored)
bool LangPhi::found_in_phi_nodes(PhiNodes& phi_nodes, const VarInfo& var_info)
{
    return simple::binary_search(phi_nodes.begin(), phi_nodes.end(), var_info,
                                 PhiNodeVarInfoLess());
}

// Is the var_info in the array? (version is ignored)
bool LangPhi::found_in_var_infos(VarInfos& var_infos, const VarInfo& var_info)
{
    return simple::binary_search(var_infos.begin(), var_infos.end(), var_info,
                                 VarInfoLess());
}

// Get first node that reference the specified variable
AstNode* LangPhi::get_first_reference_node(BasicBlock* block, const VarInfo& var_info)
{
    if (var_info.storage == VAR_ARGUMENT)
        // This is an argument, set the first reference node to #0
        return m_cfg->m_nodes[0];

    for (AstNodeNo index = 0; index < block->count; index++)
    {
        // Get node
        auto* node = m_cfg->m_nodes[block->begin + index];
        if (node->get_node_type() != AST_EXPR_VARIABLE)
            // Skip non AST_EXPR_VARIABLE
            continue;

        auto* expr = (AstExprVariable*)node;
        if (expr->var_storage != var_info.storage ||
            expr->var_no != var_info.var_no)
            // No refer to this variable
            continue;

        if (expr->is_right_value())
            return node;
    }

    return 0;
}

// Get version no from versions
// Set the version no
// Lookup upward in IDOM tree, if not found
//   - create new var info in outputs of block#0
//   - do set_maybe_unitialized() for local var
LangPhi::Incoming LangPhi::get_input_var_incoming(VarInfo& var_info, BasicBlock* start_block, AstNode* node)
{
    // Lookup through IDOM tree from start block
    BasicBlock* block;
    auto id = start_block->id;
    for (;;)
    {
        block = m_cfg->m_blocks[id];
        auto it = simple::lower_bound(block->outputs.begin(), block->outputs.end(),
                                      var_info, VarInfoLess());
        if (it != block->outputs.end() && !VarInfoLess()(var_info, *it))
            // Found
            return Incoming(id, it->version_no);

        if (!block->id)
            // Reached block#0
            break;
        id = block->idom;
    }

    // Create new var_info in block#0's outputs to prevent furture errors
    VarInfo new_var_info(var_info);
    if (var_info.storage == VAR_LOCAL_VAR)
        new_var_info.set_uninitialized();

    insert_sorted(block->outputs, new_var_info, VarInfoLess());
    return Incoming(block->id, new_var_info.version_no);
}

// Get version no from versions
// Set the version no the *no & add current version no by reserved
// Auto put version 0 into versions if not found in versions
void LangPhi::reserve_output_var_version(VersionNo* version_no, VersionNo reserved,
                                         VarInfo& var_info, VarInfos& current_versions)
{
    auto it = simple::lower_bound(current_versions.begin(), current_versions.end(),
                                  var_info, VarInfoLess());
    if (it == current_versions.end() || VarInfoLess()(var_info, *it))
    {
        // Not found the variable
        // Add new var info for object var/argument
        var_info.version_no = 0; // Init output version no
        it = current_versions.insert(it, var_info);
    }

    *version_no = it->version_no;
    it->version_no += reserved;
}

// Update version of var_info in versions when got any assign operation
void LangPhi::update_input_var_version(VarInfo& var_info, VarInfos& current_versions)
{
    auto it = simple::lower_bound(current_versions.begin(), current_versions.end(),
                                  var_info, VarInfoLess());
    if (it == current_versions.end() || VarInfoLess()(var_info, *it))
    {
        // Not existed, add to current_versions
        current_versions.insert(it, var_info);
    } else
        // Already existed, update version no
        it->version_no = var_info.version_no;
}

}
