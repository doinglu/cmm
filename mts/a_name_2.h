#pragma once

#include "a_name.h"
#include "a_desc.h"

namespace cmm
{

class __feature_name_ob : public Object
{
private:
    typedef __feature_name_impl Impl;

public:
    static Program *create_program()
    {
        auto r = ReserveStack(1);
        String& temp = (String&)r[0];

        Program *program = XNEW(Program, temp = "/feature/name", Program::COMPILED_TO_NATIVE);

        program->define_object_var(temp = "name", STRING);
        program->define_object_var(temp = "unused", MIXED);

        program->add_component(temp = "/feature/name");
        program->add_component(temp = "/feature/desc");

        Function *function;
        program->define_function(temp = "set_name", (Function::ScriptEntry)&Impl::set_name, 1, 1);
        program->define_function(temp = "get_name", (Function::ScriptEntry)&Impl::get_name, 0, 0);
        program->define_function(temp = "test_call_private", (Function::ScriptEntry)&Impl::test_call_private, 0, 0, Function::Attrib::PRIVATE);
        program->define_function(temp = "test_call", (Function::ScriptEntry)&Impl::test_call, 0, 0);
        program->define_function(temp = "do_nothing", (Function::ScriptEntry)&Impl::do_nothing, 0, 0);
        function = program->define_function(temp = "test_error", (Function::ScriptEntry)&Impl::test_error, 2, 0);
        function->define_parameter(temp = "other_oid", ValueType::OBJECT, (LocalVariable::Attrib)0);
        function->define_parameter(temp = "msg", ValueType::STRING, (LocalVariable::Attrib)0);
        function->finish_adding_parameters();

        return program;
    }
};

}
