// cmm_lang_phi.h
// Initial version by doing Mar/4/2016
// Figure out input/output & add phi nodes

#pragma once

#include "cmm_lang_component.h"

namespace cmm
{

class Lang;
class LangCFG;

class LangPhi : LangComponent
{
    friend Lang;

public:
    LangPhi(Lang* lang_context);

public:
    void create_phi_info();

    // Steps to create phi nodes
private:
    void generate_block_var_info();
    void generate_block_var_name_of_phi();
    void generate_block_output_var_version();
    void generate_block_uninitilized_var_info();
    void generate_block_phi_incoming();
    void generate_block_input_var_version();

private:
    void        add_to_check_list(const VarInfo& var_info);
    void        add_to_input(BasicBlock* block, const VarInfo& var_info);
    void        add_to_output(BasicBlock* block, const VarInfo& var_info);
    BasicBlock* find_phi_incoming_block(BasicBlock* block, BasicBlockId pred_id, const VarInfo& var_info);
    bool        found_in_phi_nodes(PhiNodes& phi_nodes, const VarInfo& phi_node);
    bool        found_in_var_infos(VarInfos& var_infos, const VarInfo& var_info);
    AstNode*    get_first_reference_node(BasicBlock* block, const VarInfo& var_info);
    Incoming    get_input_var_incoming(VarInfo& var_info, BasicBlock* start_block, AstNode* node);
    void        reserve_output_var_version(VersionNo* version_no, VersionNo reserved, VarInfo& var_info, VarInfos& current_versions);
    void        update_input_var_version(VarInfo& var_info, VarInfos& current_versions);

private:
    LangCFG* m_cfg;

    // List to check uninitialized
    VarInfos m_check_list;
};

}
