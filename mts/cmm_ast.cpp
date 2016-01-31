// cmm_ast.cpp

#include "cmm.h"
#include "cmm_ast.h"

namespace cmm
{

// Enum to string
const char* AstNodeTypeToString(AstNodeType nodeType)
{
    switch (nodeType)
    {
    case AST_ROOT:              return "Root";
    default:                    return "Unknown";
    }
}

// Append node to list
AstNode *append_sibling_node(AstNode *node, AstNode *next)
{
    if (!node)
        return next;
    node->append_sibling(next);
    return node;
}

}
