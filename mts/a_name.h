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
#include "cmm_memory.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

class __feature_name_impl : public AbstractComponent
{
public:
    // Function 0
    Value set_name(Thread *__thread, Object *__this_object, ComponentNo __component_no, Value *__args, ArgNo __n)
    {
        if (__n != 1)
            throw simple::string().snprintf("Bad parameters, expected %d, got %d.", 1, __n);

        if (__args[0].m_type != Value::STRING)
            throw simple::string().snprintf("Parameter 1 'name' is not string.", 256);

        // Enter function
//        Value __local[] = { };
        CallContextNode __context(__thread, __this_object, __component_no, __args, __n, (Value*)0, 0);
        __thread->enter_function_call(&__context);
        String* &name = __args[0].m_string;

        this->name = name;

        return __thread->leave_function_call(&__context, Value());
    }

    // Function 1
    Value get_name(Thread *__thread, Object *__this_object, ComponentNo __component_no, Value *__args, ArgNo __n)
    {
        if (__n != 0)
            throw simple::string().snprintf("Bad parameters, expected %d, got %d.", 1, __n);

        CallContextNode __context(__thread, __this_object, __component_no, __args, __n, (Value*)0, 0);
        __thread->enter_function_call(&__context);

        return __thread->leave_function_call(&__context, this->name);
    }

    // Function 2
    Value output(Thread *__thread, Object *__this_object, ComponentNo __component_no, Value *__args, ArgNo __n)
    {
        if (__n != 0)
            throw simple::string().snprintf("Bad parameters, expected %d, got %d.", 1, __n);

        CallContextNode __context(__thread, __this_object, __component_no, __args, __n, (Value*)0, 0);
        __thread->enter_function_call(&__context);

        std_freq_t b, e;
        Integer i;
        double t;
        b = std_get_current_us_counter();
        for (i = 0; i < 1000000; i++)
            call_near(__thread, this, &__feature_name_impl::get_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per near call is %.2gns.\nAppromix %.2fM cps.\n", t, (1000. / t));
        
        b = std_get_current_us_counter();
        for (i = 0; i < 1000000; i++)
            call_far(__thread, 0, 1);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per far call is %.2gns.\nAppromix %.2fM cps.\n", t, (1000. / t));

        Value fun_name = "get_name";
        b = std_get_current_us_counter();
        for (i = 0; i < 1000000; i++)
            call_other(__thread, __this_object->get_oid(), fun_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per other call is %.2gns.\nAppromix %.2fM cps.\n", t, (1000. / t));

        Value ret = call_other(__thread, __this_object->get_oid(), "print");
        return __thread->leave_function_call(&__context, ret);
    }

private:
    Value name;
};

}
