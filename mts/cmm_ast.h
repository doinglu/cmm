// cmm_ast.h

#pragma once

#include "cmm.h"
#include "cmm_output.h"
#include "cmm_value.h"

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
    AST_REGISTER,
    AST_RETURN,
    AST_WHILE_LOOP,
    AST_STATEMENTS,
    AST_SWITCH_CASE,
    AST_VAR_TYPE,
};

// Type of goto/break/continue/switch-case
enum AstGotoType
{
    AST_DIRECT_JMP = 0,
    AST_BREAK_LOOP = 1,
    AST_BREAK_CASE = 2,
    AST_CONTINUE_LOOP = 3,
};

// Function attrib
enum AstFunctionAttrib
{
    AST_PRIVATE = 0x0001,
    AST_OVERRIDE = 0x0002,
    AST_RANDOM_ARG = 0x0004,
    AST_UNMASKABLE = 0x0008,
    AST_PUBLIC = 0x1000         // For AST node use only
};

// Function type
enum AstFunctionType
{
    AST_UNKNOWN = 0,
    AST_NEAR_FUN = 1,
    AST_FAR_FUN = 2,
    AST_EFUN = 4,
    AST_OBJECT_FUN = 5,
};

// Type of Index
enum AstIndexType
{
    AST_INDEX_SINGLE = 0,     // x[index]
    AST_INDEX_RANGE = 25,     // x[index..to]
};

// Runtime value id
enum AstRuntimeValueId
{
    AST_RV_INPUT_ARGUMENTS,
    AST_RV_INPUT_ARGUMENTS_COUNT,
};

// Operator
enum Op
{
    OP_ADD = '+',
    OP_SUB = '-',
    OP_MUL = '*',
    OP_DIV = '/',
    OP_MOD = '%',
    OP_LSH = '<<',
    OP_RSH = '>>',
    OP_AND = '&',
    OP_OR = '|',
    OP_XOR = '^',
    OP_EQ = '==',
    OP_NE = '!=',
    OP_GE = '>=',
    OP_GT = '>',
    OP_LE = '<=',
    OP_LT = '<',
    OP_LAND = '&&',
    OP_LOR = '||',
    OP_REV = '~',
    OP_NOT = '!',
    OP_NEG = '0-',
    OP_INC_PRE = '++()',
    OP_DEC_PRE = '--()',
    OP_INC_POST = '()++',
    OP_DEC_POST = '()++',
    OP_ASSIGN = '=',
    OP_ADD_EQ = '+=',
    OP_SUB_EQ = '-=',
    OP_MULT_EQ = '*=',
    OP_DIV_EQ = '/=',
    OP_MOD_EQ = '%=',
    OP_AND_EQ = '&=',
    OP_OR_EQ = '|=',
    OP_XOR_EQ = '^=',
    OP_RSH_EQ = '>>=',
    OP_LSH_EQ = '<<=',
    OP_IF_REF = 'iref',
};

// Type of variant
enum AstVarAttrib
{
    AST_VAR_REF_ARGUMENT = 0x01,
    AST_VAR_MAY_NULL = 0x02,
    AST_VAR_NO_SAVE = 0x04,
    AST_VAR_CONST = 0x08,
};

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
    String file;
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
    ValueType basic_var_type;       // basic variable type
    Uint8     var_attrib;           // variable attribute
};

// Forward declaration
struct AstNode;
struct AstExpr;
struct AstFunction;
struct AstFunctionArg;
struct AstLValue;
struct AstPrototype;
struct AstRegister;

// Concat ast nodes list
AstNode *append_sibling_node(AstNode *node, AstNode *next);

// Function attrib to string
String ast_function_attrib_to_string(AstFunctionAttrib attrib);

// Enum to string
const char* ast_node_type_to_string(AstNodeType nodeType);

// Operator to string
String ast_op_to_string(Uint32 op);

// VarType to string
String ast_var_type_to_string(AstVarType var_type);

// basic value to string
const char* value_type_to_string(ValueType value_type);

// The AST nodes
// AST node: abstract type
struct AstNode
{
    virtual AstNodeType get_node_type() = 0;
    AstNode* sibling;
    AstNode* children;
    SourceLocation location;

    AstNode() :
        sibling(0),
        children(0)
    {
    }

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

    // Output for thie AstNode
    virtual String to_string()
    {
        return EMPTY_STRING;
    }
};

// Root
struct AstRoot : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_ROOT; }
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

    virtual String to_string();

    AstCase() :
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
    AstVarType var_type; // variable type
    String     name;     // variable name
    AstExpr*   expr;     // assign expression

    virtual void collect_children()
    {
        collect_children_of(expr);
    }

    virtual String to_string();

    AstDeclaration() :
        name(EMPTY_STRING),
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

    AstDeclarations() :
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

    AstDoWhile() :
        block(0),
        cond(0)
    {
    }
};

// Primary Expression
// Abstract node
struct AstExpr : AstNode
{
    ValueType type;          // expression return type
    Uint8  attrib;           // Attribute of expression
    bool   is_constant;      // if this is a constant expression
    Int32  reg_index;        // expression output register index

    AstExpr() :
        type(MIXED),
        attrib(0),
        is_constant(false),
        reg_index(0)
    {
    }
};

// Assgin-op expression
struct AstExprAssign : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_ASSIGN; }
    Uint32     op;      // Expression operation
    AstLValue* expr1;   // Left expression
    AstExpr*   expr2;   // Right expression

    virtual void collect_children()
    {
        collect_children_of(expr1, expr2);
    }

    // eg. *=
    virtual String to_string();

    AstExprAssign() :
        op(0),
        expr1(0),
        expr2(0)
    {
    }
};

// Binary-op expression
struct AstExprBinary : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_BINARY; }
    Uint32   op;        // Expression operation
    AstExpr* expr1;     // Left expression
    AstExpr* expr2;     // Right expression

    virtual void collect_children()
    {
        collect_children_of(expr1, expr2);
    }

    // eg. +
    virtual String to_string();

    AstExprBinary() :
        op(0),
        expr1(0),
        expr2(0)
    {
    }
};

// Cast expression
struct AstExprCast : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CAST; }
    AstVarType var_type;  // Type to cast
    AstExpr*   expr1;     // To be operated

    virtual void collect_children()
    {
        collect_children_of(expr1);
    }

    virtual String to_string();

    AstExprCast() :
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

    AstExprClosure() :
        function(0)
    {
    }
};

// Expression element of constant 
struct AstExprConstant : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CONSTANT; }
    Value   value;          // Value

    virtual String to_string()
    {
        Output output;
        return output.type_value(&value);
    }

    AstExprConstant() :
        value(UNDEFINED)
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

    AstExprCreateArray()
    {
    }
};

// Expression element of creating function
struct AstExprCreateFunction : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_CREATE_FUNCTION; }
    String   name;
    AstExpr* expr_list;

    virtual void collect_children()
    {
        children = expr_list;
    }

    AstExprCreateFunction() :
        name(EMPTY_STRING)
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

    AstExprCreateMapping()
    {
    }
};

// Call a function
struct AstExprFunctionCall : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_FUNCTION_CALL; }
    AstExpr*   target;            // target object or array
    String     callee_name;       // call function name
    AstExpr*   arguments;         // arguments

    virtual void collect_children()
    {
        // The arguments is a list
        children = append_sibling_node(target, arguments);
    }

    virtual String to_string();

    AstExprFunctionCall() :
        callee_name(EMPTY_STRING),
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

    AstExprFunctionCallEx() :
        arguments(0)
    {
    }
};

// Get/Assign Index
struct AstExprIndex : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_INDEX; }
    AstExpr* container;         // container
    AstExpr* index_from;        // from index
    AstExpr* index_to;          // to index
    Int16    index_type;        // type of index
    bool     is_reverse_from;   // is from index reverse
    bool     is_reverse_to;     // is to index reverse

    virtual void collect_children()
    {
        // The arguments is a list
        collect_children_of(container, index_from, index_to);
    }

    virtual String to_string();

    AstExprIndex() :
        container(0),
        index_from(0),
        index_to(0),
        index_type(0),  // AST_INDEX_SINGLE
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
    String name;

    AstExprIsRef() :
        name(EMPTY_STRING)
    {

    }
};

// Get runtime value
struct AstExprRuntimeValue : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_RUNTIME_VALUE; }
    AstVarType var_type;
    Uint16     value_id;

    AstExprRuntimeValue() :
        value_id(0)
    {
    }
};

// Segement to
struct AstExprSegment : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_SEGMENT; }
    AstExpr*   expr;          // primary
    String     name;          // segement part

    virtual void collect_children()
    {
        // The arguments is a list
        collect_children_of(expr);
    }

    AstExprSegment() :
        name(EMPTY_STRING),
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

    AstExprSingleValue() :
        expr_list(0)
    {
    }
};

// Ternary-op expression
struct AstExprTernary : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_TERNARY; }
    Uint32   op;        // Expression operation
    AstExpr* expr1;     // 1st expression
    AstExpr* expr2;     // 2nd expression
    AstExpr* expr3;     // 3rd expression

    virtual void collect_children()
    {
        collect_children_of(expr1, expr2, expr3);
    }

    // eg. ?
    virtual String to_string();

    AstExprTernary() :
        op(0),
        expr1(0),
        expr2(0),
        expr3(0)
    {
    }
};

// Unary-op expression
struct AstExprUnary : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_UNARY; }
    Uint32   op;        // Expression operation
    AstExpr* expr1;     // To be operated

    virtual void collect_children()
    {
        collect_children_of(expr1);
    }

    // eg. --
    virtual String to_string();

    AstExprUnary() :
        op(0),
        expr1(0)
    {
    }
};

// Expression element of variable 
struct AstExprVariable : AstExpr
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_EXPR_VARIABLE; }
    String name;   // Variable name

    virtual String to_string();

    AstExprVariable() :
        name(EMPTY_STRING)
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

    AstForLoop() :
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

    // Prototype
    AstPrototype* prototype;

    // Function body
    AstNode* body;

    // All local variable declaration
    AstDeclaration* local_vars;

    // All registers
    AstRegister* registers;

    virtual void collect_children()
    {
        collect_children_of(prototype, body, local_vars, registers);
    }

    AstFunction() :
        body(0),
        local_vars(0),
        registers(0)
    {
    }
};

// Argument list with extra attributes of function prototype
struct AstFunctionArgsEx
{
    Int16 ddd;
    AstFunctionArg* arg_list;
};

// Function argument
struct AstFunctionArg : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_FUNCTION_ARG; }
    String     name;            // function argument name
    AstVarType var_type;        // argument variable type
    AstExpr*   default_value;   // default argument value

    virtual void collect_children()
    {
        collect_children_of(default_value);
    }

    virtual String to_string();

    AstFunctionArg() :
        name(EMPTY_STRING),
        default_value(0)
    {
    }
};

// Goto
// No child
struct AstGoto : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_GOTO; }
    Int16  goto_type;
    String target_label;     // Label to jmp

    virtual String to_string();

    AstGoto() :
        target_label(EMPTY_STRING)
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

    AstIfElse() :
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
    String name;

    virtual String to_string();

    AstLabel() :
        name(EMPTY_STRING)
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

    AstLoopEach() :
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

    virtual String to_string();

    AstLoopRange() :
        decl_or_variable(0),
        begin(0),
        to(0),
        direction(0)
    {
    }
};

// Left-value information
struct AstLValue : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_LVALUE; }
    AstExprVariable*    assigner_varible;// assignable value
    AstExpr*            index_from;      // from index
    AstExpr*            index_to;        // to index
    Int16               lvalue_type;     // type of L-Value
    bool                is_reverse_from; // is from index reverse
    bool                is_reverse_to;   // is to index reverse

    virtual void collect_children()
    {
        collect_children_of(assigner_varible, index_from, index_to);
    }

    AstLValue() :
        assigner_varible(0),
        index_from(0),
        index_to(0),
        is_reverse_from(false),
        is_reverse_to(false)
    {
    }
};

// Prototype: var_type function_name(arg_list)
struct AstPrototype : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_PROTOTYPE; }

    // Function name
    String name;

    // Function attrib
    Uint16 attrib;

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

    virtual String to_string();

    AstPrototype() :
        name(EMPTY_STRING),
        arg_list(0),
        has_ret(false)
    {
    }
};

// Register
// No child
struct AstRegister : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_REGISTER; }
    ValueType type;                        // register type
    Uint16    index;                       // register index
    bool      is_using;                    // is this register using currently
    bool      is_fixed_reg;                // is fixed register

    AstRegister() :
        type(MIXED),
        index(0),
        is_using(false),
        is_fixed_reg(false)
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
};

// Statements
struct AstStatements : AstNode
{
    virtual AstNodeType get_node_type() { return AstNodeType::AST_STATEMENTS; }
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

    AstSwitchCase() :
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

    AstWhileLoop() :
        cond(0),
        block(0)
    {
    }
};

}
