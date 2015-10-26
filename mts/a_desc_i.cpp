#if 0
// a_desc_i.cpp
// interpreter of feature/desc

#include "cmm.h"
#include "cmm_program.h"
#include "cmm_value.h"

namespace cmm
{

Program *create_desc_i_program()
{
    Program *program = XNEW(Program, "/feature/desc", 0);

    program->add_component("/feature/desc");
    program->add_component("/feature/name");

    Function *function;
    program->define_function("print", (Function::ScriptEntry)&Impl::print, 0, 0);
    function = program->define_function("perror", (Function::ScriptEntry)&Impl::perror, 3, 0);
    function->define_parameter("a1", ValueType::MIXED, (Parameter::Attrib)0);
    function->define_parameter("a2", ValueType::MIXED, (Parameter::Attrib)0);
    function->define_parameter("a3", ValueType::MIXED, (Parameter::Attrib)0);
    function->finish_adding_parameters();

    return program;
}

}
#endif
