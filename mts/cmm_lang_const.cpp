// cmm_lang_const.cpp:
// Initial version 2011.07.16 by shenyq
// Immigrated 2015.11.3 by doing

#include <math.h>
#include "std_port/std_port.h"
#include "cmm_common_util.h"
#include "cmm_lang.h"
#include "cmm_lang_lexer.h"
#include "cmm_value.h"

#if 0
namespace cmm
{
#include "cmm_grammar.h"

Value          create_const_value(Lang* lang_context, AstElement* element);
Integer        const_to_int(Lang* lang_context, Value val);
Real           const_to_real(Lang* lang_context, Value val);
String         const_to_string(Lang* lang_context, Value val);
bool           const_to_bool(Lang* lang_context, Value val);
AstElement* generate_const_to_element(Lang* lang_context, Value val);
AstElement* cast_const_to_number(Lang* lang_context, AstElement* element);
AstElement* cast_const_to_real(Lang* lang_context, AstElement* element);
AstElement* cast_const_to_string(Lang* lang_context, AstElement* element);
AstElement* add_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* sub_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* mul_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* mod_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* div_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* and_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* or_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* xor_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* neg_const(Lang* lang_context, AstElement* element);
AstElement* rev_const(Lang* lang_context, AstElement* element);
AstElement* ge_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* le_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* gt_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* lt_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* eq_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* ne_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* lsh_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* rsh_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* land_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* lor_const(Lang* lang_context, AstElement* element, AstElement* node2);
AstElement* lnot_const(Lang* lang_context, AstElement* element);
AstElement* select_const(Lang* lang_context, AstElement* element, AstElement* node2, AstElement* pNodes3);

/* Generate operation const */
AstExpression* Lang::gen_op_const(Uint32 op, AstExpression* left, AstExpression* right)
{
    auto* lang_context = this;
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
        element = add_const(lang_context, const_left, const_right);
        break;

    case '-':
        if (const_left && const_right)
            /* sub */
            element = sub_const(lang_context, const_left, const_right);
        else if (const_right)
            /* negativ */
            element = neg_const(lang_context, const_right);

        break;

    case '*':
        element = mul_const(lang_context, const_left, const_right);
        break;

    case '%':
        element = mod_const(lang_context, const_left, const_right);
        break;

    case '/':
        element = div_const(lang_context, const_left, const_right);
        break;

    case '&':
        element = and_const(lang_context, const_left, const_right);
        break;

    case '|':
        element = or_const(lang_context, const_left, const_right);
        break;

    case '^':
        element = xor_const(lang_context, const_left, const_right);
        break;

    case '~':
        element = rev_const(lang_context, const_right);
        break;

    case F_GE:
        element = ge_const(lang_context, const_left, const_right);
        break;

    case F_LE:
        element = le_const(lang_context, const_left, const_right);
        break;

    case F_GT:
        element = gt_const(lang_context, const_left, const_right);
        break;

    case F_LT:
        element = lt_const(lang_context, const_left, const_right);
        break;

    case L_EQ:
        element = eq_const(lang_context, const_left, const_right);
        break;
    
    case L_NE:
        element = ne_const(lang_context, const_left, const_right);
        break;

    case F_NE:
        element = ne_const(lang_context, const_left, const_right);
        break;

    case L_LSH:
        element = lsh_const(lang_context, const_left, const_right);
        break;

    case L_RSH:
        element = rsh_const(lang_context, const_left, const_right);
        break;
        
    case L_LOR:
        element = lor_const(lang_context, const_left, const_right);
        break;

    case L_LAND:
        element = land_const(lang_context, const_left, const_right);
        break;

    case L_LNOT:
    case '!':
        element = lnot_const(lang_context, const_right);
        break;

    default:

        STD_ASSERT(0 && "not supported const operation!");
    }

    expression = lang_context->syntax_create_expression(element);
    expression->is_constant = 1;

    return expression;
}


/* cast const to type */
AstExpression* Lang::gen_cast_const(AstExpression* exp, IntR type)
{
    auto* lang_context = this;
    AstElement* element = NULL;
    AstElement* element = NULL;
    AstExpression* expression = NULL;

    if (! exp->element || ! exp->element->is_constant)
        return NULL;

    element = exp->element;
    
    switch (type)
    {
    case INTEGER:
        element = cast_const_to_number(lang_context, element);
        break;

    case REAL:
        element = cast_const_to_real(lang_context, element);
        break;

    case STRING:
        element = cast_const_to_string(lang_context, element);
        break;

    default:
        /* Impossible to cast */
        return NULL;
    }

    expression = lang_context->syntax_create_expression(element);
    expression->is_constant = 1;
    return expression;
}

/* Get value of const value */
/* Return integer/real/string in pointer */
Value create_const_value(Lang* lang_context, AstElement* element)
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
Integer const_to_int(Lang* lang_context, Value val)
{
    return (Integer)val;
}

/* Convert const value to real */
Real const_to_real(Lang* lang_context, Value val)
{
    return (Real)val;
}

/* Convert const value to string (char *) */
String const_to_string(Lang* lang_context, Value val)
{
    return (String)val;
}

/* Convert const value to string (1 or 0) */
bool const_to_bool(Lang* lang_context, Value val)
{
    return (bool)val;
}

/* Generate element by const value */
AstElement* generate_const_to_element(Lang* lang_context, Value val)
{
    char output[64];
    AstElement* element = lang_context->syntax_create_element();

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
AstElement* cast_const_to_number(Lang* lang_context, AstElement* element)
{
    Value a;

    a = create_const_value(lang_context, element);
    a = const_to_int(lang_context, a);
    return generate_const_to_element(lang_context, a);
}

/* cast type: (float) */
AstElement* cast_const_to_real(Lang* lang_context, AstElement* element)
{
    Value a;
    a = create_const_value(lang_context, element);
    a = const_to_real(lang_context, a);
    return generate_const_to_element(lang_context, a);
}

/* cast type: (string) */
AstElement* cast_const_to_string(Lang* lang_context, AstElement* element)
{
    Value a;
    a = create_const_value(lang_context, element);
    a = const_to_string(lang_context, a);
    return generate_const_to_element(lang_context, a);
}

/* a + b */
AstElement* add_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a + b;
    return generate_const_to_element(lang_context, r);
}

/* a - b */
AstElement* sub_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a - b;
    return generate_const_to_element(lang_context, r);
}

/* a * b */
AstElement* mul_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a * b;
    return generate_const_to_element(lang_context, r);
}

/* a % b */
AstElement* mod_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a % b;
    return generate_const_to_element(lang_context, r);
}

/* a / b */
AstElement* div_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a / b;
    return generate_const_to_element(lang_context, r);
}

/* a & b */
AstElement* and_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a & b;
    return generate_const_to_element(lang_context, r);
}

/* a | b */
AstElement* or_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a | b;
    return generate_const_to_element(lang_context, r);
}

/* a ^ b */
AstElement* xor_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a ^ b;
    return generate_const_to_element(lang_context, r);
}

/* -a */
AstElement* neg_const(Lang* lang_context, AstElement* element)
{
    Value a, r;

    a = create_const_value(lang_context, element);
    r = -a;
    return generate_const_to_element(lang_context, r);
}

/* ~a */
AstElement* rev_const(Lang* lang_context, AstElement* element)
{
    Value a, r;
    a = create_const_value(lang_context, element);
    r = ~a;
    return generate_const_to_element(lang_context, r);
}

/* a >= b */
AstElement* ge_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (a >= b);
    return generate_const_to_element(lang_context, r);
}

/* a <= b */
AstElement* le_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (a <= b);
    return generate_const_to_element(lang_context, r);
}

/* a > b */
AstElement* gt_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (a > b);
    return generate_const_to_element(lang_context, r);
}

/* a < b */
AstElement* lt_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (a < b);
    return generate_const_to_element(lang_context, r);
}

/* a == b */
AstElement* eq_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (a == b);
    return generate_const_to_element(lang_context, a);
}

/* a != b */
AstElement* ne_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (a != b);
    return generate_const_to_element(lang_context, r);
}

/* a << b */
AstElement* lsh_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a << b;
    return generate_const_to_element(lang_context, r);
}

/* a >> b */
AstElement* rsh_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = a >> b;
    return generate_const_to_element(lang_context, r);
}

/* a && b */
AstElement* land_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (const_to_bool(lang_context, a) && const_to_bool(lang_context, b));
    return generate_const_to_element(lang_context, r);
}

/* a || b */
AstElement* lor_const(Lang* lang_context, AstElement* element, AstElement* node2)
{
    Value a, b, r;
    a = create_const_value(lang_context, element);
    b = create_const_value(lang_context, node2);
    r = (const_to_bool(lang_context, a) || const_to_bool(lang_context, b));
    return generate_const_to_element(lang_context, r);
}

/* ! a */
AstElement* lnot_const(Lang* lang_context, AstElement* element)
{
    Value a, r;
    a = create_const_value(lang_context, element);
    r = !const_to_bool(lang_context, a);
    return generate_const_to_element(lang_context, r);
}

/* a ? b : c */
AstElement* select_const(Lang* lang_context, AstElement* element, AstElement* node2, AstElement* node3)
{
    Value a;
    a = create_const_value(lang_context, element);
    return const_to_bool(lang_context, a) ? node2 : node3;
}

}
#endif
