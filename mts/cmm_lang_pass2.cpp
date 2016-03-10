// cmm_lang_pass2.cpp
// Initial version Feb/2/2016 by doing
// AST: pass2

#include "cmm_lang.h"
#include "cmm_lang_cfg.h"
#include "cmm_lang_phi.h"
#include "cmm_lang_symbols.h"

namespace cmm
{

bool Lang::pass2(AstFunction* ast_function)
{
    if (!ast_function->body)
    {
        // Not defined
        auto* prototype = ast_function->prototype;
        this->syntax_errors(this,
            "%s(%d): error %d: function '%s' declared but not defined\n",
            prototype->location.file->c_str(), prototype->location.line,
            C_FUNCTION_NOT_DEFINED,
            prototype->name.c_str());
        return false;
    }

    // Save current function
    m_in_ast_function = ast_function;

    auto b = std_get_current_us_counter();////----
    // Prepare to build CFG
    m_cfg->init(ast_function);

    // Collect branch information
    lookup_and_find_out_branch(ast_function->body);

    // Create basic blocks from AST
    if (!lookup_and_gather_nodes(ast_function->body))
        // Failed to lookup, stop parse this functino
        return false;
    m_cfg->create_blocks();

    // Generate DOM/IDOM/DF
    m_cfg->generate_dom();
    auto e = std_get_current_us_counter();
    printf("CFG cost %zuus.\n", (size_t)(e - b));////----

    // Generate variable & phi info
    m_phi->create_phi_info();

    // Print blocks
    STD_TRACE("Blocks of '%s'(%zu), count:%zu, nodes:%zu\n",
        ast_function->prototype->name.c_str(),
        (size_t)ast_function->no,
        m_cfg->m_blocks.size(),
        m_cfg->m_nodes.size());

    m_cfg->print_blocks();

    return true;
}

// Scan the tree (bottom to top)
// Find out all branch/join points
void Lang::lookup_and_find_out_branch(AstNode* node)
{
    // Bottom first
    for (auto* p = node->children; p != 0; p = p->sibling)
        lookup_and_find_out_branch(p);

    switch (node->get_node_type())
    {
        case AST_DO_WHILE:
        {
            auto* do_while = (AstDoWhile*)node;
            // Goto continue/break by cond
            m_cfg->add_edge(do_while->cond, do_while->get_continue_node(), LangCFG::EDGE_NOT_GOTO);
            m_cfg->add_edge(do_while->cond, do_while->get_break_node(), LangCFG::EDGE_NOT_GOTO);
            break;
        }

        case AST_EXPR_BINARY:
        {
            auto* expr = (AstExprTernary*)node;
            if (expr->op == OP_LAND || expr->op == OP_LOR)
            {
                // Bypass expr2 by expr1
                m_cfg->add_edge(expr->expr1, AstNode::get_first_node(expr->expr2), LangCFG::EDGE_NOT_GOTO);
                m_cfg->add_edge(expr->expr1, expr, LangCFG::EDGE_NOT_GOTO);

                // Expr2 goto end
                m_cfg->add_edge(expr->expr2, expr, LangCFG::EDGE_NOT_GOTO);
            }
            break;
        }

        case AST_EXPR_TERNARY:
        {
            auto* expr = (AstExprTernary*)node;
            if (expr->op != OP_QMARK)
                break;
            // Goto expr2/3 by expr1
            m_cfg->add_edge(expr->expr1, AstNode::get_first_node(expr->expr2), LangCFG::EDGE_NOT_GOTO);
            m_cfg->add_edge(expr->expr1, AstNode::get_first_node(expr->expr3), LangCFG::EDGE_NOT_GOTO);
            // Expr2/3 goto end
            m_cfg->add_edge(expr->expr2, expr, LangCFG::EDGE_NOT_GOTO);
            m_cfg->add_edge(expr->expr3, expr, LangCFG::EDGE_NOT_GOTO);
            break;
        }

        case AST_FOR:
        {
            auto* for_loop = (AstForLoop*)node;
            auto* body = AstNode::get_valid_node(for_loop->cond, for_loop->statement);
            // Goto cond @ init
            if (for_loop->init)
                m_cfg->add_edge(for_loop->init, AstNode::get_first_node(body), LangCFG::EDGE_NOT_GOTO);

            // Goto statement/break by cond
            if (for_loop->cond)
            {
                m_cfg->add_edge(for_loop->cond, AstNode::get_first_node(for_loop->statement), LangCFG::EDGE_NOT_GOTO);
                m_cfg->add_edge(for_loop->cond, for_loop->get_break_node(), LangCFG::EDGE_NOT_GOTO);
            }
            // Goto continue @ end of statement
            m_cfg->add_edge(for_loop->statement, for_loop->get_continue_node(), LangCFG::EDGE_NOT_GOTO);
            // Goto cond @ step
            if (for_loop->step)
                m_cfg->add_edge(for_loop->step, AstNode::get_first_node(body), LangCFG::EDGE_NOT_GOTO);
            break;
        }

        case AST_GOTO:
        {
            // Goto specified point
            auto* goto_node = (AstGoto*)node;
            AstNode* dest = 0;

            switch (goto_node->goto_type)
            {
            case AST_DIRECT_JMP:
                // Branch to specified label
                dest = m_symbols->get_label_info(goto_node->target_label);
                if (!dest)
                {
                    this->syntax_errors(this,
                        "%s(%d): error %d: label '%s' was undefined\n",
                        node->location.file->c_str(), node->location.line,
                        C_UNDECLARED_IDENTIFER,
                        goto_node->target_label.c_str());
                    this->m_stop_error_code = PASS2_ERROR;
                }
                break;

            case AST_BREAK:
                dest = goto_node->loop_switch->get_break_node();
                break;

            case AST_CONTINUE:
                dest = goto_node->loop_switch->get_continue_node();
                break;
            }
            if (!dest)
                break;

            // Add an edge for this jmp
            m_cfg->add_edge(goto_node, dest, LangCFG::EDGE_GOTO);
            break;
        }

        case AST_IF_ELSE:
        {
            auto* if_else = (AstIfElse*)node;
            // Goto then/else by cond
            // Statement then/else goto end
            if (if_else->statement_then)
            {
                m_cfg->add_edge(if_else->cond, AstNode::get_first_node(if_else->statement_then), LangCFG::EDGE_NOT_GOTO);
                m_cfg->add_edge(if_else->statement_then, if_else, LangCFG::EDGE_NOT_GOTO);
            }

            if (if_else->statement_else)
            {
                m_cfg->add_edge(if_else->cond, AstNode::get_first_node(if_else->statement_else), LangCFG::EDGE_NOT_GOTO);
                m_cfg->add_edge(if_else->statement_else, if_else, LangCFG::EDGE_NOT_GOTO);
            }

            // Goto end directly from cond if one of else/then is empty
            if (!if_else->statement_then || !if_else->statement_else)
                m_cfg->add_edge(if_else->cond, if_else, LangCFG::EDGE_NOT_GOTO);
            break;
        }

        case AST_LOOP:
        {
            auto* loop = (AstLoop*)node;
            // Goto cond @ init
            m_cfg->add_edge(loop->init, AstNode::get_first_node(loop->step_cond), LangCFG::EDGE_NOT_GOTO);
            // Goto statement/break @ by iterator cond
            m_cfg->add_edge(loop->step_cond, AstNode::get_first_node(loop->statement), LangCFG::EDGE_NOT_GOTO);
            m_cfg->add_edge(loop->step_cond, loop->get_break_node(), LangCFG::EDGE_NOT_GOTO);
            // Goto continue @ end of statement
            m_cfg->add_edge(loop->statement, loop->get_continue_node(), LangCFG::EDGE_NOT_GOTO);
            break;
        }

        case AST_RETURN:
            // Add an edge for this jmp (out of function)
            m_cfg->add_edge(node, m_in_ast_function->body, LangCFG::EDGE_GOTO);
            break;

        case AST_SWITCH_CASE:
        {
            auto* switch_case = (AstSwitchCase*)node;
            auto* case1 = switch_case->cases;
            while (case1)
            {
                // Switch to case
                m_cfg->add_edge(switch_case->expr, AstNode::get_first_node(case1), LangCFG::EDGE_NOT_GOTO);

                // Get last effect node of statements node
                auto* effect_node = AstNode::get_last_effect_node(case1);
                if (effect_node->get_node_type() != AST_GOTO)
                {
                    // Link case to next case
                    auto* next = case1->sibling;
                    if (next)
                        next = AstNode::get_first_node(next);
                    else
                        next = switch_case;
                    m_cfg->add_edge(case1, next, LangCFG::EDGE_NOT_GOTO);
                }

                case1 = (decltype(case1))case1->sibling;
            }
            break;
        }

        case AST_WHILE:
        {
            auto* while_loop = (AstWhileLoop*)node;
            // Goto statement/break by cond
            m_cfg->add_edge(while_loop->cond, AstNode::get_first_node(while_loop->statement), LangCFG::EDGE_NOT_GOTO);
            m_cfg->add_edge(while_loop->cond, while_loop->get_break_node(), LangCFG::EDGE_NOT_GOTO);
            // Goto continue @ end of statement
            m_cfg->add_edge(while_loop->statement, while_loop->get_continue_node(), LangCFG::EDGE_NOT_GOTO);
            break;
        }

        default:
            // Skip
            break;
    }
}

// Scan the tree (bottom to top)
// Mark no for each node & put them in the flat array: m_nodes
// When encountering branch point (jmp label, if-then-else, loop), break & create new node
bool Lang::lookup_and_gather_nodes(AstNode* node)
{
    // Bottom first
    for (auto* p = node->children; p != 0; p = p->sibling)
        if (!lookup_and_gather_nodes(p))
            return false;

    // Add no & put into array
    return m_cfg->add_node(node);
}

}