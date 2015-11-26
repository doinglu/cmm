// cmm_lang_const.cpp:
// Initial version 2011.07.16 by shenyq
// Immigrated 2015.11.3 by doing

#include <math.h>
#include "std_port/std_port.h"
#include "cmm_common_util.h"
#include "cmm_const.h"
#include "cmm_grammar.h"
#include "cmm_lang.h"
#include "cmm_lexer.h"
#include "cmm_value.h"

namespace cmm
{

Value          create_const_value(LangContext* context, AstElement* element);
Integer        const_to_int(LangContext* context, Value val); 
Real           const_to_real(LangContext* context, Value val);
String         const_to_string(LangContext* context, Value val);
bool           const_to_bool(LangContext* context, Value val);
AstElement* generate_const_to_element(LangContext* context, Value val);
AstElement* cast_const_to_number(LangContext* context, AstElement* element);
AstElement* cast_const_to_real(LangContext* context, AstElement* element);
AstElement* cast_const_to_string(LangContext* context, AstElement* element);
AstElement* add_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* sub_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* mul_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* mod_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* div_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* and_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* or_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* xor_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* neg_const(LangContext* context, AstElement* element);
AstElement* rev_const(LangContext* context, AstElement* element);
AstElement* ge_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* le_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* gt_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* lt_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* eq_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* ne_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* lsh_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* rsh_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* land_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* lor_const(LangContext* context, AstElement* element, AstElement* node2);
AstElement* lnot_const(LangContext* context, AstElement* element);
AstElement* select_const(LangContext* context, AstElement* element, AstElement* node2, AstElement* pNodes3);

/* Generate operation const */
AstExpression* LangContext::gen_op_const(Uint32 op, AstExpression* left, AstExpression* right)
{
    auto* context = this;
    AstElement* element = NULL;
    AstExpression* expression = NULL;
    AstElement* const_left = NULL, *const_right = NULL;

    STD_ASSERT(left || right);

    /* must be constant */
    if (left)
    {
        const_left = left->element;
        if (! const_left)
            return NULL;
    }

    if (right)
    {
        const_right = right->element;
        if (! const_right)
            return NULL;
    }

    if (const_left)
    {
        if (! const_left->is_constant)
            return NULL;
    }
        
    if (const_right)
    {
        if (! const_right->is_constant)
            return NULL;
    }
    
    switch (op)
    {
    case '+':
        element = add_const(context, const_left, const_right);
        break;

    case '-':
        if (const_left && const_right)
            /* sub */
            element = sub_const(context, const_left, const_right);
        else if (const_right)
            /* negativ */
            element = neg_const(context, const_right);

        break;

    case '*':
        element = mul_const(context, const_left, const_right);
        break;

    case '%':
        element = mod_const(context, const_left, const_right);
        break;

    case '/':
        element = div_const(context, const_left, const_right);
        break;

    case '&':
        element = and_const(context, const_left, const_right);
        break;

    case '|':
        element = or_const(context, const_left, const_right);
        break;

    case '^':
        element = xor_const(context, const_left, const_right);
        break;

    case '~':
        element = rev_const(context, const_right);
        break;

    case F_GE:
        element = ge_const(context, const_left, const_right);
        break;

    case F_LE:
        element = le_const(context, const_left, const_right);
        break;

    case F_GT:
        element = gt_const(context, const_left, const_right);
        break;

    case F_LT:
        element = lt_const(context, const_left, const_right);
        break;

    case L_EQ:
        element = eq_const(context, const_left, const_right);
        break;
    
    case L_NE:
        element = ne_const(context, const_left, const_right);
        break;

    case F_NE:
        element = ne_const(context, const_left, const_right);
        break;

    case L_LSH:
        element = lsh_const(context, const_left, const_right);
        break;

    case L_RSH:
        element = rsh_const(context, const_left, const_right);
        break;
        
    case L_LOR:
        element = lor_const(context, const_left, const_right);
        break;

    case L_LAND:
        element = land_const(context, const_left, const_right);
        break;

    case L_LNOT:
    case '!':
        element = lnot_const(context, const_right);
        break;

    default:

        STD_ASSERT(0 && "not supported const operation!");
    }

    expression = context->syntax_create_expression(element);
    expression->is_constant = 1;

    return expression;
}


/* cast const to type */
AstExpression* LangContext::gen_cast_const(AstExpression* exp, IntR type)
{
    auto* context = this;
    AstElement* element = NULL;
    AstElement* element = NULL;
    AstExpression* expression = NULL;

    if (! exp->element || ! exp->element->is_constant)
        return NULL;

    element = exp->element;
    
    switch (type)
    {
    case INTEGER:
        element = cast_const_to_number(context, element);
        break;

    case REAL:
        element = cast_const_to_real(context, element);
        break;

    case STRING:
        element = cast_const_to_string(context, element);
        break;

    default:
        /* Impossible to cast */
        return NULL;
    }

    expression = context->syntax_create_expression(element);
    expression->is_constant = 1;
    return expression;
}

/* Get value of const value */
/* Return integer/real/string in pointer */
Value create_const_value(LangContext* context, AstElement* element)
{
    STD_ASSERT(element->is_constant);

    switch (element->type)
    {
    case INTEGER:
        /* Got number */
        return (Integer)strtol64(element->output.c_str(), NULL, 10, true);

    case REAL:
        /* Got real */
        return (Real)atof(element->output.c_str());

    case STRING:
        /* Got string */
        return element->output.ptr();

    default:
        /* Impossible to here */
        STD_ASSERT(0);
        return 0;
    }
}

/* Convert const value to integer */
Integer const_to_int(LangContext* context, Value val)
{
    return (Integer)val;
}

/* Convert const value to real */
Real const_to_real(LangContext* context, Value val)
{
    return (Real)val;
}

/* Convert const value to string (char *) */
String const_to_string(LangContext* context, Value val)
{
    return (String)val;
}

/* Convert const value to string (1 or 0) */
bool const_to_bool(LangContext* context, Value val)
{
    return (bool)val;
}

/* Generate element by const value */
AstElement* generate_const_to_element(LangContext* context, Value val)
{
    char output[64];
    AstElement* element = context->syntax_create_element();

    element->is_constant = true;
    element->type = val.m_type;

    switch (val.m_type)
    {
    case INTEGER:
		int64_to_string(output, sizeof(output), val.m_int, 10, 1, 0);
        element->output = output;
        break;

    case REAL:
        snprintf(output, sizeof(output), "%.12g", (double)val.m_real);
        element->output = output;
        break;

    case STRING:
        element->output = val.m_string;
        break;
    
    default:

        /* can not be herer */
        STD_ASSERT(0);
    }

    return element;
}


/* Const operations */
/* cast type: (IntR) */
AstElement* cast_const_to_number(LangContext* context, AstElement* element)
{
    Value a;

    a = create_const_value(context, element);
    a = const_to_int(context, a);
    return generate_const_to_element(context, a);
}

/* cast type: (float) */
AstElement* cast_const_to_real(LangContext* context, AstElement* element)
{
    Value a;
    a = create_const_value(context, element);
    a = const_to_real(context, a);
    return generate_const_to_element(context, a);
}

/* cast type: (string) */
AstElement* cast_const_to_string(LangContext* context, AstElement* element)
{
    Value a;
    a = create_const_value(context, element);
    a = const_to_string(context, a);
    return generate_const_to_element(context, a);
}

/* a + b */
AstElement* add_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a + b;
    return generate_const_to_element(context, r);
}

/* a - b */
AstElement* sub_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a - b;
    return generate_const_to_element(context, r);
}

/* a * b */
AstElement* mul_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a * b;
    return generate_const_to_element(context, r);
}

/* a % b */
AstElement* mod_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a % b;
    return generate_const_to_element(context, r);
}

/* a / b */
AstElement* div_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a / b;
    return generate_const_to_element(context, r);
}

/* a & b */
AstElement* and_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a & b;
    return generate_const_to_element(context, r);
}

/* a | b */
AstElement* or_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a | b;
    return generate_const_to_element(context, r);
}

/* a ^ b */
AstElement* xor_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a ^ b;
    return generate_const_to_element(context, r);
}

/* -a */
AstElement* neg_const(LangContext* context, AstElement* element)
{
    Value a, r;

    a = create_const_value(context, element);
    r = -a;
    return generate_const_to_element(context, r);
}

/* ~a */
AstElement* rev_const(LangContext* context, AstElement* element)
{
    Value a, r;
    a = create_const_value(context, element);
    r = ~a;
    return generate_const_to_element(context, r);
}

/* a >= b */
AstElement* ge_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (a >= b);
    return generate_const_to_element(context, r);
}

/* a <= b */
AstElement* le_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (a <= b);
    return generate_const_to_element(context, r);
}

/* a > b */
AstElement* gt_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (a > b);
    return generate_const_to_element(context, r);
}

/* a < b */
AstElement* lt_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (a < b);
    return generate_const_to_element(context, r);
}

/* a == b */
AstElement* eq_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (a == b);
    return generate_const_to_element(context, a);
}

/* a != b */
AstElement* ne_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (a != b);
    return generate_const_to_element(context, r);
}

/* a << b */
AstElement* lsh_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a << b;
    return generate_const_to_element(context, r);
}

/* a >> b */
AstElement* rsh_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = a >> b;
    return generate_const_to_element(context, r);
}

/* a && b */
AstElement* land_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (const_to_bool(context, a) && const_to_bool(context, b));
    return generate_const_to_element(context, r);
}

/* a || b */
AstElement* lor_const(LangContext* context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(context, element);
    b = create_const_value(context, node2);
    r = (const_to_bool(context, a) || const_to_bool(context, b));
    return generate_const_to_element(context, r);
}

/* ! a */
AstElement* lnot_const(LangContext* context, AstElement* element)
{
    Value a, r;
    a = create_const_value(context, element);
    r = !const_to_bool(context, a);
    return generate_const_to_element(context, r);
}

/* a ? b : c */
AstElement* select_const(LangContext* context, AstElement* element, AstElement* node2, AstElement* node3)
{
    Value a;
    a = create_const_value(context, element);
    return const_to_bool(context, a) ? node2 : node3;
}

}
