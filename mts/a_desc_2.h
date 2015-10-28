#pragma once

#include "a_desc.h"
#include "a_name.h"

namespace cmm
{

class __feature_desc_ob : public Object
{
private:
    typedef __feature_desc_impl Impl;

public:
    static Program *create_program()
    {
        Program *program = XNEW(Program, "/feature/desc", Program::COMPILED_TO_NATIVE);

        program->define_member("desc", STRING);

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
};

}
