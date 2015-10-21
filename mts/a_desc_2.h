#pragma once

#include "a_desc.h"
#include "a_name.h"

namespace cmm
{

class __feature_desc_ob : public Object
{
private:
    typedef __feature_desc_ob Self;
    typedef __feature_desc_impl Impl;
    __feature_desc_impl m_desc;
    __feature_name_impl m_name;

public:
    static Object *new_instance()
    {
        return XNEW(Self);
    }

    static Program *create_program()
    {
        Program *program = XNEW(Program, "/feature/desc");

        program->define_object(sizeof(Self));
        program->set_new_instance_func(&new_instance);

        program->add_component("/feature/desc", MEMBER_OFFSET(m_desc));
        program->add_component("/feature/name", MEMBER_OFFSET(m_name));

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
