// cmm_ast.cpp

#include "cmm.h"
#include "cmm_ast.h"
#include "cmm_lang.h"

namespace cmm
{

AstNode::AstNode(Lang* context) :
    sibling(0),
    children(0)
{
    location.file = context->m_lexer.get_current_file_name();
    location.line = context->m_lexer.get_current_line();
    in_function_no = context->m_in_function ? context->m_in_function->no : 0;
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
    case AST_FOR_LOOP: return "For-Loop";
    case AST_FUNCTION: return "Function";
    case AST_FUNCTION_ARG: return "FunctionArg";
    case AST_GOTO: return "Goto";
    case AST_IF_ELSE: return "IfElse";
    case AST_LABEL: return "Label";
    case AST_LOOP_EACH: return "Loop-Each";
    case AST_LOOP_RANGE: return "Loop-Range";
    case AST_LVALUE: return "LValue";
    case AST_PROTOTYPE: return "Prototype";
    case AST_REGISTER: return "Register";
    case AST_RETURN: return "Return";
    case AST_WHILE_LOOP: return "While-Loop";
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
    switch (value_type)
    {
    case NIL:       return "nil";
    case INTEGER:   return "int";
    case REAL:      return "real";
    case STRING:    return "string";
    case BUFFER:    return "buffer";
    case OBJECT:    return "object";
    case FUNCTION:  return "function";
    case ARRAY:     return "array";
    case MAPPING:   return "mapping";
    case TVOID:     return "void";
    default:
    case MIXED:     return "mixed";
    }
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
             ident_type == IDENT_LOCAL_VAR ? "local" :
             ident_type == IDENT_OBJECT_VAR ? "object_var" : "", no);
    return ast_var_type_to_string(var_type) + " " + name + buf;
}

simple::string AstExpr::to_string()
{
    simple::string str = ast_var_type_to_string(var_type);
    if (is_constant)
        str += " (constant)";
    return str;
}

// eg. *=
simple::string AstExprAssign::to_string()
{
    return AstExpr::to_string() + " " + ast_op_to_string(op);
}

// eg. +
simple::string AstExprBinary::to_string()
{
    return AstExpr::to_string() + " " + ast_op_to_string(op);
}

// eg. (int)
simple::string AstExprCast::to_string()
{
    return simple::string("(") + ast_var_type_to_string(var_type) + ")";
}

// eg. ?
simple::string AstExprTernary::to_string()
{
    return AstExpr::to_string() + " " + ast_op_to_string(op);
}

// eg. --
simple::string AstExprUnary::to_string()
{
    return AstExpr::to_string() + " " + ast_op_to_string(op);
}

// eg. i
simple::string AstExprVariable::to_string()
{
    return AstExpr::to_string() + " " + name;
}

// eg. write
simple::string AstExprFunctionCall::to_string()
{
    return callee_name;
}

// eg. [..<]
simple::string AstExprIndex::to_string()
{
    char buf[16];
    if (op == OP_IDX)
    {
        // []
        snprintf(buf, sizeof(buf), "[%s]", is_reverse_from ? "<" : "");
    }
    else
    {
        // [..]
        snprintf(buf, sizeof(buf), "[%s..%s]",
                 is_reverse_from ? "<" : "",
                 is_reverse_to ? "<" : "");
    }
    return buf;
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
simple::string AstLoopRange::to_string()
{
    char str[16];
    snprintf(str, sizeof(str), "Step %d", direction);
    return str;
}

// eg. private void write
simple::string AstPrototype::to_string()
{
    simple::string ret = ast_function_attrib_to_string((AstFunctionAttrib)attrib);
    if (ret.length() > 0)
        ret += " ";
    ret += ast_var_type_to_string(ret_var_type);
    ret += " ";
    ret += name;
    return ret;
}



}
