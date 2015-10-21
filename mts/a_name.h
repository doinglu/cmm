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

class __feature_name_impl : public AbstractComponent
{
public:
    // Function 0
    Value set_name(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n < 1)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        if (__args[0].m_type != ValueType::STRING)
            throw_error("Parameter 1 'name' is not string.\n");

        // Enter function
//        Value __local[] = { };
        StringImpl* &name = __args[0].m_string;

        this->name = name;
        return Value();
    }

    // Function 1
    Value get_name(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n != 0)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        return this->name;
    }

    // Function 2
    Value test_call_private(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n != 1)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        ObjectId other_oid;
        other_oid.i64 = __args[0].m_int;

        call_far(_thread, 0, 5, other_oid, "---Inject error---");

        auto *__this_object = _thread->get_this_object();

        std_freq_t b, e;
        Integer i;
        double t;
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_near(_thread, this, &__feature_name_impl::get_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per near call is %.3gns.\nAppromix %.3fM cps.\n", t, (1000. / t));

        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_far(_thread, 0, 1);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per far call is %.3gns.\nAppromix %.3fM cps.\n", t, (1000. / t));

        Value fun_name = "get_name";
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_other(_thread, __this_object->get_oid(), fun_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per other call is %.3gns.\nAppromix %.3fM cps.\n", t, (1000. / t));

        fun_name = "do_nothing";
        b = std_get_current_us_counter();
        for (i = 0; i < 10000; i++)
            call_other(_thread, other_oid, fun_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per domain call is %.4gns.\nAppromix %.4fM cps.\n", t, (1000. / t));

        fun_name = "set_name";
        Value set_to = "Name was set";
        b = std_get_current_us_counter();
        for (i = 0; i < 10000; i++)
            call_other(_thread, other_oid, fun_name, set_to, set_to, set_to, set_to, set_to, set_to);////---
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per domain call with parameter is %.4gns.\nAppromix %.4fM cps.\n", t, (1000. / t));

        Value str_a = "a", str_b = "b";
        Value m = Map(8).bind_to(_thread->get_current_domain());
        m["a"] = 1;
        m["b"] = 2;
        b = std_get_current_us_counter();
        for (i = 0; i < 1000; i++)
            m[str_a];
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per mapping read is %.4gns.\nAppromix %.4fM cps.\n", t, (1000. / t));

        m = Map().bind_to(_thread->get_current_domain());
        m["a"] = 1;
        m["b"] = 2;
        b = std_get_current_us_counter();
        for (i = 0; i < 1000; i++)
            m[str_b] = 5;
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per mapping write is %.4gns.\nAppromix %.4fM cps.\n", t, (1000. / t));

        Value ret = call_other(_thread, other_oid, String("print"));
        ret = Value(5);
        return ret;
    }

    // Function 3
    Value test_call(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n != 1)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        ObjectId other_oid;
        other_oid.i64 = __args[0].m_int;

        auto *__this_object = _thread->get_this_object();

        Value ret = __this_object->get_program()->invoke_self(_thread, "test_call_private", __args, __n);
        return ret;
    }

    // Function 4
    Value do_nothing(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n != 0)
            throw_error("Bad parameters, expected %lld, got %lld.", (Integer)0, (Integer)__n);
        return Value();
    }

    // Function 5
    // (other_oid, msg)
    Value test_error(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n != 2)
            throw_error("Bad parameters, expected %lld, got %lld.", (Integer)0, (Integer)__n);
 
        auto *this_call_context = _thread->get_this_call_context();
        try
        {
            ObjectId other_oid;
            other_oid.i64 = __args[0].m_int;
            Value ret = call_other(_thread, other_oid, String("perror"), __args[1], "More", 1.5);
        }
        catch (const char *str)
        {
            printf("Exception[%p]: %s\n", &str, str);
            _thread->restore_call_stack_for_error(this_call_context);
        }
        catch (...)
        {
            _thread->restore_call_stack_for_error(this_call_context);
        }
        return Value();
    }

private:
    Value name;
};

}
