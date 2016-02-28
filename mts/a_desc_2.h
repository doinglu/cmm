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
        auto r = ReserveStack(1);
        String& temp = (String&)r[0];
        Program *program = XNEW(Program, temp = "/feature/desc", Program::COMPILED_TO_NATIVE);

        program->define_object_var(temp = "desc", STRING);

        program->add_component(temp = "/feature/desc");
        program->add_component(temp = "/feature/name");

        Function *function;
        program->define_function(temp = "print", (Function::ScriptEntry)&Impl::print, 0, 0);
        function = program->define_function(temp = "perror", (Function::ScriptEntry)&Impl::perror, 3, 0);
        function->define_parameter(temp = "a1", ValueType::MIXED, (Parameter::Attrib)0);
        function->define_parameter(temp = "a2", ValueType::MIXED, (Parameter::Attrib)0);
        function->define_parameter(temp = "a3", ValueType::MIXED, (Parameter::Attrib)0);
        function->finish_adding_parameters();

        return program;
    }
};

}
