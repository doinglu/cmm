// cmm_vm.c

#include <math.h>
#include <stdio.h>
#include "std_port/std_port.h"
#include "cmm.h"
#include "cmm_call.h"
#include "cmm_common_util.h"
#include "cmm_output.h"
#include "cmm_thread.h"
#include "cmm_vm.h"

namespace cmm
{

// Define a instruction
#define _INST(code, opern, memo) { Instruction::code, #code, &Simulator::x##code, opern, memo }

Simulator::InstructionInfo Simulator::m_instruction_info[] =
{
    _INST(NOP,      0, "$$"),
    _INST(ADDI,     3, "$$ $1, $2, $3"),
    _INST(ADDR,     3, "$$ $1, $2, $3"),
    _INST(ADDX,     3, "$$ $1, $2, $3"),
    _INST(SUBI,     3, "$$ $1, $2, $3"),
    _INST(SUBR,     3, "$$ $1, $2, $3"),
    _INST(SUBX,     3, "$$ $1, $2, $3"),
    _INST(MULI,     3, "$$ $1, $2, $3"),
    _INST(MULR,     3, "$$ $1, $2, $3"),
    _INST(MULX,     3, "$$ $1, $2, $3"),
    _INST(DIVI,     3, "$$ $1, $2, $3"),
    _INST(DIVR,     3, "$$ $1, $2, $3"),
    _INST(DIVX,     3, "$$ $1, $2, $3"),
    _INST(MODI,     3, "$$ $1, $2, $3"),
    _INST(MODR,     3, "$$ $1, $2, $3"),
    _INST(MODX,     3, "$$ $1, $2, $3"),
    _INST(EQI,      3, "$$ $1, $2, $3"),
    _INST(EQR,      3, "$$ $1, $2, $3"),
    _INST(EQX,      3, "$$ $1, $2, $3"),
    _INST(GTI,      3, "$$ $1, $2, $3"),
    _INST(GTR,      3, "$$ $1, $2, $3"),
    _INST(GTX,      3, "$$ $1, $2, $3"),
    _INST(LTI,      3, "$$ $1, $2, $3"),
    _INST(LTR,      3, "$$ $1, $2, $3"),
    _INST(LTX,      3, "$$ $1, $2, $3"),
    _INST(NEI,      3, "$$ $1, $2, $3"),
    _INST(NER,      3, "$$ $1, $2, $3"),
    _INST(NEX,      3, "$$ $1, $2, $3"),
    _INST(GEI,      3, "$$ $1, $2, $3"),
    _INST(GER,      3, "$$ $1, $2, $3"),
    _INST(GEX,      3, "$$ $1, $2, $3"),
    _INST(LEI,      3, "$$ $1, $2, $3"),
    _INST(LER,      3, "$$ $1, $2, $3"),
    _INST(LEX,      3, "$$ $1, $2, $3"),
    _INST(ANDI,     3, "$$ $1, $2, $3"),
    _INST(ANDX,     3, "$$ $1, $2, $3"),
    _INST(ORI,      3, "$$ $1, $2, $3"),
    _INST(ORX,      3, "$$ $1, $2, $3"),
    _INST(XORI,     3, "$$ $1, $2, $3"),
    _INST(XORX,     3, "$$ $1, $2, $3"),
    _INST(REVI,     3, "$$ $1, $2, $3"),
    _INST(REVX,     3, "$$ $1, $2, $3"),
    _INST(NEGI,     3, "$$ $1, $2, $3"),
    _INST(NEGX,     3, "$$ $1, $2, $3"),
    _INST(LSHI,     3, "$$ $1, $2, $3"),
    _INST(LSHX,     3, "$$ $1, $2, $3"),
    _INST(RSHI,     3, "$$ $1, $2, $3"),
    _INST(RSHX,     3, "$$ $1, $2, $3"),
    _INST(CAST,     3, "$$ $1, ($2.type)$3"),
    _INST(ISTYPE,   3, "$$ $1, $3 is $2.type"),
    _INST(LDMULX,   3, "$$ $1, $2 x$3"),
    _INST(LDI,      2, "$$ $1, $23.i"),
    _INST(LDR,      2, "$$ $1, $23.f"),
    _INST(LDX,      2, "$$ $1, $2"),
    _INST(STI,      2, "$$ $1.pt, $23.i"),
    _INST(STR,      2, "$$ $1.pt, $23.f"),
    _INST(STX,      2, "$$ $1.pt, $2"),
    _INST(LDARGN,   1, "$$ $1, argn"),
    _INST(RIDXXX,   3, "$$ $1, $2[$3]"),
    _INST(LIDXXX,   3, "$$ $1[$3], $2"),
    _INST(MKEARR,   3, "$$ $1, [] capacity:$2"),
    _INST(MKIARR,   3, "$$ $1, [$3... x$2]"),
    _INST(MKEMAP,   3, "$$ $1, {} capacity:$2"),
    _INST(MKIMAP,   3, "$$ $1, {$3:... x$2}"),
    _INST(PUSHNX,   2, "$$ $1..x$2"),
    _INST(JMP,      1, "$$ $23.offset"),
    _INST(JCOND,    2, "$$ $23.offset when $1 != 0"),
    _INST(CALLNEAR, 2, "$$ $1=$2.fun($3)"),
    _INST(CALLFAR,  3, "$$ $1=$2.farfun($3)"),
    _INST(CALLNAME, 2, "$$ $1=$2.name($3)"),
    _INST(CALLOTHER,3, "$$ $1=$2.other_fun($3)"),
    _INST(CALLEFUN, 3, "$$ $1=$2.efun($3)"),
    _INST(RET,      1, "$$ $1"),
    _INST(LOOPIN,   3, "$$ ($1=$2 to $3)"),
    _INST(LOOPRANGE,3, "$$ ($1 in $2)"),
    _INST(LOOPEND,  0, "LOOP END"),
    { (Instruction::Code)0, 0, 0,  } // 0 Mark end
};

// Map Code to index
Uint8 Simulator::m_code_map[256];

bool Simulator::init()
{
    // Build code map: code->index
    for (Uint8 i = 0; m_instruction_info[i].name != 0; i++)
        m_code_map[m_instruction_info[i].code] = i;
    return true;
}

void Simulator::shutdown()
{
}

#if 0
// Get value by parameter
// If "is_imm_only" is false, get the value offset by type:imm, or return
// the imm (in Value *) directly 
Value *Simulator::get_parameter_value(int index)
{
    int type = 0;       // Type of this code parameter
    int imm = 0;        // Immidiate number
    switch (index)
    {
    case 0: type = m_this_code->t1; imm = m_this_code->p1; break;
    case 1: type = m_this_code->t2; imm = m_this_code->p2; break;
    case 2: type = m_this_code->t3; imm = m_this_code->p3; break;
    default: break;
    }

    // Derive the value
    Value *p = 0;
    switch (type)
    {
    case Instruction::CONSTANT:   p = m_constants;    break;
    case Instruction::ARGUMENT:   p = m_frame;        break;
    case Instruction::LOCAL_VAR:  p = m_frame;        break;
    case Instruction::OBJECT_VAR: p = m_object_vars;  break;
    default: break;
    }

    return p + imm;
}

// Get immediately value in parameter
Instruction::ParaValue Simulator::get_parameter_imm(int index)
{
    Instruction::ParaValue imm = 0;        // Immidiate number
    switch (index)
    {
    case 0: imm = m_this_code->p1; break;
    case 1: imm = m_this_code->p2; break;
    case 2: imm = m_this_code->p3; break;
    default: break;
    }
    return imm;
}
#endif

// Interpter of VM
void InterpreterComponent::interpreter(Thread *_thread, ArgNo __n)
{
    Simulator::interpreter(this, _thread, __n);
}

// Interpter of VM
void Simulator::interpreter(AbstractComponent *_component, Thread *_thread, ArgNo __n)
{
    // Create simulation context
    Simulator sim;
    sim.m_thread = _thread;
    sim.m_component = _component;
    sim.m_domain = _thread->get_current_domain();
    sim.m_function = _thread->get_this_function();
    sim.m_program = sim.m_function->get_program();
    sim.m_argn = __n;
    sim.m_frame = _thread->get_stack_top();
    sim.check_arguments();

    // Reserve locals
    sim.m_localn = sim.m_function->get_max_local_no();
    _thread->reserve_stack(sim.m_localn);
    memset(_thread->get_stack_top(), 0, sim.m_localn * sizeof(Value));
    _thread->get_this_call_context()->m_local_no = sim.m_localn;

    // Get context information
    sim.m_constantn = sim.m_program->get_constants_count();
    sim.m_constants = sim.m_program->get_constant(0);
    sim.m_object_varn = sim.m_component->m_program->get_object_vars_count();
    sim.m_object_vars = sim.m_component->m_object_vars;
    memset(sim.m_frame - sim.m_localn, 0, sizeof(Value) * sim.m_localn);

    // Set byte codes
    sim.m_byte_codes = sim.m_function->get_byte_codes_addr();

    // Start simulation
    sim.run();
}

// Make a constant include component_no:function_no
Integer Simulator::make_function_constant(ComponentNo component_no, FunctionNo function_no)
{
    return (((Integer)component_no << 16) | function_no);
}

#define GET_P1      Value* p1 = &m_frame[m_this_code.p1i]
#define GET_P2      Value* p2 = &m_frame[m_this_code.p2i]
#define GET_P3      Value* p3 = &m_frame[m_this_code.p3i]

#define _CASE(id)    case Instruction::id: x##id(); break;

// Run the function
void Simulator::run()
{
    m_ip = m_byte_codes;
    for (;;)
    {
        // Get index of InstructionInfo for this code
        size_t index = m_code_map[m_ip->code];
        auto* info = &m_instruction_info[index];
        m_this_code = *m_ip;

        m_ip++;
        switch (m_this_code.code)
        {
            _CASE(NOP);
            _CASE(ADDI);
            _CASE(ADDR);
            _CASE(ADDX);
            _CASE(SUBI);
            _CASE(SUBR);
            _CASE(SUBX);
            _CASE(MULI);
            _CASE(MULR);
            _CASE(MULX);
            _CASE(DIVI);
            _CASE(DIVR);
            _CASE(DIVX);
            _CASE(MODI);
            _CASE(MODR);
            _CASE(MODX);
            _CASE(EQI);
            _CASE(EQR);
            _CASE(EQX);
            _CASE(NEI);
            _CASE(NER);
            _CASE(NEX);
            _CASE(GTI);
            _CASE(GTR);
            _CASE(GTX);
            _CASE(LTI);
            _CASE(LTR);
            _CASE(LTX);
            _CASE(GEI);
            _CASE(GER);
            _CASE(GEX);
            _CASE(LEI);
            _CASE(LER);
            _CASE(LEX);
            _CASE(ANDI);
            _CASE(ANDX);
            _CASE(ORI);
            _CASE(ORX);
            _CASE(XORI);
            _CASE(XORX);
            _CASE(REVI);
            _CASE(REVX);
            _CASE(NEGI);
            _CASE(NEGX);
            _CASE(LSHI);
            _CASE(LSHX);
            _CASE(RSHI);
            _CASE(RSHX);
            _CASE(CAST);
            _CASE(ISTYPE);
            _CASE(LDMULX);
            _CASE(LDI);
            _CASE(LDR);
            _CASE(LDX);
            _CASE(STI);
            _CASE(STR);
            _CASE(STX);
            _CASE(LDARGN);
            _CASE(RIDXXX);
            _CASE(LIDXXX);
            _CASE(MKEARR);
            _CASE(MKIARR);
            _CASE(MKEMAP);
            _CASE(MKIMAP);
            _CASE(PUSHNX);
            _CASE(JMP);
            _CASE(JCOND);
            _CASE(CALLNEAR);
            _CASE(CALLFAR);
            _CASE(CALLNAME);
            _CASE(CALLOTHER);
            _CASE(CALLEFUN);
            _CASE(LOOPIN);
            _CASE(LOOPRANGE);
            _CASE(LOOPEND);
            case Instruction::RET:
            {
                GET_P1;
                m_thread->set_ret(*p1);
                return;
            }
            default:
                throw_error("Unknown code: %d.\n", (int)m_this_code.code);
                break;
        }

#ifdef _DEBUG
        // Get the pointer to return value (p1)
        GET_P1;
        // The return value (p1) must be binded to current domain
        STD_ASSERT(p1->m_type < REFERENCE_VALUE ||
                   (p1->m_reference->attrib & ReferenceImplAttrib::CONSTANT) ||
                    p1->m_reference->owner == m_domain->get_value_list());
#endif
    }
}

#undef _CASE

void Simulator::check_arguments()
{
    // Check the count & type of all arguments
    // Pad default if necessary
    if (m_argn < m_function->get_min_arg_no())
        throw_error("Two few arguments to %s, expected %d, got %d.\n",
                    m_function->get_name()->c_str(),
                    m_function->get_min_arg_no(), m_argn);

    if (m_argn > m_function->get_min_arg_no() &&
        !m_function->get_attrib() & Function::RANDOM_ARG)
        throw_error("Two many arguments to %s, expected %d, got %d.\n",
                    m_function->get_name()->c_str(),
                    m_function->get_max_arg_no(), m_argn);

    // Check types
    auto& parameters = (Parameters&)m_function->get_parameters();
    Value *p = m_frame;
    for (auto& it : parameters)
    {
        if (it->get_type() != p->m_type &&
            (!it->is_nullable() || p->m_type != NIL))
            // Type is not matched
            throw_error("Bad type of arguments %s, expected %s, got %s.\n",
                        it->get_name()->c_str(),
                        Value::type_to_name(it->get_type()), Value::type_to_name(p->m_type));
        p++;
    }
    // Append the default parameters if necessary
    if (m_argn < m_function->get_max_arg_no())
    {
        STD_ASSERT(("Bad m_frame.", m_frame == m_thread->get_stack_top()));
        auto move = m_function->get_max_arg_no() - m_argn;
        m_thread->reserve_stack(move);

        // Update frame base pointer & this_call_context
        m_frame = m_thread->get_stack_top();
        m_thread->get_this_call_context()->m_frame = m_frame;
        m_thread->get_this_call_context()->m_arg_no = m_function->get_max_arg_no();
        // Move the arguments to new location
        memmove(m_frame, m_frame + move, m_argn * sizeof(Value));
        // Insert new values as default arguments
        while (m_argn < m_function->get_max_arg_no())
        {
            m_frame[m_argn] = parameters[m_argn]->get_default();
            m_argn++;
        }
    }
}

// Do nothing
void Simulator::xNOP()
{
}

// ADD two integers, p1 = p2 + p3
void Simulator::xADDI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int + p3->m_int;
}

void Simulator::xADDR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::REAL;
    p1->m_real = p2->m_real + p3->m_real;
}

void Simulator::xADDX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_add(p1, *p2, *p3);
}

void Simulator::xSUBI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int - p3->m_int;
}

void Simulator::xSUBR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::REAL;
    p1->m_real = p2->m_real - p3->m_real;
}

void Simulator::xSUBX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_sub(p1, *p2, *p3);
}

void Simulator::xMULI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int * p3->m_int;
}

void Simulator::xMULR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::REAL;
    p1->m_real = p2->m_real * p3->m_real;
}

void Simulator::xMULX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_mul(p1, *p2, *p3);
}

void Simulator::xDIVI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int / p3->m_int;
}

void Simulator::xDIVR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::REAL;
    p1->m_real = p2->m_real / p3->m_real;
}

void Simulator::xDIVX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_div(p1, *p2, *p3);
}

void Simulator::xMODI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int % p3->m_int;
}

void Simulator::xMODR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::REAL;
    p1->m_real = fmod(p2->m_real, p3->m_real);
}

void Simulator::xMODX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_mod(p1, *p2, *p3);
}

void Simulator::xEQI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_int == p3->m_int);
}

void Simulator::xEQR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_real == p3->m_real);
}

void Simulator::xEQX()
{
    GET_P1; GET_P2; GET_P3;
    *p1 = (*p2 == *p3);
}

void Simulator::xNEI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_int != p3->m_int);
}

void Simulator::xNER()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_real != p3->m_real);
}

void Simulator::xNEX()
{
    GET_P1; GET_P2; GET_P3;
    *p1 = (*p2 != *p3);
}

void Simulator::xGTI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_int > p3->m_int);
}

void Simulator::xGTR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_real > p3->m_real);
}

void Simulator::xGTX()
{
    GET_P1; GET_P2; GET_P3;
    *p1 = (*p2 > *p3);
}

void Simulator::xLTI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_int < p3->m_int);
}

void Simulator::xLTR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_real < p3->m_real);
}

void Simulator::xLTX()
{
    GET_P1; GET_P2; GET_P3;
    *p1 = (*p2 < *p3);
}

void Simulator::xGEI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_int >= p3->m_int);
}

void Simulator::xGER()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_real >= p3->m_real);
}

void Simulator::xGEX()
{
    GET_P1; GET_P2; GET_P3;
    *p1 = (*p2 >= *p3);
}

void Simulator::xLEI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_int <= p3->m_int);
}

void Simulator::xLER()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = (Integer)(p2->m_real <= p3->m_real);
}

void Simulator::xLEX()
{
    GET_P1; GET_P2; GET_P3;
    *p1 = (*p2 <= *p3);
}

void Simulator::xANDI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int & p3->m_int;
}

void Simulator::xANDX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_and(p1, *p2, *p3);
}

void Simulator::xORI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int | p3->m_int;
}

void Simulator::xORX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_or(p1, *p2, *p3);
}

void Simulator::xXORI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int ^ p3->m_int;
}

void Simulator::xXORX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_xor(p1, *p2, *p3);
}

void Simulator::xREVI()
{
    GET_P1; GET_P2;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = ~p2->m_int;
}

void Simulator::xREVX()
{
    GET_P1; GET_P2;
    Value::op_rev(p1, *p2);
}

void Simulator::xNEGI()
{
    GET_P1; GET_P2;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = -p2->m_int;
}

void Simulator::xNEGX()
{
    GET_P1; GET_P2;
    Value::op_neg(p1, *p2);
}

void Simulator::xLSHI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int << p3->m_int;
}

void Simulator::xLSHX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_lsh(p1, *p2, *p3);
}

void Simulator::xRSHI()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ValueType::INTEGER;
    p1->m_int = p2->m_int >> p3->m_int;
}

void Simulator::xRSHX()
{
    GET_P1; GET_P2; GET_P3;
    Value::op_rsh(p1, *p2, *p3);
}

void Simulator::xCAST()
{
    GET_P1; GET_P3;
    auto type = (ValueType)m_this_code.p2i;
    if (p3->m_type == type)
    {
        *p1 = *p3;
        return;
    }

    switch (type)
    {
    case INTEGER:
        *p1 = p3->cast_int();
        break;

    case REAL:
        *p1 = p3->cast_real();
        break;

    case STRING:
        *p1 = p3->cast_string();
        break;

    case BUFFER:
        *p1 = p3->cast_buffer();
        break;

    case ARRAY:
        *p1 = p3->cast_array();
        break;

    case MAPPING:
        *p1 = p3->cast_map();
        break;

    default:
        throw_error("Failed to do cast %s to %s.\n",
                    Value::type_to_name(p3->m_type),
                    Value::type_to_name(type));
    }
}

void Simulator::xISTYPE()
{
    GET_P1; GET_P3;
    auto type = (ValueType)m_this_code.p2i;
    p1->m_type = INTEGER;
    p1->m_int = (Integer)(p3->m_type == type);
}

void Simulator::xLDMULX()
{
    GET_P1; GET_P2; GET_P3;
    // P1 must be local for LDMULX
    if (m_this_code.p1i + p3->m_int > m_argn)
        throw_error("Overflow when copy values (%lld) to local (offset = %lld).\n",
                    (Int64)p3->m_int, (Int64)m_this_code.p1i);
    memcpy(p1, p2, sizeof(Value) * p3->m_int);
}

void Simulator::xLDI()
{
    GET_P1;
    p1->m_type = INTEGER;
    p1->m_int = (Integer)m_this_code.p23i;
}

void Simulator::xLDR()
{
    GET_P1;
    p1->m_type = REAL;
    p1->m_real = (Real)m_this_code.p23f;
}

void Simulator::xLDX()
{
    GET_P1;
    auto* p2 = &m_value_ptrs[m_this_code.pt][m_this_code.p2i];
    *p1 = *p2;
}

void Simulator::xSTI()
{
    auto* p1 = &m_object_vars[m_this_code.p1u];
    p1->m_type = INTEGER;
    p1->m_int = (Integer)m_this_code.p23i;
}

void Simulator::xSTR()
{
    auto* p1 = &m_object_vars[m_this_code.p1u];
    p1->m_type = REAL;
    p1->m_real = (Real)m_this_code.p23f;
}

void Simulator::xSTX()
{
    auto* p1 = &m_object_vars[m_this_code.p1u];
    GET_P2;
    *p1 = *p2;
}

void Simulator::xLDARGN()
{
    GET_P1;
    p1->m_type = INTEGER;
    p1->m_int = (Integer)m_argn;
}

void Simulator::xRIDXXX()
{
    GET_P1; GET_P2; GET_P3;
    switch (p2->m_type)
    {
    case STRING:
        if (p3->m_type == INTEGER)
        {
            if (p3->m_int < 0 || p3->m_int >= (Integer)p2->m_string->length())
                throw_error("Index (%lld) is out of string range (%zu).\n",
                            (Int64)p3->m_int, p2->m_string->length());
            p1->m_type = INTEGER;
            p1->m_int = p2->m_string->get(p3->m_int);
            break;
        }
    case BUFFER:
        if (p3->m_type == INTEGER)
        {
            if (p3->m_int < 0 || p3->m_int >= (Integer)p2->m_buffer->length())
                throw_error("Index (%lld) is out of buffer range (%zu).\n",
                            (Int64)p3->m_int, p2->m_buffer->length());
            p1->m_type = INTEGER;
            p1->m_int = p2->m_buffer->get(p3->m_int);
            break;
        }
    case ARRAY:
        if (p3->m_type == INTEGER)
        {
            if (p3->m_int < 0 || p3->m_int >= (Integer)p2->m_array->size())
                throw_error("Index (%lld) is out of array range (%zu).\n",
                            (Int64)p3->m_int, p2->m_array->size());
            p2->m_array->get(p3->m_int, p1);
            break;
        }

    case MAPPING:
        p2->m_map->get(*p3, p1);
        break;

    default:
        break;
    }

    throw_error("Failed to do index like %s[%s].\n",
                Value::type_to_name(p2->m_type),
                Value::type_to_name(p3->m_type));
}

void Simulator::xLIDXXX()
{
    GET_P1; GET_P2; GET_P3;
    switch (p2->m_type)
    {
    case ARRAY:
        if (p3->m_type == INTEGER)
        {
            if (p3->m_int < 0 || p3->m_int >= (Integer)p2->m_array->size())
                throw_error("Index (%lld) is out of array range (%zu).\n",
                            (Int64)p3->m_int, p2->m_array->size());
            p2->m_array->set(p3->m_int, *p1);
            break;
        }

    case MAPPING:
        p2->m_map->set(*p3, *p1);
        break;

    default:
        break;
    }

    throw_error("Failed to do assign index like %s[%s].\n",
                Value::type_to_name(p2->m_type),
                Value::type_to_name(p3->m_type));
}

// Make an empty array
void Simulator::xMKEARR()
{
    GET_P1; GET_P2;
    p1->m_type = ARRAY;
    p1->m_array = XNEW(ArrayImpl, (size_t)p2->m_int);
    m_domain->bind_value(p1->m_reference);
}

// Make and initialize an array
void Simulator::xMKIARR()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = ARRAY;
    p1->m_array = XNEW(ArrayImpl, (size_t)p2->m_int);
    p1->m_array->push_back_array(p3, (size_t)p2->m_int);
    m_domain->bind_value(p1->m_reference);
}

// Make an empty mapping
void Simulator::xMKEMAP()
{
    GET_P1; GET_P2;
    p1->m_type = MAPPING;
    p1->m_map = XNEW(MapImpl, (size_t)p2->m_int);
    m_domain->bind_value(p1->m_reference);
}

// Make and initialize a mapping
void Simulator::xMKIMAP()
{
    GET_P1; GET_P2; GET_P3;
    p1->m_type = MAPPING;
    p1->m_map = XNEW(MapImpl, (size_t)p2->m_int);
    m_domain->bind_value(p1->m_reference);
    for (Integer i = 0; i < p2->m_int; i++)
        p1->m_map->set(p3[i * 2], p3[i * 2 + 1]);
}

// Push N values to stack
void Simulator::xPUSHNX()
{
    GET_P1; GET_P2;
    auto* ptr = m_thread->reserve_stack_without_init((size_t)p2->m_int);
    memcpy(ptr, p1, (size_t)p2->m_int * sizeof(Value));
}

void Simulator::xJMP()
{
    m_ip += m_this_code.p23i;
}

void Simulator::xJCOND()
{
    GET_P1;
    if (p1->is_non_zero())
        m_ip += m_this_code.p23i;
}

void Simulator::xCALLNEAR()
{
    GET_P1; GET_P2; GET_P3;
    auto function_no = (FunctionNo)p2->m_int;
    Call::invoke_far_impl(m_thread, 0/* current component */, function_no, p1 /* ret */, (ArgNo)p3->m_int);
}

void Simulator::xCALLFAR()
{
    GET_P1; GET_P2; GET_P3;
    // p2 is constant: FunctionNo & ComponentNo
    auto function_no = (FunctionNo)(p2->m_int >> 16);
    auto component_no = (ComponentNo)(p2->m_int & 0xFFFF);
    Call::invoke_far_impl(m_thread, component_no, function_no, p1 /* ret */, (ArgNo)p3->m_int);
}

void Simulator::xCALLNAME()
{
    GET_P1; GET_P2; GET_P3;
    if (p2->m_type != STRING)
        throw_error("Bad type to call, expected string got %s.\n",
                    Value::type_to_name(p1->m_type));
    // p2 is constant: name
    m_program->invoke_self(m_thread, p2[0] /* function name */, p1 /* ret */, (ArgNo)p3->m_int);
}

void Simulator::xCALLOTHER()
{
    GET_P1; GET_P2; GET_P3;
    if (p2->m_type != STRING)
        throw_error("Bad type to call, expected string got %s.\n",
                    Value::type_to_name(p1->m_type));
    // p2 is locals: name, oid
    m_program->invoke(m_thread, p2[1].m_oid /* object */, p2[0] /* function name */, p1 /* ret */, (ArgNo)p3->m_int);
}

void Simulator::xCALLEFUN()
{
    GET_P1; GET_P2; GET_P3;
    if (p2->m_type != STRING)
        throw_error("Bad type to call, expected string got %s.\n",
                    Value::type_to_name(p1->m_type));
    Efun::invoke(m_thread, p2[0] /* function name */, p1 /* ret */, (ArgNo)p3->m_int);
}

void Simulator::xRET()
{
    // NEVER USE THIS ROUTINE
    throw_error("The instruction RET shouldn't be invoked.\n");
}

void Simulator::xLOOPIN()
{
    // More to be added
}

void Simulator::xLOOPRANGE()
{
    // More to be added
}

void Simulator::xLOOPEND()
{
    // More to be added
}

}
