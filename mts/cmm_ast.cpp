// cmm_ast.cpp

#include "cmm.h"
#include "cmm_ast.h"
#include "cmm_lang.h"

namespace cmm
{

AstNode::AstNode(Lang* context) :
    sibling(0),
    children(0),
    attrib((AstNodeAttrib)0)
{
    location.file = context->m_lexer.get_current_file_name();
    location.line = context->m_lexer.get_current_line();
    in_function_no = context->m_in_ast_function ? context->m_in_ast_function->no : 0;
}

// Get last effect node from a statements
AstNode* AstNode::get_last_effect_node(AstNode* node)
{
    for (;;)
    {
        if (!node->children)
            // No children, return me
            return node;

        if (is_effect_node(node))
            // Got effect node
            return node;

        // Get last child
        node = node->children;
        while (node->sibling)
            node = node->sibling;
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

// AstFunctionAttrib to string
simple::string ast_function_attrib_to_string(AstFunctionAttrib attrib)
{
    char buf[256];
    char* p = buf;
    char* e = buf + sizeof(buf);    

    if (attrib & AstFunctionAttrib::AST_OVERRIDE)
    {
        strncpy(p, "override|", e - p);
        p += strlen(p);
    }

    if (attrib & AstFunctionAttrib::AST_PRIVATE)
    {
        strncpy(p, "private|", e - p);
        p += strlen(p);
    }

    if (attrib & AstFunctionAttrib::AST_PUBLIC)
    {
        strncpy(p, "public|", e - p);
        p += strlen(p);
    }

    if (attrib & AstFunctionAttrib::AST_RANDOM_ARG)
    {
        strncpy(p, "random_arg|", e - p);
        p += strlen(p);
    }

    if (attrib & AstFunctionAttrib::AST_UNMASKABLE)
    {
        strncpy(p, "unmaskable|", e - p);
        p += strlen(p);
    }

    // Remove last char '|'
    if (p > buf)
        p--;
    *p = 0;
    return buf;
}

// Enum to string
const char* ast_node_type_to_c_str(AstNodeType nodeType)
{
    switch (nodeType)
    {
    case AST_ROOT: return "Root";
    case AST_CASE: return "Case";
    case AST_DECLARATION: return "Declaration";
    case AST_DECLARATIONS: return "Declarations";
    case AST_DECLARE_FUNCTION: return "DeclareFunction";
    case AST_DO_WHILE: return "Do-While";
    case AST_EXPR_ASSIGN: return "ExprAssign";
    case AST_EXPR_BINARY: return "ExprBinary";
    case AST_EXPR_CAST: return "ExprCast";
    case AST_EXPR_CLOSURE: return "ExprClosure";
    case AST_EXPR_CONSTANT: return "ExprConstant";
    case AST_EXPR_CREATE_ARRAY: return "ExprCreateArray";
    case AST_EXPR_CREATE_FUNCTION: return "ExprCreateFunction";
    case AST_EXPR_CREATE_MAPPING: return "ExprCreateMapping";
    case AST_EXPR_FUNCTION_CALL: return "ExprFunctionCall";
    case AST_EXPR_FUNCTION_CALL_EX: return "ExprFunctionCallEx";
    case AST_EXPR_INDEX: return "ExprIndex";
    case AST_EXPR_IS_REF: return "ExprIsRef";
    case AST_EXPR_RUNTIME_VALUE: return "ExprRuntimeValue";
    case AST_EXPR_SEGMENT: return "ExprSegment";
    case AST_EXPR_SINGLE_VALUE: return "ExprSingleValue";
    case AST_EXPR_TERNARY: return "ExprTernary";
    case AST_EXPR_UNARY: return "ExprUnary";
    case AST_EXPR_VARIABLE: return "ExprVariable";
    case AST_FOR: return "For-Loop";
    case AST_FUNCTION: return "Function";
    case AST_FUNCTION_ARG: return "FunctionArg";
    case AST_GOTO: return "Goto";
    case AST_IF_ELSE: return "IfElse";
    case AST_LABEL: return "Label";
    case AST_LOOP: return "Loop";
    case AST_LOOP_INIT_EACH: return "LoopInitEach";
    case AST_LOOP_INIT_RANGE: return "LoopInitRange";
    case AST_LVALUE: return "LValue";
    case AST_NOP: return "Nop";
    case AST_PROTOTYPE: return "Prototype";
    case AST_RETURN: return "Return";
    case AST_WHILE: return "While-Loop";
    case AST_STATEMENTS: return "Statements";
    case AST_SWITCH_CASE: return "Switch-Case";
    case AST_VAR_TYPE: return "VarType";
    default:                    return "Unknown";
    }
}

// Operator to string
simple::string ast_op_to_string(Op op)
{
    char str[sizeof(Op) + 1];
    for (auto i = 0; i < sizeof(Op); i++)
        str[i] = (char)((op >> (i * 8)) & 0xFF);
    str[sizeof(Op)] = 0;
    return str;
}

// Expression output to string
simple::string ast_output_to_string(AstExpr* expr)
{
    // Append output information
    char buf[32];
    if (expr->get_node_type() == AST_EXPR_VARIABLE)
    {
        // This is named variable
        auto* expr_variable = (AstExprVariable*)expr;
        snprintf(buf, sizeof(buf), "%s(%d)", // eg. x(0)
                 expr_variable->name.c_str(),
                 expr_variable->is_left_value() ? expr_variable->var_output_version_no : 
                                                  expr_variable->var_input_version_no);
    } else
    {
        // Anonymouse expression
        snprintf(buf, sizeof(buf), "%s%d",   // eg. L1
                 var_storage_to_c_str(expr->output.storage),
                 (int)expr->output.no);
    }
    return buf;
}

// VarType to string
simple::string ast_var_type_to_string(AstVarType var_type)
{
    char buf[256];
    char* p = buf;
    char* e = buf + sizeof(buf);

    if (var_type.var_attrib & AstVarAttrib::AST_VAR_CONST)
    {
        strncpy(p, "const|", e - p);
        p += strlen(p);
    }

    if (var_type.var_attrib & AstVarAttrib::AST_VAR_NO_SAVE)
    {
        strncpy(p, "no_save|", e - p);
        p += strlen(p);
    }

    if (var_type.var_attrib & AstVarAttrib::AST_VAR_REF_ARGUMENT)
    {
        strncpy(p, "ref|", e - p);
        p += strlen(p);
    }

    // Replace last char '|' with ' '
    if (p > buf)
        *(p - 1) = ' ';

    strncpy(p, value_type_to_c_str(var_type.basic_var_type), e - p);
    p += strlen(p);

    // Add '?' if may null
    if (var_type.var_attrib & AstVarAttrib::AST_VAR_MAY_NIL)
        strncpy(p, "?", e - p);

    return buf;
}

// Value type
const char* value_type_to_c_str(ValueType value_type)
{
    return Value::type_to_name(value_type);
}

// eg. ""
simple::string AstNode::to_string()
{
    return "";
}

// eg. default
simple::string AstCase::to_string()
{
    if (is_default)
        return "Default";

    return "";
}

// eg. int x
simple::string AstDeclaration::to_string()
{
    char buf[16];
    snprintf(buf, sizeof(buf), " %s@%d",
             ident_type == IDENT_ARGUMENT ? "arg" :
             ident_type == IDENT_LOCAL_VAR ? "local" :
             ident_type == IDENT_OBJECT_VAR ? "object_var" : "", var_no);
    return ast_var_type_to_string(var_type) + " " + name + buf;
}

simple::string AstExpr::to_string()
{
    char buf[32];
    simple::string str = ast_var_type_to_string(var_type);
    if (output.storage == VAR_NONE)
        // No output
        return str;

    // Append output information
    snprintf(buf, sizeof(buf), " %s%d :=%s",   // type index "(constant)"
             var_storage_to_c_str(output.storage),
             (int)output.no,
             is_constant ? " (constant)" : "");
    str += buf;
    return str;
}

// eg. *=
simple::string AstExprAssign::to_string()
{
    return simple::string().snprintf("%s %s %s %s", 256,
        AstExpr::to_string().c_str(),
        ast_output_to_string(expr1).c_str(),
        ast_op_to_string(op).c_str(),
        ast_output_to_string(expr2).c_str());
}

// eg. +
simple::string AstExprBinary::to_string()
{
    return simple::string().snprintf("%s %s %s %s", 256,
        AstExpr::to_string().c_str(),
        ast_output_to_string(expr1).c_str(),
        ast_op_to_string(op).c_str(),
        ast_output_to_string(expr2).c_str());
}

// eg. (int)
simple::string AstExprCast::to_string()
{
    return simple::string().snprintf("%s (%s)%s", 256,
        AstExpr::to_string().c_str(),
        ast_var_type_to_string(var_type).c_str(),
        ast_output_to_string(expr1).c_str());
}

// eg. write()
simple::string AstExprClosure::to_string()
{
    return ast_function->prototype->name + "()";
}

// eg. "abc"
simple::string AstExprConstant::to_string()
{
    Output output;
    return AstExpr::to_string() + " " + output.type_value(&value).c_str();
}

// eg. is_ref(xxx)
simple::string AstExprIsRef::to_string()
{
    return simple::string().snprintf("%s is_ref(%s)", 256,
        AstExpr::to_string().c_str(),
        name.c_str());
}

// eg. $1
simple::string AstExprRuntimeValue::to_string()
{
    return simple::string().snprintf("%s ", 256,
        AstExpr::to_string().c_str(),
        value_id == AST_RV_INPUT_ARGUMENTS ? "$<" :
        value_id == AST_RV_INPUT_ARGUMENTS_COUNT ? "$?" :
        "<Bad>");
}

// eg: (x, y, z)
simple::string AstExprSingleValue::to_string()
{
    auto str = AstExpr::to_string() + " (";
    auto* p = children;
    while (p)
    {
        str += ast_output_to_string((AstExpr*)p);
        p = p->sibling;
        if (p)
            str += ", ";
    }
    str += ")";
    return str;
}

// eg. ?
simple::string AstExprTernary::to_string()
{
    return simple::string().snprintf("%s %s %s %s, %s", 256,
        AstExpr::to_string().c_str(),
        ast_output_to_string(expr1).c_str(),
        ast_op_to_string(op).c_str(),
        ast_output_to_string(expr2).c_str(),
        ast_output_to_string(expr3).c_str());
}

// eg. --
simple::string AstExprUnary::to_string()
{
    return simple::string().snprintf("%s %s%s", 256,
        AstExpr::to_string().c_str(),
        ast_op_to_string(op).c_str(),
        ast_output_to_string(expr1).c_str());
}

// eg. i
simple::string AstExprVariable::to_string()
{
    if (is_left_value_only())
        return simple::string().snprintf("%s ref %s(%d)", 256,
            AstExpr::to_string().c_str(), name.c_str(), (int)var_output_version_no);
    
    if (is_right_value_only())
        return simple::string().snprintf("%s %s(%d)", 256,
            AstExpr::to_string().c_str(), name.c_str(), (int)var_input_version_no);

    return simple::string().snprintf("%s ref %s(%d/%d)", 256,
            AstExpr::to_string().c_str(), name.c_str(), (int)var_output_version_no, (int)var_input_version_no);
}

// eg. write
simple::string AstExprFunctionCall::to_string()
{
    auto str = simple::string().snprintf("%s %s(", 256,
        AstExpr::to_string().c_str(), callee_name.c_str());
    auto* p = children;
    while (p)
    {
        str += ast_output_to_string((AstExpr*)p);
        p = p->sibling;
        if (p)
            str += ", ";
    }
    str += ")";
    return str;
}

// eg. c[x..<y]
simple::string AstExprIndex::to_string()
{
    if (op == OP_IDX)
    {
        // c[x]
        return simple::string().snprintf("%s %s%s[%s%s]", 256,
                    AstExpr::to_string().c_str(),
                    is_left_value() ? "ref " : "",
                    ast_output_to_string(container).c_str(),
                    is_reverse_from ? "<" : "",
                    ast_output_to_string(index_from).c_str());
    } else
    {
        // c[x..y]
        return simple::string().snprintf("%s %s%s[%s%s..%s%s]", 256,
                    AstExpr::to_string().c_str(),
                    is_left_value() ? "ref " : "",
                    ast_output_to_string(container).c_str(),
                    is_reverse_from ? "<" : "",
                    ast_output_to_string(index_from).c_str(),
                    is_reverse_to ? "<" : "",
                    ast_output_to_string(index_to).c_str());
    }
}

// Get name of argument
const simple::string& AstFunction::get_arg_name(ArgNo no) const
{
    return args[no]->name;
}

// Get name of local variable
const simple::string& AstFunction::get_local_var_name(LocalNo no) const
{
    return local_vars[no]->name;
}

// eg. mixed func() @1
simple::string AstFunction::to_string()
{
    char buf[16];
    snprintf(buf, sizeof(buf), "() @%d", (int)no);
    return ast_var_type_to_string(prototype->ret_var_type) + " " +
           prototype->name + buf;
}

// eg. int x
simple::string AstFunctionArg::to_string()
{
    return ast_var_type_to_string(var_type) + " " + name;
}

// eg. target
simple::string AstGoto::to_string()
{
    switch ((AstGotoType)goto_type)
    {
    default:
    case AST_DIRECT_JMP:
        return target_label;

    case AST_BREAK:
        return "<break>";

    case AST_CONTINUE:
        return "<continue>";
    }
}

// eg. label1:
simple::string AstLabel::to_string()
{
    return name;
}

// eg. step -1
simple::string AstLoopInitRange::to_string()
{
    char str[16];
    snprintf(str, sizeof(str), "Step %d", direction);
    return str;
}

// eg. private void write
simple::string AstPrototype::to_string()
{
    simple::string ret = ast_function_attrib_to_string(attrib);
    if (ret.length() > 0)
        ret += " ";
    ret += ast_var_type_to_string(ret_var_type);
    ret += " ";
    ret += name;
    return ret;
}



}
