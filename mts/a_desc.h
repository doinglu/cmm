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

class __feature_desc_impl : public AbstractComponent
{
public:
    void print(Thread *_thread, ArgNo __n)
    {
        if (__n != 0)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        auto r = ReserveStack(1, _thread);
        Value &r1 = r[0];
        printf("Name: %s.\n", call_far(_thread, 1 /* Component:Name */, 1 /* get_name() */, &r1).m_string->c_str());
    }

    void perror(Thread *_thread, ArgNo __n)
    {
        if (__n != 3)
            throw_error("Bad parameters, expected %lld, got %lld.\n", (Int64)1, (Int64)__n);

        auto r = ReserveStack(2, _thread);
        Value &r1 = r[0];
        Value &efun_name = r[1];
        auto* __args = &_thread->get_arg(0);
        call_efun(_thread, efun_name = "error", &r1, "perror(%O, %O, %O)!\n", __args[0], __args[1], __args[2]);
    }
};

}
