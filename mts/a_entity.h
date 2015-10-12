#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "std_template/simple_hash_map.h"
#include "std_template/simple_list.h"
#include "std_template/simple_string.h"
#include "std_port/std_port.h"
#include "std_port/std_port_type.h"
#include "std_memmgr/std_memmgr.h"
#include "cmm_call.h"
#include "cmm_domain.h"
#include "cmm_object.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

class __clone_entity_impl : public AbstractComponent
{
public:
    Value create(Thread *_thread, Object *__this_object, ComponentNo __component_no, Value *__args, ArgNo __n)
    {
        if (__n != 0)
            throw simple::string().snprintf("Bad parameters, expected %d, got %d.", 1, __n);

        CallContextNode __context(_thread, __this_object, 0, (Value *)0, 0, (Value *)0, 0);
        _thread->enter_function_call(&__context);

        call_far(_thread, 1, 0, simple::string().snprintf("Entity(%llx)", 256, __this_object->get_oid().i64));

        return _thread->leave_function_call(&__context, Value());
    }
};

}
