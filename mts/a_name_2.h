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
        Program *program = XNEW(Program, "/feature/name", Program::COMPILED_TO_NATIVE);

        program->define_member("name", STRING);
        program->define_member("unused", MIXED);

        program->add_component("/feature/name");
        program->add_component("/feature/desc");

        Function *function;
        program->define_function("set_name", (Function::ScriptEntry)&Impl::set_name, 1, 1);
        program->define_function("get_name", (Function::ScriptEntry)&Impl::get_name, 0, 0);
        program->define_function("test_call_private", (Function::ScriptEntry)&Impl::test_call_private, 0, 0, Function::Attrib::PRIVATE);
        program->define_function("test_call", (Function::ScriptEntry)&Impl::test_call, 0, 0);
        program->define_function("do_nothing", (Function::ScriptEntry)&Impl::do_nothing, 0, 0);
        function = program->define_function("test_error", (Function::ScriptEntry)&Impl::test_error, 2, 0);
        function->define_parameter("other_oid", ValueType::OBJECT, (LocalVariable::Attrib)0);
        function->define_parameter("msg", ValueType::STRING, (LocalVariable::Attrib)0);
        function->finish_adding_parameters();

        return program;
    }
};

}
