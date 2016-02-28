// a_desc_i.cpp
// interpreter of feature/desc

#include "cmm.h"
#include "cmm_program.h"
#include "cmm_value.h"
#include "cmm_vm.h"

namespace cmm
{

Program *create_desc_i_program()
{
    auto r = ReserveStack(3);
    Value& temp = (String&)r[0];
    Value& value = r[1];
    Value& m = r[2];

    Program *program = XNEW(Program, temp = "/feature/desc", Program::Attrib::INTERPRETED);

    program->add_component(temp = "/feature/desc");
    program->add_component(temp = "/feature/name");

    m = XNEW(MapImpl, 3);
    m.set(temp = "purpose", value = "test");

    Function *function;
    function = program->define_function(temp = "printf", 0, 1, 1, (Function::Attrib)(Function::Attrib::RANDOM_ARG|Function::Attrib::INTERPRETED));
    function->define_parameter(temp = "format", ValueType::STRING);
    function->reserve_local(10);

    auto stroff1 = (Instruction::ParaValue23)program->define_constant(temp = "(VM) Name: %s.\n%O\n");
    auto stroff2 = (Instruction::ParaValue23)program->define_constant(temp = "printf");
    auto intoff3 = (Instruction::ParaValue23)program->define_constant(temp = 2);
    auto intoff4 = (Instruction::ParaValue23)program->define_constant(temp = Simulator::make_function_constant(1, 1));
    auto mapoff5 = (Instruction::ParaValue23)program->define_constant(temp = m);
    auto stroff6 = (Instruction::ParaValue23)program->define_constant(temp = "After loop, r3 = %d.\n");
    // Create bytes
#define I Instruction
#define P0 (Instruction::ParaValue)0
#define LOCAL   Instruction::LOCAL_VAR
    Instruction arr1[] =
    {
        { I::LDI, -3, 1048576 },                // LDI L2, 1048576
        { I::LDI, -4, 5 },                      // LDI L3, 5
        { I::LDI, -5, 1 },                      // LDI L4, 1
        { I::LEI, -2, -3, -4 },                 // LEI L1, L2, L3   (label1)
        { I::JCOND, -2, 2 },                    // JCOND label_2, L1    (jmp if L2 <= L3)
        { I::ADDI, -4, -4, -5 },                // ADDI, L3, L3, L4
        { I::JMP, 0, -4 },                      // JMP -4  (label_1)
                                                // (label_2)
        { I::LDI, -2, 2 },                      // LDI L1, 2
        { I::LDX, I::CONSTANT, -5, stroff6 },   // LDX L4, "After loop, l3 = %d.\n"
        { I::PUSHNX, -5, -2, P0 },              // PUSHNX L4... (xL1)
        { I::LDX, I::CONSTANT, -6, stroff2 },   // LDX L5, "printf"
        { I::CALLEFUN, -1, -6, -2 },            // CALLEFUN L0(ret), L5(printf), L1(argn)
                                                                         
//      printf("Name: %s, %O.\n", call_far(_thread, 1 /* Component:Name */, 1 /* get_name() */).m_string->c_str());
        { I::LDI, -3, 0 },                      // LDI L2, 0
        { I::LDX, I::CONSTANT, -6, intoff4 },
        { I::CALLFAR, -4, -6, -3 },             // CALLFAR L3(ret), L5(get_name), L2(argn)
        { I::LDX, I::CONSTANT, -3, stroff1 },   // LDX L2, "(VM) Name: %s.\n"
        { I::LDARGN, -2, 0, P0 },               // LDARGN L1
        { I::PUSHNX, 0, -2, P0 },               // PUSHNX arg0... (xL1)
        { I::LDX, I::CONSTANT, -6, stroff2 },   // LDX L5, "printf"
        { I::CALLEFUN, -1, -6, -2},             // CALLEFUN L0, "printf", L1...
        { I::RET, -1, P0, P0 },                 // RET L0
    };
    function->set_byte_codes(arr1, STD_SIZE_N(arr1));

    return program;
}

}
