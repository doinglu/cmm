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

        Value& name = __args[0];

        // Enter function
        Value& m_name = this->m_object_vars[0];

        m_name = name;
        return m_name;
    }

    // Function 1
    Value get_name(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n != 0)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        Value& m_name = this->m_object_vars[0];
        return m_name;
    }

    // Function 2
    Value test_call_private(Thread *_thread, Value *__args, ArgNo __n)
    {
        if (__n != 1)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        ObjectId other_oid;
        other_oid.i64 = __args[0].m_int;

#if false
        call_far(_thread, 0, 5, other_oid, "---Inject error---");
#endif

        auto *__this_object = _thread->get_this_object();

        std_freq_t b, e;
        Integer i;
        double t;
        Value fun_name = NIL;
        Value set_to = NIL;
#if 1
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_near(_thread, this, &__feature_name_impl::get_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.3fM cps.\n", "near-call", t, (1000. / t));
#endif

#if 0
        set_to = "Name was set.1";
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_near(_thread, this, &__feature_name_impl::set_name, set_to, set_to, set_to, set_to, set_to, set_to);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.3fM cps.\n", "near-calls", t, (1000. / t));

        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_far(_thread, 0, 1);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.3fM cps.\n", "far-call", t, (1000. / t));

        set_to = "Name was set.2";
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_far(_thread, 0, 0, set_to, set_to, set_to, set_to, set_to, set_to);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.3fM cps.\n", "far-calls", t, (1000. / t));

        fun_name = "get_name";
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_other(_thread, __this_object->get_oid(), fun_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.3fM cps.\n", "other-call", t, (1000. / t));

        fun_name = "set_name";
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            call_other(_thread, __this_object->get_oid(), fun_name, set_to, set_to, set_to, set_to, set_to, set_to);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.3fM cps.\n", "other-calls", t, (1000. / t));

        fun_name = "do_nothing";
        b = std_get_current_us_counter();
        for (i = 0; i < 10000; i++)
            call_other(_thread, other_oid, fun_name);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.4fM cps.\n", "domain-call", t, (1000. / t));

        fun_name = "set_name";
        set_to = "Name was set";
        b = std_get_current_us_counter();
        for (i = 0; i < 1000; i++)
            call_other(_thread, other_oid, fun_name, set_to, set_to, set_to, set_to, set_to, set_to);////---
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.4fM cps.\n", "domain-calls", t, (1000. / t));

        Value str_a = "a", str_b = "b";
        Value m = Map(8);
        m.set(str_a, 1);
        m.set(str_b, 2);
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            m.get(str_a);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.4fM cps.\n", "mapping-rd", t, (1000. / t));

        m = Map(8);
        m.set(str_a, 1);
        m.set(str_b, 2);
        b = std_get_current_us_counter();
        for (i = 0; i < 100000; i++)
            m.set(str_b, 5);
        e = std_get_current_us_counter();
        t = (double)(e - b);
        t = t / (double)i;
        t *= 1000;
        printf("Per %-12s is %5.3gns/%.4fM cps.\n", "mapping-wr", t, (1000. / t));
#endif
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
        return NIL;
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
        return NIL;
    }
};

}
