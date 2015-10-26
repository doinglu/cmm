#pragma once

#include "a_entity.h"
#include "a_desc.h"
#include "a_name.h"

namespace cmm
{

    class __clone_entity_ob : public Object
    {
    private:
        typedef __clone_entity_impl Impl;

    public:
        static Program *create_program()
        {
            Program *program = XNEW(Program, "/clone/entity", Program::COMPILED_TO_NATIVE);

            program->define_member("id", ValueType::INTEGER);

            program->add_component("/clone/entity", 0);
            program->add_component("/feature/name", 0);
            program->add_component("/feature/desc", 0);

            program->define_function("create", (Function::ScriptEntry)&Impl::create, 0, 0);

            return program;
        }
    };

}
