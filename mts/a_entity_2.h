#pragma once

#include "a_entity.h"
#include "a_desc.h"
#include "a_name.h"

namespace cmm
{

    class __clone_entity_ob : public Object
    {
    private:
        typedef __clone_entity_ob Self;
        typedef __clone_entity_impl Impl;
        __clone_entity_impl m_entity;
        __feature_name_impl m_name;
        __feature_desc_impl m_desc;

    public:
        static Object *new_instance()
        {
            return XNEW(Self);
        }

        static Program *create_program()
        {
            Program *program = XNEW(Program, "/clone/entity");

            program->define_object(sizeof(Self));
            program->set_new_instance_func(&new_instance);

            program->add_component("/clone/entity", MEMBER_OFFSET(m_entity));
            program->add_component("/feature/name", MEMBER_OFFSET(m_name));
            program->add_component("/feature/desc", MEMBER_OFFSET(m_desc));

            program->define_function("create", (Function::Entry)&Impl::create, 0, 0);

            return program;
        }
    };

}
