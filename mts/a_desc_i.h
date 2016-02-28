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
    Program *program = XNEW(Program, "/feature/desc", Program::Attrib::INTERPRETED);

    program->add_component("/feature/desc");
    program->add_component("/feature/name");

    Map m = XNEW(MapImpl, 3);
//    Map m2 = XNEW(MapImpl, 3);
//    m2["key2"] = "value2";
//    m["m2"] = m2;
    m.set(String("purpose"), "test");

    Function *function;
    function = program->define_function("printf", 0, 1, 1, (Function::Attrib)(Function::Attrib::RANDOM_ARG|Function::Attrib::INTERPRETED));
    function->define_parameter("format", ValueType::STRING);
    function->reserve_local(8);
    auto stroff1 = (Instruction::ParaValue)program->define_constant("(VM) Name: %s.\n%O\n");
    auto stroff2 = (Instruction::ParaValue)program->define_constant("printf");
    auto intoff3 = (Instruction::ParaValue)program->define_constant(2);
    auto intoff4 = (Instruction::ParaValue)program->define_constant(Simulator::make_function_constant(1, 1));
    auto mapoff5 = (Instruction::ParaValue)program->define_constant(m);
    auto stroff6 = (Instruction::ParaValue)program->define_constant("After loop, r3 = %d.\n");
    // Create bytes
#define I Instruction
#define P0 (Instruction::ParaType)0
#define NG (Instruction::ParaValue)
    Instruction arr1[] =
    {
        { I::LDI, I::LOCAL, P0, P0, 2, 16, 0 },                         // LDI r2, 1048576
        { I::LDI, I::LOCAL, P0, P0, 3, 0, 5 },                          // LDI r3, 5
        { I::LDI, I::LOCAL, P0, P0, 4, 0, 1 },                          // LDI r4, 1
        { I::LEI, I::LOCAL, I::LOCAL, I::LOCAL, 1, 2, 3 },              // LTI r1, r2, r3   (label1)
        { I::JCOND, I::LOCAL, P0, P0, 1, 0, 2 },                        // JCOND label_2, r1    (jmp if r2 <= r3)
        { I::ADDI, I::LOCAL, I::LOCAL, I::LOCAL, 3, 3, 4 },             // ADDI, r3, r3, r4
        { I::JMP, P0, P0, P0, 0, NG-1, NG-4 },                          // JMP -4  (label_1)
                                                                        // (label_2)
        { I::LDI, I::LOCAL, P0, P0, 1, 0, 2 },                          // LDI r1, 2
        { I::LDX, I::LOCAL, I::CONSTANT, P0, 2, stroff6, 0 },           // LDX r2, "After loop, r3 = %d.\n"
        { I::CALLEFUN, I::LOCAL, I::CONSTANT, I::LOCAL, 0, stroff2, 1 }, // CALLEFUN r0, "printf", r1...

//      printf("Name: %s, %O.\n", call_far(_thread, 1 /* Component:Name */, 1 /* get_name() */).m_string->c_str());
        { I::LDI, I::LOCAL, P0, P0, 2, 0, 0 },                          // LDI r2, 0
        { I::CALLFAR, I::LOCAL, I::CONSTANT, I::LOCAL, 3, intoff4, 0 }, // CALLFAR r3, get_name, r2
        { I::LDX, I::LOCAL, I::CONSTANT, P0, 2, stroff1, 0 },           // LDX r2, "(VM) Name: %s.\n"
        { I::LDARGN, I::LOCAL, P0, P0, 1, 0, 0 },                       // LDARGN r1
        { I::LDMULX, I::LOCAL, I::ARGUMENT, I::LOCAL, 4, 0, 1 },        // LDMULX r4, arg0 xr1
        { I::ADDX, I::LOCAL, I::LOCAL, I::CONSTANT, 1, 1, intoff3 },    // ADDX r1, r1, 2
        { I::CALLEFUN, I::LOCAL, I::CONSTANT, I::LOCAL, 0, stroff2, 1}, // CALLEFUN r0, "printf", r1...

        { I::RET, I::LOCAL, P0, P0, 0, 0, 0 },
    };
    function->set_byte_codes(arr1, STD_SIZE_N(arr1));

    return program;
}

}
