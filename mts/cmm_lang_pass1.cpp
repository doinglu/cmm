// cmm_lang_pass1.cpp
// Initial version Feb/2/2016 by doing
// AST: pass1

#include "cmm_ast.h"
#include "cmm_buffer_new.h"
#include "cmm_lang.h"
#include "cmm_program.h"

namespace cmm
{

struct ExprOpType
{
    ValueType ret_type;
    Op        op;
    ValueType operand1_type;
    ValueType operand2_type;
    ValueType operand3_type;
};

// Prototypes of operator (To derive the result type)
ExprOpType expr_op_types[] =
{
    // Unary operation

    { INTEGER,  OP_REV,      INTEGER                     },
    { INTEGER,  OP_REV,      MIXED                       },

    { INTEGER,  OP_NOT,      ANY_TYPE                    },

    { INTEGER,  OP_NEG,      INTEGER                     },
    { REAL,     OP_NEG,      REAL                        },
    { MIXED,    OP_NEG,      MIXED                       },

    // Cast operation
    { INTEGER,  OP_CAST,     INTEGER,  NIL,              },
    { INTEGER,  OP_CAST,     INTEGER,  INTEGER,          },
    { INTEGER,  OP_CAST,     INTEGER,  REAL,             },
    { INTEGER,  OP_CAST,     INTEGER,  STRING,           },
    { INTEGER,  OP_CAST,     INTEGER,  BUFFER,           },
    { INTEGER,  OP_CAST,     INTEGER,  MIXED,            },
    { REAL,     OP_CAST,     REAL,     NIL,              },
    { REAL,     OP_CAST,     REAL,     INTEGER,          },
    { REAL,     OP_CAST,     REAL,     REAL,             },
    { REAL,     OP_CAST,     REAL,     STRING,           },
    { REAL,     OP_CAST,     REAL,     BUFFER,           },
    { REAL,     OP_CAST,     REAL,     MIXED,            },
    { OBJECT,   OP_CAST,     OBJECT,   OBJECT,           },
    { OBJECT,   OP_CAST,     OBJECT,   MIXED,            },
    { STRING,   OP_CAST,     STRING,   ANY_TYPE,         },
    { BUFFER,   OP_CAST,     BUFFER,   NIL,              },
    { BUFFER,   OP_CAST,     BUFFER,   INTEGER,          },
    { BUFFER,   OP_CAST,     BUFFER,   REAL,             },
    { BUFFER,   OP_CAST,     BUFFER,   STRING,           },
    { BUFFER,   OP_CAST,     BUFFER,   BUFFER,           },
    { BUFFER,   OP_CAST,     BUFFER,   MIXED,            },
    { FUNCTION, OP_CAST,     FUNCTION, FUNCTION,         },
    { FUNCTION, OP_CAST,     FUNCTION, MIXED,            },
    { ARRAY,    OP_CAST,     ARRAY,    NIL,              },
    { ARRAY,    OP_CAST,     ARRAY,    ARRAY,            },
    { ARRAY,    OP_CAST,     ARRAY,    MIXED,            },
    { MAPPING,  OP_CAST,     MAPPING,  NIL,              },
    { MAPPING,  OP_CAST,     MAPPING,  MAPPING,          },
    { MAPPING,  OP_CAST,     MAPPING,  MIXED,            },

    // Binary operation

    { INTEGER,  OP_ADD,      INTEGER,  INTEGER           },
    { REAL,     OP_ADD,      REAL,     INTEGER           },
    { REAL,     OP_ADD,      INTEGER,  REAL              },
    { REAL,     OP_ADD,      REAL,     REAL              },
    { STRING,   OP_ADD,      STRING,   INTEGER           },
    { STRING,   OP_ADD,      STRING,   REAL              },
    { STRING,   OP_ADD,      STRING,   STRING            },
    { STRING,   OP_ADD,      STRING,   MIXED             },
    { BUFFER,   OP_ADD,      BUFFER,   BUFFER            },
    { BUFFER,   OP_ADD,      BUFFER,   MIXED             },
    { ARRAY,    OP_ADD,      ARRAY,    ARRAY             },
    { ARRAY,    OP_ADD,      ARRAY,    MIXED             },
    { MAPPING,  OP_ADD,      MAPPING,  MAPPING           },
    { MAPPING,  OP_ADD,      MAPPING,  MIXED             },
    { MIXED,    OP_ADD,      MIXED,    ANY_TYPE          },
    { MIXED,    OP_ADD,      ANY_TYPE, MIXED             },

    { INTEGER,  OP_SUB,      INTEGER,  INTEGER           },
    { REAL,     OP_SUB,      REAL,     INTEGER           },
    { REAL,     OP_SUB,      INTEGER,  REAL              },
    { REAL,     OP_SUB,      REAL,     REAL              },
    { ARRAY,    OP_SUB,      ARRAY,    ARRAY             },
    { ARRAY,    OP_SUB,      ARRAY,    MIXED             },
    { MAPPING,  OP_SUB,      MAPPING,  MAPPING           },
    { MAPPING,  OP_SUB,      MAPPING,  MIXED             },
    { MIXED,    OP_SUB,      MIXED,    ANY_TYPE          },
    { MIXED,    OP_SUB,      ANY_TYPE, MIXED             },

    { INTEGER,  OP_MUL,      INTEGER,  INTEGER           },
    { REAL,     OP_MUL,      REAL,     INTEGER           },
    { REAL,     OP_MUL,      INTEGER,  REAL              },
    { REAL,     OP_MUL,      REAL,     REAL              },
    { MIXED,    OP_MUL,      MIXED,    ANY_TYPE          },
    { MIXED,    OP_MUL,      ANY_TYPE, MIXED             },

    { INTEGER,  OP_DIV,      INTEGER,  INTEGER           },
    { REAL,     OP_DIV,      REAL,     INTEGER           },
    { REAL,     OP_DIV,      INTEGER,  REAL              },
    { REAL,     OP_DIV,      REAL,     REAL              },
    { MIXED,    OP_DIV,      MIXED,    ANY_TYPE          },
    { MIXED,    OP_DIV,      ANY_TYPE, MIXED             },

    { INTEGER,  OP_MOD,      INTEGER,  INTEGER           },
    { REAL,     OP_MOD,      REAL,     INTEGER           },
    { REAL,     OP_MOD,      INTEGER,  REAL              },
    { REAL,     OP_MOD,      REAL,     REAL              },
    { MIXED,    OP_MOD,      MIXED,    ANY_TYPE          },
    { MIXED,    OP_MOD,      ANY_TYPE, MIXED             },

    { INTEGER,  OP_LSH,      INTEGER,  INTEGER           },
    { INTEGER,  OP_LSH,      INTEGER,  MIXED             },
    { MAPPING,  OP_LSH,      MAPPING,  MAPPING           },
    { MAPPING,  OP_LSH,      MAPPING,  MIXED             },
    { INTEGER,  OP_LSH,      MIXED,    INTEGER           },
    { MAPPING,  OP_LSH,      MIXED,    MAPPING           },
    { MIXED,    OP_LSH,      MIXED,    MIXED             },

    { INTEGER,  OP_RSH,      INTEGER,  INTEGER           },
    { INTEGER,  OP_RSH,      INTEGER,  MIXED             },
    { INTEGER,  OP_RSH,      MIXED,    INTEGER           },
    { INTEGER,  OP_RSH,      MIXED,    MIXED             },

    { INTEGER,  OP_AND,      INTEGER,  INTEGER           },
    { INTEGER,  OP_AND,      INTEGER,  MIXED             },
    { INTEGER,  OP_AND,      MIXED,    INTEGER           },
    { ARRAY,    OP_AND,      ARRAY,    ARRAY             },
    { ARRAY,    OP_AND,      ARRAY,    MIXED             },
    { ARRAY,    OP_AND,      MIXED,    ARRAY             },
    { MIXED,    OP_AND,      MIXED,    MIXED             },

    { INTEGER,  OP_OR,       INTEGER,  INTEGER           },
    { INTEGER,  OP_OR,       INTEGER,  MIXED             },
    { INTEGER,  OP_OR,       MIXED,    INTEGER           },
    { ARRAY,    OP_OR,       ARRAY,    ARRAY             },
    { ARRAY,    OP_OR,       ARRAY,    MIXED             },
    { ARRAY,    OP_OR,       MIXED,    ARRAY             },
    { MIXED,    OP_OR,       MIXED,    MIXED             },

    { INTEGER,  OP_XOR,      INTEGER,  INTEGER           },
    { INTEGER,  OP_XOR,      INTEGER,  MIXED             },
    { INTEGER,  OP_XOR,      MIXED,    INTEGER           },
    { ARRAY,    OP_XOR,      ARRAY,    ARRAY             },
    { ARRAY,    OP_XOR,      ARRAY,    MIXED             },
    { ARRAY,    OP_XOR,      MIXED,    ARRAY             },
    { MIXED,    OP_XOR,      MIXED,    MIXED             },

    { MIXED,    OP_DOLLAR,   INTEGER                     },
    { MIXED,    OP_DOLLAR,   MIXED                       },

    { INTEGER,  OP_EQ,       ANY_TYPE, ANY_TYPE          },

    { INTEGER,  OP_NE,       ANY_TYPE, ANY_TYPE          },

    { INTEGER,  OP_ORDER,    INTEGER,  INTEGER           },
    { INTEGER,  OP_ORDER,    INTEGER,  REAL              },
    { INTEGER,  OP_ORDER,    INTEGER,  MIXED             },
    { INTEGER,  OP_ORDER,    REAL,     INTEGER           },
    { INTEGER,  OP_ORDER,    REAL,     REAL              },
    { INTEGER,  OP_ORDER,    REAL,     MIXED             },
    { INTEGER,  OP_ORDER,    STRING,   STRING            },
    { INTEGER,  OP_ORDER,    STRING,   MIXED             },
    { INTEGER,  OP_ORDER,    BUFFER,   BUFFER            },
    { INTEGER,  OP_ORDER,    BUFFER,   MIXED             },
    { INTEGER,  OP_ORDER,    MIXED,    INTEGER           },
    { INTEGER,  OP_ORDER,    MIXED,    REAL              },
    { INTEGER,  OP_ORDER,    MIXED,    STRING            },
    { INTEGER,  OP_ORDER,    MIXED,    BUFFER            },
    { INTEGER,  OP_ORDER,    MIXED,    MIXED             },

    { INTEGER,  OP_LAND,     ANY_TYPE, INTEGER           },
    { REAL,     OP_LAND,     ANY_TYPE, REAL              },
    { OBJECT,   OP_LAND,     ANY_TYPE, OBJECT            },
    { STRING,   OP_LAND,     ANY_TYPE, STRING            },
    { BUFFER,   OP_LAND,     ANY_TYPE, BUFFER            },
    { ARRAY,    OP_LAND,     ANY_TYPE, ARRAY             },
    { MAPPING,  OP_LAND,     ANY_TYPE, MAPPING           },
    { MIXED,    OP_LAND,     ANY_TYPE, ANY_TYPE          },

    { INTEGER,  OP_LOR,      INTEGER,  INTEGER           },
    { REAL,     OP_LOR,      REAL,     REAL              },
    { OBJECT,   OP_LOR,      OBJECT,   OBJECT            },
    { STRING,   OP_LOR,      STRING,   STRING            },
    { BUFFER,   OP_LOR,      BUFFER,   BUFFER,           },
    { ARRAY,    OP_LOR,      ARRAY,    ARRAY,            },
    { MAPPING,  OP_LOR,      MAPPING,  MAPPING,          },
    { MIXED,    OP_LOR,      ANY_TYPE, ANY_TYPE          },

    { INTEGER,  OP_INC_PRE,  INTEGER                     },
    { REAL,     OP_INC_PRE,  REAL                        },
    { MIXED,    OP_INC_PRE,  MIXED                       },

    { INTEGER,  OP_DEC_PRE,  INTEGER                     },
    { REAL,     OP_DEC_PRE,  REAL                        },
    { MIXED,    OP_DEC_PRE,  MIXED                       },

    { INTEGER,  OP_INC_POST, INTEGER                     },
    { REAL,     OP_INC_POST, REAL                        },
    { MIXED,    OP_INC_POST, MIXED },

    { INTEGER,  OP_DEC_POST, INTEGER                     },
    { REAL,     OP_DEC_POST, REAL                        },
    { MIXED,    OP_DEC_POST, MIXED                       },

    // Ternary operation

    { INTEGER,  OP_QMARK,    ANY_TYPE, INTEGER,  INTEGER },
    { REAL,     OP_QMARK,    ANY_TYPE, REAL,     REAL    },
    { OBJECT,   OP_QMARK,    ANY_TYPE, OBJECT,   OBJECT  },
    { FUNCTION, OP_QMARK,    ANY_TYPE, FUNCTION, FUNCTION},
    { STRING,   OP_QMARK,    ANY_TYPE, STRING,   STRING  },
    { BUFFER,   OP_QMARK,    ANY_TYPE, BUFFER,   BUFFER  },
    { ARRAY,    OP_QMARK,    ANY_TYPE, ARRAY,    ARRAY   },
    { MAPPING,  OP_QMARK,    ANY_TYPE, MAPPING,  MAPPING },
    { MIXED,    OP_QMARK,    ANY_TYPE, ANY_TYPE, ANY_TYPE},

    // Subscript index

    { INTEGER,  OP_IDX,      STRING,   INTEGER           },
    { INTEGER,  OP_IDX,      STRING,   MIXED             },
    { INTEGER,  OP_IDX,      BUFFER,   INTEGER           },
    { INTEGER,  OP_IDX,      BUFFER,   MIXED             },
    { MIXED,    OP_IDX,      ARRAY,    INTEGER           },
    { MIXED,    OP_IDX,      ARRAY,    MIXED             },
    { MIXED,    OP_IDX,      MAPPING,  ANY_TYPE          },

    { STRING,   OP_IDX_RANGE,STRING,   INTEGER, INTEGER  },
    { STRING,   OP_IDX_RANGE,STRING,   INTEGER, MIXED    },
    { STRING,   OP_IDX_RANGE,STRING,   MIXED,   INTEGER  },
    { STRING,   OP_IDX_RANGE,STRING,   MIXED,   MIXED    },
    { BUFFER,   OP_IDX_RANGE,BUFFER,   INTEGER, INTEGER  },
    { BUFFER,   OP_IDX_RANGE,BUFFER,   INTEGER, MIXED    },
    { BUFFER,   OP_IDX_RANGE,BUFFER,   MIXED,   INTEGER  },
    { BUFFER,   OP_IDX_RANGE,BUFFER,   MIXED,   MIXED    },
    { ARRAY,    OP_IDX_RANGE,ARRAY,    INTEGER, INTEGER  },
    { ARRAY,    OP_IDX_RANGE,ARRAY,    INTEGER, MIXED    },
    { ARRAY,    OP_IDX_RANGE,ARRAY,    MIXED,   INTEGER  },
    { ARRAY,    OP_IDX_RANGE,ARRAY,    MIXED,   MIXED    },
};

struct AssignOpMap
{
    Op assign_op;
    Op op;
};

AssignOpMap assign_op_map[] =
{
    OP_ADD_EQ,  OP_ADD,
    OP_SUB_EQ,  OP_SUB,
    OP_MUL_EQ,  OP_MUL,
    OP_DIV_EQ,  OP_DIV,
    OP_MOD_EQ,  OP_MOD,
    OP_AND_EQ,  OP_AND,
    OP_OR_EQ,   OP_OR,
    OP_XOR_EQ,  OP_XOR,
    OP_RSH_EQ,  OP_RSH,
    OP_LSH_EQ,  OP_LSH,
};

struct HashOpFunc
{
    size_t operator()(Op op) const { return (size_t)op; }
};
typedef simple::hash_map<Op, int, HashOpFunc> OpStartIndex;
OpStartIndex *op_start_index = 0;

// Initialize the index for each op
bool Lang::init_expr_op_type()
{
    op_start_index = XNEW(OpStartIndex);

    // Gather index for op
    Op current_op = (Op)0;
    for (auto i = 0; i < STD_SIZE_N(expr_op_types); i++)
    {
        // Default operand type is ANY_TYPE (complete the struct fields)

        if (!expr_op_types[i].operand2_type)
            expr_op_types[i].operand2_type = ANY_TYPE;

        if (!expr_op_types[i].operand3_type)
            expr_op_types[i].operand3_type = ANY_TYPE;

        if (current_op == expr_op_types[i].op)
            continue;
        // Got new op
        current_op = expr_op_types[i].op;
        op_start_index->put(current_op, i);
    }
    return true;
}

void Lang::destruct_expr_op_type()
{
    XDELETE(op_start_index);
}

// Lookup the AST & get all object var and functions
bool Lang::pass1()
{
    // Init symbol table for object var & functions
    init_symbol_table();

    // Lookup the AST from top to bottom
    // Work out all definitions
    lookup_and_map_identifiers(m_root);

    // Lookup the AST from bottom to top
    // Work out type of all expressions
    lookup_expr_types(m_root);

    // Lookup functions in whole AST
    if (m_error_code != ErrorCode::OK)
        // Failed to define functions
        return false;

    return true;
}

// Put all object var & object's functions to symbol table
void Lang::init_symbol_table()
{
    for (auto it : m_object_vars)
    {
        auto* info = BUFFER_NEW(IdentInfo, this);
        info->object_var_no = it->object_var_no;
        info->type = IDENT_OBJECT_VAR;
        info->decl = it;
        m_symbols.add_ident_info(it->name, info, it);
    }

    for (auto it : m_functions)
    {
        auto* prototype = it->prototype;
        if (!(prototype->attrib & AST_MEMBER_METHOD))
            continue;
        auto* info = BUFFER_NEW(IdentInfo, this);
        info->function_no = it->no;
        info->type = IDENT_OBJECT_FUN;
        info->function = it;
        m_symbols.add_ident_info(prototype->name, info, it);
    }
}

// Lookup the whole AST from top to bottom
// For those identifiers such as variables & functions, find out
// their information & update to AST node
void Lang::lookup_and_map_identifiers(AstNode *node)
{
    bool new_frame = node->contains_new_frame();

    if (new_frame)
        create_new_frame();

    // Get context function
    auto* in_function = m_functions[node->in_function_no];
    switch (node->get_node_type())
    {
    case AST_DECLARATION:
    {
        auto* info = BUFFER_NEW(IdentInfo, this);
        auto* decl = (AstDeclaration*)node;
        if (is_in_top_frame())
        {
            // This is a object var
            STD_ASSERT(("Top frame must be in entry function",
                        in_function == m_entry_function));
            info->type = IDENT_OBJECT_VAR;
            info->object_var_no = (VariableNo)m_object_vars.size();
            m_object_vars.push_back(decl);
            decl->object_var_no = info->object_var_no;
        } else
        {
            // This is a local var
            info->type = IDENT_LOCAL_VAR;
            info->local_var_no = (LocalNo)in_function->local_vars.size();
            in_function->local_vars.push_back(decl);
            decl->local_var_no = info->local_var_no;
        }
        info->decl = decl;
        decl->ident_type = info->type;
        m_symbols.add_ident_info(decl->name, info, node);
        break;
    }

    case AST_FUNCTION:
    {
        auto* function = (AstFunction*)node;
        if (function->prototype->attrib & AST_ANONYMOUS_CLOSURE)
            // Skip anonymouse closure, don't add into symbol table
            break;
        if (function->prototype->attrib & AST_MEMBER_METHOD)
            // Member method, already added into symbol table
            break;
        auto* info = BUFFER_NEW(IdentInfo, this);
        info->function_no = function->no;
        info->type = IDENT_OBJECT_FUN;
        m_symbols.add_ident_info(function->prototype->name, info, node);
        break;
    }

    case AST_EXPR_VARIABLE:
    {
        // Get var type of variable
        auto* variable = (AstExprVariable*)node;
        auto* info = m_symbols.get_ident_info(variable->name, IDENT_ALL);
        if (!info)
        {
            this->syntax_errors(this,
                "%s(%d): error %d: '%s': undeclared identifier\n",
                node->location.file.c_str(), node->location.line,
                C_UNDECLARED_IDENTIFER,
                variable->name.c_str());
            this->m_error_code = PASS1_ERROR;
            break;
        }
        if (!(info->type & IDENT_VAR))
        {
            this->syntax_errors(this,
                "%s(%d): error %d: '%s': identifier is not variable\n",
                node->location.file.c_str(), node->location.line,
                C_UNDECLARED_IDENTIFER,
                variable->name.c_str());
            this->m_error_code = PASS1_ERROR;
            break;
        }
        variable->var_type = info->decl->var_type;
        variable->is_constant = false; // Variant, not constant
        break;
    }

    default:
        // Don't process for other nodes
        break;
    }

    for (auto* p = node->children; p != 0; p = p->sibling)
        lookup_and_map_identifiers(p);

    if (new_frame)
        // Destruct the frame & definitions
        destruct_current_frame();
}

// Lookup the whole AST from bottom to top
// Derive expr types
void Lang::lookup_expr_types(AstNode *node)
{
    for (auto* p = node->children; p != 0; p = p->sibling)
        lookup_expr_types(p);

    switch (node->get_node_type())
    {
        case AST_EXPR_CONSTANT:
        {
            // Get constant value type
            auto* expr = (AstExprConstant*)node;
            expr->var_type.basic_var_type = expr->value.m_type;
            expr->var_type.var_attrib = 0;
            expr->is_constant = true;
            break;
        }
        case AST_EXPR_CLOSURE :
        {
            // The closure is a function
            auto* expr = (AstExprClosure*)node;
            expr->var_type.basic_var_type = FUNCTION;
            expr->var_type.var_attrib = 0;
            break;
        }
        case AST_EXPR_ASSIGN:
        {
            // Get acceptable assigment var type

            auto* expr = (AstExprAssign*)node;

            // Is expr1 a valid lvalue?
            check_lvalue(expr->expr1);

            AstVarType result_var_type;
            auto operand1_attrib = expr->expr1->var_type.var_attrib;
            auto operand1_type   = expr->expr1->var_type.basic_var_type;
            auto operand2_type   = expr->expr2->var_type.basic_var_type;

            if (expr->op == OP_QMARK_EQ)
            {
                // Assign non-null value & return the value
                // eq. int a; a ?= NIL;
                // ATTENTION:
                // The result of type v ?= expr is same as type? v = expr,
                // but if the expr is NIL, the v won't be assigned to the value
                // eg. a ?= NIL == NIL
                result_var_type = expr->expr2->var_type;
                operand1_attrib |= AST_VAR_MAY_NIL;
            } else
            if (expr->op == OP_ASSIGN)
            {
                // Assign directly
                // eg. int a; a = 12;
                result_var_type = expr->expr2->var_type;
            } else
            {
                // Operate then assign
                Op map_op;
                if (!try_map_assign_op_to_op(expr->op, &map_op))
                    STD_FATAL("Not found such assign operator.");

                result_var_type.basic_var_type =
                    derive_type_of_op(expr, map_op, operand1_type, operand2_type, ANY_TYPE);
                result_var_type.var_attrib = 0;
            }

            // Check type to assign
            if (result_var_type.basic_var_type == MIXED)
            {
                // Assign a MIXED to lvalue, keep the left possible type
                result_var_type.basic_var_type = operand1_type;
                result_var_type.var_attrib = operand1_attrib;
            } else
            if (operand1_type == MIXED)
            {
                // Accept any type, keep the result_var_type
            } else
            if (result_var_type.basic_var_type == NIL)
            {
                // Result is NIL
                if (!(operand1_attrib & AST_VAR_MAY_NIL))
                {
                    // Not accept nil
                    this->syntax_errors(this,
                        "%s(%d): error %d: cannot assign %s with NIL\n",
                        expr->location.file.c_str(), expr->location.line,
                        C_CANNOT_OPER,
                        ast_var_type_to_string(expr->expr1->var_type).c_str());
                }
            } else
            if (operand1_type != result_var_type.basic_var_type)
            {
                // Assign with unacceptable type and not accept NIL
                this->syntax_errors(this,
                    "%s(%d): error %d: cannot assign %s to %s\n",
                    expr->location.file.c_str(), expr->location.line,
                    C_CANNOT_OPER,
                    ast_var_type_to_string(result_var_type).c_str(),
                    ast_var_type_to_string(expr->expr1->var_type).c_str());
            }

            // Got result type
            expr->var_type = result_var_type;
            break;
        }
        case AST_EXPR_CAST:
        {
            // Get cast to var type

            auto* expr = (AstExprCast*)node;
            expr->is_constant = expr->expr1->is_constant;
            ValueType to_type = expr->var_type.basic_var_type;
            if (expr->var_type.var_attrib & AST_VAR_MAY_NIL)
            {
                this->syntax_warns(this,
                    "%s(%d): warning %d: cast to %s? is useless\n",
                    expr->location.file.c_str(), expr->location.line,
                    C_CAST_USELESS_QMARK,
                    value_type_to_c_str(to_type));
                expr->var_type.var_attrib &= ~AST_VAR_MAY_NIL;
            }
            if (!(expr->var_type.var_attrib & AST_VAR_CONST) &&
                (expr->expr1->var_type.var_attrib & AST_VAR_CONST))
            {
                this->syntax_errors(this,
                    "%s(%d): warning %d: cast const to non-const is forbidden\n",
                    expr->location.file.c_str(), expr->location.line,
                    C_CAST_TO_NON_CONST);
                expr->var_type.var_attrib |= AST_VAR_CONST;
            }
            ValueType from_type = expr->expr1->var_type.basic_var_type;
            // Check valid cast
            derive_type_of_op(expr, OP_CAST, to_type, from_type, ANY_TYPE);
            break;
        }
        case AST_EXPR_BINARY:
        case AST_EXPR_TERNARY:
        case AST_EXPR_UNARY:
        {
            // Get operator result var type by operands prototype

            auto* expr = (AstExprOp*)node;
            expr->is_constant = are_expr_list_constant((AstExpr*)expr->children);

            // Get children's type
            AstExpr* operand_node;
            ValueType operand1_type, operand2_type = ANY_TYPE, operand3_type = ANY_TYPE;
            operand_node = (AstExpr*)node->children;
            operand1_type = operand_node->var_type.basic_var_type;
            if ((operand_node = (AstExpr*)operand_node->sibling) != 0)
            {
                operand2_type = operand_node->var_type.basic_var_type;
                if ((operand_node = (AstExpr*)operand_node->sibling) != 0)
                    operand3_type = operand_node->var_type.basic_var_type;
            }

            // Derive & set var_type to expr
            expr->var_type.basic_var_type =
                derive_type_of_op(expr, expr->op, operand1_type, operand2_type, operand3_type);
            break;
        }
        case AST_EXPR_CREATE_ARRAY:
        {
            auto* expr = (AstExprCreateArray*)node;
            expr->var_type.basic_var_type = ARRAY;
            expr->is_constant = are_expr_list_constant(expr->expr_list);
            break;
        }
        case AST_EXPR_CREATE_FUNCTION:
        {
            auto* expr = (AstExprCreateFunction*)node;
            expr->var_type.basic_var_type = FUNCTION;
            break;
        }
        case AST_EXPR_CREATE_MAPPING:
        {
            auto* expr = (AstExprCreateMapping*)node;
            expr->var_type.basic_var_type = MAPPING;
            expr->is_constant = are_expr_list_constant(expr->expr_list);
            break;
        }
        case AST_EXPR_INDEX:
        {
            auto* expr = (AstExprIndex*)node;
            expr->is_constant = are_expr_list_constant((AstExpr*)expr->children);

            ValueType operand1_type, operand2_type, operand3_type = ANY_TYPE;
            operand1_type = expr->container->var_type.basic_var_type;
            operand2_type = expr->index_from->var_type.basic_var_type;
            if (expr->index_to)
                operand3_type = expr->index_to->var_type.basic_var_type;

            // Derive & set var_type to expr
            expr->var_type.basic_var_type =
                derive_type_of_op(expr, expr->op, operand1_type, operand2_type, operand3_type);
            break;
        }
        case AST_EXPR_SINGLE_VALUE:
        {
            // Get var type of last expr in list

            auto* expr = (AstExprSingleValue*)node;

            // Get last child
            auto* p = expr->children;
            if (p->sibling)
            {
                while (p->sibling->sibling)
                    p = p->sibling;
            }

            expr->var_type = ((AstExpr*)p)->var_type;
        }
        default:
            // Skip
            break;
    }
}

// Are all the children constant expr?
bool Lang::are_expr_list_constant(AstExpr* expr_list)
{
    auto* p = expr_list;
    while (p)
    {
        if (!p->is_constant)
            // Not constant
            return false;
        p = (AstExpr*)p->sibling;
    }
    // Yes
    return true;
}

// Work out the type of op result
// Attention: op != expr->op for AstExprAssign
ValueType Lang::derive_type_of_op(
    AstExprOp* expr, Op op,
    ValueType operand1_type, ValueType operand2_type, ValueType operand3_type
)
{
    int start_index;
    Op  key_op;

    // Convert LT/LE/GT/GE to OP_ORDER
    switch (op)
    {
        case OP_LT: key_op = OP_ORDER; break;
        case OP_LE: key_op = OP_ORDER; break;
        case OP_GT: key_op = OP_ORDER; break;
        case OP_GE: key_op = OP_ORDER; break;
        default:    key_op = op;       break;
    }

    if (!op_start_index->try_get(key_op, &start_index))
        STD_FATAL("Not found such operator.");

    // Try to find the matched operator prototype
    for (auto i = start_index; i < STD_SIZE_N(expr_op_types); i++)
    {
        auto* op_prototype = &expr_op_types[i];
        if (op_prototype->op != op)
            break;

        if ((op_prototype->operand1_type == operand1_type || op_prototype->operand1_type == ANY_TYPE) &&
            (op_prototype->operand2_type == operand2_type || op_prototype->operand2_type == ANY_TYPE) &&
            (op_prototype->operand3_type == operand3_type || op_prototype->operand3_type == ANY_TYPE))
        {
            return op_prototype->ret_type;
        }
    }

    size_t opern = expr->get_children_count();
    // Not expected operator, stop scanning
    if (opern == 1)
    {
        this->syntax_errors(this,
            "%s(%d): error %d: cannot '%s' %s\n",
            expr->location.file.c_str(), expr->location.line,
            C_CANNOT_OPER,
            ast_op_to_string(expr->op).c_str(),
            value_type_to_c_str(operand1_type));
    }
    else
    if (opern == 2)
    {
        this->syntax_errors(this,
            "%s(%d): error %d: cannot '%s' %s with %s\n",
            expr->location.file.c_str(), expr->location.line,
            C_CANNOT_OPER,
            ast_op_to_string(expr->op).c_str(),
            value_type_to_c_str(operand1_type),
            value_type_to_c_str(operand2_type));
    } else
    {
        this->syntax_errors(this,
            "%s(%d): error %d: cannot '%s' %s and %s, %s\n",
            expr->location.file.c_str(), expr->location.line,
            C_CANNOT_OPER,
            ast_op_to_string(expr->op).c_str(),
            value_type_to_c_str(operand1_type),
            value_type_to_c_str(operand2_type),
            value_type_to_c_str(operand3_type));
    }
    return MIXED;
}

// Check an expr for lvalue
bool Lang::check_lvalue(AstExpr* node)
{
    switch (node->get_node_type())
    {
        case AST_EXPR_VARIABLE:
            // OK
            return true;

        case AST_EXPR_INDEX:
        {
            // OK
            auto* expr_index = (AstExprIndex*)node;
           
            if (expr_index->var_type.is_const())
            {
                this->syntax_errors(this,
                    "%s(%d): error %d: you cannot assign to a variable that is const\n",
                    node->location.file.c_str(), node->location.line,
                    C_ASSIGN_TO_CONST);
                return false;
            }

            if (expr_index->container->is_constant)
            {
                this->syntax_errors(this,
                    "%s(%d): error %d: you cannot assign to a variable that is const\n",
                    node->location.file.c_str(), node->location.line,
                    C_ASSIGN_TO_CONST);
                return false;
            }

            if (expr_index->var_type.basic_var_type == STRING)
            {
                this->syntax_errors(this,
                    "%s(%d): error %d: you cannot modify char(s) in string\n",
                    node->location.file.c_str(), node->location.line,
                    C_ASSIGN_TO_CONST);
                return false;
            }
            return true;
        }

        default:
            // Not L-value
            break;
    }

    this->syntax_errors(this,
        "%s(%d): error %d: '=': left operand must be l-value\n",
        node->location.file.c_str(), node->location.line,
        C_NOT_LVALUE);
    return false;
}

// Get op from assign op
// += to +
// *= to * and so on
bool Lang::try_map_assign_op_to_op(Op assign_op, Op* out_op)
{
    for (auto i = 0; i < STD_SIZE_N(assign_op_map); i++)
        if (assign_op_map[i].assign_op == assign_op)
        {
            *out_op = assign_op_map[i].op;
            return true;
        }

    return (Op)false;
}

}
