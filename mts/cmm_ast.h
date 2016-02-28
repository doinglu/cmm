// cmm_ast.h

#pragma once

#include "cmm.h"
#include "cmm_lang_symbols.h"
#include "cmm_output.h"
#include "cmm_value.h"
#include "cmm_mmm_value.h"

namespace cmm
{

// Node type
enum AstNodeType
{
    AST_ROOT,
    AST_CASE,
    AST_DECLARATION,
    AST_DECLARATIONS,
    AST_DECLARE_FUNCTION,
    AST_DO_WHILE,
    AST_EXPR_ASSIGN,
    AST_EXPR_BINARY,
    AST_EXPR_CLOSURE,
    AST_EXPR_CAST,
    AST_EXPR_CONSTANT,
    AST_EXPR_CREATE_ARRAY,
    AST_EXPR_CREATE_FUNCTION,
    AST_EXPR_CREATE_MAPPING,
    AST_EXPR_FUNCTION_CALL,
    AST_EXPR_FUNCTION_CALL_EX,
    AST_EXPR_INDEX,
    AST_EXPR_IS_REF,
    AST_EXPR_RUNTIME_VALUE,
    AST_EXPR_SEGMENT,
    AST_EXPR_SINGLE_VALUE,
    AST_EXPR_TERNARY,
    AST_EXPR_UNARY,
    AST_EXPR_VARIABLE,
    AST_FOR_LOOP,
    AST_FUNCTION,
    AST_FUNCTION_ARG,
    AST_GOTO,
    AST_IF_ELSE,
    AST_LABEL,
    AST_LOOP_EACH,
    AST_LOOP_RANGE,
    AST_LVALUE,
    AST_PROTOTYPE,
    AST_RETURN,
    AST_WHILE_LOOP,
    AST_STATEMENTS,
    AST_SWITCH_CASE,
    AST_VAR_TYPE,
};

// Type of goto/break/continue/switch-case
enum AstGotoType : Uint8
{
    AST_DIRECT_JMP = 0,
    AST_BREAK      = 12,
    AST_CONTINUE   = 37,
};

// Function attrib
enum AstFunctionAttrib : Uint16
{
    AST_PRIVATE = 0x0001,
    AST_OVERRIDE = 0x0002,
    AST_RANDOM_ARG = 0x0004,
    AST_UNMASKABLE = 0x0008,
    AST_ANONYMOUS_CLOSURE = 0x0010,
    AST_MEMBER_METHOD = 0x0020,
    AST_PUBLIC = 0x1000         // For AST node use only
};
DECLARE_BITS_ENUM(AstFunctionAttrib, Uint16);

// Function type
enum AstFunctionType : Uint8
{
    AST_UNKNOWN = 0,
    AST_NEAR_FUN = 1,
    AST_FAR_FUN = 2,
    AST_EFUN = 4,
    AST_OBJECT_FUN = 5,
};

// Runtime value id
enum AstRuntimeValueId : Uint8
{
    AST_RV_INPUT_ARGUMENTS,
    AST_RV_INPUT_ARGUMENTS_COUNT,
};

// Define 4 chars as an integer
#define MULTI_CHARS(c1, c2, c3, c4) ((c1) | ((c2) << 8) | ((c3) << 16) | ((c4) << 24))

// Operator
enum Op
{
    // Unary operator
    OP_REV      = MULTI_CHARS('~', 0,   0,   0),
    OP_NOT      = MULTI_CHARS('!', 0,   0,   0),
    OP_NEG      = MULTI_CHARS('0', '-', 0,   0),
    OP_INC_PRE  = MULTI_CHARS('+', '+', '(', ')'),
    OP_DEC_PRE  = MULTI_CHARS('-', '-', '(', ')'),
    OP_INC_POST = MULTI_CHARS('(', ')', '+', '+'),
    OP_DEC_POST = MULTI_CHARS('(', ')', '+', '+'),
    OP_IF_REF   = MULTI_CHARS('i', 'r', 'e', 'f'),
    OP_CAST     = MULTI_CHARS('c', 'a', 's', 't'),

    // Binary operator
    OP_ADD      = MULTI_CHARS('+', 0,   0,   0),
    OP_SUB      = MULTI_CHARS('-', 0,   0,   0),
    OP_MUL      = MULTI_CHARS('*', 0,   0,   0),
    OP_DIV      = MULTI_CHARS('/', 0,   0,   0),
    OP_MOD      = MULTI_CHARS('%', 0,   0,   0),
    OP_LSH      = MULTI_CHARS('<', '<', 0,   0),
    OP_RSH      = MULTI_CHARS('>', '>', 0,   0),
    OP_AND      = MULTI_CHARS('&', 0,   0,   0),
    OP_OR       = MULTI_CHARS('|', 0,   0,   0),
    OP_XOR      = MULTI_CHARS('^', 0,   0,   0),
    OP_DOLLAR   = MULTI_CHARS('$', 0,   0,   0),
    OP_EQ       = MULTI_CHARS('=', '=', 0,   0),
    OP_NE       = MULTI_CHARS('!', '=', 0,   0),
    OP_GE       = MULTI_CHARS('>', '=', 0,   0),
    OP_GT       = MULTI_CHARS('>', 0,   0,   0),
    OP_LE       = MULTI_CHARS('<', '=', 0,   0),
    OP_LT       = MULTI_CHARS('<', 0,   0,   0),
    OP_LAND     = MULTI_CHARS('&', '&', 0,   0),
    OP_LOR      = MULTI_CHARS('|', '|', 0,   0),
    OP_ASSIGN   = MULTI_CHARS('=', 0,   0,   0),
    OP_ADD_EQ   = MULTI_CHARS('+', '=', 0,   0),
    OP_SUB_EQ   = MULTI_CHARS('-', '=', 0,   0),
    OP_MUL_EQ   = MULTI_CHARS('*', '=', 0,   0),
    OP_DIV_EQ   = MULTI_CHARS('/', '=', 0,   0),
    OP_MOD_EQ   = MULTI_CHARS('%', '=', 0,   0),
    OP_AND_EQ   = MULTI_CHARS('&', '=', 0,   0),
    OP_OR_EQ    = MULTI_CHARS('|', '=', 0,   0),
    OP_XOR_EQ   = MULTI_CHARS('^', '=', 0,   0),
    OP_QMARK_EQ = MULTI_CHARS('?', '=', 0,   0),
    OP_RSH_EQ   = MULTI_CHARS('>', '>', '=', 0),
    OP_LSH_EQ   = MULTI_CHARS('<', '<', '=', 0),

    // Ternary operator
    OP_QMARK    = MULTI_CHARS('?', ':', 0,   0),

    // Subscript index
    OP_IDX      = MULTI_CHARS('[', ']', 0,   0),
    OP_IDX_RANGE= MULTI_CHARS('[', '.', '.', ']'),

    // Imply LT/LE/GT/GE, for coding convenience
    OP_ORDER    = MULTI_CHARS('<', '>', '=', 0),
};

// Type of variant
enum AstVarAttrib : Uint8
{
    AST_VAR_REF_ARGUMENT = 0x01,
    AST_VAR_MAY_NIL = 0x02,
    AST_VAR_NO_SAVE = 0x04,
    AST_VAR_CONST = 0x08,
};
DECLARE_BITS_ENUM(AstVarAttrib, Uint8);

// Node utilties

template <typename T>
size_t tf_get_sibling_count(T* list)
{
    size_t count = 0;
    auto* p = list;
    while (p)
    {
        count++;
        p = p->sibling;
    }
    return count;
}

template <typename T>
T* tf_append(T** pplist, T* node)
{
    auto** pp = pplist;
    while (*pp)
        pp = &(*pp)->sibling;
    *pp = node;
    return *pplist;
}

template <typename T>
bool tf_remove(T** pplist, T* node)
{
    T* p;
    while ((p = *pplist))
    {
        if (p == node)
        {
            *pplist = p->sibling;
            return true;
        }
        pplist = &(T*)p->sibling;
    }
    return false;
}

template <typename T>
void tf_free(T** pplist)
{
    T* p;
    while ((p = *pplist))
    {
        *pplist = (T*)p->next;
        BUFFER_DELETE(p);
    }
}

template <typename T>
T* tf_get(T* list, size_t index)
{
    auto* p = list;
    while (index-- && p)
        p = p->sibling;
    return p;
}

// Source location
struct SourceLocation
{
    StringImpl* file;
    int line;
    int column;

    SourceLocation() :
        file(EMPTY_STRING),
        line(0),
        column(0)
    {
    }
};

// Variable type
struct AstVarType
{
    ValueType    basic_var_type;       // basic variable type
    AstVarAttrib var_attrib;           // variable attribute

    // Is this a const (readonly) varaible
    bool is_const()
    {
        return (var_attrib & AST_VAR_CONST) ? true : false;
    }
};

// Forward declaration
class Lang;
struct AstNode;
struct AstExpr;
struct AstFunction;
struct AstFunctionArg;
struct AstLValue;
struct AstPrototype;

// Concat ast nodes list
AstNode *append_sibling_node(AstNode *node, AstNode *next);

// Function attrib to string
simple::string ast_function_attrib_to_string(AstFunctionAttrib attrib);

// Enum to string
const char* ast_node_type_to_c_str(AstNodeType nodeType);

// Operator to string
simple::string ast_op_to_string(Op op);

// VarType to string
simple::string ast_var_type_to_string(AstVarType var_type);

// basic value to string
const char* value_type_to_c_str(ValueType value_type);

// The AST nodes
// AST node: abstract type
struct AstNode
{
    virtual AstNodeType get_node_type() = 0;
    AstNode* sibling;
    AstNode* children;
    SourceLocation location;
    FunctionNo in_function_no;

    AstNode(Lang* context);

    // Get count of my sibling
    size_t get_sibling_count()
    {
        return tf_get_sibling_count(sibling);
    }

    // Get count of children
    size_t get_children_count()
    {
        return tf_get_sibling_count(children);
    }

    // Get a child at specified location
    AstNode* get_child_at(size_t index)
    {
        return tf_get(children, index);
    }

    // Add a new child (the child can be 0)
    void add_child(AstNode* child)
    {
        tf_append(&children, child);
    }

    // Add a new sibling (the sibling can be 0)
    void append_sibling(AstNode* one)
    {
        tf_append(&sibling, one);
    }

    // Collect array of single node as children
    // ATTENTION: the node should not have sibling
    void collect_children_at(AstNode** array, size_t count)
    {
        children = 0;
        for (size_t i = 0; i < count; i++)
        {
            STD_ASSERT(!array[i] || !array[i]->sibling);
            add_child(array[i]);
        }
    }

    template <typename... T>
    void collect_children_of(T... args)
    {
        AstNode* nodes[] = { args... };
        collect_children_at(nodes, sizeof(nodes) / sizeof(nodes[0]));
    }

    // Specified routine for collect children of this AstNode
    // Should be called only once after node created
    virtual void collect_children()
    {
    }

    // Does this node contain new frame definition?
    // For those node like block: '{' statements '}', they will increase
    // tag to accept new local declarartion in node.
    virtual bool contains_new_frame()
    {
        return false;
    }

    // Output for thie AstNode
    virtual simple::string to_string()
    {
        return "";
    }
};

// Root
struct AstRoot : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_ROOT; }

    AstRoot(Lang* context) :
        AstNode(context)
    {
    }
};

// Switch - case
struct AstCase : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_CASE; }
    AstExpr* case_value;
    bool     is_default;
    AstNode* block;

    virtual void collect_children()
    {
        collect_children_of(case_value, block);
    }

    virtual simple::string to_string();

    AstCase(Lang* context) :
        AstNode(context),
        is_default(false),
        block(0)
    {
    }
};

// Variable declaration
// No child
struct AstDeclaration : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_DECLARATION; }
    AstVarType     var_type;   // variable type
    simple::string name;       // variable name
    IdentType      ident_type; // IdentType: identifier type, local or object
    union
    {
        LocalNo     local_var_no;   // Number of var as local
        VariableNo object_var_no;  // Number of var in object
        int         no;
    };
    AstExpr*    expr;     // assign expression

    virtual void collect_children()
    {
        collect_children_of(expr);
    }

    virtual simple::string to_string();

    AstDeclaration(Lang* context) :
        AstNode(context),
        name(""),
        no(0),
        expr(0)
    {
    }
};

// Variable declaration
// No child
struct AstDeclarations : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_DECLARATIONS; }
    AstVarType      var_type;   // variable type
    AstDeclaration* decl_list;

    virtual void collect_children()
    {
        children = decl_list;
    }

    AstDeclarations(Lang* context) :
        AstNode(context),
        decl_list(0)
    {
    }
};

// Do-While
struct AstDoWhile : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_DO_WHILE; }
    AstNode* block;
    AstExpr* cond;

    virtual void collect_children()
    {
        collect_children_of(block, cond);
    }

    virtual bool contains_new_frame()
    {
        return true;
    }

    AstDoWhile(Lang* context) :
        AstNode(context),
        block(0),
        cond(0)
    {
    }
};

enum OutputType
{
    OUTPUT_NONE        = 0,
    OUTPUT_OBJECT_VAR  = 19,    // To object var
    OUTPUT_LOCAL_VAR   = 47,    // To local var/argument
    OUTPUT_VIRTUAL_REG = 62,    // To a virtual register
};

typedef int VirtualRegNo;       // No of virtual register

// Primary Expression
// Abstract node
struct AstExpr : AstNode
{
    AstVarType  var_type;       // Value type of this expression
    bool        is_constant;    // if this is a constant expression
    struct
    {
        OutputType  type;       // Output to member variable or to local (include argument) 
        int         index;      // Index of output variable
    } output;

    virtual simple::string to_string();

    AstExpr(Lang* context) :
        AstNode(context),
        is_constant(false),
        output{ OUTPUT_NONE, 0 }
    {
        var_type.basic_var_type = MIXED;
        var_type.var_attrib = (AstVarAttrib)0;
    }
};

// Operator - abstract class
struct AstExprOp : AstExpr
{
    Op op;

    AstExprOp(Lang* context) :
        AstExpr(context),
        op((Op)0)
    {
    }
};

// Assgin-op expression
struct AstExprAssign : AstExprOp
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_ASSIGN; }
    AstExpr*   expr1;   // Left expression
    AstExpr*   expr2;   // Right expression

    virtual void collect_children()
    {
        collect_children_of(expr1, expr2);
    }

    // eg. *=
    virtual simple::string to_string();

    AstExprAssign(Lang* context) :
        AstExprOp(context),
        expr1(0),
        expr2(0)
    {
    }
};

// Binary-op expression
struct AstExprBinary : AstExprOp
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_BINARY; }
    AstExpr* expr1;     // Left expression
    AstExpr* expr2;     // Right expression

    virtual void collect_children()
    {
        collect_children_of(expr1, expr2);
    }

    // eg. +
    virtual simple::string to_string();

    AstExprBinary(Lang* context) :
        AstExprOp(context),
        expr1(0),
        expr2(0)
    {
    }
};

// Cast expression
struct AstExprCast : AstExprOp
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CAST; }
    AstVarType var_type;  // Type to cast
    AstExpr*   expr1;     // To be operated

    virtual void collect_children()
    {
        collect_children_of(expr1);
    }

    virtual simple::string to_string();

    AstExprCast(Lang* context) :
        AstExprOp(context),
        expr1(0)
    {
    }
};

// Closure
struct AstExprClosure : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CLOSURE; }
    AstFunction* function;

    virtual void collect_children()
    {
        collect_children_of(function);
    }

    AstExprClosure(Lang* context) :
        AstExpr(context),
        function(0)
    {
    }
};

// Expression element of constant 
struct AstExprConstant : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CONSTANT; }
    MMMValue value;          // Value

    virtual simple::string to_string()
    {
        Output output;
        return output.type_value(&value).c_str();
    }

    AstExprConstant(Lang* context) :
        AstExpr(context)
    {
        is_constant = true;
    }
};

// Expression element of creating array
struct AstExprCreateArray : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CREATE_ARRAY; }
    AstExpr* expr_list;

    virtual void collect_children()
    {
        children = expr_list;
    }

    AstExprCreateArray(Lang* context) :
        AstExpr(context)
    {
    }
};

// Expression element of creating function
struct AstExprCreateFunction : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CREATE_FUNCTION; }
    simple::string  name;
    AstExpr*        expr_list;

    virtual void collect_children()
    {
        children = expr_list;
    }

    AstExprCreateFunction(Lang* context) :
        AstExpr(context),
        name("")
    {
    }
};

// Expression element of creating mapping
struct AstExprCreateMapping : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CREATE_MAPPING; }
    AstExpr* expr_list;

    virtual void collect_children()
    {
        children = expr_list;
    }

    AstExprCreateMapping(Lang* context) :
        AstExpr(context)
    {
    }
};

// Call a function
struct AstExprFunctionCall : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_FUNCTION_CALL; }
    AstExpr*        target;            // target object or array
    simple::string  callee_name;       // call function name
    AstExpr*        arguments;         // arguments

    virtual void collect_children()
    {
        // The arguments is a list
        children = append_sibling_node(target, arguments);
    }

    virtual simple::string to_string();

    AstExprFunctionCall(Lang* context) :
        AstExpr(context),
        callee_name(""),
        arguments(0)
    {
    }
};

// Call a function (function is first argument)
struct AstExprFunctionCallEx : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_FUNCTION_CALL_EX; }
    AstExpr* arguments;       // arguments

    virtual void collect_children()
    {
        // The arguments is a list
        children = arguments;
    }

    AstExprFunctionCallEx(Lang* context) :
        AstExpr(context),
        arguments(0)
    {
    }
};

// Get/Assign Index
struct AstExprIndex : AstExprOp
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_INDEX; }
    AstExpr*     container;         // container
    AstExpr*     index_from;        // from index
    AstExpr*     index_to;          // to index
    bool         is_reverse_from;   // is from index reverse
    bool         is_reverse_to;     // is to index reverse

    virtual void collect_children()
    {
        // The arguments is a list
        collect_children_of(container, index_from, index_to);
    }

    virtual simple::string to_string();

    AstExprIndex(Lang* context) :
        AstExprOp(context),
        container(0),
        index_from(0),
        index_to(0),
        is_reverse_from(false),
        is_reverse_to(false)
    {
    }
};

// Is-Ref
// No child
struct AstExprIsRef : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_IS_REF; }
    simple::string name;

    AstExprIsRef(Lang* context) :
        AstExpr(context),
        name("")
    {

    }
};

// Get runtime value
struct AstExprRuntimeValue : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_RUNTIME_VALUE; }
    AstVarType var_type;
    Uint16     value_id;

    AstExprRuntimeValue(Lang* context) :
        AstExpr(context),
        value_id(0)
    {
    }
};

// Segment .
struct AstExprSegment : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_SEGMENT; }
    AstExpr*        expr; // primary
    simple::string  name; // segement part

    virtual void collect_children()
    {
        // The arguments is a list
        collect_children_of(expr);
    }

    AstExprSegment(Lang* context) :
        AstExpr(context),
        name(""),
        expr(0)
    {
    }
};

// Peek single value of expression list
struct AstExprSingleValue : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_SINGLE_VALUE; }
    AstExpr* expr_list;

    virtual void collect_children()
    {
        children = expr_list;
    }

    AstExprSingleValue(Lang* context) :
        AstExpr(context),
        expr_list(0)
    {
    }
};

// Ternary-op expression
struct AstExprTernary : AstExprOp
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_TERNARY; }
    AstExpr* expr1;     // 1st expression
    AstExpr* expr2;     // 2nd expression
    AstExpr* expr3;     // 3rd expression

    virtual void collect_children()
    {
        collect_children_of(expr1, expr2, expr3);
    }

    // eg. ?
    virtual simple::string to_string();

    AstExprTernary(Lang* context) :
        AstExprOp(context),
        expr1(0),
        expr2(0),
        expr3(0)
    {
    }
};

// Unary-op expression
struct AstExprUnary : AstExprOp
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_UNARY; }
    AstExpr* expr1;     // To be operated

    virtual void collect_children()
    {
        collect_children_of(expr1);
    }

    // eg. --
    virtual simple::string to_string();

    AstExprUnary(Lang* context) :
        AstExprOp(context),
        expr1(0)
    {
    }
};

// Expression element of variable 
struct AstExprVariable : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_VARIABLE; }
    simple::string name;   // Variable name

    virtual simple::string to_string();

    AstExprVariable(Lang* context) :
        AstExpr(context),
        name("")
    {
    }
};

// For-Loop
struct AstForLoop : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_FOR_LOOP; }
    AstNode* init;
    AstExpr* cond;
    AstExpr* step;
    AstNode* block;

    virtual void collect_children()
    {
        collect_children_of(init, cond, step, block);
    }

    virtual bool contains_new_frame()
    {
        return true;
    }

    AstForLoop(Lang* context) :
        AstNode(context),
        init(0),
        cond(0),
        step(0),
        block(0)
    {
    }
};

// Function: prototype statements
struct AstFunction : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_FUNCTION; }

    // Function number in program
    FunctionNo no;

    // Prototype
    AstPrototype* prototype;

    // Function body
    AstNode* body;

    // All local variable declaration
    simple::vector<AstDeclaration*> local_vars;

    virtual void collect_children()
    {
        collect_children_of(prototype, body);
    }

    virtual bool contains_new_frame()
    {
        return true;
    }

    virtual simple::string to_string();

    AstFunction(Lang* context) :
        AstNode(context),
        body(0),
        local_vars(0)
    {
    }
};

// Argument list with extra attributes of function prototype
struct AstFunctionArgsEx
{
    bool ddd;
    AstFunctionArg* arg_list;
};

// Function argument
struct AstFunctionArg : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_FUNCTION_ARG; }
    simple::string  name;            // function argument name
    AstVarType      var_type;        // argument variable type
    ArgNo           arg_no;          // argument index
    AstExpr*        default_value;   // default argument value

    virtual void collect_children()
    {
        collect_children_of(default_value);
    }

    virtual simple::string to_string();

    AstFunctionArg(Lang* context) :
        AstNode(context),
        name(""),
        arg_no(0),
        default_value(0)
    {
    }
};

// Goto
// No child
struct AstGoto : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_GOTO; }
    AstGotoType     goto_type;
    AstNode*        loop_switch;
    simple::string  target_label;     // Label to jmp

    virtual simple::string to_string();

    AstGoto(Lang* context) :
        AstNode(context),
        target_label("")
    {
    }
};

// If-Then-Else 
struct AstIfElse : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_IF_ELSE; }
    AstExpr* cond;
    AstNode* block_then;
    AstNode* block_else;

    virtual void collect_children()
    {
        collect_children_of(cond, block_then, block_else);
    }

    AstIfElse(Lang* context) :
        AstNode(context),
        cond(0),
        block_then(0),
        block_else(0)
    {
    }
};

// Label
// No child
struct AstLabel : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_LABEL; }
    simple::string name;

    virtual simple::string to_string();

    AstLabel(Lang* context) :
        AstNode(context),
        name("")
    {
    }
};

// Loop-Each
struct AstLoopEach : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_LOOP_EACH; }
    AstNode* decl_or_variable;
    AstExpr* container;

    virtual void collect_children()
    {
        collect_children_of(decl_or_variable, container);
    }

    virtual bool contains_new_frame()
    {
        return true;
    }

    AstLoopEach(Lang* context) :
        AstNode(context),
        decl_or_variable(0),
        container(0)
    {
    }
};

// Loop-Range
struct AstLoopRange : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_LOOP_RANGE; }
    AstNode* decl_or_variable;
    AstExpr* begin;
    AstExpr* to;
    int      direction; // 1(upto) or -1(downto)

    virtual void collect_children()
    {
        collect_children_of(decl_or_variable, begin, to);
    }

    virtual bool contains_new_frame()
    {
        return true;
    }

    virtual simple::string to_string();

    AstLoopRange(Lang* context) :
        AstNode(context),
        decl_or_variable(0),
        begin(0),
        to(0),
        direction(0)
    {
    }
};

// Prototype: var_type function_name(arg_list)
struct AstPrototype : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_PROTOTYPE; }

    // Function name
    simple::string name;

    // Function attrib
    AstFunctionAttrib attrib;

    // Return type
    AstVarType ret_var_type;

    // Argument list
    AstFunctionArg* arg_list;

    // has return value
    bool has_ret;

    virtual void collect_children()
    {
        children = arg_list;
    }

    virtual simple::string to_string();

    AstPrototype(Lang* context) :
        AstNode(context),
        name(""),
        arg_list(0),
        has_ret(false)
    {
    }
};

// Return
struct AstReturn : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_RETURN; }
    AstExpr* expr;

    virtual void collect_children()
    {
        collect_children_of(expr);
    }

    AstReturn(Lang* context) :
        AstNode(context)
    {
    }
};

// Statements
struct AstStatements : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_STATEMENTS; }

    virtual bool contains_new_frame()
    {
        return true;
    }

    AstStatements(Lang* context) :
        AstNode(context)
    {
    }
};

// Switch-Case
struct AstSwitchCase : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_SWITCH_CASE; }
    AstExpr* expr;
    AstCase* cases;

    virtual void collect_children()
    {
        // The cases is a list
        children = append_sibling_node(expr, cases);
    }

    AstSwitchCase(Lang* context) :
        AstNode(context),
        cases(0)
    {
    }
};

// While
struct AstWhileLoop : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_WHILE_LOOP; }
    AstExpr* cond;
    AstNode* block;

    virtual void collect_children()
    {
        collect_children_of(cond, block);
    }

    virtual bool contains_new_frame()
    {
        return true;
    }

    AstWhileLoop(Lang* context) :
        AstNode(context),
        cond(0),
        block(0)
    {
    }
};

}
