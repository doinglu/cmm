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
            auto r = ReserveStack(1);
            String& temp = (String&)r[0];

            Program *program = XNEW(Program, temp = "/clone/entity", Program::COMPILED_TO_NATIVE);

            program->define_object_var(temp = "id", INTEGER);

            program->add_component(temp = "/clone/entity");
            program->add_component(temp = "/feature/name");
            program->add_component(temp = "/feature/desc");

            program->define_function(temp = "create", (Function::ScriptEntry)&Impl::create, 0, 0);

            return program;
        }
    };

}
