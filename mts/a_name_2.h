#pragma once

#include "a_name.h"
#include "a_desc.h"

namespace cmm
{

class __feature_name_ob : public Object
{
private:
    typedef __feature_name_ob Self;
    typedef __feature_name_impl Impl;
    __feature_name_impl m_name;
    __feature_desc_impl m_desc;

public:
    static Object *new_instance()
    {
        return new Self();
    }

    static Program *create_program()
    {
        Program *program = new Program("/feature/name");

        program->set_new_instance_func(&new_instance);

        program->add_component("/feature/name", MEMBER_OFFSET(m_name));
        program->add_component("/feature/desc", MEMBER_OFFSET(m_desc));

        program->define_function("set_name", (Function::Entry)&Impl::set_name, 1, 1);
        program->define_function("get_name", (Function::Entry)&Impl::get_name, 0, 0);
        program->define_function("output", (Function::Entry)&Impl::output, 0, 0);

        return program;
    }
};

}
